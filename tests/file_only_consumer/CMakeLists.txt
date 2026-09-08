cmake_minimum_required(VERSION 3.21)

# What vnm_msdf_text and vnm_plot do: add this repository for the path to the
# verbatim files, and link nothing from it. Qt is present here, so the library
# target exists - the point of the fixture is that merely existing must not
# cost the consumer a 29 MB resource compilation.
project(vnm_fonts_file_only_consumer LANGUAGES CXX)

if(NOT VNM_FONTS_SOURCE_DIR)
    message(FATAL_ERROR "VNM_FONTS_SOURCE_DIR must name the vnm_fonts checkout.")
endif()

add_subdirectory(${VNM_FONTS_SOURCE_DIR} vnm_fonts)

if(NOT TARGET vnm::fonts)
    message(FATAL_ERROR
        "Qt was found for this fixture, so vnm::fonts should exist. The fixture "
        "is measuring what the default build costs, not whether Qt is present.")
endif()
if(NOT VNM_FONTS_DIRECTORY)
    message(FATAL_ERROR "VNM_FONTS_DIRECTORY is not set.")
endif()

# The property that guarantees the artifact is absent. Asserted here as well as
# by the driver globbing the tree, because this half is generator-independent
# and says plainly what is being relied on.
get_target_property(vnm_fonts_excluded vnm_fonts EXCLUDE_FROM_ALL)
if(NOT vnm_fonts_excluded)
    message(FATAL_ERROR
        "vnm_fonts is part of the default build target, so every consumer that "
        "only wanted VNM_FONTS_DIRECTORY compiles the font resource for nothing.")
endif()

# A header-free translation unit, so building this fixture needs the compiler
# and the archiver and no toolchain environment beyond them.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/atlas.cpp "int vnm_fonts_file_only_consumer() { return 0; }\n")
add_library(atlas STATIC ${CMAKE_CURRENT_BINARY_DIR}/atlas.cpp)
target_compile_definitions(atlas PRIVATE VNM_FONTS_DIR="${VNM_FONTS_DIRECTORY}")
