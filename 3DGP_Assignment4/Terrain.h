#pragma once

class Terrain
{
public:
    void Reset(UINT width, UINT length, float cellX, float cellZ, float halfWidth, float halfLength);
    void PushHeight(float height);

    bool Empty() const;
    UINT Width() const;
    UINT Length() const;
    float CellX() const;
    float CellZ() const;
    float HalfWidth() const;
    float HalfLength() const;

    bool Contains(float worldX, float worldZ) const;
    float HeightAt(float worldX, float worldZ) const;

private:
    float Sample(int x, int z) const;

    std::vector<float> m_heights;
    UINT m_width = 0;
    UINT m_length = 0;
    float m_cellX = 1.0f;
    float m_cellZ = 1.0f;
    float m_halfWidth = 0.0f;
    float m_halfLength = 0.0f;
};
