#include "pch.h"
#include "AssignmentGame.h"


using namespace DirectX;


namespace
{
    constexpr const wchar_t* ApacheModelFileName = L"Apache.txt";

    XMFLOAT4X4 IdentityMatrix()
    {
        XMFLOAT4X4 matrix{};
        XMStoreFloat4x4(&matrix, XMMatrixIdentity());
        return matrix;
    }

    std::string_view TrimLeft(std::string_view text)
    {
        while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
        {
            text.remove_prefix(1);
        }
        return text;
    }

    int CountIndent(std::string_view text)
    {
        int indent = 0;
        for (const char ch : text)
        {
            if (ch != ' ' && ch != '\t')
            {
                break;
            }
            ++indent;
        }
        return indent;
    }

    std::optional<std::string_view> PayloadAfterTag(std::string_view line, std::string_view tag)
    {
        const std::string_view trimmed = TrimLeft(line);
        if (!trimmed.starts_with(tag))
        {
            return std::nullopt;
        }
        return TrimLeft(trimmed.substr(tag.size()));
    }

    // <Frame> 행에서 프레임 이름만 분리
    std::string ParseFrameName(std::string_view payload)
    {
        std::istringstream stream{ std::string(payload) };
        int frameIndex = 0;
        std::string frameName;
        stream >> frameIndex >> frameName;
        return frameName;
    }

    std::optional<XMFLOAT4X4> ParseMatrix(std::string_view payload)
    {
        std::istringstream stream{ std::string(payload) };
        float values[16]{};
        for (float& value : values)
        {
            if (!(stream >> value))
            {
                return std::nullopt;
            }
        }

        XMFLOAT4X4 matrix{};
        XMStoreFloat4x4(
            &matrix,
            XMMatrixSet(
                values[0], values[1], values[2], values[3],
                values[4], values[5], values[6], values[7],
                values[8], values[9], values[10], values[11],
                values[12], values[13], values[14], values[15]));
        return matrix;
    }

    // 색상 값은 0 ~ 1 범위로 보정
    XMFLOAT4 NormalizeMaterialColor(const XMFLOAT4& color)
    {
        XMFLOAT4 normalized
        {
            std::clamp(color.x, 0.0f, 1.0f),
            std::clamp(color.y, 0.0f, 1.0f),
            std::clamp(color.z, 0.0f, 1.0f),
            1.0f
        };

        // 값이 너무 작으면 최소 밝기로 설정
        if (normalized.x + normalized.y + normalized.z < 0.03f)
        {
            normalized = { 0.04f, 0.04f, 0.04f, 1.0f };
        }
        return normalized;
    }

    std::filesystem::path ExecutableDirectory()
    {
        std::array<wchar_t, MAX_PATH> modulePath{};
        const DWORD length = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
        if (length == 0 || length >= modulePath.size())
        {
            return {};
        }
        return std::filesystem::path(modulePath.data()).parent_path();
    }

    std::optional<std::filesystem::path> FindApacheModelPath()
    {
        std::vector<std::filesystem::path> candidates =
        {
            std::filesystem::path(L"Models") / ApacheModelFileName,
            std::filesystem::path(L"3DGP_Assignment4") / L"Models" / ApacheModelFileName,
            std::filesystem::path(L"..") / L"Models" / ApacheModelFileName,
            std::filesystem::path(L"..") / L".." / L"3DGP_Assignment4" / L"Models" / ApacheModelFileName
        };

        const std::filesystem::path exeDirectory = ExecutableDirectory();
        if (!exeDirectory.empty())
        {
            candidates.push_back(exeDirectory / L"Models" / ApacheModelFileName);
            candidates.push_back(exeDirectory / L".." / L"Models" / ApacheModelFileName);
            candidates.push_back(exeDirectory / L".." / L".." / L"3DGP_Assignment4" / L"Models" / ApacheModelFileName);
        }

        for (const std::filesystem::path& candidate : candidates)
        {
            std::error_code error;
            if (std::filesystem::exists(candidate, error))
            {
                return candidate;
            }
        }
        return std::nullopt;
    }

    struct ApacheFrameState
    {
        int indent = 0;
        XMFLOAT4X4 parentWorld = IdentityMatrix();
        XMFLOAT4X4 world = IdentityMatrix();
        std::string name;
    };

    // GPU 리소스를 만들기 전 CPU에서 보관하는 Apache 파트 데이터
    struct ApacheCpuPart
    {
        std::string name;
        std::vector<XMFLOAT3> positions;
        std::vector<XMFLOAT3> normals;
        std::vector<std::uint32_t> indices;
        XMFLOAT4 color{ 0.72f, 0.72f, 0.72f, 1.0f };
        bool hasColor = false;
    };

