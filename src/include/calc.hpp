#pragma once
#include <unordered_map>

// 人口が最も集中している箇所の人口の総和を求める
int get_highest_density(const std::unordered_map<int, int> &population, float radius);
