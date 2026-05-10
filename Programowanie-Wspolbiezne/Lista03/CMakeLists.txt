cmake_minimum_required(VERSION 3.15)

project(Lista03 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(prog1 prog1.cpp)
add_executable(prog2 prog2.cpp)
add_executable(prog3 prog3.cpp)
add_executable(prog4 prog4.cpp)
add_executable(zad2 zad2.cpp)
add_executable(zad3 zad3.cpp)

target_link_libraries(prog1 PRIVATE Threads::Threads)
target_link_libraries(prog2 PRIVATE Threads::Threads)
target_link_libraries(prog3 PRIVATE Threads::Threads)
target_link_libraries(prog4 PRIVATE Threads::Threads)
target_link_libraries(zad2 PRIVATE Threads::Threads)
target_link_libraries(zad3 PRIVATE Threads::Threads)