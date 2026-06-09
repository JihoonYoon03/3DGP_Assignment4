#include "pch.h"
#include "GameManager.h"

void GameAssets::ResetModels()
{
    models = {};
    modelParts.clear();
}

const ModelHandle& GameAssets::Model(ModelType type) const
{
    return models[static_cast<std::size_t>(type)];
}

bool GameAssets::HasModel(ModelType type) const
{
    return Model(type).Loaded();
}

void GameManager::CreateMeshResources()
{
    const MeshData cubeMesh = MeshFactory::CreateCube();
    CreateMeshResource(m_assets.meshes[static_cast<std::size_t>(MeshType::Cube)], cubeMesh);

    TerrainMeshData terrainBuild = MeshFactory::CreateTerrainFromDefaultHeightMap();
    m_scene.terrain = std::move(terrainBuild.terrain);
    CreateMeshResource(m_assets.meshes[static_cast<std::size_t>(MeshType::Terrain)], terrainBuild.mesh);

    m_assets.ResetModels();

    const auto loadTextModel = [this](ModelType type, const std::wstring& fileName)
    {
        TextMeshModel textModel;
        const std::optional<std::filesystem::path> modelPath = TextMeshModel::FindPath(fileName);
        if (!modelPath || !textModel.LoadFromTextFile(*modelPath))
        {
            return;
        }

        ModelHandle handle{};
        handle.firstPart = m_assets.modelParts.size();
        m_assets.modelParts.reserve(m_assets.modelParts.size() + textModel.Parts().size());

        for (const TextMeshModel::Part& modelPart : textModel.Parts())
        {
            ModelMeshPart part{};
            part.center = modelPart.center;
            part.extents = modelPart.extents;
            part.name = modelPart.name;
            part.mainRotor = modelPart.mainRotor;
            part.tailRotor = modelPart.tailRotor;

            CreateMeshResource(part.mesh, modelPart.meshData);
            if (part.mesh.indexCount > 0)
            {
                m_assets.modelParts.push_back(std::move(part));
                ++handle.partCount;
            }
        }

        m_assets.models[static_cast<std::size_t>(type)] = handle;
    };

    loadTextModel(ModelType::Apache, L"Apache.txt");
    loadTextModel(ModelType::Tank, L"AbramsTank.txt");
    loadTextModel(ModelType::Rock, L"Rock.txt");
    loadTextModel(ModelType::Rock2, L"Rock2.txt");
}
