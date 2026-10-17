#include "wvectordb/collection.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

float squared_l2_distance(const wvectordb::Vector& left,
                          const wvectordb::Vector& right) {
    double distance = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        const double difference = static_cast<double>(left[index]) -
                                  static_cast<double>(right[index]);
        distance += difference * difference;
    }

    if (!std::isfinite(distance) ||
        distance > static_cast<double>(std::numeric_limits<float>::max())) {
        throw std::overflow_error(
            "squared L2 distance is not representable as float");
    }
    return static_cast<float>(distance);
}

}  // namespace

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
        // Exactly one record was appended in this call. On failure, remove
        // that record and rethrow the original error.
        records_.pop_back();
        throw;
    }
}

void Collection::insert_batch(const std::vector<VectorEntry>& entries) {
    if (entries.empty()) {
        return;
    }

    // Prepare all input before changing either container. A later duplicate
    // ID replaces an earlier entry in the same batch.
    std::vector<VectorEntry> prepared_entries;
    prepared_entries.reserve(entries.size());
    for (const VectorEntry& entry : entries) {
        validate_vector(entry.vector);
        prepared_entries.push_back(VectorEntry{entry.id, entry.vector});
    }

    std::unordered_map<VectorId, std::size_t> last_occurrences;
    last_occurrences.reserve(prepared_entries.size());
    for (std::size_t index = 0; index < prepared_entries.size(); ++index) {
        last_occurrences[prepared_entries[index].id] = index;
    }

    std::vector<std::size_t> final_entry_indices;
    final_entry_indices.reserve(last_occurrences.size());
    for (std::size_t index = 0; index < prepared_entries.size(); ++index) {
        if (last_occurrences.at(prepared_entries[index].id) == index) {
            final_entry_indices.push_back(index);
        }
    }

    // Only existing records that will be replaced need backups. All possible
    // allocations happen before mutation, so the mutation phase can roll back
    // with non-throwing vector moves, map erases, and pop operations.
    std::vector<std::pair<std::size_t, Vector>> backups;
    backups.reserve(final_entry_indices.size());
    std::size_t new_record_count = 0;
    for (const std::size_t index : final_entry_indices) {
        const auto position = positions_.find(prepared_entries[index].id);
        if (position == positions_.end()) {
            ++new_record_count;
        } else {
            backups.emplace_back(position->second,
                                 records_[position->second].vector);
        }
    }

    const std::size_t original_record_count = records_.size();
    records_.reserve(original_record_count + new_record_count);
    positions_.reserve(positions_.size() + new_record_count);

    std::vector<VectorId> inserted_ids;
    inserted_ids.reserve(new_record_count);
    try {
        for (const std::size_t index : final_entry_indices) {
            const VectorEntry& entry = prepared_entries[index];
            const auto existing = positions_.find(entry.id);
            if (existing != positions_.end()) {
                records_[existing->second].vector =
                    std::move(prepared_entries[index].vector);
                continue;
            }

            const std::size_t new_index = records_.size();
            records_.push_back(
                Record{entry.id, std::move(prepared_entries[index].vector)});
            const auto [iterator, inserted] =
                positions_.emplace(entry.id, new_index);
            static_cast<void>(iterator);
            if (!inserted) {
                throw std::logic_error("batch ID index insertion failed");
            }
            inserted_ids.push_back(entry.id);
        }
    } catch (...) {
        for (const VectorId inserted_id : inserted_ids) {
            positions_.erase(inserted_id);
        }
        records_.resize(original_record_count);
        for (auto& backup : backups) {
            records_[backup.first].vector = std::move(backup.second);
        }
        throw;
    }
}

bool Collection::erase(VectorId id) noexcept {
    const auto position = positions_.find(id);
    if (position == positions_.end()) {
        return false;
    }

    const std::size_t removed_index = position->second;
    const std::size_t last_index = records_.size() - 1;

    if (removed_index != last_index) {
        const VectorId moved_id = records_.back().id;
        records_[removed_index] = std::move(records_.back());

        const auto moved_position = positions_.find(moved_id);
        assert(moved_position != positions_.end());
        moved_position->second = removed_index;
    }

    records_.pop_back();
    positions_.erase(position);
    return true;
}

std::vector<SearchResult> Collection::search(const Vector& query,
                                             std::size_t k) const {
    validate_vector(query);
    if (k == 0 || records_.empty()) {
        return {};
    }

    std::vector<SearchResult> results;
    results.reserve(records_.size());
    for (const Record& record : records_) {
        results.push_back(SearchResult{
            record.id, squared_l2_distance(query, record.vector)});
    }

    const auto compare = [](const SearchResult& left,
                            const SearchResult& right) {
        if (left.distance != right.distance) {
            return left.distance < right.distance;
        }
        return left.id < right.id;
    };

    if (k < results.size()) {
        std::partial_sort(results.begin(), results.begin() + k,
                          results.end(), compare);
    } else {
        std::sort(results.begin(), results.end(), compare);
    }

    if (k < results.size()) {
        results.resize(k);
    }
    return results;
}

std::vector<std::vector<SearchResult>> Collection::search_batch(
    const std::vector<Vector>& queries,
    std::size_t k) const {
    std::vector<std::vector<SearchResult>> batch_results;
    batch_results.reserve(queries.size());
    for (const Vector& query : queries) {
        batch_results.push_back(search(query, k));
    }
    return batch_results;
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
