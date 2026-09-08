cmake_minimum_required(VERSION 3.21)

# A consumer that has nothing but an install tree: it finds the package, links
# vnm::fonts and calls into it. Nothing here names a path inside the vnm_fonts
# build or source tree, which is the whole point - the in-tree consumer fixture
# links through the alias the build defines and cannot tell a missing package
# from a working one.
project(vnm_fonts_installed_package_consumer LANGUAGES CXX)

find_package(vnm_fonts REQUIRED)

if(NOT TARGET vnm::fonts)
    message(FATAL_ERROR "The installed package did not define vnm::fonts.")
endif()
if(NOT VNM_FONTS_DIRECTORY)
    message(FATAL_ERROR "The installed package did not set VNM_FONTS_DIRECTORY.")
endif()
if(NOT EXISTS ${VNM_FONTS_DIRECTORY}/RobotoCondensed-Regular.ttf)
    message(FATAL_ERROR
        "The installed VNM_FONTS_DIRECTORY (${VNM_FONTS_DIRECTORY}) does not hold "
        "the shipped fonts, so the file contract does not survive installation.")
endif()

# Linking is the assertion. An unresolved vnm_fonts::register_shipped_font is
# exactly how the missing package surfaced in vnm_framework's packaging smoke
# test, so the call has to be reachable from a real link, not merely compiled.
add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE vnm::fonts)
