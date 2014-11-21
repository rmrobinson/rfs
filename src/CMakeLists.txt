
file(GLOB RFS_SRC *.c)

add_library(rfs ${RFS_SRC})
target_link_libraries(rfs uv)
