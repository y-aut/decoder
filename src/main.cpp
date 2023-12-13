#include "color.hpp"
#include "process.hpp"
#include "util.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>

using namespace std;

void show_usage() {
    cout << R"(*** USAGE ***

value [-r RADIUS] FILENAME LATITUDE LONGITUDE
    指定した地点の雨量を取得する
    半径 (px) を指定したときは，中心からの距離がそれ以下の点について平均をとる

rank [-n COUNT] [-r RADIUS] [-d DISTANCE] FILENAME
    値が高い地点を順に指定した数だけ出力する
    指定した距離 (px) 以内の 2 点はランキング中に含まれない

convert [-t THRESHOLD] [-o OUT_FILE] FILENAME
    ファイルを変換する

merge [-o OUT_FILE] FILENAME1 FILENAME2
    ファイルをマージする

image [-o OUT_FILE] [-l LATITUDE LONGITUDE] [-c] FILENAME
    ビットマップを作成する
    -l を指定すると，指定した地点に印をつける．複数指定可能
    -c を指定すると，降水量に基づいた色分けを行う
)"
         << endl;
}

int value_cmd(queue<string> &args) {
    string in_file;
    float latitude = 0, longitude = 0, radius = 0;

    // コマンドライン引数をパース
    while (!args.empty()) {
        if (args.front() == "-r") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            radius = stof(args.front());
        } else {
            if (!in_file.empty() || args.size() < 3) {
                show_usage();
                return 1;
            }
            in_file = args.front();
            args.pop();
            latitude = stof(args.front());
            args.pop();
            longitude = stof(args.front());
        }
        args.pop();
    }

    if (in_file.empty()) {
        cout << "入力ファイルが指定されていません" << endl;
        return 1;
    }

    ifstream in(in_file, ios::in | ios::binary);
    if (!in) {
        cout << "入力ファイルが開けませんでした: " << in_file << endl;
        return 1;
    }

    Info info;
    read_info(in, info);

    cout << get_value(in, info, latitude, longitude, radius) << endl;

    return 0;
}

int rank_cmd(queue<string> &args) {
    string in_file;
    int count = 5;
    float radius = 0, distance = 0;

    // コマンドライン引数をパース
    while (!args.empty()) {
        if (args.front() == "-c") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            count = stoi(args.front());
        } else if (args.front() == "-r") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            radius = stof(args.front());
        } else if (args.front() == "-d") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            distance = stof(args.front());
        } else {
            if (!in_file.empty()) {
                show_usage();
                return 1;
            }
            in_file = args.front();
        }
        args.pop();
    }

    if (in_file.empty()) {
        cout << "入力ファイルが指定されていません" << endl;
        return 1;
    }

    ifstream in(in_file, ios::in | ios::binary);
    if (!in) {
        cout << "入力ファイルが開けませんでした: " << in_file << endl;
        return 1;
    }

    Info info;
    read_info(in, info);

    auto ranking = get_ranking(in, info, count, radius, distance);
    for (auto item : ranking) {
        auto coord = get_coord(item.first % WIDTH, item.first / WIDTH);
        cout << coord.first << " " << coord.second << " " << item.second << endl;
    }

    return 0;
}

int convert_cmd(queue<string> &args) {
    string in_file, out_file = "out.bin";
    int threshold = 2;

    // コマンドライン引数をパース
    while (!args.empty()) {
        if (args.front() == "-t") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            threshold = stoi(args.front());
        } else if (args.front() == "-o") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            out_file = args.front();
        } else {
            if (!in_file.empty()) {
                show_usage();
                return 1;
            }
            in_file = args.front();
        }
        args.pop();
    }

    if (in_file.empty()) {
        cout << "入力ファイルが指定されていません" << endl;
        return 1;
    }

    ifstream in(in_file, ios::in | ios::binary);
    if (!in) {
        cout << "入力ファイルが開けませんでした: " << in_file << endl;
        return 1;
    }

    ofstream out(out_file, ios::out | ios::binary | ios::trunc);
    if (!out) {
        cout << "出力ファイルが開けませんでした: " << out_file << endl;
        return 1;
    }

    Info in_info, out_info;
    read_info(in, in_info);

    auto f = [&](int lv) {
        if (lv == 0) return 0;
        return lv >= threshold ? 2 : 1;
    };

    out_info.point_count = in_info.point_count;
    out_info.bits = 8;
    out_info.V = 2;
    out_info.M = 2;
    out_info.E = 0;
    out_info.value_is_level_1 = true;
    write_info(out, out_info);

    convert(in, out, in_info, out_info, f);

    out.seekp(ios_base::beg);
    write_info(out, out_info);

    out.close();
    in.close();

    cout << "変換後のファイルを出力しました: " << out_file << endl;

    return 0;
}

