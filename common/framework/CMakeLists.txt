file(GLOB_RECURSE SRC_LIST *.h *.cc *.cpp *.c)
set(TARGET astra_framework)
add_library(${TARGET} STATIC ${SRC_LIST})
target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})