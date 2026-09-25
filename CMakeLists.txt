cmake_minimum_required(VERSION 3.14)
project(pwr_simulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(pwr_server
    src/main.cpp
    src/reactor.cpp
    src/simulation.cpp
    src/server.cpp
)

target_include_directories(pwr_server PRIVATE
    include
)

find_package(Threads REQUIRED)
target_link_libraries(pwr_server PRIVATE
    Threads::Threads
)

if(MSVC)
    target_compile_options(pwr_server PRIVATE /W4)
else()
    target_compile_options(pwr_server PRIVATE -Wall -Wextra -O3)
endif()
