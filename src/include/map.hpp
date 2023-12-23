#pragma once
#include "color.hpp"
#include "util.hpp"
#include <functional>

// 画像の特定のピクセルに色を設定する
void set_color(unsigned char img[], int x, int y, const Color &c);
void set_color(unsigned char img[], std::pair<int, int> pixel, const Color &c);

// 海岸線を描画する
void draw_coastline(unsigned char img[]);

// 位置を描画する
void draw_location(unsigned char img[], double latitude, double longitude, const Color &color);

// 人口密度を描画する
void draw_population(unsigned char img[], const std::function<Color(double)> &color);
