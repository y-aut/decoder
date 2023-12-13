#include "process.hpp"
#include "bitmap.hpp"
#include "map.hpp"
#include "segment.hpp"
#include "util.hpp"
#include <math.h>
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
            info.V = read(in, 4);
            info.M = read(in, 4);
            info.E = read(in, 4);
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
    write(out, 4, info.value_is_level_1 ? 18 : 18 + 4 * info.M);
    write(out, 1, 15);
    write(out, 4, info.V);
    write(out, 4, info.M);
    write(out, 4, info.E);
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

// 区間 [a, b) と [c, d) にともに含まれる整数の個数を求める
int intersect(int a, int b, int c, int d) {
    if (a > c) return intersect(c, d, a, b);
    return max(0, min(b - c, d - c));
}

// 中心から radius 以内にある点の数
int in_radius_count(int old_index, int new_index, int center, float radius) {
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
        float x_dif = radius * radius - abs(y - center_y) * abs(y - center_y);
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

// 指定した地点の雨量を取得する
float get_value(std::ifstream &in, const Info &info, float latitude, float longitude, float radius) {
    auto pixel = get_pixel(latitude, longitude);
    if (!is_in(get<0>(pixel), get<1>(pixel))) {
        return -2;
    }
    int index = get<0>(pixel) + get<1>(pixel) * WIDTH;
    int start_index = index - max(0, (int)ceilf(radius) * (WIDTH + 1)) - 1;
    int end_index = index + max(0, (int)ceilf(radius) * (WIDTH + 1)) + 1;

    int in_bytes = info.bits / 8;
    int read_bytes = 0;
    int cur_index = 0;

    float ans = 0;
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

void create_image(std::ifstream &in, std::string out_file, const Info &info, const std::function<Color(int)> &color, float latitude, float longitude) {
    auto img = new unsigned char[WIDTH * HEIGHT * 3];
    int index = 0;

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
            if (seg.length) {
                // seg を書き出す
                auto c = color(seg.value);
                for (int i = 0; i < seg.length; i++) {
                    set_color(img, index % WIDTH, index / WIDTH, c);
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
            set_color(img, index % WIDTH, index / WIDTH, c);
            index++;
        }
    }

    draw_coastline(img);
    if (latitude != 0) {
        draw_location(img, latitude, longitude);
    }

    save_bitmap(img, out_file, WIDTH, HEIGHT);
    delete[] img;
}
