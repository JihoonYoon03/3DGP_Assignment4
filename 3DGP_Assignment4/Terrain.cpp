#include "pch.h"
#include "Terrain.h"

void Terrain::Reset(UINT width, UINT length, float cellX, float cellZ, float halfWidth, float halfLength)
{
    m_width = width;
    m_length = length;
    m_cellX = cellX;
    m_cellZ = cellZ;
    m_halfWidth = halfWidth;
    m_halfLength = halfLength;
    m_heights.clear();
    m_heights.reserve(static_cast<std::size_t>(width) * length);
}

void Terrain::PushHeight(float height)
{
    m_heights.push_back(height);
}

bool Terrain::Empty() const
{
    return m_heights.empty();
}

UINT Terrain::Width() const
{
    return m_width;
}

UINT Terrain::Length() const
{
    return m_length;
}

float Terrain::CellX() const
{
    return m_cellX;
}

float Terrain::CellZ() const
{
    return m_cellZ;
}

float Terrain::HalfWidth() const
{
    return m_halfWidth;
}

float Terrain::HalfLength() const
{
    return m_halfLength;
}

bool Terrain::Contains(float worldX, float worldZ) const
{
    const float localX = (worldX + m_halfWidth) / m_cellX;
    const float localZ = (worldZ + m_halfLength) / m_cellZ;
    return localX >= 0.0f &&
        localZ >= 0.0f &&
        localX <= static_cast<float>(m_width - 1) &&
        localZ <= static_cast<float>(m_length - 1);
}

float Terrain::HeightAt(float worldX, float worldZ) const
{
    if (m_heights.empty() || m_width < 2 || m_length < 2)
    {
        return 0.0f;
    }

    const float localX = (worldX + m_halfWidth) / m_cellX;
    const float localZ = (worldZ + m_halfLength) / m_cellZ;
    if (localX < 0.0f || localZ < 0.0f || localX > static_cast<float>(m_width - 1) || localZ > static_cast<float>(m_length - 1))
    {
        return 0.0f;
    }

    const int x0 = static_cast<int>(std::floor(localX));
    const int z0 = static_cast<int>(std::floor(localZ));
    const int x1 = std::min(x0 + 1, static_cast<int>(m_width) - 1);
    const int z1 = std::min(z0 + 1, static_cast<int>(m_length) - 1);
    const float tx = localX - static_cast<float>(x0);
    const float tz = localZ - static_cast<float>(z0);

    const float h00 = Sample(x0, z0);
    const float h10 = Sample(x1, z0);
    const float h01 = Sample(x0, z1);
    const float h11 = Sample(x1, z1);
    const float h0 = h00 + (h10 - h00) * tx;
    const float h1 = h01 + (h11 - h01) * tx;
    return h0 + (h1 - h0) * tz;
}

float Terrain::Sample(int x, int z) const
{
    return m_heights[static_cast<std::size_t>(z) * m_width + x];
}
