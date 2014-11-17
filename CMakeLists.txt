project(rfs)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Werror -Wall -Wextra -g -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-equal -Wundef -Wshadow -Wpointer-arith")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align -Wcast-qual")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes -Wstrict-prototypes")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-overflow=5 -Wwrite-strings")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstack-protector -Wformat-security -Wunreachable-code")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)

add_subdirectory(test)

