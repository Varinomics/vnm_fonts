# Configures this repository with Qt deliberately unavailable and asserts that
# the file contract survives: VNM_FONTS_DIRECTORY is set and points at the
# shipped files, the manifest test is registered and passes, and the library
# contract is withheld rather than half-built.
#
# vnm_msdf_text and vnm_plot configure on four platforms with no Qt installed.
# Restoring REQUIRED to the find_package call, or moving VNM_FONTS_DIRECTORY
# inside the Qt branch, breaks all of them; this fails here instead.
#
# Driven by CTest through cmake -P, with the toolchain forwarded so it does not
# depend on the environment the caller of ctest happens to have.

foreach(argument
        VNM_FONTS_SOURCE_DIR VNM_FONTS_BUILD_ROOT VNM_FONTS_GENERATOR VNM_FONTS_CTEST)
    if(NOT DEFINED ${argument})
        message(FATAL_ERROR "${argument} must be set.")
    endif()
endforeach()

# A stale cache would carry a previous run's Qt discovery into this one.
file(REMOVE_RECURSE ${VNM_FONTS_BUILD_ROOT})

# CMAKE_DISABLE_FIND_PACKAGE_Qt6 makes the find_package call report not-found,
# which is what a machine without Qt produces. It only works because the call is
# QUIET rather than REQUIRED: REQUIRED would fail here outright, which is the
# breakage this gate exists to catch.
execute_process(
    COMMAND ${CMAKE_COMMAND}
            -S ${VNM_FONTS_SOURCE_DIR}
            -B ${VNM_FONTS_BUILD_ROOT}
            -G ${VNM_FONTS_GENERATOR}
            -DCMAKE_MAKE_PROGRAM=${VNM_FONTS_MAKE_PROGRAM}
            -DCMAKE_CXX_COMPILER=${VNM_FONTS_CXX_COMPILER}
            -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
            -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE  configure_output)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR
        "Configuring without Qt failed. A consumer that only wants "
        "VNM_FONTS_DIRECTORY cannot use this repository.\n${configure_output}")
endif()

# The file contract: the directory a Qt-free consumer came for.
load_cache(${VNM_FONTS_BUILD_ROOT} READ_WITH_PREFIX cached_ VNM_FONTS_DIRECTORY)
if(NOT cached_VNM_FONTS_DIRECTORY)
    message(FATAL_ERROR "VNM_FONTS_DIRECTORY is not set in a Qt-free configure.")
endif()
if(NOT IS_DIRECTORY ${cached_VNM_FONTS_DIRECTORY})
    message(FATAL_ERROR
        "VNM_FONTS_DIRECTORY is ${cached_VNM_FONTS_DIRECTORY}, which is not a directory.")
endif()
foreach(font RobotoCondensed-Regular.ttf FontAwesome7Free-Solid.otf UbuntuMono-Bront.ttf)
    if(NOT EXISTS ${cached_VNM_FONTS_DIRECTORY}/${font})
        message(FATAL_ERROR
            "VNM_FONTS_DIRECTORY does not contain ${font}, so it is not the shipped set.")
    endif()
endforeach()

# The library contract is withheld, not half-built: a Qt-free configure must
# register no test that needs Qt.
execute_process(
    COMMAND ${VNM_FONTS_CTEST} --test-dir ${VNM_FONTS_BUILD_ROOT} -N
    RESULT_VARIABLE listing_result
    OUTPUT_VARIABLE listing_output
    ERROR_VARIABLE  listing_output)
if(NOT listing_result EQUAL 0)
    message(FATAL_ERROR "Listing the Qt-free tests failed.\n${listing_output}")
endif()
if(NOT listing_output MATCHES "vnm_fonts_manifest")
    message(FATAL_ERROR
        "A Qt-free configure did not register vnm_fonts_manifest, which is the "
        "whole of what a file-only consumer can check.\n${listing_output}")
endif()
foreach(qt_test vnm_font_namespace vnm_fonts_consumer)
    if(listing_output MATCHES "${qt_test}")
        message(FATAL_ERROR
            "A Qt-free configure registered ${qt_test}, which needs Qt.\n${listing_output}")
    endif()
endforeach()

# And the manifest test actually passes there, rather than merely existing.
execute_process(
    COMMAND ${VNM_FONTS_CTEST} --test-dir ${VNM_FONTS_BUILD_ROOT} -R vnm_fonts_manifest
            --output-on-failure
    RESULT_VARIABLE manifest_result
    OUTPUT_VARIABLE manifest_output
    ERROR_VARIABLE  manifest_output)
if(NOT manifest_result EQUAL 0)
    message(FATAL_ERROR "The manifest test failed without Qt.\n${manifest_output}")
endif()

message(STATUS "The file contract configures and checks with no Qt present.")
