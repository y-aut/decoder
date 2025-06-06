#include "process.hpp"
#include "bitmap.hpp"
#include "calc.hpp"
#include "io.hpp"
#include "map.hpp"
#include "segment.hpp"
#include "util.hpp"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <queue>
#include <sstream>
#include <vector>

#define IS_LITTLE_ENDIAN

using namespace std;

inline uint16_t swap16(uint16_t value) {
    uint16_t ret;
    ret = value << 8;
    ret |= value >> 8;
    return ret;
}

inline uint32_t swap24(uint32_t value) {
    uint32_t ret;
    ret = (value & 0x000000FF) << 16;
    ret |= (value & 0x0000FF00);
    ret |= value >> 16;
    return ret;
}

inline uint32_t swap32(uint32_t value) {
    uint32_t ret;
    ret = value << 24;
    ret |= (value & 0x0000FF00) << 8;
    ret |= (value & 0x00FF0000) >> 8;
    ret |= value >> 24;
    return ret;
}

inline int read(std::ifstream &in, int n) {
    unsigned int res = 0;
    in.read((char *)&res, n);
#ifndef IS_LITTLE_ENDIAN
    return res;
#else
    if (n == 1) return res;
    else if (n == 2) return swap16(res);
    else if (n == 3) return swap24(res);
    else if (n == 4) return swap32(res);
    throw "Not implemented";
#endif
}

inline void write(std::ostream &out, int n, int val) {
#ifdef IS_LITTLE_ENDIAN
    if (n == 2) val = swap16(val);
    else if (n == 3) val = swap24(val);
    else if (n == 4) val = swap32(val);
    else if (n != 1) throw "Not implemented";
#endif
    out.write((char *)&val, n);
}

// 必要な情報を読み取って，データ列の先頭まで進める
void read_info(std::ifstream &in, Info &info) {
    // 第 0 節は 16 bytes
    in.seekg(16, ios_base::beg);

    while (!in.eof()) {
        int length = read(in, 4);   // 節の長さ
        char section = read(in, 1); // 節番号

        if (section == 5) {
            info.point_count = read(in, 4);
            in.seekg(2, ios_base::cur);
            info.bits = read(in, 1);
            info.V = read(in, 2);
            info.M = read(in, 2);
            info.E = read(in, 1);
            for (int m = 0; m < info.M; m++) {
                info.value.push_back(read(in, 2));
            }
        } else if (section == 7) {
            info.data_length = length - 5;
            break;
        } else if (section == 15) {
            // 追加セクション
            uint version = read(in, 4);
            if (version >> 24 != 0xff) {
                info.V = version;
                version = 1;
            } else {
                version ^= 0xff << 24;
            }
            if (version != LATEST_VERSION) {
                cout << "このファイルは最新バージョンではありません．バージョン番号: " << version << endl;
            }
            if (version >= 2) {
                info.V = read(in, 4);
            }
            info.M = read(in, 4);
            info.E = read(in, 4);
            if (version >= 2) {
                info.count = read(in, 4);
            }
            info.value_is_level_1 = (read(in, 1) == 1);
            if (!info.value_is_level_1) {
                info.value.clear();
                for (int m = 0; m < info.M; m++) {
                    info.value.push_back(read(in, 4));
                }
            }
        } else {
            in.seekg(length - 5, ios_base::cur);
        }
    }
}

// 必要な情報を書き込んで，データ列の先頭まで進める
void write_info(std::ofstream &out, Info &info) {
    char zero[16] = {};
    out.write(zero, 16);
    // 第 5 節
    write(out, 4, 17);
    write(out, 1, 5);
    write(out, 4, info.point_count);
    write(out, 2, 0);
    write(out, 1, info.bits);
    write(out, 2, 0); // V
    write(out, 2, 0); // M
    write(out, 1, 0); // E
    // 第 15 節
    write(out, 4, info.value_is_level_1 ? 22 : 22 + 4 * info.M);
    write(out, 1, 15);
    write(out, 4, 0xff << 24 | LATEST_VERSION); // バージョン番号
    write(out, 4, info.V);
    write(out, 4, info.M);
    write(out, 4, info.E);
    write(out, 4, info.count);
    write(out, 1, info.value_is_level_1 ? 1 : 0);
    if (!info.value_is_level_1) {
        for (int m = 0; m < info.M; m++) {
            write(out, 4, info.value[m]);
        }
    }
    // 第 7 節
    write(out, 4, info.data_length + 5);
    write(out, 1, 7);
}

