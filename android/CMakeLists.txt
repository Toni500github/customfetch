# Same file as the one before, just used to be compiled for android

cmake_minimum_required(VERSION 3.10)

project(customfetch)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -O0 -DDEBUG=1 -Wall -Wextra -Wpedantic -Wno-unused-parameter")

include_directories(${CMAKE_SOURCE_DIR}/../include)

file(GLOB
		SRC
		"${CMAKE_SOURCE_DIR}/../src/*.cpp"
		"${CMAKE_SOURCE_DIR}/../src/query/unix/*.cpp"
		"${CMAKE_SOURCE_DIR}/../src/query/android/*.cpp"
		"${CMAKE_SOURCE_DIR}/../src/query/unix/utils/*.cpp"
		"${CMAKE_SOURCE_DIR}/../src/toml++/toml.cpp"
		"app/src/main/cpp/customfetch_android.cpp"
)

add_library(customfetch SHARED ${SRC})

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
    DANDROID_APP=1
)

# fmt
add_library(fmt STATIC
        "${CMAKE_SOURCE_DIR}/../src/fmt/os.cc"
        "${CMAKE_SOURCE_DIR}/../src/fmt/format.cc")

# add library
target_link_libraries(customfetch fmt log android)
