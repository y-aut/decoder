#pragma once
#include <unordered_map>

// 地球の半径（キロメートル）
const double EARTH_RADIUS_KM = 6371.0088;

// 人口が最も集中している箇所の人口の総和を求める
int get_highest_density(const std::unordered_map<int, int> &population, float radius);

// 緯度経度から距離を求める
double haversine(double lat1, double lon1, double lat2, double lon2);
