#include "wvectordb/collection.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
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
using wvectordb::VectorId;

constexpr std::uint32_t kSeed = 20261017U;
constexpr std::size_t kQueryCount = 20;
constexpr std::size_t kWarmupRuns = 2;
constexpr std::size_t kMeasuredRuns = 7;

struct Configuration {
    std::size_t count;
    std::size_t dimension;
    std::size_t k;
};

struct Statistics {
    double mean_ns{};
    double median_ns{};
    double min_ns{};
    double max_ns{};
    double p95_ns{};
};

enum class SearchVariant {
    default_collection,
    full_sort,
    partial_sort,
    nth_element,
};

const char* variant_name(SearchVariant variant) {
    switch (variant) {
    case SearchVariant::default_collection:
        return "default_collection";
    case SearchVariant::full_sort:
        return "full_sort_reference";
    case SearchVariant::partial_sort:
        return "partial_sort";
    case SearchVariant::nth_element:
        return "nth_element";
    }
    return "unknown";
}

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
    std::sort(results.begin(), results.end(),
              [](const SearchResult& left, const SearchResult& right) {
                  if (left.distance != right.distance) {
                      return left.distance < right.distance;
                  }
                  return left.id < right.id;
              });
    if (k < results.size()) {
        results.resize(k);
    }
    return results;
}

std::vector<SearchResult> sort_variant_search(
    const std::vector<Vector>& vectors,
    const Vector& query,
    std::size_t k,
    SearchVariant variant) {
    std::vector<SearchResult> results;
    if (k == 0 || vectors.empty()) {
        return results;
    }

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

    if (k >= results.size() || variant == SearchVariant::full_sort) {
        std::sort(results.begin(), results.end(), compare);
        if (k < results.size()) {
            results.resize(k);
        }
        return results;
    }

    if (variant == SearchVariant::partial_sort) {
        std::partial_sort(results.begin(), results.begin() + k,
                          results.end(), compare);
        results.resize(k);
        return results;
    }

    std::nth_element(results.begin(), results.begin() + k, results.end(),
                     compare);
    std::sort(results.begin(), results.begin() + k, compare);
    results.resize(k);
    return results;
}

void verify_results(const std::vector<SearchResult>& actual,
                    const std::vector<SearchResult>& expected) {
    if (actual.size() != expected.size()) {
        throw std::runtime_error("benchmark search returned wrong result count");
    }

    std::unordered_set<VectorId> ids;
    for (std::size_t index = 0; index < actual.size(); ++index) {
        const SearchResult& result = actual[index];
        const SearchResult& reference = expected[index];
        if (result.id != reference.id ||
            !std::isfinite(result.distance) ||
            std::abs(result.distance - reference.distance) > 1.0e-5F) {
            throw std::runtime_error("benchmark search result mismatch");
        }
        if (!ids.insert(result.id).second) {
            throw std::runtime_error("benchmark search returned duplicate ID");
        }
        if (index > 0) {
            const SearchResult& previous = actual[index - 1];
            if (previous.distance > result.distance ||
                (previous.distance == result.distance &&
                 previous.id > result.id)) {
                throw std::runtime_error(
                    "benchmark search result is not deterministically sorted");
            }
        }
    }
}

Statistics summarize(std::vector<double> samples) {
    if (samples.empty()) {
        throw std::runtime_error("no benchmark samples were collected");
    }
    std::sort(samples.begin(), samples.end());
    double total = 0.0;
    for (const double sample : samples) {
        total += sample;
    }
    const std::size_t p95_index =
        std::min(samples.size() - 1,
                 (samples.size() * 95U + 99U) / 100U - 1U);
    return Statistics{
        total / static_cast<double>(samples.size()),
        samples[samples.size() / 2],
        samples.front(),
        samples.back(),
        samples[p95_index]};
}

void write_csv_header(std::ostream& output) {
    output << "# build=Release-like (-O2 -DNDEBUG)\n";
    output << "# compiler=" << __VERSION__ << "\n";
    output << "# cxx_standard=C++20\n";
    output << "# seed=" << kSeed << "\n";
    output << "# queries=" << kQueryCount << ",warmups=" << kWarmupRuns
           << ",repetitions=" << kMeasuredRuns << "\n";
    output << "N,D,k,queries,warmups,repetitions,mean_ns,median_ns,min_ns,max_ns,p95_ns,checksum\n";
}

