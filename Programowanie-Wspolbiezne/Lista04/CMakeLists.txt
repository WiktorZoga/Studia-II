cmake_minimum_required(VERSION 3.15)

project(Lista04 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSTION OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(zad1 zad1.cpp)
add_executable(zad2 zad2.cpp)
add_executable(zad3 zad3.cpp)

target_link_libraries(zad1 PRIVATE Threads::Threads)
target_link_libraries(zad2 PRIVATE Threads::Threads)
target_link_libraries(zad3 PRIVATE Threads::Threads)