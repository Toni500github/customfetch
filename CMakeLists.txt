# Please use the makefile if something goes wrong
#

cmake_minimum_required(VERSION 3.10)

project(customfetch)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -O0 -DDEBUG=1 -Wall -Wextra -Wpedantic -Wno-unused-parameter")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include_directories(include)
include_directories(include/libcufetch)
include_directories(include/libs)

set_source_files_properties(
    "src/libs/toml++/toml.cpp"
    "src/libs/tiny-process-library/process.cpp"
    PROPERTIES COMPILE_FLAGS "-fvisibility=default"
)

file(GLOB
	SRC
	"src/*.cpp"
	"src/core-modules/*.cc"
	"src/core-modules/linux/*.cc"
	"src/core-modules/android/*.cc"
	"src/core-modules/macos/*.cc"
	"src/core-modules/linux/utils/*.cc"
	"src/libs/json/json.cpp"
	"src/libs/toml++/toml.cpp"
	"src/libs/getopt_port/getopt.c")

function(enable_lto target)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "enable_lto called on non-existent target: ${target}")
    endif()

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
    	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        	# GCC supports -ffat-lto-objects for archives
        	target_compile_options(${target} PRIVATE -flto=auto -ffat-lto-objects)
        	target_link_options(${target} PRIVATE -flto=auto)
    	elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        	# Use ThinLTO on Clang (better incremental builds)
        	target_compile_options(${target} PRIVATE -flto=thin)
        	target_link_options(${target} PRIVATE -flto=thin)
	else()
		message(WARNING "LTO not configured for compiler: ${CMAKE_CXX_COMPILER_ID}")
	endif()
    endif()
endfunction()

if(GUI_APP)
        set(TARGET_NAME customfetch-gui)
else()
        set(TARGET_NAME customfetch)
endif()

add_executable(${TARGET_NAME} ${SRC})
enable_lto(${TARGET_NAME})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,${ORIGIN}/")
endif()

# Get git info hash and branch
execute_process(COMMAND ./scripts/generateVersion.sh
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# https://github.com/libcpr/cpr/blob/5f475522597b8f3721e2440daddeced7a969f24c/CMakeLists.txt#L39
macro(add_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT DEFINE_OPTION)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow overriding the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME} AND DEFINE_OPTION)
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "  ${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

message(STATUS "Set flags:")
message(STATUS "=================")
add_option(GUI_APP "Build GTK3 application" 0 1)
add_option(USE_DCONF "Compile customfetch with dconf support" 1 0)
add_option(VARS "Add additional flags at CXXFLAGS" "" 1)
message(STATUS "=================")

if(GUI_APP)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTKMM REQUIRED gtkmm-3.0 gdkmm-3.0)

    target_include_directories(${TARGET_NAME} PUBLIC ${GTKMM_INCLUDE_DIRS})
    target_compile_options(${TARGET_NAME} PUBLIC ${GTKMM_CFLAGS_OTHER})
    target_link_directories(${TARGET_NAME} PUBLIC ${GTKMM_LIBRARY_DIRS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${GTKMM_LIBRARIES})
endif()

if(USE_DCONF)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DCONF dconf)

    if(DCONF_FOUND)
        message(STATUS "Found dconf - enabling dconf support")
        target_compile_definitions(${TARGET_NAME} PRIVATE USE_DCONF=1)
        target_include_directories(${TARGET_NAME} PUBLIC ${DCONF_INCLUDE_DIRS})
        target_compile_options(${TARGET_NAME} PUBLIC ${DCONF_CFLAGS_OTHER})
    else()
        message(STATUS "dconf not found - disabling dconf support")
        target_compile_definitions(${TARGET_NAME} PRIVATE USE_DCONF=0)
    endif()
else()
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_DCONF=0)
endif()

# fmt
add_library(fmt STATIC
	"src/libs/fmt/os.cc"
	"src/libs/fmt/format.cc")

set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
enable_lto(fmt)
target_link_libraries(${TARGET_NAME} PUBLIC fmt)

# tiny-process-library (integrated from its own CMakeLists.txt)
add_library(tiny-process-library STATIC
    "src/libs/tiny-process-library/process.cpp")
add_library(tiny-process-library::tiny-process-library ALIAS tiny-process-library)

