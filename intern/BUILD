cc_library(
    name ="intern",
    hdrs = ["intern_list.hpp"],
    srcs = glob(["*.cpp"]),
)

cc_library(
    name = "test_utils",
    deps = [
        ":intern",
    ],
    hdrs = ["tests/test_utils.hpp"],
    srcs = ["tests/test_utils.cpp"],
    visibility = [":__subpackages__"],
)

cc_test(
    name = "intern-gt",
    srcs = glob(["tests/*_test.cpp"]),
    deps = [
        ":intern",
        ":test_utils",
        "@googletest//:gtest_main",
    ],
    size="small",
)

cc_binary(
    name = "intern-perf",
    srcs = glob(["benches/*.cpp"]),
    deps = [
        ":intern",
        ":test_utils",
        "@google_benchmark//:benchmark_main",
    ],
)
