#include "wvectordb/collection.hpp"
#include "wvectordb/types.hpp"

#include <iostream>

int main() {
    wvectordb::Collection collection(3);

    if (collection.dimension() != 3 || collection.size() != 0) {
        std::cerr << "Collection skeleton check failed\n";
        return 1;
    }

    const wvectordb::SearchResult result{7, 1.5F};
    if (result.id != 7 || result.distance != 1.5F) {
        std::cerr << "Public type check failed\n";
        return 1;
    }

    std::cout << "Phase 1 smoke test passed\n";
    return 0;
}
