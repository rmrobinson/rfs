project(rfs)

cmake_minimum_required(VERSION 2.8)

# TODO Return -pedantic once log macros are fixed
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Werror -Wall -Wextra -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-equal -Wundef -Wshadow -Wpointer-arith")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align -Wcast-qual")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes -Wstrict-prototypes")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-overflow=5 -Wwrite-strings")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstack-protector -Wformat-security -Wunreachable-code")
# This is to allow the logging C macros to compile
if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wno-gnu-zero-variadic-macro-arguments")
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)

add_subdirectory(test)

