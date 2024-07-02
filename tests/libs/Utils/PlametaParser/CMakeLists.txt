set(TEST_TARGET ParserTest)

add_executable(
        ${TEST_TARGET}
        ParserTest.cpp
)
target_link_libraries(
        ${TEST_TARGET}
        PRIVATE PlametaParser
        PRIVATE EasyLogging
        GTest::gtest_main
        GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