void show_info(const Info &info) {
    cout << "point_count: " << info.point_count << endl;
    cout << "bits: " << info.bits << endl;
    cout << "V: " << info.V << endl;
    cout << "M: " << info.M << endl;
    cout << "E: " << info.E << endl;
    cout << "count: " << info.count << endl;
    cout << "data_length: " << info.data_length << endl;
    cout << "value_is_level_1: " << info.value_is_level_1 << endl;
}

// 区間 [a, b) と [c, d) にともに含まれる整数の個数を求める
int intersect(int a, int b, int c, int d) {
    if (a > c) return intersect(c, d, a, b);
    return max(0, min(b - c, d - c));
}

// 中心から radius 以内にある点の数
int in_radius_count(int old_index, int new_index, int center, double radius) {
    if (radius < 1) {
        if (old_index <= center && center < new_index) return 1;
        else return 0;
    }
    new_index--;
    int old_x = old_index % WIDTH, old_y = old_index / WIDTH;
    int new_x = new_index % WIDTH, new_y = new_index / WIDTH;
    int center_x = center % WIDTH, center_y = center / WIDTH;
    int ans = 0;
    for (int y = old_y; y <= new_y; y++) {
        double x_dif = radius * radius - abs(y - center_y) * abs(y - center_y);
        if (x_dif < 0) continue;
        x_dif = sqrt(x_dif);
        int x_start = center_x - (int)x_dif, x_end = center_x + (int)x_dif + 1;
        if (old_y == new_y) {
            ans += intersect(old_x, new_x + 1, x_start, x_end);
        } else if (y == old_y) {
            ans += intersect(old_x, WIDTH, x_start, x_end);
        } else if (y == new_y) {
            ans += intersect(0, new_x + 1, x_start, x_end);
        } else {
            ans += intersect(0, WIDTH, x_start, x_end);
        }
    }
    return ans;
}

// 全地点のレベル値を取得する
std::vector<int> decode(std::ifstream &in, const Info &info) {
    vector<int> res;

    int in_bytes = info.bits / 8;
    int read_bytes = 0;

    Segment seg;
    int seg_len = 0;
    while (!in.eof()) {
        int val = read(in, in_bytes);
        read_bytes += in_bytes;

        auto new_seg = decode(info, seg, seg_len, val);
        if (new_seg.length) {
            seg_len = 0;
            for (int i = 0; i < seg.length; i++) {
                res.push_back(seg.value);
            }
            seg = new_seg;
        } else {
            seg_len++;
        }

        if (read_bytes >= info.data_length) break;
    }

    for (int i = 0; i < seg.length; i++) {
        res.push_back(seg.value);
    }

    return res;
}

unordered_map<int, double> get_prob(ifstream &in, ifstream &merged, const Info &info, const Info &merged_info) {
    // 雨の平均回数: 15194.45125 (人口が 0 でない地域), 13792.14131 (人口による重み付け)
    CSD MEAN_RAIN_COUNT = 15194.45125;

    // 適合率
    CSD Pr = 0.9;
    // 再現率はツイート数に応じて決める
    // Sigmoid を利用，MEAN_RAIN_COUNT でだいたい 0.9 になるようにする
    double Re = 2. / (1. + exp(-info.count * 3. / MEAN_RAIN_COUNT)) - 1.;

    auto population = load_population();

    auto values = decode(in, info);
    auto merged_values = decode(merged, merged_info);

    unordered_map<int, double> res;
    for (auto item : population) {
        if (values[item.first] == 0 || merged_values[item.first] == 0) continue;
        int m = merged_info.get_value(merged_values[item.first]);
        if (m == 0) continue;
        int Tpp = info.get_value(values[item.first]);
        int Tpn = info.count - Tpp;
        int Tnp = m - Tpp;
        int Tnn = DATA_COUNT - Tpp - Tpn - Tnp;
        double r = (double)m / DATA_COUNT;
        double alpha = r / (1 - r) * (1 - Pr) / Pr * Re;
        res[item.first] = Tpp * log(Re) + Tnp * log(1 - Re) + Tpn * log(alpha) + Tnn * log(1 - alpha) + log(item.second);
    }
    return res;
}

