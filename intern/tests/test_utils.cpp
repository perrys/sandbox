#include "test_utils.hpp"

#include <random>
#include <string>

namespace scp::test_utils {

void fill(scp::InternList& list, size_t nStrings, size_t maxLen, size_t minLen) {
    const std::string alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<size_t> charPicks(0, alphabet.length());
    std::uniform_int_distribution<size_t> lengths(minLen, maxLen);

    char buf[maxLen];

    for (size_t i = 0; i < nStrings; ++i) {
        const size_t len = lengths(generator);
        for (std::size_t i = 0; i < len; ++i) {
            buf[i] = alphabet[charPicks(generator)];
        }
        list.intern({buf, len});
    }
}

} // namespace scp::test_utils
