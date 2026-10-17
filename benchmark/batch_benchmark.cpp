#include "wvectordb/collection.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using wvectordb::SearchResult;
using wvectordb::Vector;
using wvectordb::VectorEntry;
using wvectordb::VectorId;

constexpr std::uint32_t kSeed = 20261017U;
constexpr std::size_t kInsertRuns = 3;
constexpr std::size_t kSearchQueries[] = {1, 10, 100};
constexpr std::size_t kSearchRuns = 7;

struct Statistics {
    double mean_ns{};
    double median_ns{};
    double min_ns{};
    double max_ns{};
};

struct SearchConfiguration {
    std::size_t count;
    std::size_t dimension;
    std::size_t k;
    std::size_t query_count;
};

std::vector<Vector> make_vectors(std::size_t count,
                                 std::size_t dimension,
                                 std::mt19937& generator) {
    std::uniform_real_distribution<float> distribution(-1.0F, 1.0F);
    std::vector<Vector> vectors;
    vectors.reserve(count);
    for (std::size_t row = 0; row < count; ++row) {
        Vector vector;
        vector.reserve(dimension);
        for (std::size_t column = 0; column < dimension; ++column) {
            vector.push_back(distribution(generator));
        }
        vectors.push_back(std::move(vector));
    }
    return vectors;
}

float squared_l2_distance(const Vector& left, const Vector& right) {
    double distance = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        const double difference = static_cast<double>(left[index]) -
                                  static_cast<double>(right[index]);
        distance += difference * difference;
    }
    return static_cast<float>(distance);
}

std::vector<SearchResult> reference_search(const std::vector<Vector>& vectors,
                                           const Vector& query,
                                           std::size_t k) {
    std::vector<SearchResult> results;
    results.reserve(vectors.size());
    for (std::size_t index = 0; index < vectors.size(); ++index) {
        results.push_back(SearchResult{
            static_cast<VectorId>(index),
            squared_l2_distance(query, vectors[index])});
    }
    const auto compare = [](const SearchResult& left,
                            const SearchResult& right) {
        if (left.distance != right.distance) {
            return left.distance < right.distance;
        }
        return left.id < right.id;
    };
    std::sort(results.begin(), results.end(), compare);
    if (k < results.size()) {
        results.resize(k);
    }
    return results;
}

void verify_results(const std::vector<SearchResult>& actual,
                    const std::vector<SearchResult>& expected) {
    if (actual.size() != expected.size()) {
        throw std::runtime_error("batch benchmark returned wrong result count");
    }
    std::unordered_set<VectorId> ids;
    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (actual[index].id != expected[index].id ||
            !std::isfinite(actual[index].distance) ||
            std::abs(actual[index].distance - expected[index].distance) >
                1.0e-5F ||
            !ids.insert(actual[index].id).second) {
            throw std::runtime_error("batch benchmark result mismatch");
        }
    }
}

Statistics summarize(std::vector<double> samples) {
    std::sort(samples.begin(), samples.end());
    double total = 0.0;
    for (const double sample : samples) {
        total += sample;
    }
    return Statistics{
        total / static_cast<double>(samples.size()),
        samples[samples.size() / 2],
        samples.front(),
        samples.back()};
}

std::vector<std::size_t> make_batch_sizes(std::size_t count) {
    std::vector<std::size_t> sizes{1, 10, 100, count};
    std::sort(sizes.begin(), sizes.end());
    sizes.erase(std::unique(sizes.begin(), sizes.end()), sizes.end());
    return sizes;
}

std::vector<std::vector<VectorEntry>> split_entries(
    const std::vector<VectorEntry>& entries,
    std::size_t batch_size) {
    std::vector<std::vector<VectorEntry>> batches;
    for (std::size_t offset = 0; offset < entries.size();
         offset += batch_size) {
        const std::size_t end =
            std::min(entries.size(), offset + batch_size);
        batches.emplace_back(entries.begin() + offset, entries.begin() + end);
    }
    return batches;
}

void write_insert_header(std::ostream& output) {
    output << "# build=Release-like (-O2 -DNDEBUG)\n";
    output << "# compiler=" << __VERSION__ << "\n";
    output << "# cxx_standard=C++20\n";
    output << "# seed=" << kSeed << "\n";
    output << "# repetitions=" << kInsertRuns << "\n";
    output << "N,D,variant,batch_size,repetitions,mean_ns_per_entry,median_ns_per_entry,min_ns_per_entry,max_ns_per_entry\n";
}

void write_search_header(std::ostream& output) {
    output << "# build=Release-like (-O2 -DNDEBUG)\n";
    output << "# compiler=" << __VERSION__ << "\n";
    output << "# cxx_standard=C++20\n";
    output << "# seed=" << kSeed << "\n";
    output << "# repetitions=" << kSearchRuns << "\n";
    output << "N,D,k,Q,variant,repetitions,mean_ns_per_query,median_ns_per_query,min_ns_per_query,max_ns_per_query,checksum\n";
}