// 指定した地点の雨量を取得する
double get_value(std::ifstream &in, const Info &info, double latitude, double longitude, double radius) {
    auto pixel = get_pixel(latitude, longitude);
    if (!is_in(pixel.first, pixel.second)) {
        return -2;
    }
    int index = get_index(pixel);
    int start_index = index - max(0, (int)ceilf(radius) * (WIDTH + 1)) - 1;
    int end_index = index + max(0, (int)ceilf(radius) * (WIDTH + 1)) + 1;

    int in_bytes = info.bits / 8;
    int read_bytes = 0;
    int cur_index = 0;

    double ans = 0;
    int count = 0;

    Segment seg;
    int seg_len = 0;
    while (!in.eof()) {
        int val = read(in, in_bytes);
        read_bytes += in_bytes;

        auto new_seg = decode(info, seg, seg_len, val);
        if (new_seg.length) {
            seg_len = 0;
            if (seg.length) {
                cur_index += seg.length;
                if (seg.value != 0 && cur_index > start_index) {
                    int c = in_radius_count(cur_index - seg.length, cur_index, index, radius);
                    ans += info.get_value(seg.value) * c;
                    count += c;
                }
                if (cur_index > end_index) break;
            }
            seg = new_seg;
        } else {
            seg_len++;
        }

        if (read_bytes >= info.data_length) break;
    }

    if (cur_index <= end_index && seg.length) {
        cur_index += seg.length;
        if (seg.value != 0 && cur_index > start_index) {
            int c = in_radius_count(cur_index - seg.length, cur_index, index, radius);
            ans += info.get_value(seg.value) * c;
            count += c;
        }
    }

    if (count == 0) return -1;
    return ans / count / pow_int(10, info.E);
}

