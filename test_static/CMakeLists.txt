cmake_minimum_required(VERSION 3.2)
project(Static_Tests)

#include(CTest) # We include CTest which is part of CMake
enable_testing()

set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive")

add_compile_definitions(UNIT_TEST)

include_directories(../.pio/libdeps/esp32cam/ArduinoJson/src)

set(SOURCE_FILES static_tests.cpp ../src/connectors/request.cpp)
add_executable(static_tests ${SOURCE_FILES})

add_test(NAME static_tests COMMAND static_tests)
