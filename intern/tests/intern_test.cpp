#include "intern/intern_list.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>
#include <signal.h>

#include <string>

TEST(InternList, canInternOneString) {
    scp::InternList list(1024);
    const auto sv = list.intern("_foobar");
    auto optsv = list.find("_foobar");
    ASSERT_TRUE(optsv);
    EXPECT_EQ(sv, optsv);
    EXPECT_EQ(sv.data(), list.find("_foobar").value().data());
}

TEST(InternList, canInternManyStrings) {
    constexpr size_t maxLen = 100;
    constexpr size_t nStrings = 1'000'000;
    scp::InternList list(maxLen * nStrings / scp::InternList::pageSize());
    scp::test_utils::fill(list, nStrings, maxLen);
    // may be slightly less due to random duplicates
    EXPECT_GE(nStrings, list.size());
    EXPECT_GT(list.size(), nStrings * 9 / 10);
}

TEST(InternList, isCorrectlyAligned) {
    constexpr size_t maxLen = 100;
    constexpr size_t nStrings = 100'000;
    scp::InternList list(maxLen * nStrings / scp::InternList::pageSize(), true);
    scp::test_utils::fill(list, nStrings, maxLen);
    for (auto sv : list.getContents()) {
        ASSERT_EQ((uintptr_t)sv.data() % 64, 0);
    }
}

TEST(InternList, canFind) {
    constexpr size_t maxLen = 100;
    constexpr size_t nStrings = 100'001;
    scp::InternList list(maxLen * nStrings / scp::InternList::pageSize());
    scp::test_utils::fill(list, nStrings / 2, maxLen);
    const auto sv = list.intern("_foobar");
    scp::test_utils::fill(list, nStrings / 2, maxLen);
    ASSERT_TRUE(list.find("_foobar"));
    EXPECT_EQ(sv, list.find("_foobar"));
    EXPECT_EQ(sv.data(), list.find("_foobar").value().data());
}

TEST(InternList, iterators) {
    constexpr size_t maxLen = 100;
    constexpr size_t nStrings = 1'001;
    scp::InternList list(maxLen * nStrings / scp::InternList::pageSize());
    scp::test_utils::fill(list, nStrings, maxLen);
    const auto contents = list.getContents();
    auto iter = contents.cbegin();
    for (const auto& sv : list) {
        ASSERT_EQ(sv.data(), (iter)->data());
        ASSERT_EQ(sv.size(), (iter)->size());
        ++iter;
    }
}

namespace {
void signalHandler(int sig, siginfo_t* si, void* unused) {
    // Note: calling printf() from a signal handler is not safe and should not be done in
    // production code
    fprintf(stderr, "Got signal %d at address: %p\n", sig, si->si_addr);
    exit(-1);
}
} // namespace

TEST(InternList, canFreeze) {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = signalHandler;
    ASSERT_NE(sigaction(SIGSEGV, &sa, NULL), -1);

    constexpr size_t maxLen = 100;
    constexpr size_t nStrings = 10'000;
    scp::InternList list(maxLen * nStrings / scp::InternList::pageSize());
    scp::test_utils::fill(list, nStrings, maxLen);
    scp::InternedStringView sv = list.intern(std::string("_foobar"));
    list.freeze(0);
    list.intern(sv.toString()); // okay because there is no write
    EXPECT_TRUE(list.find(sv.toString()));
    char msg[256];
    snprintf(msg, 256, "Got signal %d at address: %p", SIGSEGV, sv.data() + sv.size() + 1);
    EXPECT_DEATH(list.intern("_foobaz"), msg);
}
