# -----------------------------------------------------------------------------
# @brief  : Root cmake file.
# @author : Enrico Fraccaroli
# -----------------------------------------------------------------------------

# Set the minimum CMake version, the project name and default build type.
cmake_minimum_required(VERSION 3.1...3.18)

# Set the project name.
project(gnuplotcpp CXX)

# Set the default build type to Debug.
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# -----------------------------------------------------------------------------
# ENABLE FETCH CONTENT
# -----------------------------------------------------------------------------

# We need this in order to import external projects.
include(FetchContent)
# Hide fetchcontent variables.
mark_as_advanced(FORCE
    FETCHCONTENT_QUIET
    FETCHCONTENT_BASE_DIR
    FETCHCONTENT_FULLY_DISCONNECTED
    FETCHCONTENT_UPDATES_DISCONNECTED
)

# -----------------------------------------------------------------------------
# OPTIONS
# -----------------------------------------------------------------------------

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(STRICT_WARNINGS "Enable strict compiler warnings" ON)
option(WARNINGS_AS_ERRORS "Treat all warnings as errors" OFF)

# -----------------------------------------------------------------------------
# DEPENDENCY (SYSTEM LIBRARIES)
# -----------------------------------------------------------------------------

find_package(Doxygen)

# -----------------------------------------------------------------------------
# LIBRARY
# -----------------------------------------------------------------------------

# Add the C++ Library.
add_library(gnuplotcpp INTERFACE)
add_library(gnuplotcpp::gnuplotcpp ALIAS gnuplotcpp)
# Inlcude header directories.
target_include_directories(gnuplotcpp INTERFACE ${PROJECT_SOURCE_DIR}/include)
# Set the library to use c++-20
target_compile_features(gnuplotcpp INTERFACE cxx_std_11)

# -----------------------------------------------------------------------------
# COMPILATION FLAGS
# -----------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Disable warnings that suggest using MSVC-specific safe functions
    target_compile_definitions(gnuplotcpp INTERFACE _CRT_SECURE_NO_WARNINGS)
    if(WARNINGS_AS_ERRORS)
        target_compile_options(gnuplotcpp INTERFACE /WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(WARNINGS_AS_ERRORS)
        target_compile_options(gnuplotcpp INTERFACE -Werror)
    endif()
endif()

if(STRICT_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # Mark system headers as external for MSVC explicitly
        # https://devblogs.microsoft.com/cppblog/broken-warnings-theory
        target_compile_options(gnuplotcpp INTERFACE /experimental:external)
        target_compile_options(gnuplotcpp INTERFACE /external:I ${CMAKE_BINARY_DIR})
        target_compile_options(gnuplotcpp INTERFACE /external:anglebrackets)
        target_compile_options(gnuplotcpp INTERFACE /external:W0)

        target_compile_options(gnuplotcpp INTERFACE /W4)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(gnuplotcpp INTERFACE -Wall -Wextra -Wconversion -pedantic)
    endif()
endif()

# -----------------------------------------------------------------------------
# EXAMPLES
# -----------------------------------------------------------------------------

if(BUILD_EXAMPLES)
    # Add the example.
    add_executable(gnuplotcpp_usage examples/example.cpp)
    target_include_directories(gnuplotcpp_usage PUBLIC ${PROJECT_SOURCE_DIR}/examples)
    target_link_libraries(gnuplotcpp_usage PUBLIC gnuplotcpp)

    # Add the example.
    add_executable(gnuplotcpp_example_line_plot examples/example_line_plot.cpp)
    target_include_directories(gnuplotcpp_example_line_plot PUBLIC ${PROJECT_SOURCE_DIR}/examples)
    target_link_libraries(gnuplotcpp_example_line_plot PUBLIC gnuplotcpp)
    
    # Add the example.
    add_executable(gnuplotcpp_example_scatter_plot examples/example_scatter_plot.cpp)
    target_include_directories(gnuplotcpp_example_scatter_plot PUBLIC ${PROJECT_SOURCE_DIR}/examples)
    target_link_libraries(gnuplotcpp_example_scatter_plot PUBLIC gnuplotcpp)
    
    # Add the example.
    add_executable(gnuplotcpp_example_line_plot_with_error_bars examples/example_line_plot_with_error_bars.cpp)
    target_include_directories(gnuplotcpp_example_line_plot_with_error_bars PUBLIC ${PROJECT_SOURCE_DIR}/examples)
    target_link_libraries(gnuplotcpp_example_line_plot_with_error_bars PUBLIC gnuplotcpp)
    
    # Add the example.
    add_executable(gnuplotcpp_example_3d_surface_plot examples/example_3d_surface_plot.cpp)
    target_include_directories(gnuplotcpp_example_3d_surface_plot PUBLIC ${PROJECT_SOURCE_DIR}/examples)
    target_link_libraries(gnuplotcpp_example_3d_surface_plot PUBLIC gnuplotcpp)
endif()

# -----------------------------------------------------------------------------
# TESTS
# -----------------------------------------------------------------------------

if(BUILD_TESTS)

    # CMake has support for adding tests to a project.
    enable_testing()

endif()

# -----------------------------------------------------------------------------
# DOCUMENTATION
# -----------------------------------------------------------------------------

if(DOXYGEN_FOUND)
    
    # Record the options that describe how to populate the specified content.
    FetchContent_Declare(
        doxygenawesome
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css
        GIT_TAG main
    )
    # Retrieve the properties related to the content.
    FetchContent_GetProperties(doxygenawesome)
    # If not populated, make the content available.
    if(NOT doxygenawesome_POPULATED)
        message(STATUS "Retrieving `doxygen-awesome-css`...")
        # Ensures the named dependencies have been populated.
        FetchContent_MakeAvailable(doxygenawesome)
        # Hide fetchcontent variables, otherwise with ccmake it's a mess.
        mark_as_advanced(FORCE
            FETCHCONTENT_UPDATES_DISCONNECTED_DOXYGENAWESOME
            FETCHCONTENT_SOURCE_DIR_DOXYGENAWESOME
        )
    endif()

    # = CUSTOMIZATION =========================================================
    set(DOXYGEN_WARN_FORMAT "$file:$line: $text")
    set(DOXYGEN_PROJECT_NAME gnuplotcpp)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${PROJECT_SOURCE_DIR}/README.md)
    set(DOXYGEN_SHOW_INCLUDE_FILES NO)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_HTML_HEADER ${doxygenawesome_SOURCE_DIR}/doxygen-custom/header.html)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET ${doxygenawesome_SOURCE_DIR}/doxygen-awesome.css)
    set(DOXYGEN_HTML_EXTRA_FILES
        ${doxygenawesome_SOURCE_DIR}/doxygen-awesome-fragment-copy-button.js
        ${doxygenawesome_SOURCE_DIR}/doxygen-awesome-paragraph-link.js
        ${doxygenawesome_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.js
    )
    doxygen_add_docs(
        gnuplotcpp_documentation
        ${PROJECT_SOURCE_DIR}/README.md
        ${PROJECT_SOURCE_DIR}/include/gnuplotcpp/gnuplot.hpp
        ${PROJECT_SOURCE_DIR}/include/gnuplotcpp/gnuplot.i.hpp
    )
endif()
