#pragma once

#include "intern/intern_list.hpp"

namespace scp::test_utils {

void fill(scp::InternList& list, size_t nStrings, size_t maxLen, size_t minLen = 0);

}
