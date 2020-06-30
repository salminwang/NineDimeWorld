
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SRC_LIST *.h *.cc *.cpp *.c)
set(TARGET common)
add_library(${TARGET} STATIC ${SRC_LIST})
target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})