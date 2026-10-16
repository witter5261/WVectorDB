#include "wvectordb/collection.hpp"

namespace wvectordb {

Collection::Collection(std::size_t dimension) noexcept
    : dimension_(dimension) {}

std::size_t Collection::dimension() const noexcept {
    return dimension_;
}

std::size_t Collection::size() const noexcept {
    return size_;
}

}  // namespace wvectordb
