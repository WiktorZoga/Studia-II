cmake_minimum_required(VERSION 3.20)

project(ConcurrentProgrammingCpp LANGUAGES CXX)

include(CTest)
include(cmake/Exercise.cmake)

option(ENABLE_SANITIZERS "Enable AddressSanitizer or ThreadSanitizer" OFF)
option(USE_TSAN "Use ThreadSanitizer instead of AddressSanitizer" OFF)

foreach(list_number RANGE 1 14)
    if(list_number LESS 10)
        set(list_suffix "0${list_number}")
    else()
        set(list_suffix "${list_number}")
    endif()
    set(list_directory "${CMAKE_CURRENT_SOURCE_DIR}/Lista${list_suffix}")
    if(EXISTS "${list_directory}/CMakeLists.txt")
        add_subdirectory("Lista${list_suffix}")
    endif()
endforeach()
