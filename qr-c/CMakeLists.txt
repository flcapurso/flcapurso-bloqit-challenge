cmake_minimum_required(VERSION 3.15)
project(qr-c)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

add_executable( qr-c
    src/main.cpp
    src/logger.cpp
    src/serial.cpp
    src/socket.cpp
)