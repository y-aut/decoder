#pragma once
#include "color.hpp"
#include "util.hpp"

// 画像の特定のピクセルに色を設定する
void set_color(unsigned char img[], int x, int y, const Color &c);

// 海岸線を描画する
void draw_coastline(unsigned char img[]);

// 位置を描画する
void draw_location(unsigned char img[], float latitude, float longitude);
