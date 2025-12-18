cmake_minimum_required(VERSION 3.10)

project (gitlite)

# set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set path for compiled executable files
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# all cpp  files in scr
file(GLOB SRC_FILES "src/*.cpp")

# executable file gitlite
add_executable(gitlite main.cpp ${SRC_FILES})