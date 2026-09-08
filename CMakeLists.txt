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

project(vnm_fonts VERSION 1.0.0 LANGUAGES CXX)

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

# Redistributing these fonts means redistributing their licences: all twelve are
# OFL-1.1, Apache-2.0 or the Ubuntu Font Licence, and each requires the licence
# and copyright notice to accompany every copy. The obligation travelled here
# with the fonts, so the install rules do too, and they sit outside the Qt
# branch because a file-only consumer redistributes the bytes just as much as a
# Qt one does.
include(GNUInstallDirs)

# Spelled from DATAROOTDIR rather than taken from CMAKE_INSTALL_DOCDIR, which
# GNUInstallDirs derives from whichever project happened to include it first -
# the consumer's name, when this repository is a subproject.
set(VNM_FONTS_INSTALL_DOCDIR ${CMAKE_INSTALL_DATAROOTDIR}/doc/vnm_fonts
    CACHE INTERNAL "Install destination of the font licences and notices")

# A consumer whose package is built from named CPack components must put these
# files in one of them: CPACK_COMPONENTS_ALL drops everything else, silently.
# vnm_terminal, for one, packages only vnm_terminal_runtime.
set(VNM_FONTS_INSTALL_COMPONENT "vnm_fonts_licenses"
    CACHE STRING "CPack component the font licences and notices install into")

set(VNM_FONTS_LICENSES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/LICENSES
    CACHE INTERNAL "Source directory of the upstream licence texts")
