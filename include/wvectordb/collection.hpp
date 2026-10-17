#pragma once

#include "wvectordb/types.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace wvectordb {

class Collection {
public:
    explicit Collection(std::size_t dimension);

    std::size_t dimension() const noexcept;
    std::size_t size() const noexcept;

    void insert(VectorId id, const Vector& vector);
    bool erase(VectorId id) noexcept;

private:
    struct Record {
        VectorId id;
        Vector vector;
    };

    void validate_vector(const Vector& vector) const;

    std::size_t dimension_;
    std::vector<Record> records_;
    std::unordered_map<VectorId, std::size_t> positions_;
};

}  // namespace wvectordb
