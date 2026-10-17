#include "wvectordb/collection.hpp"
#include "wvectordb/types.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

template <typename Function>
void expect_invalid_argument(Function&& function, const char* message) {
    try {
        function();
    } catch (const std::invalid_argument&) {
        return;
    } catch (...) {
        std::cerr << message << " (wrong exception type)\n";
        std::exit(1);
    }

    std::cerr << message << " (no exception)\n";
    std::exit(1);
}

}  // namespace

int main() {
    expect_invalid_argument(
        [] { wvectordb::Collection invalid(0); },
        "zero dimension was accepted");

    wvectordb::Collection collection(3);
    if (collection.dimension() != 3 || collection.size() != 0) {
        std::cerr << "initial Collection state is incorrect\n";
        return 1;
    }

    collection.insert(0, {1.0F, 2.0F, 3.0F});
    if (collection.size() != 1) {
        std::cerr << "first insert did not change size\n";
        return 1;
    }

    collection.insert(0, {4.0F, 5.0F, 6.0F});
    if (collection.size() != 1) {
        std::cerr << "duplicate ID created a second record\n";
        return 1;
    }

    collection.insert(7, {7.0F, 8.0F, 9.0F});
    if (collection.size() != 2) {
        std::cerr << "second ID did not create a record\n";
        return 1;
    }

    expect_invalid_argument(
        [&] { collection.insert(11, {1.0F, 2.0F}); },
        "wrong vector dimension was accepted");
    if (collection.size() != 2) {
        std::cerr << "failed new insert changed size\n";
        return 1;
    }

    expect_invalid_argument(
        [&] { collection.insert(0, {1.0F, 2.0F}); },
        "wrong dimension was accepted for an existing ID");
    if (collection.size() != 2) {
        std::cerr << "failed replacement changed size\n";
        return 1;
    }

    collection.insert(11, {10.0F, 11.0F, 12.0F});
    if (collection.size() != 3) {
        std::cerr << "a failed new insert left a stale ID mapping\n";
        return 1;
    }

    collection.insert(0, {13.0F, 14.0F, 15.0F});
    if (collection.size() != 3) {
        std::cerr << "a failed replacement lost the existing ID mapping\n";
        return 1;
    }

    expect_invalid_argument(
        [&] {
            collection.insert(
                11,
                {1.0F, std::numeric_limits<float>::quiet_NaN(), 3.0F});
        },
        "NaN vector value was accepted");
    if (collection.size() != 3) {
        std::cerr << "non-finite insert changed size\n";
        return 1;
    }

    wvectordb::Collection erase_collection(2);
    erase_collection.insert(10, {10.0F, 10.0F});
    erase_collection.insert(20, {20.0F, 20.0F});
    erase_collection.insert(30, {30.0F, 30.0F});
    erase_collection.insert(40, {40.0F, 40.0F});

    if (erase_collection.erase(999)) {
        std::cerr << "erasing a missing ID returned true\n";
        return 1;
    }
    if (erase_collection.size() != 4) {
        std::cerr << "missing erase changed size\n";
        return 1;
    }

    if (!erase_collection.erase(20) || erase_collection.size() != 3) {
        std::cerr << "middle erase failed\n";
        return 1;
    }
    erase_collection.insert(40, {41.0F, 41.0F});
    if (erase_collection.size() != 3) {
        std::cerr << "moved ID index was not updated after middle erase\n";
        return 1;
    }

    if (!erase_collection.erase(30) || erase_collection.size() != 2) {
        std::cerr << "last erase failed\n";
        return 1;
    }
    if (!erase_collection.erase(10) || erase_collection.size() != 1) {
        std::cerr << "first erase failed\n";
        return 1;
    }
    erase_collection.insert(40, {42.0F, 42.0F});
    if (erase_collection.size() != 1) {
        std::cerr << "moved ID index was not updated after first erase\n";
        return 1;
    }
    if (!erase_collection.erase(40) || erase_collection.size() != 0) {
        std::cerr << "final erase failed\n";
        return 1;
    }

    wvectordb::Collection search_collection(2);
    search_collection.insert(100, {0.0F, 0.0F});
    search_collection.insert(200, {3.0F, 4.0F});
    search_collection.insert(300, {1.0F, 0.0F});
    search_collection.insert(400, {-1.0F, 0.0F});
    search_collection.insert(500, {2.0F, 0.0F});

    if (!search_collection.search({0.0F, 0.0F}, 0).empty()) {
        std::cerr << "k=0 did not return an empty result\n";
        return 1;
    }

    const auto top_three = search_collection.search({0.0F, 0.0F}, 3);
    if (top_three.size() != 3 || top_three[0].id != 100 ||
        top_three[0].distance != 0.0F || top_three[1].id != 300 ||
        top_three[1].distance != 1.0F || top_three[2].id != 400 ||
        top_three[2].distance != 1.0F) {
        std::cerr << "top-3 search ordering or distance is incorrect\n";
        return 1;
    }

    const auto all_results = search_collection.search({0.0F, 0.0F}, 99);
    if (all_results.size() != 5 || all_results[3].id != 500 ||
        all_results[3].distance != 4.0F || all_results[4].id != 200 ||
        all_results[4].distance != 25.0F) {
        std::cerr << "k larger than collection size was handled incorrectly\n";
        return 1;
    }

    const auto exact_all_results = search_collection.search({0.0F, 0.0F}, 5);
    if (exact_all_results.size() != 5 ||
        exact_all_results[4].id != 200 ||
        exact_all_results[4].distance != 25.0F) {
        std::cerr << "k equal to collection size was handled incorrectly\n";
        return 1;
    }

    wvectordb::Collection empty_search_collection(2);
    if (!empty_search_collection.search({1.0F, 2.0F}, 5).empty()) {
        std::cerr << "empty collection search was not empty\n";
        return 1;
    }

    expect_invalid_argument(
        [&] { search_collection.search({1.0F}, 2); },
        "wrong query dimension was accepted");
    expect_invalid_argument(
        [&] {
            search_collection.search(
                {1.0F, std::numeric_limits<float>::infinity()}, 2);
        },
        "non-finite query value was accepted");

    wvectordb::Collection batch_collection(2);
    batch_collection.insert(1, {0.0F, 0.0F});
    batch_collection.insert_batch({});
    if (batch_collection.size() != 1 ||
        !batch_collection.search_batch({}, 1).empty()) {
        std::cerr << "empty batch operation changed state or output\n";
        return 1;
    }
    batch_collection.insert_batch({
        {2, {1.0F, 0.0F}},
        {3, {2.0F, 0.0F}},
        {2, {3.0F, 0.0F}},
    });
    if (batch_collection.size() != 3) {
        std::cerr << "batch insert created an incorrect number of records\n";
        return 1;
    }

    const auto batch_results = batch_collection.search_batch(
        {{0.0F, 0.0F}, {3.0F, 0.0F}}, 1);
    if (batch_results.size() != 2 || batch_results[0].size() != 1 ||
        batch_results[0][0].id != 1 || batch_results[1].size() != 1 ||
        batch_results[1][0].id != 2 ||
        batch_results[1][0].distance != 0.0F) {
        std::cerr << "batch search did not preserve query order or results\n";
        return 1;
    }

    const std::size_t size_before_failed_batch = batch_collection.size();
    expect_invalid_argument(
        [&] {
            batch_collection.insert_batch({
                {4, {4.0F, 0.0F}},
                {5, {5.0F}},
            });
        },
        "invalid batch insert was accepted");
    if (batch_collection.size() != size_before_failed_batch) {
        std::cerr << "failed batch insert changed Collection state\n";
        return 1;
    }
    const auto preserved_after_failed_batch =
        batch_collection.search({3.0F, 0.0F}, 1);
    if (preserved_after_failed_batch.size() != 1 ||
        preserved_after_failed_batch[0].id != 2 ||
        preserved_after_failed_batch[0].distance != 0.0F) {
        std::cerr << "failed batch insert did not restore an overwritten record\n";
        return 1;
    }

    expect_invalid_argument(
        [&] {
            batch_collection.search_batch(
                {{0.0F, 0.0F}, {1.0F}}, 1);
        },
        "invalid batch query was accepted");

    std::cout << "Phase 5 Collection batch API test passed\n";
    return 0;
}
