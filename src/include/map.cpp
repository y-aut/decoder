#include "map.hpp"
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

void draw_location(unsigned char img[], float latitude, float longitude) {
    // 地点を赤丸で囲んで表示する
    CSF RADIUS = 10.0;
    auto c = Color("FF0000");

    auto pos = get_pixel(latitude, longitude);
    for (float theta = 0; theta < 2 * M_PI; theta += 0.1) {
        int x = pos.first + (int)(RADIUS * cosf(theta));
        int y = pos.second + (int)(RADIUS * sinf(theta));
        for (int x_ = x; x_ < x + 2; x_++) {
            for (int y_ = y; y_ < y + 2; y_++) {
                set_color(img, x_, y_, c);
            }
        }
    }
}
