cmake_minimum_required(VERSION 3.16)
project(llvm_header_rmw_coalescing C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(LLVM 14 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(${LLVM_CMAKE_DIR}/AddLLVM.cmake)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_library(RmwCoalesce MODULE src/RmwCoalesce.cpp)
target_compile_options(RmwCoalesce PRIVATE -fno-rtti)
set_target_properties(RmwCoalesce PROPERTIES PREFIX "lib" SUFFIX ".so")
