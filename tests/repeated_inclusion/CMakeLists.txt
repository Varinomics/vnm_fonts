cmake_minimum_required(VERSION 3.21)

# logonomic pulls vnm_fonts in through several dependency paths in one
# configure. This fixture reproduces that: adding the repository twice must
# configure, and must not redefine a target or overwrite a cache variable a
# consumer already set.
project(vnm_fonts_repeated_inclusion LANGUAGES CXX)

if(NOT VNM_FONTS_SOURCE_DIR)
    message(FATAL_ERROR "VNM_FONTS_SOURCE_DIR must name the vnm_fonts checkout.")
endif()

set(VNM_FONTS_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/a-consumer-choice"
    CACHE PATH "Set first, so a FORCE inside vnm_fonts would show up here")

add_subdirectory(${VNM_FONTS_SOURCE_DIR} vnm_fonts_first)
add_subdirectory(${VNM_FONTS_SOURCE_DIR} vnm_fonts_second)

if(NOT TARGET vnm_fonts)
    message(FATAL_ERROR "vnm_fonts did not define its library target.")
endif()
if(NOT TARGET vnm::fonts)
    message(FATAL_ERROR "vnm_fonts did not define its alias target.")
endif()
if(NOT VNM_FONTS_DIRECTORY STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/a-consumer-choice")
    message(FATAL_ERROR
        "vnm_fonts overwrote VNM_FONTS_DIRECTORY, which is now "
        "${VNM_FONTS_DIRECTORY}. It must not FORCE a cache variable.")
endif()
