# CMake integration

## Building and installing the Qt library

The library requires CMake 3.21 or newer, a C++17 compiler, and Qt 6.12 or newer
with the Core and Gui modules. For a standalone build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVNM_FONTS_BUILD_TESTS=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix /path/to/install
```

Set `CMAKE_PREFIX_PATH` to your Qt installation if CMake cannot find it.
Installation includes the static library, public header, CMake package, font
files, licenses, and notices. An application can then use
`find_package(vnm_fonts REQUIRED)` and link `vnm::fonts`; set `CMAKE_PREFIX_PATH`
to the install prefix if needed.

## Including the source in another project

`add_subdirectory(vnm_fonts)` makes `VNM_FONTS_DIRECTORY` available, defaulting
to the source checkout's `fonts/` directory. A value supplied by the consumer is
preserved. Repeated inclusion through several dependency paths is supported.

Qt is optional for source inclusion. When the required Qt modules are found,
`VNM_FONTS_LIBRARY_AVAILABLE` is true and the `vnm::fonts` target is defined.
Adding the source alone does not build the library or its resources; linking
the target builds them. File consumers can use the directory without Qt:

```cmake
add_subdirectory(vnm_fonts)
target_compile_definitions(glyph_atlas PRIVATE
    VNM_FONTS_DIR="${VNM_FONTS_DIRECTORY}")
```

A source consumer that installs a library depending on `vnm::fonts` calls
`vnm_fonts_install_package()` after dependency discovery to enable installation
of the fonts package. Repeated calls are harmless. Standalone builds enable
package installation automatically; consumers of an installed package do not
call this function.

The installed package requires Qt and sets `VNM_FONTS_DIRECTORY` to its
installed font directory.

## Including licenses in application packages

The source subdirectory installs the font licenses and `THIRD_PARTY_NOTICES.md`
under `share/doc/vnm_fonts`, including when Qt is absent. The destination is
available as `VNM_FONTS_INSTALL_DOCDIR`.

For packages using a selected list of CPack components, set
`VNM_FONTS_INSTALL_COMPONENT` to a component included in the application package.
Its default is `vnm_fonts_licenses`.

If packaging copies files directly, use `VNM_FONTS_LICENSE_FILES` and
`VNM_FONTS_NOTICES_FILE` to locate the required texts. `VNM_FONTS_LICENSES_DIR`
provides the source license directory.

Using `add_subdirectory(... EXCLUDE_FROM_ALL)` also excludes the subdirectory's
install rules. Omit that argument to keep the license installation, or include
the license files and notices separately in your package.
