cmake_minimum_required(VERSION 3.0)

project(AstraProj VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/dep)
include_directories(${CMAKE_SOURCE_DIR}/dep/protobuf/include)

link_directories(${CMAKE_SOURCE_DIR}/dep/tinyxml/lib)
link_directories(${CMAKE_SOURCE_DIR}/dep/protobuf/lib)

add_subdirectory(common)
add_subdirectory(gamesvr)