#pragma once
#include <string>

typedef unsigned char uchar;

struct Color {
    uchar red = 0;
    uchar green = 0;
    uchar blue = 0;

    Color(int _red, int _green, int _blue);
    Color(std::string code);
    static Color from_hsl(float h, float s, float l);
};
