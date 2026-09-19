cmake_minimum_required(VERSION 3.15)

project(Lista13 CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

option(USE_TSAN "Uzyj ThreadSanitizer zamiast AddressSanitizer" OFF)
option(ENABLE_SANITIZERS "Wlacz AddressSanitizer lub ThreadSanitizer" ON)

if(ENABLE_SANITIZERS)
    if(USE_TSAN)
        message(STATUS "Aktywny Sanitizer: ThreadSanitizer (TSan)")
        set(SANITIZER_FLAGS "-fsanitize=thread")
    else()
        message(STATUS "Aktywny Sanitizer: AddressSanitizer (ASan)")
        set(SANITIZER_FLAGS "-fsanitize=address" "-fno-omit-frame-pointer")
    endif()

    add_compile_options(-g)
    add_compile_options(${SANITIZER_FLAGS})
    add_link_options(${SANITIZER_FLAGS})
else()
    message(STATUS "Sanitizery sa wylaczone")
endif()

add_executable(zad1 zad1.cpp)
target_link_libraries(zad1 PRIVATE Threads::Threads)

add_executable(zad2 zad2.cpp)
target_link_libraries(zad2 PRIVATE Threads::Threads)

add_executable(zad3 zad3.cpp)
target_link_libraries(zad3 PRIVATE Threads::Threads)

add_executable(zad4 zad4.cpp)
target_link_libraries(zad4 PRIVATE Threads::Threads)

if(BUILD_TESTING)
    add_test(NAME Lista13.zad1_ebr COMMAND zad1)
    add_test(NAME Lista13.zad2_ibr COMMAND zad2)
    add_test(NAME Lista13.zad3_history_set COMMAND zad3)
    add_test(NAME Lista13.zad4_versioned_set COMMAND zad4)
endif()
