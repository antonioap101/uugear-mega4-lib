cmake_minimum_required(VERSION 3.16)
project(uugear_mega4_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include(CTest)

# =======================================================
# 1. Look for libusb, or fetch it if not found
# =======================================================
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0)
endif()

if (NOT LIBUSB_FOUND)
    message(STATUS "libusb not found in system — fetching via FetchContent")
    FetchContent_Declare(
            libusb
            GIT_REPOSITORY https://github.com/libusb/libusb.git
            GIT_TAG v1.0.29
    )
    FetchContent_MakeAvailable(libusb)
    set(LIBUSB_INCLUDE_DIRS ${libusb_SOURCE_DIR}/libusb)
    set(LIBUSB_LIBRARIES usb-1.0)
endif()

# =======================================================
# 2. Define the uugear_mega4_lib library
# =======================================================
add_library(uugear_mega4_lib STATIC src/Mega4Hub.cpp)
target_include_directories(uugear_mega4_lib PUBLIC include ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(uugear_mega4_lib ${LIBUSB_LIBRARIES})

message(STATUS "LIBUSB_INCLUDE_DIRS: ${LIBUSB_INCLUDE_DIRS}")
message(STATUS "LIBUSB_LIBRARIES: ${LIBUSB_LIBRARIES}")

# =======================================================
# 3. Usage example: toggle_port
# =======================================================
add_executable(toggle_port examples/toggle_port.cpp)
target_link_libraries(toggle_port uugear_mega4_lib)

# =======================================================
# 4. Tests with GoogleTest
# =======================================================
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
)
# Avoid pthreads and multithreading warnings in tests
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests
        tests/test_Mega4Hub.cpp
        tests/test_Mega4Hub.cpp
        tests/test_main.cpp
#        include/Mega4Hub.hpp
)
target_link_libraries(tests
        uugear_mega4_lib
        GTest::gtest
        GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(tests)

# =======================================================
# 5. Output build information
# =======================================================
message(STATUS "LIBUSB include dirs: ${LIBUSB_INCLUDE_DIRS}")
message(STATUS "LIBUSB libraries: ${LIBUSB_LIBRARIES}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