    // 메시 데이터 임시 보관용
    struct PendingApacheMesh
    {
        bool active = false;
        std::string name;
        XMFLOAT4X4 world = IdentityMatrix();
        std::vector<XMFLOAT3> positions;
        std::vector<XMFLOAT3> normals;
        std::vector<std::uint32_t> indices;
        XMFLOAT4 color{ 0.72f, 0.72f, 0.72f, 1.0f };
        bool hasColor = false;

        void Reset()
        {
            active = false;
            name.clear();
            world = IdentityMatrix();
            positions.clear();
            normals.clear();
            indices.clear();
            color = { 0.72f, 0.72f, 0.72f, 1.0f };
            hasColor = false;
        }
    };
}


bool AssignmentGame::CreateApacheMesh()
{
    m_apacheParts.clear();
    const std::optional<std::filesystem::path> modelPath = FindApacheModelPath();
    if (!modelPath)
    {
        return false;
    }

    return LoadApacheModelFile(modelPath->wstring());
}

bool AssignmentGame::LoadApacheModelFile(const std::wstring& filePath)
{
    std::ifstream file{ std::filesystem::path(filePath) };
    if (!file)
    {
        return false;
    }

    std::vector<ApacheCpuPart> cpuParts;
    cpuParts.reserve(40);

    const XMFLOAT4X4 identity = IdentityMatrix();
    std::vector<ApacheFrameState> frameStack;
    PendingApacheMesh pendingMesh;

    auto flushPendingMesh = [&]()
    {
        if (!pendingMesh.active)
        {
            return;
        }

        if (!pendingMesh.positions.empty() && !pendingMesh.indices.empty())
        {
            if (pendingMesh.normals.size() != pendingMesh.positions.size())
            {
                pendingMesh.normals.assign(pendingMesh.positions.size(), { 0.0f, 1.0f, 0.0f });
            }

            ApacheCpuPart part{};
            part.name = pendingMesh.name;
            part.positions = std::move(pendingMesh.positions);
            part.normals = std::move(pendingMesh.normals);
            part.indices = std::move(pendingMesh.indices);
            part.color = pendingMesh.hasColor ? NormalizeMaterialColor(pendingMesh.color) : pendingMesh.color;
            part.hasColor = pendingMesh.hasColor;
            cpuParts.push_back(std::move(part));
        }

        pendingMesh.Reset();
    };

    std::string line;
    while (std::getline(file, line))
    {
        const std::string_view lineView(line);
        const std::string_view trimmed = TrimLeft(lineView);

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Frame>:"))
        {
            flushPendingMesh();

            const int indent = CountIndent(lineView);
            while (!frameStack.empty() && frameStack.back().indent >= indent)
            {
                frameStack.pop_back();
            }

            ApacheFrameState frame{};
            frame.indent = indent;
            frame.parentWorld = frameStack.empty() ? identity : frameStack.back().world;
            frame.world = frame.parentWorld;
            frame.name = ParseFrameName(*payload);
            frameStack.push_back(frame);
            continue;
        }

        if (trimmed.starts_with("</Frame>"))
        {
            flushPendingMesh();
            if (!frameStack.empty())
            {
                frameStack.pop_back();
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<TransformMatrix>:"))
        {
            if (!frameStack.empty())
            {
                if (const std::optional<XMFLOAT4X4> localMatrix = ParseMatrix(*payload))
                {
                    const XMMATRIX local = XMLoadFloat4x4(&(*localMatrix));
                    const XMMATRIX parent = XMLoadFloat4x4(&frameStack.back().parentWorld);
                    XMStoreFloat4x4(&frameStack.back().world, local * parent);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Mesh>:"))
        {
            flushPendingMesh();

            pendingMesh.active = true;
            pendingMesh.name = frameStack.empty() ? "ApachePart" : frameStack.back().name;
            pendingMesh.world = frameStack.empty() ? identity : frameStack.back().world;
            std::istringstream stream{ std::string(*payload) };
            int vertexCount = 0;
            if (stream >> vertexCount)
            {
                pendingMesh.positions.reserve(static_cast<std::size_t>(std::max(vertexCount, 0)));
                pendingMesh.normals.reserve(static_cast<std::size_t>(std::max(vertexCount, 0)));
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Positions>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                int positionCount = 0;
                stream >> positionCount;

                const XMMATRIX world = XMLoadFloat4x4(&pendingMesh.world);
                for (int index = 0; index < positionCount; ++index)
                {
                    float x = 0.0f;
                    float y = 0.0f;
                    float z = 0.0f;
                    if (!(stream >> x >> y >> z))
                    {
                        break;
                    }

                    const XMVECTOR localPosition = XMVectorSet(x, y, z, 1.0f);
                    XMFLOAT3 worldPosition{};
                    XMStoreFloat3(&worldPosition, XMVector3TransformCoord(localPosition, world));
                    pendingMesh.positions.push_back(worldPosition);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<Normals>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                int normalCount = 0;
                stream >> normalCount;

                const XMMATRIX world = XMLoadFloat4x4(&pendingMesh.world);
                for (int index = 0; index < normalCount; ++index)
                {
                    float x = 0.0f;
                    float y = 1.0f;
                    float z = 0.0f;
                    if (!(stream >> x >> y >> z))
                    {
                        break;
                    }

                    const XMVECTOR localNormal = XMVectorSet(x, y, z, 0.0f);
                    XMFLOAT3 worldNormal{};
                    XMStoreFloat3(&worldNormal, XMVector3Normalize(XMVector3TransformNormal(localNormal, world)));
                    pendingMesh.normals.push_back(worldNormal);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<SubMesh>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                int subMeshIndex = 0;
                int indexCount = 0;
                stream >> subMeshIndex >> indexCount;

                pendingMesh.indices.reserve(pendingMesh.indices.size() + static_cast<std::size_t>(std::max(indexCount, 0)));
                for (int index = 0; index < indexCount; ++index)
                {
                    std::uint32_t vertexIndex = 0;
                    if (!(stream >> vertexIndex))
                    {
                        break;
                    }
                    pendingMesh.indices.push_back(vertexIndex);
                }
            }
            continue;
        }

        if (const std::optional<std::string_view> payload = PayloadAfterTag(lineView, "<AlbedoColor>:"))
        {
            if (pendingMesh.active)
            {
                std::istringstream stream{ std::string(*payload) };
                stream >> pendingMesh.color.x >> pendingMesh.color.y >> pendingMesh.color.z >> pendingMesh.color.w;
                pendingMesh.hasColor = true;
                flushPendingMesh();
            }
            continue;
        }
    }

    flushPendingMesh();
    if (cpuParts.empty())
    {
        return false;
    }

    // Apache 모델의 중심 구하기 / 각 파트 정점을 같은 모델 로컬 좌표계로
    XMFLOAT3 modelMin{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    XMFLOAT3 modelMax{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
    for (const ApacheCpuPart& part : cpuParts)
    {
        for (const XMFLOAT3& position : part.positions)
        {
            modelMin.x = std::min(modelMin.x, position.x);
            modelMin.y = std::min(modelMin.y, position.y);
            modelMin.z = std::min(modelMin.z, position.z);
            modelMax.x = std::max(modelMax.x, position.x);
            modelMax.y = std::max(modelMax.y, position.y);
            modelMax.z = std::max(modelMax.z, position.z);
        }
    }

    const XMFLOAT3 modelCenter
    {
        (modelMin.x + modelMax.x) * 0.5f,
        (modelMin.y + modelMax.y) * 0.5f,
        (modelMin.z + modelMax.z) * 0.5f
    };

    m_apacheParts.clear();
    m_apacheParts.reserve(cpuParts.size());
    for (ApacheCpuPart& cpuPart : cpuParts)
    {
        std::vector<Vertex> vertices;
        vertices.reserve(cpuPart.positions.size());

        XMFLOAT3 partMin{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
        XMFLOAT3 partMax{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
        for (std::size_t index = 0; index < cpuPart.positions.size(); ++index)
        {
            XMFLOAT3 position = cpuPart.positions[index];
            position.x -= modelCenter.x;
            position.y -= modelCenter.y;
            position.z -= modelCenter.z;

            partMin.x = std::min(partMin.x, position.x);
            partMin.y = std::min(partMin.y, position.y);
            partMin.z = std::min(partMin.z, position.z);
            partMax.x = std::max(partMax.x, position.x);
            partMax.y = std::max(partMax.y, position.y);
            partMax.z = std::max(partMax.z, position.z);

            vertices.push_back({ position, cpuPart.color, cpuPart.normals[index] });
        }

        ApacheMeshPart part{};
        part.name = std::move(cpuPart.name);
        part.center =
        {
            (partMin.x + partMax.x) * 0.5f,
            (partMin.y + partMax.y) * 0.5f,
            (partMin.z + partMax.z) * 0.5f
        };
        part.extents =
        {
            (partMax.x - partMin.x) * 0.5f,
            (partMax.y - partMin.y) * 0.5f,
            (partMax.z - partMin.z) * 0.5f
        };

        const XMFLOAT3 fullSize{ part.extents.x * 2.0f, part.extents.y * 2.0f, part.extents.z * 2.0f };
        part.mainRotor = (part.name.find("rotor") != std::string::npos);
        part.tailRotor = !part.mainRotor && part.center.z < -35.0f && fullSize.z > 8.0f && fullSize.x < 4.0f && fullSize.y < 4.0f;

        CreateMeshResource(part.mesh, vertices, cpuPart.indices, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        if (part.mesh.indexCount > 0)
        {
            m_apacheParts.push_back(std::move(part));
        }
    }

    return !m_apacheParts.empty();
}
