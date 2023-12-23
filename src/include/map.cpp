#include "map.hpp"
#include "io.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>

using namespace std;

void set_color(unsigned char img[], int x, int y, const Color &c) {
    if (!is_in(x, y)) return;
    int index = (x + (HEIGHT - y - 1) * WIDTH) * 3;
    img[index] = c.blue;
    img[index + 1] = c.green;
    img[index + 2] = c.red;
}

void set_color(unsigned char img[], std::pair<int, int> pixel, const Color &c) {
    set_color(img, pixel.first, pixel.second, c);
}

void draw_coastline(unsigned char img[]) {
    string coast_file = "/home/yamashita/disk02/geo/prefecture/pixel.csv";
    ifstream ifs(coast_file, ios::in);
    if (!ifs) {
        cout << "海岸線ファイルが開けませんでした: " << coast_file << endl;
        return;
    }

    auto c = Color("000000");

    string line;
    while (getline(ifs, line)) {
        auto pos = line.find(',');
        int x = stoi(line.substr(0, pos));
        int y = stoi(line.substr(pos + 1));
        set_color(img, x, y, c);
    }
}

void draw_location(unsigned char img[], double latitude, double longitude, const Color &color) {
    // 地点を丸で囲んで表示する
    CSF RADIUS = 10.0;

    auto pos = get_pixel(latitude, longitude);
    for (double theta = 0; theta < 2 * M_PI; theta += 0.1) {
        int x = pos.first + (int)(RADIUS * cosf(theta));
        int y = pos.second + (int)(RADIUS * sinf(theta));
        for (int x_ = x; x_ < x + 2; x_++) {
            for (int y_ = y; y_ < y + 2; y_++) {
                set_color(img, x_, y_, color);
            }
        }
    }
}

void draw_population(unsigned char img[], const std::function<Color(double)> &color) {
    auto data = load_population();
    auto max_p = max_element(data.begin(), data.end())->second;

    for (auto v : data) {
        auto pixel = get_pixel(v.first);
        set_color(img, pixel.first, pixel.second, color((double)v.second / max_p));
    }
}
