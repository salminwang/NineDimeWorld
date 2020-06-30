#获取所有文件
file(GLOB_RECURSE SRC_LIST *.h *.cc *.cpp *.c)
#设置工程名
project(gamesvr)
#目标文件
set(TARGET gamesvrd)
#目标文件
add_executable(${TARGET} ${SRC_LIST})
#添加头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# 添加链接库
target_link_libraries(${TARGET} common tinyxml pthread)