void write_comparison_header(std::ostream& output) {
    output << "# build=Release-like (-O2 -DNDEBUG)\n";
    output << "# compiler=" << __VERSION__ << "\n";
    output << "# cxx_standard=C++20\n";
    output << "# seed=" << kSeed << "\n";
    output << "# same_data_batch=true\n";
    output << "# queries=" << kQueryCount << ",warmups=" << kWarmupRuns
           << ",repetitions=" << kMeasuredRuns << "\n";
    output << "N,D,k,variant,queries,warmups,repetitions,mean_ns,median_ns,min_ns,max_ns,p95_ns,checksum\n";
}

double run_configuration(const Configuration& configuration,
                         std::mt19937& generator,
                         std::ostream& output) {
    const std::vector<Vector> vectors = make_vectors(
        configuration.count, configuration.dimension, generator);
    std::vector<Vector> queries = make_vectors(
        kQueryCount, configuration.dimension, generator);

    wvectordb::Collection collection(configuration.dimension);
    for (std::size_t index = 0; index < vectors.size(); ++index) {
        collection.insert(static_cast<VectorId>(index), vectors[index]);
    }

    for (const Vector& query : queries) {
        const auto actual = collection.search(query, configuration.k);
        const auto expected = reference_search(vectors, query, configuration.k);
        verify_results(actual, expected);
    }

    volatile double checksum_sink = 0.0;
    for (std::size_t run = 0; run < kWarmupRuns; ++run) {
        for (const Vector& query : queries) {
            const auto results = collection.search(query, configuration.k);
            if (!results.empty()) {
                checksum_sink += static_cast<double>(results.front().id) +
                                 static_cast<double>(results.front().distance);
            }
        }
    }

    std::vector<double> samples;
    samples.reserve(kMeasuredRuns);
    double checksum = 0.0;
    for (std::size_t run = 0; run < kMeasuredRuns; ++run) {
        const auto start = Clock::now();
        for (const Vector& query : queries) {
            const auto results = collection.search(query, configuration.k);
            if (!results.empty()) {
                checksum += static_cast<double>(results.front().id) +
                            static_cast<double>(results.front().distance);
            }
        }
        const auto finish = Clock::now();
        const double elapsed_ns = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
                .count());
        samples.push_back(elapsed_ns / static_cast<double>(kQueryCount));
    }
    checksum += checksum_sink;

    const Statistics statistics = summarize(std::move(samples));
    output << configuration.count << ',' << configuration.dimension << ','
           << configuration.k << ',' << kQueryCount << ',' << kWarmupRuns << ','
           << kMeasuredRuns << ',' << std::fixed << std::setprecision(2)
           << statistics.mean_ns << ',' << statistics.median_ns << ','
           << statistics.min_ns << ',' << statistics.max_ns << ','
           << statistics.p95_ns << ',' << checksum << '\n';
    return checksum;
}

double run_comparison_variant(const Configuration& configuration,
                              const std::vector<Vector>& vectors,
                              const std::vector<Vector>& queries,
                              const wvectordb::Collection& collection,
                              SearchVariant variant,
                              std::ostream& output) {
    for (const Vector& query : queries) {
        const auto actual =
            variant == SearchVariant::default_collection
                ? collection.search(query, configuration.k)
                : sort_variant_search(vectors, query, configuration.k, variant);
        const auto expected = reference_search(vectors, query, configuration.k);
        verify_results(actual, expected);
    }

    volatile double checksum_sink = 0.0;
    for (std::size_t run = 0; run < kWarmupRuns; ++run) {
        for (const Vector& query : queries) {
            const auto results =
                variant == SearchVariant::default_collection
                    ? collection.search(query, configuration.k)
                    : sort_variant_search(vectors, query, configuration.k,
                                          variant);
            if (!results.empty()) {
                checksum_sink += static_cast<double>(results.front().id) +
                                 static_cast<double>(results.front().distance);
            }
        }
    }

    std::vector<double> samples;
    samples.reserve(kMeasuredRuns);
    double checksum = 0.0;
    for (std::size_t run = 0; run < kMeasuredRuns; ++run) {
        const auto start = Clock::now();
        for (const Vector& query : queries) {
            const auto results =
                variant == SearchVariant::default_collection
                    ? collection.search(query, configuration.k)
                    : sort_variant_search(vectors, query, configuration.k,
                                          variant);
            if (!results.empty()) {
                checksum += static_cast<double>(results.front().id) +
                            static_cast<double>(results.front().distance);
            }
        }
        const auto finish = Clock::now();
        const double elapsed_ns = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
                .count());
        samples.push_back(elapsed_ns / static_cast<double>(queries.size()));
    }
    checksum += checksum_sink;

    const Statistics statistics = summarize(std::move(samples));
    output << configuration.count << ',' << configuration.dimension << ','
           << configuration.k << ',' << variant_name(variant) << ','
           << queries.size() << ',' << kWarmupRuns << ',' << kMeasuredRuns
           << ',' << std::fixed << std::setprecision(2) << statistics.mean_ns
           << ',' << statistics.median_ns << ',' << statistics.min_ns << ','
           << statistics.max_ns << ',' << statistics.p95_ns << ',' << checksum
           << '\n';
    return checksum;
}

