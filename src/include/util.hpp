#pragma once
#include <math.h>
#include <string>

#define CSI constexpr static int
#define CSF constexpr static double
#define CSD constexpr static double
#define CSS const static std::string

// 画像の大きさ
CSI WIDTH = 2560;
CSI HEIGHT = 3360;
CSI SIZE = WIDTH * HEIGHT;
// 左上格子点の緯度・経度
CSF FIRST_LATITUDE = 47.995833;
CSF FIRST_LONGITUDE = 118.006250;
// 右下格子点の緯度・経度
CSF LAST_LATITUDE = 20.004167;
CSF LAST_LONGITUDE = 149.993750;
// 格子点間の緯度・経度差
CSF DX = (LAST_LONGITUDE - FIRST_LONGITUDE) / (WIDTH - 1);
CSF DY = (LAST_LATITUDE - FIRST_LATITUDE) / (HEIGHT - 1);
// 解析雨量データの件数
CSI DATA_COUNT = 87648;

CSS MERGED_FILE = "/home/yamashita/disk02/analyze/merged/merged.bin";

// 追加セクションの最新バージョン
CSI LATEST_VERSION = 2;

// 画像内部の点かどうかを判定する
inline bool is_in(int x, int y) {
    return 0 <= x && x < WIDTH && 0 <= y && y < HEIGHT;
}

// 緯度・経度に対応するピクセルの座標を取得する
inline std::pair<int, int> get_pixel(double latitude, double longitude) {
    return {(int)round((longitude - FIRST_LONGITUDE) / DX),
            (int)round((latitude - FIRST_LATITUDE) / DY)};
}

inline std::pair<int, int> get_pixel(std::pair<double, double> coord) {
    return get_pixel(coord.first, coord.second);
}

inline int get_index(std::pair<int, int> pixel) {
    return pixel.first + pixel.second * WIDTH;
}

inline std::pair<int, int> get_pixel(int index) {
    return {index % WIDTH, index / WIDTH};
}

// ピクセルの座標に対応する緯度・経度を取得する
inline std::pair<double, double> get_coord(int x, int y) {
    return {FIRST_LATITUDE + y * DY, FIRST_LONGITUDE + x * DX};
}

inline std::pair<double, double> get_coord(std::pair<int, int> pixel) {
    return get_coord(pixel.first, pixel.second);
}

// 三次メッシュのコードに対応するメッシュの中心の緯度・経度を取得する
inline std::pair<double, double> get_coord_from_code(int code) {
    // 1 次
    double lat = (code / 1000000) / 1.5;
    double lon = ((code / 10000) % 100) + 100;
    // 2 次
    lat += 5. / 60 * ((code / 1000) % 10);
    lon += 7.5 / 60 * ((code / 100) % 10);
    // 3 次
    lat += 30. / 3600 * ((code / 10) % 10);
    lon += 45. / 3600 * (code % 10);
    // 中心座標
    lat += 30. / 3600 / 2;
    lon += 45. / 3600 / 2;
    return {lat, lon};
}

inline double dist2(std::pair<int, int> p, std::pair<int, int> q) {
    return (p.first - q.first) * (p.first - q.first) + (p.second - q.second) * (p.second - q.second);
}

inline int pow_int(int a, int b) {
    int res = 1;
    for (int i = 0; i < b; i++) res *= a;
    return res;
}

template <typename F>
inline F fix(F v, F min, F max) {
    if (v < min) return min;
    else if (v > max) return max;
    else return v;
}