set(VNM_FONTS_NOTICES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    CACHE INTERNAL "Source path of the third-party notices")
file(GLOB vnm_fonts_license_files ${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/*.txt)
set(VNM_FONTS_LICENSE_FILES ${vnm_fonts_license_files}
    CACHE INTERNAL "The upstream licence texts, for a packaging step that copies them itself")

install(FILES ${VNM_FONTS_LICENSE_FILES} ${VNM_FONTS_NOTICES_FILE}
    DESTINATION ${VNM_FONTS_INSTALL_DOCDIR}
    COMPONENT ${VNM_FONTS_INSTALL_COMPONENT})

if(Qt6_FOUND)
    # File-only consumers incur no resource compilation; linking or requesting
    # package installation brings the library into their build.
    add_library(vnm_fonts STATIC EXCLUDE_FROM_ALL
        src/vnm_font_namespace.cpp
        src/vnm_font_namespace.h
        vnm_fonts.qrc)

    # EXPORT_NAME, so the imported target is vnm::fonts and not vnm::vnm_fonts:
    # an export names a target from NAMESPACE plus this, not from the alias the
    # build tree defines. A consumer must spell the target the same way whether
    # it took this repository as a subproject or found the installed package.
    set_target_properties(vnm_fonts PROPERTIES
        AUTORCC     ON
        EXPORT_NAME fonts
        VERSION     ${PROJECT_VERSION})

    target_include_directories(vnm_fonts PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/vnm_fonts>)
    target_link_libraries(vnm_fonts PUBLIC Qt6::Core Qt6::Gui)
    target_compile_features(vnm_fonts PUBLIC cxx_std_17)

    add_library(vnm::fonts ALIAS vnm_fonts)

    # Installation is an explicit idempotent source-package operation. A later
    # consumer can request it even when a file-only dependency included us first.
    function(vnm_fonts_install_package)
        get_property(package_installed GLOBAL PROPERTY VNM_FONTS_PACKAGE_INSTALL)
        if(package_installed)
            return()
        endif()
        set_property(GLOBAL PROPERTY VNM_FONTS_PACKAGE_INSTALL TRUE)
        set_target_properties(vnm_fonts PROPERTIES EXCLUDE_FROM_ALL FALSE)
        get_target_property(fonts_source vnm_fonts SOURCE_DIR)
        get_target_property(fonts_binary vnm_fonts BINARY_DIR)
        get_target_property(fonts_version vnm_fonts VERSION)
        include(CMakePackageConfigHelpers)

        # The font files travel with the package so VNM_FONTS_DIRECTORY means
        # the same thing whether the consumer took this repository as a
        # subproject or found the installed package.
        set(VNM_FONTS_INSTALL_FONTSDIR ${CMAKE_INSTALL_DATAROOTDIR}/vnm_fonts/fonts
            CACHE INTERNAL "Install destination of the shipped font files")

        install(TARGETS vnm_fonts
            EXPORT vnm_fonts-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    COMPONENT ${VNM_FONTS_INSTALL_COMPONENT}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vnm_fonts)
        install(FILES ${fonts_source}/src/vnm_font_namespace.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vnm_fonts
            COMPONENT ${VNM_FONTS_INSTALL_COMPONENT})
        install(DIRECTORY ${fonts_source}/fonts/
            DESTINATION ${VNM_FONTS_INSTALL_FONTSDIR}
            COMPONENT ${VNM_FONTS_INSTALL_COMPONENT})
        install(EXPORT vnm_fonts-targets
            FILE vnm_fonts-targets.cmake
            NAMESPACE vnm::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vnm_fonts
            COMPONENT ${VNM_FONTS_INSTALL_COMPONENT})

        configure_package_config_file(
            ${fonts_source}/cmake/vnm_fonts-config.cmake.in
            ${fonts_binary}/vnm_fonts-config.cmake
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vnm_fonts
            PATH_VARS VNM_FONTS_INSTALL_FONTSDIR)
        write_basic_package_version_file(
            ${fonts_binary}/vnm_fonts-config-version.cmake
            VERSION ${fonts_version}
            COMPATIBILITY SameMajorVersion)
        install(FILES
            ${fonts_binary}/vnm_fonts-config.cmake
            ${fonts_binary}/vnm_fonts-config-version.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vnm_fonts
            COMPONENT ${VNM_FONTS_INSTALL_COMPONENT})
    endfunction()

    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        vnm_fonts_install_package()
    endif()
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


# add_subdirectory(... EXCLUDE_FROM_ALL) discards a subdirectory's install rules
# outright - measured on CMake 3.30 - and FetchContent passes that form when its
# declaration asks for it. The consumer then ships the fonts and none of their
# licences, with nothing to show that anything was dropped. Say so rather than
# let it pass, and name the variables that let them copy the texts themselves.
get_directory_property(vnm_fonts_directory_excluded EXCLUDE_FROM_ALL)
if(vnm_fonts_directory_excluded)
    message(WARNING
        "vnm_fonts was added with EXCLUDE_FROM_ALL, which discards its install "
        "rules, so the font licences and THIRD_PARTY_NOTICES.md will not reach "
        "your install tree. Redistributing these fonts requires them. Drop "
        "EXCLUDE_FROM_ALL, or install VNM_FONTS_LICENSE_FILES and "
        "VNM_FONTS_NOTICES_FILE yourself.")
endif()

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

        # Redistributing the fonts without their licences is the failure this
        # catches, and it is what the consumer repositories were left doing when
        # the fonts moved here and their own install rules went away. Registered
        # beside the manifest test rather than with the Qt ones, because a
        # file-only consumer redistributes the bytes too.
        add_test(
            NAME vnm_fonts_installed_licenses
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_installed_licenses.py
                    --cmake ${CMAKE_COMMAND}
                    --build-dir ${CMAKE_BINARY_DIR}
                    --prefix ${CMAKE_CURRENT_BINARY_DIR}/install_staging
                    --docdir ${VNM_FONTS_INSTALL_DOCDIR})
    endif()
endif()

if(VNM_FONTS_BUILD_TESTS AND Qt6_FOUND)
    # Forwarded into the driver scripts below, escaped so a multi-entry prefix
    # path survives as one argument rather than being split by CMake's list
    # handling on the way through the command line.
    string(REPLACE ";" "\;" vnm_fonts_forwarded_prefix_path "${CMAKE_PREFIX_PATH}")

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
                "-DCMAKE_PREFIX_PATH=${vnm_fonts_forwarded_prefix_path}"
                -DVNM_FONTS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR})

    # The offscreen platform reports one family per registered font. A real
    # font engine does not: Windows reports a face under its typographic family
    # as well as its family name, DirectWrite derives a width-stripped family
    # that is in no record of the file, and a family is repeated once per name
    # record. Every one of those was invisible to the gates above until a
    # consumer hit it on a real desktop, so the same tests run again on the
    # platform plugin the product actually uses. Labelled so a headless CI can
    # exclude them the way logonomic does.
    if(WIN32)
        add_test(
            NAME vnm_font_namespace_real_platform
            COMMAND vnm_font_namespace_tests ${VNM_FONTS_DIRECTORY})
        add_test(
            NAME vnm_fonts_consumer_real_platform
            COMMAND vnm_fonts_consumer_tests)
        set_tests_properties(
            vnm_font_namespace_real_platform vnm_fonts_consumer_real_platform
            PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=windows"
                LABELS "real-platform")
    endif()

    # Proves a consumer can link the library from an install tree rather than
    # only from inside this build. vnm_fonts_consumer above links it in-tree and
    # cannot see a missing or broken package; vnm_framework's packaging smoke
    # test could, and did.
    #
    # This gate links, so unlike the configure-only ones it needs the toolchain
    # environment: a build tree carries neither MSVC's LIB and INCLUDE nor the
    # PATH that finds its linker, and ctest runs in whatever shell the caller
    # had. They are captured here, where the configure that found the compiler
    # ran, and handed to the driver. Bracket syntax, because all three contain
    # semicolons and backslashes.
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/vnm_fonts_build_environment.cmake
"set(VNM_FONTS_ENV_PATH [==[$ENV{PATH}]==])
set(VNM_FONTS_ENV_LIB [==[$ENV{LIB}]==])
set(VNM_FONTS_ENV_INCLUDE [==[$ENV{INCLUDE}]==])
")
    add_test(
        NAME vnm_fonts_installed_package
        COMMAND ${CMAKE_COMMAND}
                -DVNM_FONTS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -DVNM_FONTS_BUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}/installed_package
                -DVNM_FONTS_GENERATOR=${CMAKE_GENERATOR}
                -DVNM_FONTS_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DVNM_FONTS_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                "-DVNM_FONTS_PREFIX_PATH=${vnm_fonts_forwarded_prefix_path}"
                -DVNM_FONTS_ENVIRONMENT_FILE=${CMAKE_CURRENT_BINARY_DIR}/vnm_fonts_build_environment.cmake
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/build_installed_package_consumer.cmake)

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
                "-DVNM_FONTS_PREFIX_PATH=${vnm_fonts_forwarded_prefix_path}"
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
