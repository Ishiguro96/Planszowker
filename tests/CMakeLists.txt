include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(mocks)

# Add unit tests
add_subdirectory(libs/Utils/AssetsManager)
add_subdirectory(libs/Utils/PlametaParser)
add_subdirectory(libs/Utils/TickThread)
