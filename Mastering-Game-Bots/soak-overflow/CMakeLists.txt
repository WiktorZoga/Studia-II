cmake_minimum_required(VERSION 3.16)
project(mastering_game_bots LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

add_executable(bot main.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bot PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(cover_tests tests/test_cover.cpp)
    add_test(NAME cover_geometry COMMAND cover_tests)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME codingame_protocol
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_protocol.py $<TARGET_FILE:bot>
    )
endif()
