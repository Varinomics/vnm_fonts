cmake_minimum_required(VERSION 3.21)

project(vnm_fonts LANGUAGES CXX)

# The library patches a font's family name on the way into QFontDatabase, so it
# needs Qt Gui for the font database and Qt Core for everything else. It must
# not grow a third dependency: consumers pull it in purely to register fonts.
find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(vnm_fonts STATIC
    src/vnm_font_namespace.cpp
    src/vnm_font_namespace.h)

target_include_directories(vnm_fonts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(vnm_fonts PUBLIC Qt6::Core Qt6::Gui)
target_compile_features(vnm_fonts PUBLIC cxx_std_17)

add_library(vnm::fonts ALIAS vnm_fonts)

# The shipped fonts are byte-verbatim upstream files. Consumers read them from
# here, or from their own resource system; this variable is the one place the
# directory is named.
set(VNM_FONTS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    CACHE PATH "Directory holding the byte-verbatim shipped font files")

option(VNM_FONTS_BUILD_TESTS "Build the vnm_fonts tests" ON)

if(VNM_FONTS_BUILD_TESTS)
    enable_testing()

    add_executable(vnm_font_namespace_tests tests/vnm_font_namespace_tests.cpp)
    target_link_libraries(vnm_font_namespace_tests PRIVATE vnm_fonts)

    # The test registers fonts, which needs a platform plugin but no display.
    add_test(
        NAME vnm_font_namespace
        COMMAND vnm_font_namespace_tests ${VNM_FONTS_DIRECTORY})
    set_tests_properties(vnm_font_namespace PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(
            NAME vnm_fonts_manifest
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_font_manifest.py)
    endif()
endif()
