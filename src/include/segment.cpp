#include "segment.hpp"
#include "util.hpp"

using namespace std;

/// @brief データの値に応じて，直前のセグメントを更新する
/// @param seg 直前のセグメント
/// @param seg_len 直前のセグメントが現れてから何データ目か
/// @param v 次のデータの値
/// @return 新しいセグメント
Segment decode(const Info &info, Segment &seg, int seg_len, int v) {
    if (v <= info.V) return Segment{v, 1};
    v -= info.V + 1;
    int L = (1 << info.bits) - 1 - info.V;
    seg.length += v * pow_int(L, seg_len);
    return Segment();
}

/// @brief Segment をバイト列に変換する
std::vector<int> encode(const Info &info, const Segment &seg) {
    vector<int> res;
    if (seg.length == 0) return res;
    res.push_back(seg.value);
    int L = (1 << info.bits) - 1 - info.V;
    int length = seg.length - 1;
    while (length) {
        res.push_back(length % L + info.V + 1);
        length /= L;
    }
    return res;
}
