#pragma once

#include <cstdint>
#include <vector>

namespace wvectordb {

using VectorId = std::uint64_t;
using Vector = std::vector<float>;

struct VectorEntry {
    VectorId id{};
    Vector vector;
};

struct SearchResult {
    VectorId id{};
    float distance{};
};

}  // namespace wvectordb
