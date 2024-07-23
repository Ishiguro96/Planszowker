cmake_minimum_required(VERSION 3.22)

project(Planszowker VERSION 0.0.1
                    LANGUAGES CXX C)

set(SFML_STATIC_LIBRARIES TRUE)

# Set compiler specific settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(SFML COMPONENTS network graphics window system REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

if (WIN32)
    # Dirty hack to get things working on Windows
    include_directories("C:/PlanszowkerDeps/include")

    set(PLANSZOWKER_WINDOWS_LIB_DIR "C:/PlanszowkerDeps/lib" CACHE INTERNAL "")

    find_library(ZIPLIB_LZMA NAMES ziplib_lzma HINTS ${PLANSZOWKER_WINDOWS_LIB_DIR})
    find_library(ZIPLIB_BZIP2 NAMES ziplib_bzip2 HINTS ${PLANSZOWKER_WINDOWS_LIB_DIR})
    find_library(ZIPLIB_ZLIB NAMES ziplib_zlib HINTS ${PLANSZOWKER_WINDOWS_LIB_DIR})
    find_library(ZIPLIB NAMES ziplib HINTS ${PLANSZOWKER_WINDOWS_LIB_DIR})
else ()
    set(ZIPLIB_BZIP2 ziplib_bzip2 CACHE INTERNAL "")
    set(ZIPLIB_LZMA ziplib_lzma CACHE INTERNAL "")
    set(ZIPLIB_ZLIB ziplib_zlib CACHE INTERNAL "")
    set(ZIPLIB ziplib CACHE INTERNAL "")
endif()

configure_file("Config.h.in" "Config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${LUA_INCLUDE_DIR})

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")

include_directories(libs)

add_subdirectory(libs/Utils/ClientInfo)
add_subdirectory(libs/Utils/ErrorHandler)
add_subdirectory(libs/Utils/Logger)
add_subdirectory(libs/Utils/CompilerUtils)
add_subdirectory(libs/Utils/TimeMeasurement)
add_subdirectory(libs/Utils/Rng)
add_subdirectory(libs/Utils/AssetsManager)
add_subdirectory(libs/Utils/TickThread)
add_subdirectory(libs/Utils/ThreadSafeQueue)
add_subdirectory(libs/Utils/External/Base64)
add_subdirectory(libs/Utils/External/EasyLogging)
add_subdirectory(libs/Utils/External/ZipLib)
add_subdirectory(libs/Network/NetworkHandler)
add_subdirectory(libs/Supervisor)
add_subdirectory(libs/Utils/PlametaParser)
add_subdirectory(libs/Games)
add_subdirectory(libs/GamesClient)
add_subdirectory(libs/GamesServer)
add_subdirectory(planszowker_client)
add_subdirectory(planszowker_server)
add_subdirectory(planszowker_designer)

# Testing
enable_testing()
add_subdirectory(functional_tests) # Functional tests
add_subdirectory(tests) # Unit tests - failing for some reason
