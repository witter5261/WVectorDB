#include "wvectordb/collection.hpp"

#include <iostream>

int main() {
    wvectordb::Collection collection(2);
    collection.insert(101, {1.0F, 1.0F});
    collection.insert(102, {2.0F, 1.0F});
    collection.insert(103, {4.0F, 3.0F});

    const auto results = collection.search({1.0F, 1.0F}, 2);
    if (results.size() != 2 || results[0].id != 101 ||
        results[1].id != 102) {
        std::cerr << "basic search example returned unexpected results\n";
        return 1;
    }

    for (const wvectordb::SearchResult& result : results) {
        std::cout << result.id << " distance = " << result.distance << '\n';
    }
    return 0;
}
