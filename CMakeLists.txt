cmake_minimum_required(VERSION 3.7)
project(goliath-broker LANGUAGES CXX)

find_package(cppzmq)
include_directories(${ZeroMQ_INCLUDE_DIR} ${cppzmq_INCLUDE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(
    broker
    broker.cpp
    )

target_link_libraries(
    broker
    ${cppzmq_LIBRARY}
    )