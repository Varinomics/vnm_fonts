cmake_minimum_required(VERSION 3.21)

# logonomic reaches this repository through several dependency paths in one
# configure - the framework, the terminal, the terminal surface, the plot, the
# MSDF text renderer and the keyboard all want it - so being added twice is
# ordinary, not a mistake. Return quietly on the second pass. The guard is a
# global property rather than a target check, because the library target does
# not exist in a Qt-free configure and there would be nothing to test.
get_property(vnm_fonts_already_included GLOBAL PROPERTY VNM_FONTS_INCLUDED)
if(vnm_fonts_already_included)
    return()
endif()
set_property(GLOBAL PROPERTY VNM_FONTS_INCLUDED TRUE)

project(vnm_fonts LANGUAGES CXX)

# This repository serves two contracts, and only one of them needs Qt.
#
#   The file contract:  VNM_FONTS_DIRECTORY, the manifest and the licences.
#                       vnm_msdf_text and vnm_plot bake a glyph atlas straight
#                       from the file bytes and never enter Qt's font database,
#                       and their CI configures on four platforms with no Qt
#                       installed at all.
#   The library contract: vnm::fonts, which registers a font under a marked
#                       family name and therefore needs QFontDatabase.
#
# Qt is looked for quietly so the file contract stays usable without it. Making
# this REQUIRED breaks every Qt-free consumer's configure, which is why
# tests/without_qt exists: restoring REQUIRED fails a gate here rather than a
# CI job on a machine none of us has.
find_package(Qt6 QUIET COMPONENTS Core Gui)

# The supported path for a consumer that needs the file rather than the bytes.
# Set before the Qt branch, because it is exactly what a Qt-free consumer came
# for. Never FORCE it: a consumer that has already set it means it.
set(VNM_FONTS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    CACHE PATH "Directory holding the byte-verbatim shipped font files")

if(Qt6_FOUND)
    # EXCLUDE_FROM_ALL, because a file-only consumer that adds this repository
    # for VNM_FONTS_DIRECTORY would otherwise compile the whole resource for
    # nothing: all ten fonts land in the generated qrc translation unit, which
    # is some 29 MB of source on every such build. A target excluded from the
    # default build is still built when another target links it, so vnm::fonts
    # keeps working for Qt consumers with no change on their side.
    add_library(vnm_fonts STATIC EXCLUDE_FROM_ALL
        src/vnm_font_namespace.cpp
        src/vnm_font_namespace.h
        vnm_fonts.qrc)

    set_target_properties(vnm_fonts PROPERTIES AUTORCC ON)

    target_include_directories(vnm_fonts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(vnm_fonts PUBLIC Qt6::Core Qt6::Gui)
    target_compile_features(vnm_fonts PUBLIC cxx_std_17)

    add_library(vnm::fonts ALIAS vnm_fonts)
else()
    # Configuring succeeds; only the library contract is withheld. A consumer
    # that links vnm::fonts now fails on that line naming the missing target,
    # with this line directly above it in the configure log saying why - rather
    # than failing inside a repository it only wanted a file path from.
    message(STATUS
        "vnm_fonts: Qt6 Core and Gui were not found, so the vnm::fonts library "
        "is not defined. VNM_FONTS_DIRECTORY (${VNM_FONTS_DIRECTORY}) and the "
        "manifest are available as usual; link vnm::fonts only where Qt is "
        "present.")
endif()

# Lets a consumer branch on which contract it has, instead of testing Qt6_FOUND
# itself and guessing what this repository did with it.
set(VNM_FONTS_LIBRARY_AVAILABLE ${Qt6_FOUND})

# A consumed copy builds no tests unless the consumer asks for them.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(vnm_fonts_tests_by_default ON)
else()
    set(vnm_fonts_tests_by_default OFF)
endif()
option(VNM_FONTS_BUILD_TESTS "Build the vnm_fonts tests" ${vnm_fonts_tests_by_default})

if(VNM_FONTS_BUILD_TESTS)
    enable_testing()

    # The file contract is testable without Qt, and must stay that way: it is
    # the whole of what a Qt-free consumer relies on.
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(
            NAME vnm_fonts_manifest
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_font_manifest.py)
    endif()
endif()

if(VNM_FONTS_BUILD_TESTS AND Qt6_FOUND)
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

    # Proves the inclusion guard: a project that adds this repository twice
    # still configures. It runs configure only, which is what it is testing, so
    # the compiler is forwarded and its check is reduced to a compile - a link
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

    # Proves the file contract costs a consumer nothing: a project that adds
    # this repository and links nothing from it must build neither the library
    # nor its resource. This one builds, unlike the configure-only fixtures, so
    # the fixture compiles a single header-free translation unit into a static
    # library and needs no toolchain environment beyond the compiler and the
    # archiver.
    add_test(
        NAME vnm_fonts_file_only_consumer
        COMMAND ${CMAKE_COMMAND}
                -DVNM_FONTS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -DVNM_FONTS_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}/file_only_consumer
                -DVNM_FONTS_GENERATOR=${CMAKE_GENERATOR}
                -DVNM_FONTS_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DVNM_FONTS_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DVNM_FONTS_PREFIX_PATH=${CMAKE_PREFIX_PATH}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/build_file_only_consumer.cmake)

    # Proves the file contract survives with no Qt. Registered only in a
    # Qt-capable configure, so the Qt-free configure it drives registers just
    # the manifest test and cannot recurse into this one.
    add_test(
        NAME vnm_fonts_without_qt
        COMMAND ${CMAKE_COMMAND}
                -DVNM_FONTS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -DVNM_FONTS_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}/without_qt
                -DVNM_FONTS_GENERATOR=${CMAKE_GENERATOR}
                -DVNM_FONTS_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DVNM_FONTS_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DVNM_FONTS_CTEST=${CMAKE_CTEST_COMMAND}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/configure_without_qt.cmake)
endif()
