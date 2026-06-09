#pragma once

#include "Mesh.h"

class GameAssets
{
public:
    using MeshArray = std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>;

    void ResetModel();

    MeshArray meshes{};
    std::vector<ModelMeshPart> modelParts;
    bool modelLoaded = false;
};
