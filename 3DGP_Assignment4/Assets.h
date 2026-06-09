#pragma once

#include "Mesh.h"

enum class ModelType : std::size_t
{
    Apache = 0,
    Tank,
    Rock,
    Rock2,
    Count
};

class ModelHandle
{
public:
    std::size_t firstPart = 0;
    std::size_t partCount = 0;

    bool Loaded() const
    {
        return partCount > 0;
    }
};

class GameAssets
{
public:
    using MeshArray = std::array<MeshResource, static_cast<std::size_t>(MeshType::Count)>;
    using ModelHandleArray = std::array<ModelHandle, static_cast<std::size_t>(ModelType::Count)>;

    void ResetModels();
    const ModelHandle& Model(ModelType type) const;
    bool HasModel(ModelType type) const;

    MeshArray meshes{};
    ModelHandleArray models{};
    std::vector<ModelMeshPart> modelParts;
};
