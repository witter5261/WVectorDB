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

    std::cout << "Phase 2 Collection lifecycle test passed\n";
    return 0;
}
