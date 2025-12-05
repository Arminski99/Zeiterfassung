cmake_minimum_required(VERSION 3.28.1)
project(time-keepers-api)

set(CMAKE_CXX_STANDARD 20)


find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_executable(time-keepers-api main.cpp)

target_link_libraries(time-keepers-api PRIVATE Crow::Crow asio::asio nlohmann_json::nlohmann_json cpr::cpr)

if (WIN32)
    target_link_libraries(time-keepers-api PRIVATE ws2_32 Mswsock)
endif()