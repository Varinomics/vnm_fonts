cmake_minimum_required(VERSION 3.21)

# logonomic reaches this repository through several dependency paths in one
# configure - the framework, the terminal, the terminal surface, the plot, the
# MSDF text renderer and the keyboard all want it - so being added twice is
# ordinary, not a mistake. Return quietly on the second pass rather than
# redefining the targets.
if(TARGET vnm_fonts)
    return()
endif()

project(vnm_fonts LANGUAGES CXX)

# The library patches a font's family name on the way into QFontDatabase, so it
# needs Qt Gui for the font database and Qt Core for everything else. It must
# not grow a third dependency: consumers pull it in purely to register fonts.
find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(vnm_fonts STATIC
    src/vnm_font_namespace.cpp
    src/vnm_font_namespace.h
    vnm_fonts.qrc)

set_target_properties(vnm_fonts PROPERTIES AUTORCC ON)

target_include_directories(vnm_fonts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(vnm_fonts PUBLIC Qt6::Core Qt6::Gui)
target_compile_features(vnm_fonts PUBLIC cxx_std_17)

add_library(vnm::fonts ALIAS vnm_fonts)

# The supported path for a consumer that needs the file rather than the bytes.
# vnm_msdf_text and vnm_plot bake an atlas straight from the file and never
# enter Qt's font database, so nothing about them can merge and no marking
# applies; they read the verbatim files from here. Never FORCE this: a consumer
# that has already set it means it.
set(VNM_FONTS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    CACHE PATH "Directory holding the byte-verbatim shipped font files")

# A consumed copy builds no tests unless the consumer asks for them.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(vnm_fonts_tests_by_default ON)
else()
    set(vnm_fonts_tests_by_default OFF)
endif()
option(VNM_FONTS_BUILD_TESTS "Build the vnm_fonts tests" ${vnm_fonts_tests_by_default})

if(VNM_FONTS_BUILD_TESTS)
    enable_testing()

    # Registering a font needs a platform plugin but no display.
    set(vnm_fonts_test_environment "QT_QPA_PLATFORM=offscreen")

    add_executable(vnm_font_namespace_tests tests/vnm_font_namespace_tests.cpp)
    target_link_libraries(vnm_font_namespace_tests PRIVATE vnm_fonts)
    add_test(
        NAME vnm_font_namespace
        COMMAND vnm_font_namespace_tests ${VNM_FONTS_DIRECTORY})
    set_tests_properties(vnm_font_namespace PROPERTIES
        ENVIRONMENT "${vnm_fonts_test_environment}")

    # Linked exactly as a consumer links it: through the alias, with no source
    # of the library in its own target and no knowledge of the resource layout
    # beyond the documented prefix.
    add_executable(vnm_fonts_consumer_tests tests/vnm_fonts_consumer_tests.cpp)
    target_link_libraries(vnm_fonts_consumer_tests PRIVATE vnm::fonts)
    add_test(NAME vnm_fonts_consumer COMMAND vnm_fonts_consumer_tests)
    set_tests_properties(vnm_fonts_consumer PROPERTIES
        ENVIRONMENT "${vnm_fonts_test_environment}")

    # Proves the guard above: a project that adds this repository twice still
    # configures. It runs configure only, which is what it is testing, so the
    # compiler is forwarded and its check is reduced to a compile - a link
    # would need a toolchain environment the caller of ctest may not have.
    add_test(
        NAME vnm_fonts_repeated_inclusion
        COMMAND ${CMAKE_COMMAND}
                -S ${CMAKE_CURRENT_SOURCE_DIR}/tests/repeated_inclusion
                -B ${CMAKE_CURRENT_BINARY_DIR}/repeated_inclusion
                -G ${CMAKE_GENERATOR}
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
                -DVNM_FONTS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR})

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(
            NAME vnm_fonts_manifest
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_font_manifest.py)
    endif()
endif()