int merge_cmd(queue<string> &args) {
    vector<string> in_files;
    string out_file = "out.bin";

    // コマンドライン引数をパース
    while (!args.empty()) {
        if (args.front() == "-o") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            out_file = args.front();
        } else {
            if (in_files.size() == 2) {
                show_usage();
                return 1;
            }
            in_files.push_back(args.front());
        }
        args.pop();
    }

    if (in_files.size() < 2) {
        cout << "入力ファイルの数が少なすぎます" << endl;
        return 1;
    }

    vector<ifstream *> in;
    for (int i = 0; i < 2; i++) {
        auto ptr = new ifstream(in_files[i], ios::in | ios::binary);
        in.push_back(ptr);
        if (!*ptr) {
            cout << "入力ファイルが開けませんでした: " << in_files[i] << endl;
            return 1;
        }
    }

    ofstream out(out_file, ios::out | ios::binary | ios::trunc);
    if (!out) {
        cout << "出力ファイルが開けませんでした: " << out_file << endl;
        return 1;
    }

    vector<Info> in_info(2);
    // 位置を保存しておく
    vector<streampos> in_pos;
    for (int i = 0; i < 2; i++) {
        read_info(*in[i], in_info[i]);
        in_pos.push_back(in[i]->tellg());
    }

    auto f = [&](int lv1, int lv2) {
        if (lv1 == 0 || lv2 == 0) return lv1 + lv2;
        return max(0, lv1 + lv2 - 1);
    };

    Info out_info;
    merge_search(in, in_info, out_info, f);
    write_info(out, out_info);

    for (int i = 0; i < 2; i++) {
        in[i]->seekg(in_pos[i]);
    }
    merge(in, out, in_info, out_info, f);

    out.seekp(ios_base::beg);
    write_info(out, out_info);

    out.close();
    for (int i = 0; i < 2; i++) {
        in[i]->close();
        delete in[i];
    }

    cout << "マージ後のファイルを出力しました: " << out_file << endl;

    return 0;
}

int image_cmd(queue<string> &args) {
    string in_file, out_file = "out.bmp";
    vector<pair<float, float>> pos;
    bool rainfall_color = false;

    // コマンドライン引数をパース
    while (!args.empty()) {
        if (args.front() == "-o") {
            args.pop();
            if (args.empty()) {
                show_usage();
                return 1;
            }
            out_file = args.front();
        } else if (args.front() == "-l") {
            args.pop();
            if (args.size() <= 1) {
                show_usage();
                return 1;
            }
            float lat = stof(args.front());
            args.pop();
            float lon = stof(args.front());
            pos.emplace_back(lat, lon);
        } else if (args.front() == "-c") {
            rainfall_color = true;
        } else {
            if (!in_file.empty()) {
                show_usage();
                return 1;
            }
            in_file = args.front();
        }
        args.pop();
    }

    if (in_file.empty()) {
        cout << "入力ファイルが指定されていません" << endl;
        return 1;
    }

    ifstream in(in_file, ios::in | ios::binary);
    if (!in) {
        cout << "入力ファイルが開けませんでした: " << in_file << endl;
        return 1;
    }

    Info info;
    read_info(in, info);

    function<Color(int)> f;
    if (rainfall_color) {
        // 降水量レベルを色に変換する
        int scale = pow_int(10, info.E);
        f = [&](int lv) {
            if (lv == 0) return Color("C0C0C0");
            int value = info.get_value(lv);
            if (value == 0) return Color("FFFFFF");
            else if (value < scale) return Color("0000FF");
            else if (value < scale * 10) return Color("00FFFF");
            else if (value < scale * 20) return Color("FFFF00");
            else if (value < scale * 40) return Color("00FF00");
            else if (value < scale * 60) return Color("FF8000");
            else return Color("FF0000");
        };
    } else {
        // グラデーションで色付け
        f = [&](int lv) {
            if (lv == 0) return Color("C0C0C0");
            if (lv == 1) return Color("FFFFFF");
            float r = info.V == 2 ? 0 : (float)(lv - 2) / (info.V - 2);
            return Color::from_hsl(240 * (1 - r), 100, 50);
        };
    }

    create_image(in, out_file, info, f, pos);
    in.close();

    cout << "画像ファイルを出力しました: " << out_file << endl;

    return 0;
}

int main(int argc, char *argv[]) {
    cout << setprecision(10);

    queue<string> args;
    for (int i = 1; i < argc; i++) {
        args.push(argv[i]);
    }

    if (args.front() == "value") {
        args.pop();
        return value_cmd(args);
    } else if (args.front() == "rank") {
        args.pop();
        return rank_cmd(args);
    } else if (args.front() == "convert") {
        args.pop();
        return convert_cmd(args);
    } else if (args.front() == "merge") {
        args.pop();
        return merge_cmd(args);
    } else if (args.front() == "image") {
        args.pop();
        return image_cmd(args);
    } else {
        show_usage();
        return 1;
    }
}
