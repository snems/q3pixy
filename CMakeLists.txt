cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)

project(q3pixy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_BUILD_TYPE Release)

include_directories(.)
include_directories(include)

file(GLOB SOURCES_COMMON
        include/*.hxx
        src/*.cxx
        )

add_executable(q3pixy ${SOURCES_COMMON})

target_link_libraries(q3pixy jsoncpp)
set_target_properties(q3pixy PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS q3pixy DESTINATION bin)

