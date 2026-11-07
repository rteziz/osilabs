cmake_minimum_required(VERSION 3.16)
project(1_6_mythread C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(mythread STATIC mythread.c)
target_include_directories(mythread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(1_6_mythread main.c)
target_link_libraries(1_6_mythread PRIVATE mythread)

enable_testing()
add_executable(test_mythread test_mythread.c)
target_compile_options(test_mythread PRIVATE -UNDEBUG)
target_link_libraries(test_mythread PRIVATE mythread)
add_test(NAME mythread_lifecycle COMMAND test_mythread)
set_tests_properties(mythread_lifecycle PROPERTIES TIMEOUT 20)
