#include "io.hpp"
#include <fstream>
#include <iostream>

using namespace std;

std::unordered_map<int, int> load_population() {
    string population_file = "/home/yamashita/disk02/geo/population/population.csv";
    ifstream ifs(population_file, ios::in);
    if (!ifs) {
        cout << "人口ファイルが開けませんでした: " << population_file << endl;
        return {};
    }

    string line;
    // ヘッダ
    if (!getline(ifs, line)) return {};

    unordered_map<int, int> res;
    while (getline(ifs, line)) {
        auto pos = line.find(',');
        int code = stoi(line.substr(0, pos));
        int population = stoi(line.substr(pos + 1));
        res[code] = population;
    }

    return res;
}
