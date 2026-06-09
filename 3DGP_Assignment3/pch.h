#pragma once

#include "framework.h"

inline constexpr float GP_WORLD_UNITS_PER_METER = 1.0f;

inline constexpr float GP_TERRAIN_BASE_CELL_METERS = 1.4f;
inline constexpr float GP_TERRAIN_SIZE_SCALE = 8.0f;
inline constexpr int GP_TERRAIN_BASE_CELL_COUNT = 32;
inline constexpr int GP_TERRAIN_GRID_CELL_COUNT = 128;
inline constexpr int GP_TERRAIN_GRID_VERTEX_COUNT = GP_TERRAIN_GRID_CELL_COUNT + 1;
inline constexpr float GP_TERRAIN_CELL_METERS = GP_TERRAIN_BASE_CELL_METERS * GP_TERRAIN_SIZE_SCALE *
    static_cast<float>(GP_TERRAIN_BASE_CELL_COUNT) /
    static_cast<float>(GP_TERRAIN_GRID_CELL_COUNT);
inline constexpr float GP_TERRAIN_HALF_SIZE_METERS = GP_TERRAIN_CELL_METERS * static_cast<float>(GP_TERRAIN_GRID_CELL_COUNT) * 0.5f;

inline constexpr float GP_APACHE_SOURCE_UNIT_TO_METER = 0.09f;
inline constexpr float GP_APACHE_MODEL_SCALE = GP_APACHE_SOURCE_UNIT_TO_METER * GP_WORLD_UNITS_PER_METER;

inline constexpr float GP_APACHE_MUZZLE_OFFSET_METERS = 5.2f;

inline constexpr float GP_UNREAL_CENTIMETER_TO_METER = 0.01f;
inline constexpr float GP_HEIGHTMAP_UNREAL_X_SCALE = 1578.38f;
inline constexpr float GP_HEIGHTMAP_UNREAL_Y_SCALE = 1578.38f;
inline constexpr float GP_HEIGHTMAP_UNREAL_Z_SCALE = 75.39f;
inline constexpr float GP_HEIGHTMAP_PROJECT_HORIZONTAL_SCALE = 0.10f;
inline constexpr float GP_HEIGHTMAP_PROJECT_VERTICAL_SCALE = 0.25f;
inline constexpr float GP_HEIGHTMAP_CELL_X_METERS = GP_HEIGHTMAP_UNREAL_X_SCALE * GP_UNREAL_CENTIMETER_TO_METER * GP_HEIGHTMAP_PROJECT_HORIZONTAL_SCALE;
inline constexpr float GP_HEIGHTMAP_CELL_Z_METERS = GP_HEIGHTMAP_UNREAL_Y_SCALE * GP_UNREAL_CENTIMETER_TO_METER * GP_HEIGHTMAP_PROJECT_HORIZONTAL_SCALE;
inline constexpr float GP_HEIGHTMAP_MAX_HEIGHT_METERS = GP_HEIGHTMAP_UNREAL_Z_SCALE * GP_HEIGHTMAP_PROJECT_VERTICAL_SCALE;

inline constexpr float GP_TERRAIN_BASE_HEIGHT_OFFSET_METERS = -12.0f;
// 지형 충돌 시 각 객체 중심이 지면보다 떠 있어야 하는 기본 높이
inline constexpr float GP_PLAYER_TERRAIN_CLEARANCE_METERS = 4.0f;
inline constexpr float GP_ENEMY_TERRAIN_CLEARANCE_METERS = 0.8f;

// 빛 관련
inline constexpr float GP_LIGHT_AMBIENT_STRENGTH = 0.32f;
inline constexpr float GP_LIGHT_DIFFUSE_STRENGTH = 0.78f;
inline constexpr float GP_LIGHT_SPECULAR_STRENGTH = 0.35f;
inline constexpr float GP_LIGHT_SPECULAR_POWER = 32.0f;

// 적 개수
inline constexpr int GP_LEVEL_TARGET_COUNT = 12;
