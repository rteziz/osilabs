cmake_minimum_required(VERSION 3.16)
project(1_7_umythread C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Build on Linux: this project uses ucontext (see README.md).")
endif()

add_library(uthread STATIC uthread.c)
target_include_directories(uthread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(uthread PUBLIC _XOPEN_SOURCE=600)
target_compile_options(uthread PRIVATE -Wall -Wextra -Werror)

add_executable(1_7_umythread main.c)
target_link_libraries(1_7_umythread PRIVATE uthread)

enable_testing()
add_executable(test_uthread test_uthread.c)
target_compile_options(test_uthread PRIVATE -UNDEBUG -Wall -Wextra -Werror)
target_link_libraries(test_uthread PRIVATE uthread)
add_test(NAME uthread_lifecycle COMMAND test_uthread)
set_tests_properties(uthread_lifecycle PROPERTIES TIMEOUT 10)
