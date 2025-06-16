cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(pdf2svgwrapper VERSION 0.1.0)

# Prevent CMake from adding 'lib' prefix on Unix-like systems
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# Set platform-specific compiler flags and definitions
if(WIN32)
    # Static link MinGW runtime on Windows
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
    add_definitions(-DBUILDING_DLL)
else()
    # Add position independent code flag for Linux
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    # Add visibility flags for Linux
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_GLIB REQUIRED IMPORTED_TARGET poppler-glib)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GOBJECT REQUIRED IMPORTED_TARGET gobject-2.0)

# Create a shared library
add_library(pdf2svgwrapper SHARED
    src/pdf2svg.cpp
    src/pdf2svg.h
)

# Set library properties
if(WIN32)
    set_target_properties(pdf2svgwrapper PROPERTIES 
        PREFIX ""
        OUTPUT_NAME "pdf2svgwrapper"
        SUFFIX ".dll"
    )
else()
    set_target_properties(pdf2svgwrapper PROPERTIES 
        PREFIX ""
        OUTPUT_NAME "pdf2svgwrapper"
        SUFFIX ".so"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Link against the required libraries
target_link_libraries(pdf2svgwrapper
    PRIVATE
    PkgConfig::POPPLER_GLIB
    PkgConfig::CAIRO
    PkgConfig::GLIB
    PkgConfig::GOBJECT
)

# Set include directories
target_include_directories(pdf2svgwrapper
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install targets
install(TARGETS pdf2svgwrapper
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# Custom target to collect dependencies
add_custom_target(collect_dependencies
    COMMAND ${CMAKE_COMMAND} 
        -DPROJECT_BINARY_DIR=${CMAKE_BINARY_DIR}
        -DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -P ${CMAKE_SOURCE_DIR}/scripts/collect_dependencies.cmake
    DEPENDS pdf2svgwrapper
)