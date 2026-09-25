cmake_minimum_required(VERSION 3.14)
project(pwr_simulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch cpp-httplib for REST API server
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# Fetch nlohmann_json for JSON serialization
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(pwr_server
    src/main.cpp
    src/reactor.cpp
)

target_include_directories(pwr_server PRIVATE
    include
)

target_link_libraries(pwr_server PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)

if(MSVC)
    target_compile_options(pwr_server PRIVATE /W4)
else()
    target_compile_options(pwr_server PRIVATE -Wall -Wextra -O3)
endif()
