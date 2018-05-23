cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(goliath-broker LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(cppzmq REQUIRED)
find_package(Boost
        COMPONENTS
            log
        REQUIRED
        )
find_package(Threads REQUIRED)

add_subdirectory(third_party)

set(SOURCES
        broker.cpp
        )

include_directories(
        ${ZeroMQ_INCLUDE_DIR}
        ${cppzmq_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
        )

add_executable(${PROJECT_NAME} ${SOURCES})

add_definitions(-DBOOST_LOG_DYN_LINK)

target_link_libraries(${PROJECT_NAME}
        goliath-foundation-module
        ${cppzmq_LIBRARY}
        ${Boost_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

configure_file(resources/logo-broker.txt ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/logo-broker.txt COPYONLY)
