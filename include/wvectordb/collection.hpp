#pragma once

#include <cstddef>

namespace wvectordb {

class Collection {
public:
    explicit Collection(std::size_t dimension) noexcept;

    std::size_t dimension() const noexcept;
    std::size_t size() const noexcept;

private:
    std::size_t dimension_;
    std::size_t size_{0};
};

}  // namespace wvectordb

