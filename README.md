# vnm_fonts

A collection of font files used by Varinomics applications, with an optional
Qt library for loading them under distinct family names.

## Fonts

| Face | File |
|---|---|
| Roboto Condensed Regular | `RobotoCondensed-Regular.ttf` |
| Roboto Condensed Light | `RobotoCondensed-Light.ttf` |
| Font Awesome 4 | `FontAwesome.otf` |
| Font Awesome 7 Brands | `FontAwesome7Brands-Regular.otf` |
| Font Awesome 7 Free Regular | `FontAwesome7Free-Regular.otf` |
| Font Awesome 7 Free Solid | `FontAwesome7Free-Solid.otf` |
| Noto Sans Symbols 2 | `NotoSansSymbols2-Regular.ttf` |
| JuliaMono | `JuliaMono-Regular.ttf` |
| ABeeZee | `ABeeZee-Regular.ttf` |
| Ubuntu Sans Mono derivative vnm | `UbuntuSansMonoDerivativeVnm-Regular.ttf` |
| JetBrains Mono | `JetBrainsMono-Regular.ttf` |
| Fira Code | `FiraCode-Regular.ttf` |

The files in [fonts/](fonts/) are unchanged copies of upstream releases or,
for Ubuntu Sans Mono derivative vnm, the derivative project's build output.
[Third-party notices](THIRD_PARTY_NOTICES.md) describe their sources and
licenses; the [manifests](THIRD_PARTY/) record individual file provenance.

## Using the font files

Download or clone this repository and use the TTF or OTF files in `fonts/`:

```sh
git clone https://github.com/Varinomics/vnm_fonts.git
```

Using the files directly requires no Qt or build. CMake projects can add the
source with `add_subdirectory(vnm_fonts)` and locate the fonts through
`VNM_FONTS_DIRECTORY`.

## Qt library

The static `vnm::fonts` library embeds the fonts and registers them with Qt.
It isolates their family names from installed fonts to avoid collisions,
without changing the files on disk.

The library requires CMake 3.21 or newer, C++17, and Qt 6.12 or newer with Core
and Gui. Include it as a source subdirectory:

```cmake
add_subdirectory(vnm_fonts)
target_link_libraries(my_app PRIVATE vnm::fonts)
```

Or use an installed package:

```cmake
find_package(vnm_fonts REQUIRED)
target_link_libraries(my_app PRIVATE vnm::fonts)
```

After creating the application's `QGuiApplication`, register fonts on its GUI
thread and use the returned family name:

```cpp
#include <vnm_font_namespace.h>
#include <QDebug>
#include <QFont>

const auto font = vnm_fonts::register_shipped_font(
    vnm_fonts::Shipped_font::ROBOTO_CONDENSED_REGULAR);
if (!font.is_valid()) {
    qWarning().noquote() << font.error;
}
else {
    label->setFont(QFont(font.family));
}
```

Registration errors are reported in `font.error`; failed isolation does not
fall back to an unmarked font. Successful registrations are reused when the
same face is requested again.

Registered family names have a ` (vnm)` suffix, except for the already distinct
`Ubuntu Sans Mono derivative vnm`. Roboto Condensed Regular and Light share one
family with separate styles; Font Awesome 7 Free Regular and Solid register as
separate families.

`register_shipped_font()` initializes the embedded resources. Applications
reading `:/vnm_fonts/` directly call `vnm_fonts::initialize_resources()` first.
The [public header](src/vnm_font_namespace.h) lists the font IDs and API.
[Building, installing, and packaging](cmake/integration.md) covers the CMake
options for consumers.

## Licenses

The fonts use SIL Open Font License 1.1 or Ubuntu Font Licence 1.0. When
redistributing them, include their license texts and copyright notices from
[LICENSES/](LICENSES/) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
CMake installation includes these texts; see the
[packaging instructions](cmake/integration.md#including-licenses-in-application-packages)
if your application selects CPack components or copies files directly.
