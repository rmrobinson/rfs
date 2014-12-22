
include_directories(..)

add_executable(rfs_9p_test rfs_9p_test.c)
target_link_libraries(rfs_9p_test rfs)

add_executable(rfs_api_test rfs_api_test.c)
target_link_libraries(rfs_api_test rfs)

add_executable(rfs_log_test rfs_log_test.c)
target_link_libraries(rfs_log_test rfs)