if(MSVC)
    target_compile_definitions(tiny-process-library PRIVATE /D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(tiny-process-library PRIVATE -std=c++11 -Wall -Wextra)
endif()

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    target_sources(tiny-process-library PRIVATE "src/libs/tiny-process-library/process_win.cpp")
    # If compiled using MSYS2, use sh to run commands
    if(MSYS)
        target_compile_definitions(tiny-process-library PUBLIC MSYS_PROCESS_USE_SH)
    endif()
else()
    target_sources(tiny-process-library PRIVATE "src/libs/tiny-process-library/process_unix.cpp")
endif()

set_target_properties(tiny-process-library PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

find_package(Threads REQUIRED)
target_link_libraries(tiny-process-library Threads::Threads)
target_include_directories(tiny-process-library PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libs/tiny-process-library>
    $<INSTALL_INTERFACE:include>
)
enable_lto(tiny-process-library)
target_link_libraries(${TARGET_NAME} PUBLIC tiny-process-library)

# libcufetch
set(CUFETCH_HEADERS
    include/libcufetch/config.hh
    include/libcufetch/common.hh
    include/libcufetch/cufetch.hh
)

add_library(cufetch SHARED
    libcufetch/cufetch.cc
    libcufetch/parse.cc
    src/libs/toml++/toml.cpp
    src/util.cpp
)

set_target_properties(cufetch PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME "cufetch"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(cufetch PUBLIC tiny-process-library)
target_link_libraries(cufetch PUBLIC fmt)
target_link_libraries(${TARGET_NAME} PUBLIC cufetch)

# locale
add_custom_target(locale
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/make_mo.sh ${CMAKE_SOURCE_DIR}/locale/
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating locale message object files"
)

add_dependencies(${TARGET_NAME} locale)

# Installation prefixes
set(MANPREFIX "${CMAKE_INSTALL_PREFIX}/share/man" CACHE PATH "Man page installation directory")
set(APPPREFIX "${CMAKE_INSTALL_PREFIX}/share/applications" CACHE PATH "Application desktop file directory")
set(LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale" CACHE PATH "Locale files directory")
set(ICONPREFIX "${CMAKE_INSTALL_PREFIX}/share/pixmaps" CACHE PATH "Icon installation directory")
target_compile_definitions(${TARGET_NAME} PRIVATE
    VERSION="1.0.0"
    MANPREFIX="${MANPREFIX}"
    APPPREFIX="${APPPREFIX}"
    LOCALEDIR="${LOCALEDIR}"
    ICONPREFIX="${ICONPREFIX}"
    $<$<BOOL:${GUI_APP}>:GUI_APP=1>
)

# Install executable
install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin
        COMPONENT runtime)

# Create symlink
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${TARGET_NAME} \${CMAKE_INSTALL_PREFIX}/bin/cufetch)")

# Install the rest
install(FILES ${CMAKE_SOURCE_DIR}/customfetch.1
        DESTINATION share/man/man1
        COMPONENT documentation)

install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION share/licenses/customfetch
        COMPONENT documentation)

install(FILES $<TARGET_FILE:fmt>
    DESTINATION lib
    RENAME libcufetch-fmt.a
    COMPONENT runtime
)

install(TARGETS cufetch
    LIBRARY DESTINATION lib
    COMPONENT runtime
)

install(DIRECTORY include/libcufetch/
    DESTINATION include/libcufetch
    COMPONENT development
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ascii/
        DESTINATION share/customfetch/ascii
        COMPONENT runtime)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/
        DESTINATION share/customfetch/examples
        COMPONENT runtime)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/icons/
        DESTINATION share/pixmaps/customfetch
        COMPONENT runtime)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/locale/
        DESTINATION share/locale
        COMPONENT runtime)

install(CODE "
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink libcufetch.so.1 ${CMAKE_INSTALL_PREFIX}/lib/libcufetch.so
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink libcufetch.so.1.0.0 ${CMAKE_INSTALL_PREFIX}/lib/libcufetch.so.1
    )
")

install(CODE "
    execute_process(
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/make_mo.sh ${CMAKE_SOURCE_DIR}/locale/
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
" COMPONENT runtime)

if(GUI_APP)
    install(FILES ${CMAKE_SOURCE_DIR}/customfetch.desktop
            DESTINATION share/applications
            COMPONENT runtime)
endif()

# Uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
