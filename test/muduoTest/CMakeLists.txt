project(ChatServer)
#配置编译选项
set(CMAKE_CXX_FLAGS ${CMKAE_CXX_FLAGS} -g)
message(STATUS "CMAKE_SOURCE_DIR source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_SOURCE_DIR source directory: ${CMAKE_SOURCE_DIR}")
#set(CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR})

#配置头文件搜索路径
#include_directories()

#配置库文件搜索路径
#link_directories()

#设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)

#把指定路径下的所有源文件名字放入变量名SRC_LIST里面
#aux_source_directory(. SRC_LIST)


#表示生成可执行文件server
add_executable(server ${SRC_LIST})

#设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

#表示server这个目标程序，需要链接muduo_net muduo_base pthread 这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)
