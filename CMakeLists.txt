# Please use the makefile if something goes wrong
#

cmake_minimum_required(VERSION 3.10)

project(customfetch)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -O0 -DDEBUG=1 -Wall -Wextra -Wpedantic -Wno-unused-parameter")

include_directories(include)

file(GLOB
	SRC
	"src/*.cpp"
	"src/query/unix/*.cpp"
	"src/query/android/*.cpp"
	"src/query/unix/utils/*.cpp"
	"src/toml++/toml.cpp")

add_executable(customfetch ${SRC})

# get git branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(VERSION "0.10.2")
target_compile_definitions(customfetch PRIVATE
    VERSION="${VERSION}"
    BRANCH="${GIT_BRANCH}"
)

# fmt
add_library(fmt STATIC
	"src/fmt/os.cc"
	"src/fmt/format.cc")

target_link_libraries(customfetch fmt)
