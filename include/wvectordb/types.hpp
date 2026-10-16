#pragma once

#include <cstdint>
#include <vector>

namespace wvectordb {

using VectorId = std::uint64_t;
using Vector = std::vector<float>;

struct SearchResult {
    VectorId id{};
    float distance{};
};

}  // namespace wvectordb
