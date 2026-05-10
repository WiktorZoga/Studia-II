cmake_minimum_required(VERSION 3.15)

project(Lista08 CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(zad2 zad2.cpp)

target_link_libraries(zad2 PRIVATE Threads::Threads)