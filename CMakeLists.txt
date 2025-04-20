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
	"src/query/linux/*.cpp"
	"src/query/android/*.cpp"
	"src/query/linux/utils/*.cpp"
	"src/toml++/toml.cpp")

if(GUI_APP)
set(TARGET_NAME customfetch-gui)
else()
set(TARGET_NAME customfetch)
endif()

add_executable(${TARGET_NAME} ${SRC})

# Get git info hash and branch
execute_process(COMMAND ./scripts/generateVersion.sh
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

set(VERSION "1.0.0")
# https://github.com/libcpr/cpr/blob/5f475522597b8f3721e2440daddeced7a969f24c/CMakeLists.txt#L39
macro(add_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow overriding the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "  ${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

message(STATUS "Set flags:")
message(STATUS "=================")
add_option(GUI_APP "Build GTK3 application" 0)
add_option(USE_DCONF "Compile customfetch with dconf support" 1)
add_option(VARS "Add additional flags at CXXFLAGS" "")
message(STATUS "=================")

if(GUI_APP)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTKMM REQUIRED gtkmm-3.0 gdkmm-3.0)
	target_link_libraries(${TARGET_NAME} PUBLIC ${GTKMM_LIBRARIES})
	target_include_directories(${TARGET_NAME} PUBLIC ${GTKMM_INCLUDE_DIRS})
	target_compile_options(${TARGET_NAME} PUBLIC ${GTKMM_CFLAGS_OTHER})
endif()

if(USE_DCONF)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(DCONF dconf)
        target_link_libraries(${TARGET_NAME} PUBLIC ${DCONF_LIBRARIES})
        target_include_directories(${TARGET_NAME} PUBLIC ${DCONF_INCLUDE_DIRS})
        target_compile_options(${TARGET_NAME} PUBLIC ${DCONF_CFLAGS_OTHER})
endif()


# fmt
add_library(fmt STATIC
	"src/fmt/os.cc"
	"src/fmt/format.cc")

target_link_libraries(${TARGET_NAME} PUBLIC fmt)
