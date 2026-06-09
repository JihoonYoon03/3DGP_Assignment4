#include "pch.h"
#include "AssignmentGame.h"

void AssignmentGame::CreateMeshResources()
{
    const MeshData cubeMesh = MeshFactory::CreateCube();
    CreateMeshResource(m_meshes[static_cast<std::size_t>(MeshType::Cube)], cubeMesh);

    TerrainMeshData terrainBuild = MeshFactory::CreateTerrainFromDefaultHeightMap();
    m_terrain = std::move(terrainBuild.terrain);
    CreateMeshResource(m_meshes[static_cast<std::size_t>(MeshType::Terrain)], terrainBuild.mesh);

    m_apacheParts.clear();
    m_apacheModelLoaded = false;

    ApacheModel apacheModel;
    const std::optional<std::filesystem::path> modelPath = ApacheModel::FindDefaultPath();
    if (!modelPath || !apacheModel.Load(*modelPath))
    {
        return;
    }

    m_apacheParts.reserve(apacheModel.Parts().size());
    for (const ApacheModelPart& modelPart : apacheModel.Parts())
    {
        ApacheMeshPart part{};
        part.center = modelPart.center;
        part.extents = modelPart.extents;
        part.name = modelPart.name;
        part.mainRotor = modelPart.mainRotor;
        part.tailRotor = modelPart.tailRotor;

        CreateMeshResource(part.mesh, modelPart.meshData);
        if (part.mesh.indexCount > 0)
        {
            m_apacheParts.push_back(std::move(part));
        }
    }

    m_apacheModelLoaded = !m_apacheParts.empty();
}
