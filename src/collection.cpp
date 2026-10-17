#include "wvectordb/collection.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace wvectordb {

Collection::Collection(std::size_t dimension)
    : dimension_(dimension) {
    if (dimension_ == 0) {
        throw std::invalid_argument("collection dimension must be greater than zero");
    }
}

std::size_t Collection::dimension() const noexcept {
    return dimension_;
}

std::size_t Collection::size() const noexcept {
    return records_.size();
}

void Collection::insert(VectorId id, const Vector& vector) {
    validate_vector(vector);

    // Copy before changing either container. A copy failure leaves the
    // Collection untouched.
    Vector prepared_vector = vector;
    const auto existing = positions_.find(id);

    if (existing != positions_.end()) {
        // std::vector<float>'s move assignment is noexcept with its default
        // allocator, so the prepared value can replace the old value safely.
        records_[existing->second].vector = std::move(prepared_vector);
        return;
    }

    const std::size_t new_index = records_.size();
    records_.push_back(Record{id, std::move(prepared_vector)});

    try {
        const auto [iterator, inserted] = positions_.emplace(id, new_index);
        static_cast<void>(iterator);
        if (!inserted) {
            throw std::logic_error("vector ID index insertion failed");
        }
    } catch (...) {
        // Exactly one record was appended in this call. The map either has
        // no new entry or the standard insertion operation has completed.
        // On failure, remove the appended record and rethrow the error.
        records_.pop_back();
        throw;
    }
}

void Collection::validate_vector(const Vector& vector) const {
    if (vector.size() != dimension_) {
        throw std::invalid_argument("vector dimension does not match collection dimension");
    }

    for (const float value : vector) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("vector values must be finite");
        }
    }
}

}  // namespace wvectordb
