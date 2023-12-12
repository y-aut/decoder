#pragma once
#include "color.hpp"
#include <fstream>
#include <functional>
#include <string>
#include <vector>

struct Info {
    // 全資料点の数
    int point_count = 0;
    // 1 データのビット数
    int bits = 0;
    // 今回の圧縮に用いたレベルの最大値
    int V = 0;
    // データの取り得るレベルの最大値
    int M = 0;
    // データ代表値の尺度因子
    int E = 0;
    // 各レベルに対応するデータ代表値
    std::vector<int> value;
    // データのバイト数
    int data_length = 0;
};

void read_info(std::ifstream &in, Info &info);
void write_info(std::ofstream &out, Info &info);
float get_value(std::ifstream &in, const Info &info, float latitude, float longitude);
void convert(std::ifstream &in, std::ofstream &out, const Info &in_info, Info &out_info, const std::function<int(int)> &f);
void merge_search(std::vector<std::ifstream *> &in, const std::vector<Info> &in_info, Info &out_info, const std::function<int(int, int)> &f);
void merge(std::vector<std::ifstream *> &in, std::ofstream &out, const std::vector<Info> &in_info, Info &out_info, const std::function<int(int, int)> &f);
void create_image(std::ifstream &in, std::string out_file, const Info &info, const std::function<Color(int)> &color, float latitude, float longitude);
