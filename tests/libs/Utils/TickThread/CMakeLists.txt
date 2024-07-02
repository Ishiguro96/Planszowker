set(TEST_TARGET TickThreadTest)

add_executable(
        ${TEST_TARGET}
        TickThreadTest.cpp
)
target_link_libraries(
        ${TEST_TARGET}
        PRIVATE TickThread
        GTest::gtest_main
        GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
