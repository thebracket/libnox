#pragma once

#include "../components/position.hpp"
#include <tuple>
#include "constants.hpp"
#include "../noxtypes.h"

#define BOUNDS_CHECKING 1

/*
 * Calculate the array index of an x/y/z position.
 */
inline int mapidx(const int &x, const int &y, const int &z) noexcept {
    return (z * nf::REGION_HEIGHT * nf::REGION_WIDTH) + (y * nf::REGION_WIDTH) + x;
}

/*
 * Calculate the array index of a position component
 */
inline int mapidx(const position_t &pos) noexcept {
    return mapidx(static_cast<int>(pos.x), static_cast<int>(pos.y), pos.z);
}

/*
 * Reverse an array index back into an x/y/z position
 */
inline std::tuple<int,int,int> idxmap(int idx) noexcept {
    int z = idx / (nf::REGION_HEIGHT * nf::REGION_WIDTH);
    idx -= (z * nf::REGION_WIDTH * nf::REGION_HEIGHT);

    int y = idx / nf::REGION_WIDTH;
    idx -= (y * nf::REGION_WIDTH);

    int x = idx;

    return std::make_tuple(x,y,z);
}
