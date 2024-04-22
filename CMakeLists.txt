cmake_minimum_required(VERSION 3.13)
project(FaST)
add_subdirectory(src)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(FaST_TEST "Build tests" ON)
if(FaST_TEST)
  message(STATUS "Tests enabled")
  add_subdirectory(test)
endif()


set(INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
set(BUILD_DIR "${PROJECT_SOURCE_DIR}/build")