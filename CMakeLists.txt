cmake_minimum_required(VERSION 3.15)
project(ddb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-unused-function")

include_directories(/usr/include/readline)
add_executable(ddb main.c repl.c debug.c list.c)
target_link_libraries(ddb readline)
