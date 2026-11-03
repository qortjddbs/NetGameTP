#pragma once

// ===== 별 =====
const unsigned int STAR_COUNT = 7;
const float STAR_POS[STAR_COUNT][3] = {
    { 0.0f,  0.5f,  -10.0f}, { 0.0f,  8.0f,  -45.0f}, { 0.0f,  2.0f,  -60.0f},
    {-3.0f,  5.0f,  -80.0f}, { 0.0f, 10.0f, -107.0f}, { 0.0f, 12.0f, -130.0f},
    { 0.0f, 17.0f, -143.0f}
};

// ===== 아이템 =====
enum class ITEM_TYPE : unsigned char {
    FLYING,
    ENEMY_BLIND,
    ENEMY_SLOW,
    COUNT
};

const float ITEM_DURATION[(int)ITEM_TYPE::COUNT] = { 5.0f, 10.0f, 5.0f };   // 효과 지속 시간(초)

struct ItemData { 
    ITEM_TYPE type;
    float x, y, z;
};

const unsigned int ITEM_COUNT = 6;
const float ITEM_RADIUS_CHECK = 3.0f;
const ItemData ITEM_LIST[ITEM_COUNT] = {
    { ITEM_TYPE::FLYING,      0.0f,  1.0f,  -25.0f },
    { ITEM_TYPE::ENEMY_BLIND, 0.0f,  3.0f,  -52.0f },
    { ITEM_TYPE::ENEMY_SLOW, -2.0f,  5.0f,  -70.0f },
    { ITEM_TYPE::FLYING,      0.0f,  8.0f,  -95.0f },
    { ITEM_TYPE::ENEMY_BLIND, 0.0f, 11.0f, -118.0f },
    { ITEM_TYPE::ENEMY_SLOW,  0.0f, 14.0f, -137.0f },
};
