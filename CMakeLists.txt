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
	"src/query/macos/*.cpp"
	"src/query/linux/utils/*.cpp"
	"src/libs/json/json.cpp"
	"src/libs/toml++/toml.cpp")

if(GUI_APP)
	set(TARGET_NAME customfetch-gui)
else()
	set(TARGET_NAME customfetch)
endif()

add_executable(${TARGET_NAME} ${SRC})

# Get git info hash and branch
execute_process(COMMAND ./scripts/generateVersion.sh
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

target_compile_definitions(${TARGET_NAME} PRIVATE
        VERSION="1.0.0"
)

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
    if(DCONF_FOUND)
        message(STATUS "Found dconf - enabling dconf support")
        target_compile_definitions(${TARGET_NAME} PRIVATE USE_DCONF=1)
        target_link_libraries(${TARGET_NAME} PUBLIC ${DCONF_LIBRARIES})
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

target_link_libraries(${TARGET_NAME} PUBLIC fmt)

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

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/ascii/
        DESTINATION share/customfetch/ascii
        COMPONENT runtime)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/
        DESTINATION share/customfetch/examples
        COMPONENT runtime)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/locale/
        DESTINATION share/locale
        COMPONENT runtime)

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
