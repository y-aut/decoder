#pragma once

#define CSI constexpr static int
#define CSF constexpr static float

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

inline int pow_int(int a, int b) {
    int res = 1;
    for (int i = 0; i < b; i++) res *= a;
    return res;
}

inline int fix(int v, int min, int max) {
    if (v < min) return min;
    else if (v > max) return max;
    else return v;
}