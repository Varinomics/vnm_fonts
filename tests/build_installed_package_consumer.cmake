# Installs this repository, then configures and links a separate project that
# finds the installed package and calls into the library.
#
# vnm_fonts_consumer links vnm::fonts from inside this build, where the target
# exists whether or not anything is installed. That is why it could not see the
# gap the migration hit: vnm_framework's packaging smoke test failed to link
# with an unresolved vnm_fonts::register_shipped_font because there was no
# installed package to link against. This gate is the one that can see it.
#
# Driven by CTest through cmake -P. Unlike the configure-only fixtures this one
# links, so it needs the toolchain environment: a build tree does not carry
# MSVC's LIB and INCLUDE, and they are forwarded from the configure that found
# the compiler.

foreach(argument VNM_FONTS_SOURCE_DIR VNM_FONTS_BUILD_ROOT VNM_FONTS_GENERATOR)
    if(NOT DEFINED ${argument})
        message(FATAL_ERROR "${argument} must be set.")
    endif()
endforeach()

if(VNM_FONTS_ENVIRONMENT_FILE AND EXISTS ${VNM_FONTS_ENVIRONMENT_FILE})
    include(${VNM_FONTS_ENVIRONMENT_FILE})
    if(VNM_FONTS_ENV_PATH)
        set(ENV{PATH} "${VNM_FONTS_ENV_PATH}")
    endif()
    if(VNM_FONTS_ENV_LIB)
        set(ENV{LIB} "${VNM_FONTS_ENV_LIB}")
    endif()
    if(VNM_FONTS_ENV_INCLUDE)
        set(ENV{INCLUDE} "${VNM_FONTS_ENV_INCLUDE}")
    endif()
endif()

# A prefix path with more than one entry is a CMake list, and a list item
# containing a semicolon is split again when it is placed in an execute_process
# COMMAND. Quoting at the call site does not prevent that; escaping the
# separators does, and the child then receives one argument.
string(REPLACE ";" "\;" forwarded_prefix_path "${VNM_FONTS_PREFIX_PATH}")

set(staging_prefix ${VNM_FONTS_BUILD_ROOT}/prefix)
set(library_build  ${VNM_FONTS_BUILD_ROOT}/vnm_fonts)
set(consumer_build ${VNM_FONTS_BUILD_ROOT}/consumer)

function(run_step description)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE step_result
        OUTPUT_VARIABLE step_output
        ERROR_VARIABLE  step_output)
    if(NOT step_result EQUAL 0)
        message(FATAL_ERROR "${description} failed.\n${step_output}")
    endif()
endfunction()

# A standalone configure installs the library by default, which is what a
# packager does. Nothing here passes VNM_FONTS_INSTALL_LIBRARY, so the default
# is part of what is under test.
run_step("Configuring vnm_fonts for installation"
    ${CMAKE_COMMAND}
    -S ${VNM_FONTS_SOURCE_DIR}
    -B ${library_build}
    -G ${VNM_FONTS_GENERATOR}
    -DCMAKE_MAKE_PROGRAM=${VNM_FONTS_MAKE_PROGRAM}
    -DCMAKE_CXX_COMPILER=${VNM_FONTS_CXX_COMPILER}
    "-DCMAKE_PREFIX_PATH=${forwarded_prefix_path}"
    -DCMAKE_INSTALL_PREFIX=${staging_prefix}
    -DCMAKE_BUILD_TYPE=Release
    -DVNM_FONTS_BUILD_TESTS=OFF)

run_step("Building vnm_fonts for installation"
    ${CMAKE_COMMAND} --build ${library_build} --config Release --parallel 1)

run_step("Installing vnm_fonts"
    ${CMAKE_COMMAND} --install ${library_build} --config Release)

# The package has to be findable from the staging prefix alone; Qt stays on the
# path because the config file declares it as a dependency, which is part of
# what is being checked.
string(REPLACE ";" "\;" consumer_prefix_path
       "${staging_prefix};${VNM_FONTS_PREFIX_PATH}")
run_step("Configuring a consumer against the installed package"
    ${CMAKE_COMMAND}
    -S ${VNM_FONTS_SOURCE_DIR}/tests/installed_package_consumer
    -B ${consumer_build}
    -G ${VNM_FONTS_GENERATOR}
    -DCMAKE_MAKE_PROGRAM=${VNM_FONTS_MAKE_PROGRAM}
    -DCMAKE_CXX_COMPILER=${VNM_FONTS_CXX_COMPILER}
    "-DCMAKE_PREFIX_PATH=${consumer_prefix_path}"
    -DCMAKE_BUILD_TYPE=Release)

run_step("Linking a consumer against the installed package"
    ${CMAKE_COMMAND} --build ${consumer_build} --config Release --parallel 1)

message(STATUS "A consumer outside the build tree finds and links the installed package.")
