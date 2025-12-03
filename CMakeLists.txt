cmake_minimum_required(VERSION 3.80)

project(plap LANGUAGES C)

add_library(plap INTERFACE)

target_include_directories(plap
    INTERFACE ${CMAKE_SOURCE_DIR}/include
)
