# Configures and builds a project that adds this repository for
# VNM_FONTS_DIRECTORY and links nothing from it, then asserts the font library
# was not produced.
#
# The file contract exists so that a consumer which never enters Qt's font
# database pays nothing for the ones that do. Without EXCLUDE_FROM_ALL on the
# library target, such a consumer compiles the generated resource - all ten
# fonts, some 29 MB of source - on every build, and nothing links the result.
#
# Driven by CTest through cmake -P, with the toolchain forwarded so it does not
# depend on the environment the caller of ctest happens to have. The fixture
# builds one header-free translation unit into a static library, so it needs the
# compiler and the archiver and nothing else.

foreach(argument VNM_FONTS_SOURCE_DIR VNM_FONTS_BUILD_ROOT VNM_FONTS_GENERATOR)
    if(NOT DEFINED ${argument})
        message(FATAL_ERROR "${argument} must be set.")
    endif()
endforeach()

file(REMOVE_RECURSE ${VNM_FONTS_BUILD_ROOT})

execute_process(
    COMMAND ${CMAKE_COMMAND}
            -S ${VNM_FONTS_SOURCE_DIR}/tests/file_only_consumer
            -B ${VNM_FONTS_BUILD_ROOT}
            -G ${VNM_FONTS_GENERATOR}
            -DCMAKE_MAKE_PROGRAM=${VNM_FONTS_MAKE_PROGRAM}
            -DCMAKE_CXX_COMPILER=${VNM_FONTS_CXX_COMPILER}
            -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
            -DCMAKE_PREFIX_PATH=${VNM_FONTS_PREFIX_PATH}
            -DVNM_FONTS_SOURCE_DIR=${VNM_FONTS_SOURCE_DIR}
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE  configure_output)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Configuring the file-only consumer failed.\n${configure_output}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${VNM_FONTS_BUILD_ROOT}
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE  build_output)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Building the file-only consumer failed.\n${build_output}")
endif()

# The artifact itself, under either naming convention.
file(GLOB_RECURSE produced_libraries
     ${VNM_FONTS_BUILD_ROOT}/vnm_fonts.lib
     ${VNM_FONTS_BUILD_ROOT}/libvnm_fonts.a)
if(produced_libraries)
    message(FATAL_ERROR
        "A consumer that links nothing from vnm_fonts still built it: "
        "${produced_libraries}. EXCLUDE_FROM_ALL is what keeps that from "
        "happening.")
endif()

# And the expensive part of it, in case the library name ever changes.
file(GLOB_RECURSE compiled_resource
     ${VNM_FONTS_BUILD_ROOT}/qrc_vnm_fonts.cpp.obj
     ${VNM_FONTS_BUILD_ROOT}/qrc_vnm_fonts.cpp.o)
if(compiled_resource)
    message(FATAL_ERROR
        "A consumer that links nothing from vnm_fonts still compiled the font "
        "resource: ${compiled_resource}.")
endif()

message(STATUS "A file-only consumer builds neither the library nor its resource.")
