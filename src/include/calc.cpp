#include "calc.hpp"
#include "util.hpp"
#include <cmath>
#include <iostream>

int get_density(const std::unordered_map<int, int> &population, int index, float radius) {
    int ans = 0;
    int d = (int)radius + 2;
    radius *= radius;
    auto p = get_pixel(index);
    for (int dx = -d; dx <= d; dx++) {
        int x = p.first + dx;
        if (!(0 <= x && x < WIDTH)) continue;
        for (int dy = -d; dy <= d; dy++) {
            int y = p.second + dy;
            if (!(0 <= y && y < HEIGHT)) continue;
            int i = get_index({x, y});
            if (abs(dx) * abs(dx) + abs(dy) * abs(dy) <= radius && population.count(i)) {
                ans += population.at(i);
            }
        }
    }
    return ans;
}

int get_highest_density(const std::unordered_map<int, int> &population, float radius) {
    int ans = 0;
    int index = 0;
    int cnt = 0;
    for (auto item : population) {
        if (++cnt % 1000 == 0) {
            std::cout << cnt << " / " << population.size() << std::endl;
        }
        int v = get_density(population, item.first, radius);
        if (v > ans) {
            ans = v;
            index = item.first;
        }
    }
    auto p = get_pixel(index);
    std::cout << p.first << ", " << p.second << std::endl;
    return ans;
}

// 度からラジアンへ変換
double toRadians(double degree) {
    return degree * M_PI / 180.0;
}

double haversine(double lat1, double lon1, double lat2, double lon2) {
    double dLat = toRadians(lat2 - lat1);
    double dLon = toRadians(lon2 - lon1);

    lat1 = toRadians(lat1);
    lat2 = toRadians(lat2);

    double a = pow(sin(dLat / 2), 2) +
               cos(lat1) * cos(lat2) * pow(sin(dLon / 2), 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_KM * c;
}