// 指定した複数地点の雨量を取得する
void get_values(std::ifstream &in, std::ifstream &coord, std::ofstream &out, const Info &info) {
    // 座標のリストを取得
    vector<int> indices;
    string line;
    double lat, lon;
    while (getline(coord, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        ss >> lat >> lon;
        auto pixel = get_pixel(lat, lon);
        if (!is_in(pixel.first, pixel.second)) {
            indices.push_back(-1);
        } else {
            indices.push_back(get_index(pixel));
        }
    }

    if (indices.empty()) return;

    auto indices_set = indices;
    sort(indices_set.begin(), indices_set.end());
    indices_set.erase(unique(indices_set.begin(), indices_set.end()), indices_set.end());
    size_t cur_set_ind = indices_set[0] == -1 ? 1 : 0;

    unordered_map<int, double> res;

    int in_bytes = info.bits / 8;
    int read_bytes = 0;
    int cur_index = 0;

    Segment seg;
    int seg_len = 0;
    while (!in.eof()) {
        if (cur_set_ind >= indices_set.size()) break;

        int val = read(in, in_bytes);
        read_bytes += in_bytes;

        auto new_seg = decode(info, seg, seg_len, val);
        if (new_seg.length) {
            seg_len = 0;
            if (seg.length) {
                cur_index += seg.length;
                while (cur_index > indices_set[cur_set_ind]) {
                    res[indices_set[cur_set_ind]] = seg.value == 0 ? -1 : (info.get_value(seg.value) / pow_int(10, info.E));
                    if (++cur_set_ind >= indices_set.size()) break;
                }
            }
            seg = new_seg;
        } else {
            seg_len++;
        }

        if (read_bytes >= info.data_length) break;
    }

    if (cur_set_ind < indices_set.size() && seg.length) {
        cur_index += seg.length;
        while (cur_index > indices_set[cur_set_ind]) {
            res[indices_set[cur_set_ind]] = seg.value == 0 ? -1 : (info.get_value(seg.value) / pow_int(10, info.E));
            if (++cur_set_ind >= indices_set.size()) break;
        }
    }

    for (auto index : indices) {
        out << (res.count(index) ? res[index] : -2) << '\n';
    }
}

// ランキングを表示する
std::vector<std::pair<int, double>> get_ranking(std::ifstream &in, const Info &info, int count, double radius, double distance, bool filter_zero) {
    auto population = load_population();

    // 全ての点の値を計算する
    vector<int> values(SIZE, -1);

    int in_bytes = info.bits / 8;
    int read_bytes = 0;
    int index = 0;

    Segment seg;
    int seg_len = 0;
    while (!in.eof()) {
        int val = read(in, in_bytes);
        read_bytes += in_bytes;

        auto new_seg = decode(info, seg, seg_len, val);
        if (new_seg.length) {
            seg_len = 0;
            if (seg.length) {
                if (seg.value) {
                    int v = info.get_value(seg.value);
                    for (int i = 0; i < seg.length; i++, index++) {
                        values[index] = v;
                    }
                } else {
                    index += seg.length;
                }
            }
            seg = new_seg;
        } else {
            seg_len++;
        }

        if (read_bytes >= info.data_length) break;
    }

    if (seg.length) {
        if (seg.value) {
            int v = info.get_value(seg.value);
            for (int i = 0; i < seg.length; i++, index++) {
                values[index] = v;
            }
        } else {
            index += seg.length;
        }
    }

    // 半径 radius 以内の全ての点について，オフセットを求める
    vector<pair<int, int>> offset;
    int r = (int)ceilf(radius);
    for (int x = -r; x <= r; x++) {
        for (int y = -r; y <= r; y++) {
            if (x * x + y * y <= radius * radius) {
                offset.emplace_back(x, y);
            }
        }
    }

    if (offset.empty()) {
        offset.emplace_back(0, 0);
    }

    // 周囲の点との平均をとる
    vector<pair<int, double>> average;
    for (int index = 0; index < SIZE; index++) {
        if (values[index] == -1) continue;
        auto pixel = get_pixel(index);
        int sum = 0, c = 0;
        for (auto d : offset) {
            if (!is_in(pixel.first + d.first, pixel.second + d.second)) continue;
            auto v = values[index + get_index(d)];
            if (v == -1) continue;
            sum += v;
            c++;
        }
        average.emplace_back(index, (double)sum / c);
    }
    sort(average.begin(), average.end(), [](pair<int, double> a, pair<int, double> b) {
        if (a.second == b.second) return a.first < b.first;
        else return a.second > b.second;
    });

    vector<pair<int, double>> result;

    // 順に取り出す
    for (auto item : average) {
        if (filter_zero && (!population.count(item.first) || population[item.first] == 0)) {
            continue;
        }
        // result にある点から距離 distance 以内にないか
        auto p = get_pixel(item.first);
        bool exist = false;
        for (auto r : result) {
            if (dist2(p, get_pixel(r.first)) <= distance * distance) {
                exist = true;
                break;
            }
        }
        if (exist) continue;
        result.emplace_back(item.first, item.second / pow_int(10, info.E));
        if ((int)result.size() >= count) break;
    }

    return result;
}

std::vector<std::pair<int, double>> get_prob_ranking(std::ifstream &in, std::ifstream &merged, const Info &info, const Info &merged_info, int count, double distance) {
    auto logp = get_prob(in, merged, info, merged_info);
    vector<pair<int, double>> cpy(logp.begin(), logp.end());
    sort(cpy.begin(), cpy.end(), [](const pair<int, double> &a, const pair<int, double> b) {
        if (a.second == b.second) return a.first < b.first;
        else return a.second > b.second;
    });

    vector<pair<int, double>> result;

    // 順に取り出す
    for (auto item : cpy) {
        // result にある点から距離 distance 以内にないか
        auto p = get_pixel(item.first);
        bool exist = false;
        for (auto r : result) {
            if (dist2(p, get_pixel(r.first)) <= distance * distance) {
                exist = true;
                break;
            }
        }
        if (exist) continue;
        result.emplace_back(item.first, item.second);
        if ((int)result.size() >= count) break;
    }

    return result;
}

/// @brief データの値を変換する
/// @param f レベルの値から変換する関数
void convert(std::ifstream &in, std::ofstream &out, const Info &in_info, Info &out_info, const std::function<int(int)> &f) {
    int in_bytes = in_info.bits / 8;
    int out_bytes = out_info.bits / 8;
    int read_bytes = 0;
    int write_bytes = 0;

    Segment seg, out_seg;
    int seg_len = 0;
    while (!in.eof()) {
        int val = read(in, in_bytes);
        read_bytes += in_bytes;

        auto new_seg = decode(in_info, seg, seg_len, val);
        if (new_seg.length) {
            seg_len = 0;
            if (seg.length) {
                seg.value = f(seg.value);
                if (out_seg.length) {
                    if (out_seg.value == seg.value) {
                        out_seg.length += seg.length;
                    } else {
                        // out_seg を書き出す
                        auto res = encode(out_info, out_seg);
                        for (auto i : res) {
                            write(out, out_bytes, i);
                        }
                        write_bytes += out_bytes * res.size();
                        out_seg = seg;
                    }
                } else {
                    out_seg = seg;
                }
            }
            seg = new_seg;
        } else {
            seg_len++;
        }

        if (read_bytes >= in_info.data_length) break;
    }

    if (seg.length) {
        seg.value = f(seg.value);
        if (out_seg.length) {
            if (out_seg.value == seg.value) {
                out_seg.length += seg.length;
            } else {
                // out_seg を書き出す
                auto res = encode(out_info, out_seg);
                for (auto i : res) {
                    write(out, out_bytes, i);
                }
                write_bytes += out_bytes * res.size();
                out_seg = seg;
            }
        } else {
            out_seg = seg;
        }
    }
    if (out_seg.length) {
        // out_seg を書き出す
        auto res = encode(out_info, out_seg);
        for (auto i : res) {
            write(out, out_bytes, i);
        }
        write_bytes += out_bytes * res.size();
    }
    out_info.data_length = write_bytes;

    // 第 8 節
    write(out, 4, 0x37373737);
}

/// @brief ファイルをマージした際の情報を取得し，out_info に格納する
void merge_search(std::vector<std::ifstream *> &in, const std::vector<Info> &in_info, Info &out_info, const std::function<int(int, int)> &f) {
    vector<int> in_bytes, read_bytes(2);
    for (int i = 0; i < 2; i++) {
        in_bytes.push_back(in_info[i].bits / 8);
    }

    int value_max = 0;

    vector<Segment> seg(2);
    vector<int> seg_len(2), pos(2);
    while (true) {
        int r = pos[0] <= pos[1] ? 0 : 1;
        if (in[r]->eof()) break;
        read_bytes[r] += in_bytes[r];

        int val = read(*in[r], in_bytes[r]);
        pos[r] -= seg[r].length;
        auto new_seg = decode(in_info[r], seg[r], seg_len[r], val);
        pos[r] += seg[r].length + new_seg.length;
        if (new_seg.length) {
            seg_len[r] = 0;
            if (seg[1 - r].length) {
                int v = f(new_seg.value, seg[1 - r].value);
                if (v > value_max) value_max = v;
            }
            seg[r] = new_seg;
        } else {
            seg_len[r]++;
        }

        if (read_bytes[0] >= in_info[0].data_length && read_bytes[1] >= in_info[1].data_length) {
            break;
        }
    }

    out_info.point_count = in_info[0].point_count;
    out_info.M = out_info.V = value_max;
    out_info.E = 0;
    out_info.count = in_info[0].count + in_info[1].count;
    out_info.bits = 32;
    for (int i = 8; i < 32; i += 8) {
        if (value_max < (1 << (i - 1))) {
            out_info.bits = i;
            break;
        }
    }
    out_info.value_is_level_1 = true;
}

void merge(std::vector<std::ifstream *> &in, std::ofstream &out, const std::vector<Info> &in_info, Info &out_info, const std::function<int(int, int)> &f) {
    vector<int> in_bytes, read_bytes(2);
    for (int i = 0; i < 2; i++) {
        in_bytes.push_back(in_info[i].bits / 8);
    }
    int out_bytes = out_info.bits / 8;
    int write_bytes = 0;

    vector<Segment> seg(2);
    Segment out_seg;
    vector<int> seg_len(2), pos(2);
    while (true) {
        int r = pos[0] <= pos[1] ? 0 : 1;
        if (in[r]->eof()) break;
        read_bytes[r] += in_bytes[r];

        int val = read(*in[r], in_bytes[r]);
        int old_pos = pos[r];
        pos[r] -= seg[r].length;
        auto new_seg = decode(in_info[r], seg[r], seg_len[r], val);
        pos[r] += seg[r].length + new_seg.length;
        if (new_seg.length) {
            seg_len[r] = 0;
            seg[r] = new_seg;
        } else {
            seg_len[r]++;
        }
        if (pos[r] >= pos[1 - r]) {
            // old_pos から pos[1 - r] まで
            int v = f(seg[r].value, seg[1 - r].value);
            if (out_seg.length) {
                if (out_seg.value == v) {
                    out_seg.length += pos[1 - r] - old_pos;
                } else {
                    // out_seg を書き出す
                    auto res = encode(out_info, out_seg);
                    for (auto i : res) {
                        write(out, out_bytes, i);
                    }
                    write_bytes += out_bytes * res.size();
                    out_seg.value = v;
                    out_seg.length = pos[1 - r] - old_pos;
                }
            } else {
                out_seg.value = v;
                out_seg.length = pos[1 - r] - old_pos;
            }
        } else {
            // old_pos から pos[r] まで
            int v = f(seg[r].value, seg[1 - r].value);
            if (out_seg.value == v) {
                out_seg.length += pos[r] - old_pos;
            } else {
                // out_seg を書き出す
                auto res = encode(out_info, out_seg);
                for (auto i : res) {
                    write(out, out_bytes, i);
                }
                write_bytes += out_bytes * res.size();
                out_seg.value = v;
                out_seg.length = pos[r] - old_pos;
            }
        }

        if (read_bytes[0] >= in_info[0].data_length && read_bytes[1] >= in_info[1].data_length) {
            break;
        }
    }
    if (out_seg.length) {
        // out_seg を書き出す
        auto res = encode(out_info, out_seg);
        for (auto i : res) {
            write(out, out_bytes, i);
        }
        write_bytes += out_bytes * res.size();
    }
    out_info.data_length = write_bytes;

    // 第 8 節
    write(out, 4, 0x37373737);
}

void create_image(std::ifstream &in, std::string out_file, const Info &info, const std::function<Color(int)> &color, const std::vector<std::pair<double, double>> &pos, const std::function<Color(int)> &pos_color, bool filter_zero) {
    auto population = load_population();

    auto img = new unsigned char[SIZE * 3];
    int index = 0;

    int in_bytes = info.bits / 8;
    int read_bytes = 0;

    auto empty_color = color(0);

    Segment seg;
    int seg_len = 0;
    while (!in.eof()) {
        int val = read(in, in_bytes);
        read_bytes += in_bytes;

        auto new_seg = decode(info, seg, seg_len, val);
        if (new_seg.length) {
            seg_len = 0;
            if (seg.length) {
                // seg を書き出す
                auto c = color(seg.value);
                for (int i = 0; i < seg.length; i++) {
                    if (filter_zero && (!population.count(index) || population[index] == 0)) {
                        set_color(img, get_pixel(index), empty_color);
                    } else {
                        set_color(img, get_pixel(index), c);
                    }
                    index++;
                }
            }
            seg = new_seg;
        } else {
            seg_len++;
        }

        if (read_bytes >= info.data_length) break;
    }

    if (seg.length) {
        // seg を書き出す
        auto c = color(seg.value);
        for (int i = 0; i < seg.length; i++) {
            set_color(img, get_pixel(index), c);
            index++;
        }
    }

    draw_coastline(img);
    for (size_t i = 0; i < pos.size(); i++) {
        draw_location(img, pos[i].first, pos[i].second, pos_color(i));
    }

    save_bitmap(img, out_file, WIDTH, HEIGHT);
    delete[] img;
}

// 各地点にユーザーがいる確率を計算し，画像で出力する
void create_prob_image(std::ifstream &in, std::ifstream &merged, std::string out_file, const Info &info, const Info &merged_info, const std::function<Color(double)> &color, const std::vector<std::pair<double, double>> &pos, const std::function<Color(int)> &pos_color) {
    auto logp = get_prob(in, merged, info, merged_info);

    vector<double> cpy;
    for (auto item : logp) cpy.push_back(item.second);
    auto max_logp = *max_element(cpy.begin(), cpy.end());

    // 下位 20 % の値を取得
    int n = cpy.size() * 4 / 5;
    nth_element(cpy.begin(), cpy.begin() + n, cpy.end(), greater<double>());
    double nth = cpy[n];

    auto img = new unsigned char[SIZE * 3];

    for (int i = 0; i < SIZE; i++) {
        // 最小値が 0, 最大値が 1 になるように変換
        // 最大値に近いところほど大きく変化するようにする
        double v = -1;
        if (logp.count(i)) {
            // 線形変換
            v = max(0., (logp[i] - nth) / (max_logp - nth));
        }
        auto c = color(v);
        set_color(img, get_pixel(i), c);
    }

    draw_coastline(img);
    for (size_t i = 0; i < pos.size(); i++) {
        draw_location(img, pos[i].first, pos[i].second, pos_color(i));
    }

    save_bitmap(img, out_file, WIDTH, HEIGHT);
    delete[] img;
}

unordered_map<int, double> get_normalized_prob(std::ifstream &in, std::ifstream &merged, const Info &info, const Info &merged_info) {
    auto logp = get_prob(in, merged, info, merged_info);
    auto max_logp = max_element(logp.begin(), logp.end(), [](const pair<int, double> &a, const pair<int, double> &b) {
                        return a.second < b.second;
                    })->second;
    unordered_map<int, double> p;
    double sum = 0;
    for (auto item : logp) {
        p[item.first] = exp(item.second - max_logp);
        sum += p[item.first];
    }
    for (auto item : p) {
        p[item.first] /= sum;
    }
    return p;
}

double get_cae(std::ifstream &in, std::ifstream &merged, const Info &info, const Info &merged_info, double latitude, double longitude) {
    auto p = get_normalized_prob(in, merged, info, merged_info);
    double res = 0;
    for (auto item : p) {
        auto coord = get_coord(get_pixel(item.first));
        res += haversine(latitude, longitude, coord.first, coord.second) * p[item.first];
    }
    return res;
}

double get_pra(std::ifstream &in, std::ifstream &merged, const Info &info, const Info &merged_info, double percentage) {
    percentage /= 100;
    auto p = get_normalized_prob(in, merged, info, merged_info);
    vector<double> cpy;
    for (auto item : p) cpy.push_back(item.second);
    sort(cpy.begin(), cpy.end(), greater<double>());
    double res = 0, sum = 0;
    for (auto item : cpy) {
        sum += item;
        res += 1;
        if (sum >= percentage) break;
    }
    return res;
}