double run_comparison_configuration(const Configuration& configuration,
                                    std::mt19937& generator,
                                    std::ostream& output) {
    const std::vector<Vector> vectors = make_vectors(
        configuration.count, configuration.dimension, generator);
    const std::vector<Vector> queries = make_vectors(
        kQueryCount, configuration.dimension, generator);

    wvectordb::Collection collection(configuration.dimension);
    for (std::size_t index = 0; index < vectors.size(); ++index) {
        collection.insert(static_cast<VectorId>(index), vectors[index]);
    }

    double checksum = 0.0;
    for (const SearchVariant variant : {
             SearchVariant::default_collection,
             SearchVariant::full_sort,
             SearchVariant::partial_sort,
             SearchVariant::nth_element}) {
        checksum += run_comparison_variant(configuration, vectors, queries,
                                            collection, variant, output);
    }
    return checksum;
}

std::vector<Configuration> make_configurations() {
    const std::vector<std::size_t> counts{100, 1000, 10000};
    const std::vector<std::size_t> dimensions{8, 128};
    std::vector<Configuration> configurations;

    for (const std::size_t count : counts) {
        std::vector<std::size_t> k_values{
            1,
            10,
            std::min<std::size_t>(100, count),
            count / 2,
            count,
        };
        std::sort(k_values.begin(), k_values.end());
        k_values.erase(std::unique(k_values.begin(), k_values.end()),
                       k_values.end());

        for (const std::size_t dimension : dimensions) {
            for (const std::size_t k : k_values) {
                configurations.push_back(Configuration{count, dimension, k});
            }
        }
    }
    return configurations;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string baseline_path =
        argc > 1 ? argv[1] : "benchmark/results/phase4_k_sweep_baseline.csv";
    const std::string comparison_path =
        argc > 2 ? argv[2]
                 : "benchmark/results/phase4_k_sweep_comparison.csv";
    std::ofstream baseline_output(baseline_path);
    std::ofstream comparison_output(comparison_path);
    if (!baseline_output || !comparison_output) {
        std::cerr << "failed to open benchmark output files: " << baseline_path
                  << " and " << comparison_path << '\n';
        return 1;
    }

    try {
        write_csv_header(baseline_output);
        write_comparison_header(comparison_output);
        std::mt19937 generator(kSeed);
        const std::vector<Configuration> configurations = make_configurations();

        double final_checksum = 0.0;
        for (const Configuration& configuration : configurations) {
            final_checksum +=
                run_configuration(configuration, generator, baseline_output);
        }
        baseline_output.close();

        std::mt19937 comparison_generator(kSeed);
        double comparison_checksum = 0.0;
        for (const Configuration& configuration : configurations) {
            comparison_checksum += run_comparison_configuration(
                configuration, comparison_generator, comparison_output);
        }
        comparison_output.close();

        std::cout << "Baseline benchmark completed: " << baseline_path << '\n';
        std::cout << "Sort comparison completed: " << comparison_path << '\n';
        std::cout << "Final checksum: " << std::setprecision(17)
                  << final_checksum << '\n';
        std::cout << "Comparison checksum: " << std::setprecision(17)
                  << comparison_checksum << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
