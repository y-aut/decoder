#pragma once
#include "process.hpp"
#include <vector>

struct Segment {
    int value = 0;
    int length = 0;
};

Segment decode(const Info &info, Segment &seg, int seg_len, int v);
std::vector<int> encode(const Info &info, const Segment &seg);
