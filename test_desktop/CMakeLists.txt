cmake_minimum_required(VERSION 3.2)
project(Static_Tests)

enable_testing()

set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive")

add_compile_definitions(UNIT_TEST)

include_directories(../.pio/libdeps/esp32cam/ArduinoJson/src)

set(ADDITIONAL_SOURCES ../src/connectors/request.cpp ../src/gadgets/sh_gadget.cpp ../src/connectors/ir_gadget.cpp)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)
include (gcovr)

message("Scanning for test files...")
file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(filename ${files})
    set(f_test "")
    cmake_path(GET filename FILENAME f_test)
    if (${f_test} MATCHES "test_[a-z_]+\\.cpp")
        set(SOURCE_FILES ${filename} ${ADDITIONAL_SOURCES})
        string(REPLACE ".cpp" "" f_name ${f_test})
        add_executable(${f_name} ${SOURCE_FILES})
        add_test(NAME ${f_name} COMMAND ${f_name})
        message("Added test '${f_name}' with executable '${f_name}' from '${filename}'")
    endif()
endforeach()
message("")
