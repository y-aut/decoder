#pragma once
#include "util.hpp"
#include <tuple>

// 画像内部の点かどうかを判定する
inline bool is_in(int x, int y) {
    return 0 <= x && x < WIDTH && 0 <= y && y < HEIGHT;
}

// 緯度・経度に対応するピクセルの座標を取得する
inline std::tuple<int, int> get_pixel(float latitude, float longitude) {
    return {(int)((longitude - FIRST_LONGITUDE) / DX),
            (int)((latitude - FIRST_LATITUDE) / DY)};
}

// 海岸線を描画する
void draw_coastline(unsigned char img[]);

// 位置を描画する
void draw_location(unsigned char img[], float latitude, float longitude);
