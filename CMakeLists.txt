cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(Firmware_TEST_IoT)

target_sources(app PRIVATE src/main.cpp
    src/EC21V2.cpp
    src/UARTE.cpp
    src/RTC.cpp)

target_include_directories(app PRIVATE src/)