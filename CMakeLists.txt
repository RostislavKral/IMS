cmake_minimum_required(VERSION 3.27)
project(IMS)

set(CMAKE_CXX_STANDARD 14)

add_executable(IMS main.cpp
        main.h)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} m simlib)
