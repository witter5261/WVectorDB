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
    void insert_batch(const std::vector<VectorEntry>& entries);
    bool erase(VectorId id) noexcept;
    std::vector<SearchResult> search(const Vector& query, std::size_t k) const;
    std::vector<std::vector<SearchResult>> search_batch(
        const std::vector<Vector>& queries,
        std::size_t k) const;

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
