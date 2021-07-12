cmake_minimum_required(VERSION 3.2)
project(Static_Tests)

enable_testing()

set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive")

add_compile_definitions(UNIT_TEST)

include_directories(../.pio/libdeps/esp32cam/ArduinoJson/src)

set(SOURCE_FILES test_example.cpp ../src/connectors/request.cpp)

file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(filename ${files})
    set(f_test "")
    cmake_path(GET filename FILENAME f_test)
    string(REPLACE ".cpp" "" f_name ${f_test})
    add_executable(${f_name} ${SOURCE_FILES})
    message("added executable ${f_name}")
    add_test(NAME ${f_name} COMMAND ${f_name})
    message("added test ${f_name} with Command ${f_name}")
    if (${f_test} MATCHES "test_[a-z_]+\\.cpp")
        string(REPLACE ".cpp" "" f_name ${f_test})
        add_executable(${f_name} ${SOURCE_FILES})
        message("added executable ${f_name}")
        add_test(NAME ${f_name} COMMAND ${f_name})
        message("added test ${f_name} with Command ${f_name}")
    endif()
endforeach()
