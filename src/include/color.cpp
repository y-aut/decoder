#include "color.hpp"
#include "util.hpp"
#include <assert.h>
#include <iomanip>
#include <math.h>

#define W2(v) setw(2) << setfill('0') << (int)v

using namespace std;

Color::Color(int _red, int _green, int _blue) {
    red = fix(_red, 0, 255);
    green = fix(_green, 0, 255);
    blue = fix(_blue, 0, 255);
}

Color::Color(std::string code) {
    assert(code.size() == 6);
    red = stoi(code.substr(0, 2), nullptr, 16);
    green = stoi(code.substr(2, 2), nullptr, 16);
    blue = stoi(code.substr(4, 2), nullptr, 16);
}

Color Color::from_hsl(double h, double s, double l) {
    h -= floor(h / 360) * 360;
    double max = 255 * (l + s * (100 - abs(2 * l - 100)) / 200) / 100;
    double min = 255 * (l - s * (100 - abs(2 * l - 100)) / 200) / 100;
    if (h < 60) {
        return Color(max, min + (max - min) * h / 60, min);
    } else if (h < 120) {
        return Color(min + (max - min) * (120 - h) / 60, max, min);
    } else if (h < 180) {
        return Color(min, max, min + (max - min) * (h - 120) / 60);
    } else if (h < 240) {
        return Color(min, min + (max - min) * (240 - h) / 60, max);
    } else if (h < 300) {
        return Color(min + (max - min) * (h - 240) / 60, min, max);
    } else {
        return Color(max, min, min + (max - min) * (360 - h) / 60);
    }
}

std::ostream &operator<<(std::ostream &os, const Color &color) {
    os << uppercase << hex << W2(color.red) << W2(color.green) << W2(color.blue) << std::dec;
    return os;
}
