cmake_minimum_required(VERSION 3.21)
project(vnm_fonts_install_after_file_consumer LANGUAGES CXX)

# The first consumer needs only bytes; the later package owner requests export.
add_subdirectory(files)
vnm_fonts_install_package()
vnm_fonts_install_package()
