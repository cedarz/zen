cmake_minimum_required(VERSION 3.9)


set(CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
###############################################################################
include_directories(${CURRENT_DIR}/gl460}

file(GLOB_RECURSE SRC_CPPS "${CURRENT_DIR}/*.cpp")
file(GLOB_RECURSE SRC_HPPS "${CURRENT_DIR}/*.h" "${CURRENT_DIR}/*.hpp")
