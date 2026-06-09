#include "pch.h"
#include "GameManager.h"

void GameAssets::ResetModel()
{
    modelParts.clear();
    modelLoaded = false;
}

void GameManager::CreateMeshResources()
{
    const MeshData cubeMesh = MeshFactory::CreateCube();
    CreateMeshResource(m_assets.meshes[static_cast<std::size_t>(MeshType::Cube)], cubeMesh);

    TerrainMeshData terrainBuild = MeshFactory::CreateTerrainFromDefaultHeightMap();
    m_scene.terrain = std::move(terrainBuild.terrain);
    CreateMeshResource(m_assets.meshes[static_cast<std::size_t>(MeshType::Terrain)], terrainBuild.mesh);

    m_assets.ResetModel();

    TextMeshModel textModel;
    const std::optional<std::filesystem::path> modelPath = TextMeshModel::FindDefaultPath();
    if (!modelPath || !textModel.LoadFromTextFile(*modelPath))
    {
        return;
    }

    m_assets.modelParts.reserve(textModel.Parts().size());
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
        }
    }

    m_assets.modelLoaded = !m_assets.modelParts.empty();
}
