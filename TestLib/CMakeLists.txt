cmake_minimum_required(VERSION 3.2)

project(UnitTests)

set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive")

add_compile_definitions(UNIT_TEST)

#Test Begin
add_subdirectory(MyLib)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(MyLibTests)
endif ()

enable_testing()

project(TestLib LANGUAGES CXX)

add_executable(TestLib /test_static/my_lib_tests.cpp)

target_link_libraries(TestLib PRIVATE test_static)

add_test(NAME TestLib COMMAND TestLib)
#Test End