void run_insert_configuration(std::size_t count,
                              std::size_t dimension,
                              std::mt19937& generator,
                              std::ostream& output) {
    const std::vector<Vector> vectors =
        make_vectors(count, dimension, generator);
    std::vector<VectorEntry> entries;
    entries.reserve(vectors.size());
    for (std::size_t index = 0; index < vectors.size(); ++index) {
        entries.push_back(VectorEntry{static_cast<VectorId>(index),
                                      vectors[index]});
    }

    const std::vector<std::size_t> batch_sizes = make_batch_sizes(count);
    const auto record = [&](const char* variant,
                            std::size_t batch_size,
                            const std::vector<double>& samples) {
        const Statistics statistics = summarize(samples);
        output << count << ',' << dimension << ',' << variant << ','
               << batch_size << ',' << kInsertRuns << ',' << std::fixed
               << std::setprecision(2) << statistics.mean_ns << ','
               << statistics.median_ns << ',' << statistics.min_ns << ','
               << statistics.max_ns << '\n';
    };

    std::vector<double> single_samples;
    single_samples.reserve(kInsertRuns);
    for (std::size_t run = 0; run < kInsertRuns; ++run) {
        wvectordb::Collection collection(dimension);
        const auto start = Clock::now();
        for (const VectorEntry& entry : entries) {
            collection.insert(entry.id, entry.vector);
        }
        const auto finish = Clock::now();
        if (collection.size() != count) {
            throw std::runtime_error("single insert benchmark size mismatch");
        }
        single_samples.push_back(
            static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    finish - start)
                    .count()) /
            static_cast<double>(count));
    }
    record("single_insert", 1, single_samples);

    for (const std::size_t batch_size : batch_sizes) {
        const auto batches = split_entries(entries, batch_size);
        std::vector<double> samples;
        samples.reserve(kInsertRuns);
        for (std::size_t run = 0; run < kInsertRuns; ++run) {
            wvectordb::Collection collection(dimension);
            const auto start = Clock::now();
            for (const std::vector<VectorEntry>& batch : batches) {
                collection.insert_batch(batch);
            }
            const auto finish = Clock::now();
            if (collection.size() != count) {
                throw std::runtime_error("batch insert benchmark size mismatch");
            }
            samples.push_back(
                static_cast<double>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        finish - start)
                        .count()) /
                static_cast<double>(count));
        }
        record("batch_insert", batch_size, samples);
    }
}

double run_search_variant(const SearchConfiguration& configuration,
                          const std::vector<Vector>& vectors,
                          const std::vector<Vector>& queries,
                          const wvectordb::Collection& collection,
                          bool use_batch,
                          std::ostream& output) {
    for (const Vector& query : queries) {
        verify_results(collection.search(query, configuration.k),
                       reference_search(vectors, query, configuration.k));
    }

    std::vector<double> samples;
    samples.reserve(kSearchRuns);
    double checksum = 0.0;
    for (std::size_t run = 0; run < kSearchRuns; ++run) {
        const auto start = Clock::now();
        if (use_batch) {
            const auto batch_results =
                collection.search_batch(queries, configuration.k);
            for (const std::vector<SearchResult>& results : batch_results) {
                for (const SearchResult& result : results) {
                    checksum += static_cast<double>(result.id) +
                                static_cast<double>(result.distance);
                }
            }
        } else {
            for (const Vector& query : queries) {
                const auto results = collection.search(query, configuration.k);
                for (const SearchResult& result : results) {
                    checksum += static_cast<double>(result.id) +
                                static_cast<double>(result.distance);
                }
            }
        }
        const auto finish = Clock::now();
        samples.push_back(
            static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    finish - start)
                    .count()) /
            static_cast<double>(queries.size()));
    }

    const Statistics statistics = summarize(samples);
    output << configuration.count << ',' << configuration.dimension << ','
           << configuration.k << ',' << configuration.query_count << ','
           << (use_batch ? "search_batch" : "search_loop") << ','
           << kSearchRuns << ',' << std::fixed << std::setprecision(2)
           << statistics.mean_ns << ',' << statistics.median_ns << ','
           << statistics.min_ns << ',' << statistics.max_ns << ',' << checksum
           << '\n';
    return checksum;
}

void run_search_configuration(const SearchConfiguration& configuration,
                              std::mt19937& generator,
                              std::ostream& output) {
    const std::vector<Vector> vectors =
        make_vectors(configuration.count, configuration.dimension, generator);
    const std::vector<Vector> queries = make_vectors(
        configuration.query_count, configuration.dimension, generator);

    wvectordb::Collection collection(configuration.dimension);
    std::vector<VectorEntry> entries;
    entries.reserve(vectors.size());
    for (std::size_t index = 0; index < vectors.size(); ++index) {
        entries.push_back(VectorEntry{static_cast<VectorId>(index),
                                      vectors[index]});
    }
    collection.insert_batch(entries);

    run_search_variant(configuration, vectors, queries, collection, false,
                       output);
    run_search_variant(configuration, vectors, queries, collection, true,
                       output);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string insert_path =
        argc > 1 ? argv[1] : "benchmark/results/phase5_batch_insert.csv";
    const std::string search_path =
        argc > 2 ? argv[2] : "benchmark/results/phase5_batch_search.csv";
    std::ofstream insert_output(insert_path);
    std::ofstream search_output(search_path);
    if (!insert_output || !search_output) {
        std::cerr << "failed to open batch benchmark output files\n";
        return 1;
    }

    try {
        write_insert_header(insert_output);
        write_search_header(search_output);

        std::mt19937 insert_generator(kSeed);
        for (const std::size_t count : {100U, 1000U, 5000U}) {
            for (const std::size_t dimension : {8U, 128U}) {
                run_insert_configuration(count, dimension, insert_generator,
                                         insert_output);
            }
        }

        std::mt19937 search_generator(kSeed);
        for (const std::size_t count : {1000U, 10000U}) {
            for (const std::size_t dimension : {8U, 128U}) {
                for (const std::size_t k : {1U, 10U}) {
                    for (const std::size_t query_count : kSearchQueries) {
                        run_search_configuration(
                            SearchConfiguration{count, dimension, k,
                                                query_count},
                            search_generator, search_output);
                    }
                }
            }
        }

        std::cout << "Batch benchmark completed\n";
        std::cout << "Insert CSV: " << insert_path << '\n';
        std::cout << "Search CSV: " << search_path << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Batch benchmark failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
