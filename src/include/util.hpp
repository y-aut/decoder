#pragma once
#include <math.h>

#define CSI constexpr static int
#define CSF constexpr static float
#define CSD constexpr static double

// 画像の大きさ
CSI WIDTH = 2560;
CSI HEIGHT = 3360;
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

// 追加セクションの最新バージョン
CSI LATEST_VERSION = 2;

// 画像内部の点かどうかを判定する
inline bool is_in(int x, int y) {
    return 0 <= x && x < WIDTH && 0 <= y && y < HEIGHT;
}

// 緯度・経度に対応するピクセルの座標を取得する
inline std::pair<int, int> get_pixel(float latitude, float longitude) {
    return {(int)round((longitude - FIRST_LONGITUDE) / DX),
            (int)round((latitude - FIRST_LATITUDE) / DY)};
}

// ピクセルの座標に対応する緯度・経度を取得する
inline std::pair<float, float> get_coord(int x, int y) {
    return {FIRST_LATITUDE + y * DY, FIRST_LONGITUDE + x * DX};
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
