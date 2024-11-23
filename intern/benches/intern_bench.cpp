#include "intern/intern_list.hpp"
#include "intern/tests/test_utils.hpp"

#include <benchmark/benchmark.h>

#include <memory>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace {

static std::unique_ptr<const scp::InternList> _listPtr;
static constexpr const char* TEST_KEY = "_foobarbaz::foobarbarry::foobarbazbarry";

// todo: use BM setup mechanism
static const scp::InternList& getList() {
    constexpr size_t minLen = 63;
    constexpr size_t maxLen = 63;
    constexpr size_t nStrings = 1'000'000;
    if (!_listPtr) {
        auto ptr = std::make_unique<scp::InternList>(
            (1 + maxLen) * 2 * nStrings / scp::InternList::pageSize(), true);
        scp::test_utils::fill(*ptr, nStrings, maxLen, minLen);
        [[maybe_unused]] const auto p = ptr->intern(TEST_KEY);
        assert(p.toString() == std::string(TEST_KEY));
        assert(ptr->size() > nStrings * 9 / 10);
        _listPtr = std::unique_ptr<const scp::InternList>(ptr.release());
    }
    return *_listPtr;
}

static void fragmentHeap(std::vector<std::string>& lifetime, size_t numEntries) {
    std::uniform_int_distribution<size_t> lengths(23, 256); // avoid SSO
    std::uniform_int_distribution<uint8_t> flags(0, 1);
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::vector<std::string> temporary;
    for (size_t i = 0; i < numEntries; ++i) {
        if (flags(generator)) {
            temporary.push_back(std::string(lengths(generator), 'x'));
        } else {
            lifetime.push_back(std::string(lengths(generator), 'x'));
        }
    }
}

} // namespace

static void find(benchmark::State& state) {
    const auto& list = getList();
    std::string buffer(TEST_KEY);
    std::string_view test(buffer);
    if (!list.find(test)) {
        throw std::runtime_error("intern list error: not found");
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(list.find(test));
    }
}

static void treeSearch(benchmark::State& state) {

    std::vector<std::string> lifetime;
    fragmentHeap(lifetime, 400000);

    const auto& list = getList();
    std::set<std::string> theSet;
    for (auto str : list.getContents()) {
        theSet.insert(std::string(str));
    }
    std::string test(TEST_KEY);
    if (theSet.find(test) == theSet.end()) {
        throw std::runtime_error("rb set error: not found");
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(theSet.find(test));
    }
}

static void lookup(benchmark::State& state) {
    const auto& list = getList();
    std::vector<std::string> lifetime;
    fragmentHeap(lifetime, 400000);

    std::unordered_map<scp::InternedStringView, int> haystack(list.size());
    for (auto& sv : list) {
        haystack.emplace(sv, 999);
    }

    std::string buffer(TEST_KEY);
    const auto optNeedle = list.find(std::string_view(buffer));
    if (!optNeedle.has_value()) {
        throw std::runtime_error("lookup: not found");
    }
    scp::InternedStringView needle = optNeedle.value();
    if (!haystack.contains(needle)) {
        throw std::runtime_error("lookup: missing");
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(haystack.find(needle));
    }
}

static void lookupHash(benchmark::State& state) {
    std::vector<std::string> lifetime;
    fragmentHeap(lifetime, 400000);

    const auto& list = getList();
    std::unordered_map<std::string, int> haystack(list.size());
    for (auto& sv : list) {
        haystack.emplace(sv.toString(), 999);
    }

    std::string needle(TEST_KEY);
    if (!haystack.contains(needle)) {
        throw std::runtime_error("lookupHash: not found");
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(haystack.find(needle));
    }
}

// BENCHMARK(treeSearch);
// BENCHMARK(find);
BENCHMARK(lookup);
BENCHMARK(lookupHash);

BENCHMARK_MAIN();
