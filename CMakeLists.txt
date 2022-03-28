cmake_minimum_required(VERSION 3.1)
project(MiniSQL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_BUILD_TYPE "Debug")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

find_package(Boost 1.53.0 COMPONENTS system thread filesystem)

set(readline_INC_DIR "${PROJECT_SOURCE_DIR}/third_parties/gnu_readline/include")
set(webserver_INC_DIR "${PROJECT_SOURCE_DIR}/third_parties/simple_web_server")
set(argparse_INC_DIR "${PROJECT_SOURCE_DIR}/third_parties/argparse/include")
set(SRC_INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(readline_LIB_DIR "${PROJECT_SOURCE_DIR}/third_parties/gnu_readline/lib")
include_directories(${readline_INC_DIR} ${SRC_INC_DIR} ${webserver_INC_DIR} ${Boost_INCLUDE_DIR} ${argparse_INC_DIR})
link_libraries(${Boost_LIBRARIES} "${readline_LIB_DIR}/macos_arm64/libreadline.8.1.dylib" "${readline_LIB_DIR}/macos_arm64/libhistory.8.1.dylib")
file(GLOB_RECURSE SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")
add_executable(minisql ${SRC})