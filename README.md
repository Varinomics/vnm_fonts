# vnm_fonts

The fonts Varinomics products ship, byte-verbatim from upstream releases and
owned derivative builds, plus the library that isolates their family names
where necessary.

```
fonts/         the shipped files, byte-identical to their source outputs
LICENSES/      the licence text of every upstream family
THIRD_PARTY/   the provenance manifest for every file
src/           the library that registers family names at load time (needs Qt)
vnm_fonts.qrc  the resource that carries the twelve files into a process
tests/         the tests
```

## Why the family names are marked at load time

Two font files that declare the same family name do not shadow one another.
They **merge** into a single family entry, after which glyph lookup and
rasterisation can be served from different files, and the result is
systematically wrong glyphs.

For example, two `RobotoCondensed-Regular.ttf` files declaring `Roboto Condensed`
assigned different outlines to glyph 38: `B` in the embedded font and `D` in the
installed font. When the embedded `cmap` lookup used the installed outline,
the application displayed the wrong character. Isolating the family names
prevents that collision.

The same failure occurs **between two files from one upstream release**. Font
Awesome 7 files its Solid face under the typographic family
`Font Awesome 7 Free`, which is the name the Regular face also declares, so two
different icon sets land in one family entry and the host is free to resolve
either. The library carries a family override for that case.

## Two contracts, and only one needs Qt

| Contract | What it gives you | Needs Qt |
|---|---|---|
| File | `VNM_FONTS_DIRECTORY`, the manifest, the licences | no |
| Library | `vnm::fonts`, which registers shipped fonts under isolated families | Qt 6.12 or newer |

Qt 6.12 Core and Gui are optional for configuration. Without them, the file
contract remains available and configuration succeeds, but `vnm::fonts` is not
defined. `VNM_FONTS_LIBRARY_AVAILABLE` indicates whether the library is available
for linking.

## How a Qt consumer uses it

The library carries all twelve fonts in a Qt resource under `:/vnm_fonts/`, so a
consumer links it and has them. Ask for a font by id and use the family that
comes back:

```cpp
const vnm_fonts::Registered_font sans =
    vnm_fonts::register_shipped_font(vnm_fonts::Shipped_font::ROBOTO_CONDENSED_REGULAR);
if (!sans.is_valid()) {
    // sans.error says why. There is no unmarked fallback.
}
label->setFont(QFont(sans.family));
```

That call resolves the resource path and the family override from the one table
inside the library, so a consumer handles neither. **Do not write
`"Roboto Condensed (vnm)"` into your own code, and do not keep your own override
table.** A hand-written family name drifts from the binary; a forgotten override
silently merges two families, which is the defect this repository removes.

Registering the same font twice in a process returns the first registration
rather than registering the bytes again, so it does not matter which of several
consumer libraries gets there first.

`vnm_fonts` is a static library, so the linker is free to discard the generated
resource initialiser. `register_shipped_font` references it for you. A consumer
that reads `:/vnm_fonts/...` itself must call `vnm_fonts::initialize_resources()`
first.

The family that comes back is the one the font's own name table declares. Do not
read `QFontDatabase::applicationFontFamilies()` and choose among the results
yourself, because a real font engine reports several families for one face, and picking
the first, the longest or the only one is wrong in a different way each time.
See "One face, several reported families" below.

### From an install tree

A consumer with no access to this build can find the installed package:

```cmake
find_package(vnm_fonts REQUIRED)
target_link_libraries(my_app PRIVATE vnm::fonts)
```

The target is spelled `vnm::fonts` either way, and the package sets
`VNM_FONTS_DIRECTORY` too, pointing at the installed fonts, so the file contract
means the same thing from a subproject or from an install tree.

A standalone build installs the library and its package. A source consumer that
exports a library depending on `vnm::fonts` calls `vnm_fonts_install_package()`
after dependency discovery. The call is idempotent and enables installation even
if a file-only consumer included the source first. Consumers of an already
installed `vnm::fonts` target do not need this source-package operation.

### Consumers that need a file, not bytes

`vnm_msdf_text` and `vnm_plot` bake a glyph atlas straight from the file and
never enter Qt's font database. Merging is a font-database behaviour, so nothing
can merge there, no marking applies and none is wanted. They read the verbatim
files through the `VNM_FONTS_DIRECTORY` cache variable, which is a supported
path and not an accident:

```cmake
add_subdirectory(vnm_fonts)   # or FetchContent; no Qt needed
target_compile_definitions(my_atlas PRIVATE VNM_FONTS_DIR="${VNM_FONTS_DIRECTORY}")
```

That works in a configure with no Qt present at all, which is the point: a
file-only consumer takes the repository as an ordinary subproject and does not
have to populate the checkout without configuring it.

The library target is `EXCLUDE_FROM_ALL`, so a file-only consumer builds neither
the library nor the generated resource, even when Qt is available. Linking
`vnm::fonts` builds them on demand.

The Ubuntu Sans Mono derivative vnm face is the deliberate exception. Its
published family name is already derivative-specific, so the shipped registry
preserves that exact name instead of appending the process mark.

### Redistributing the fonts means redistributing the licences

All twelve faces are OFL-1.1 or the Ubuntu Font Licence, and every one
of those requires the licence and copyright notice to accompany copies. This
repository installs them, so a consumer that installs gets them:

```
share/doc/vnm_fonts/*.txt                  the upstream licence texts
share/doc/vnm_fonts/THIRD_PARTY_NOTICES.md what points a reader at them
```

The licences and notices are installed whether Qt is available or not.

Two things a consumer may have to do:

- **Packaging from named CPack components.** `CPACK_COMPONENTS_ALL` silently
  drops every component not listed, so set `VNM_FONTS_INSTALL_COMPONENT` to your
  own runtime component. The default is `vnm_fonts_licenses`.
- **Packaging that copies files itself** rather than running `cmake --install`.
  `VNM_FONTS_LICENSE_FILES`, `VNM_FONTS_NOTICES_FILE` and
  `VNM_FONTS_LICENSES_DIR` name the sources; `VNM_FONTS_INSTALL_DOCDIR` names
  the destination this repository uses.

**Do not add this repository with `add_subdirectory(... EXCLUDE_FROM_ALL)`.**
That form discards a subdirectory's install rules, so consumer packaging must
install the licences separately. Configuration warns when this form is used.
The `EXCLUDE_FROM_ALL` on the library *target* affects no install rule.

### Being included more than once

Multiple dependency paths can include this repository in one configure, with or
without Qt. Repeated inclusion is supported, and a consumer-provided
`VNM_FONTS_DIRECTORY` keeps its value.

## What the marking does

The files in `fonts/` are byte-verbatim, so nothing this repository
redistributes differs from the upstream release or owned derivative build it
records. Family names that need isolation are patched in memory on the way into
the font database.

| Name ID | Treatment |
|---|---|
| 1, 4, 16 | `" (vnm)"` appended, in every platform, encoding and language record. |
| 6 | `-VNM` appended. A PostScript name must stay printable ASCII with no spaces or parentheses. |
| 2, 17 | Untouched. A font database groups weights by them. |
| 0, 5, 7, 9-14 | Untouched. Copyright, version, designer, vendor, description, licence and licence URL. |

Nothing else in the font changes. The patcher rebuilds the `name` table, repacks
the table directory, and recomputes the table checksums and
`head.checkSumAdjustment`; every other table comes through byte-identical.
Both sfnt flavours are handled,
`0x00010000` and `OTTO`, because half the set is `.otf`.

Failure is loud. A font whose name table cannot be parsed, or whose registration
resolves to a family without the mark, is an error the caller sees. There is no
quiet fallback to the unmarked font, because registering that is the defect.

### One face, several reported families

`QFontDatabase::applicationFontFamilies()` does not report one family per font.
Measured on Qt 6.11 under the Windows platform plugin:

| Registered font | Reported families |
|---|---|
| `RobotoCondensed-Regular.ttf` | `Roboto (vnm)`, `Roboto Condensed (vnm)` |
| `RobotoCondensed-Light.ttf` | `Roboto (vnm)`, `Roboto Condensed Light (vnm)`, `Roboto Condensed (vnm)` |
| `FontAwesome7Free-Solid.otf` | `Font Awesome 7 Free Solid (vnm)`, twice |

Three separate causes, all of them normal:

- A face is reported under its typographic family (name ID 16) as well as its
  family name (name ID 1) when the two differ. That is why Light reports both
  `Roboto Condensed (vnm)` and `Roboto Condensed Light (vnm)`.
- **DirectWrite derives a width-stripped family.** `Roboto (vnm)` appears in no
  record of the file: it is `Roboto Condensed (vnm)` with the width token
  removed. Nothing in the name table can prevent this.
- A family is reported once per name record, so a face carrying name ID 16 on
  both the Macintosh and Windows platforms is reported twice.

So the count carries no information and neither does position. The library
returns the family the name table declares, name ID 16 where the face has one,
otherwise name ID 1, after asserting it is among those reported, and that every
reported family carries the mark. An unmarked one is still a hard failure,
because it could collide with a font the user has installed.

### The two Roboto weights are one family

The Light face carries the shared typographic family in name ID 16 and its own
weight in name ID 1, which is how upstream groups the pair. Marking both records
preserves the grouping: the pair registers as one family, `Roboto Condensed
(vnm)`, with the styles `Regular` and `Light`. Twelve fonts therefore make eleven
families.

## What is shipped

| File | Upstream family | Registers as |
|---|---|---|
| `RobotoCondensed-Regular.ttf` | Roboto Condensed | `Roboto Condensed (vnm)`, style `Regular` |
| `RobotoCondensed-Light.ttf` | Roboto Condensed Light | `Roboto Condensed (vnm)`, style `Light` |
| `FontAwesome.otf` | FontAwesome | `FontAwesome (vnm)` |
| `FontAwesome7Brands-Regular.otf` | Font Awesome 7 Brands | `Font Awesome 7 Brands (vnm)` |
| `FontAwesome7Free-Regular.otf` | Font Awesome 7 Free | `Font Awesome 7 Free (vnm)` |
| `FontAwesome7Free-Solid.otf` | Font Awesome 7 Free Solid | `Font Awesome 7 Free Solid (vnm)` |
| `NotoSansSymbols2-Regular.ttf` | Noto Sans Symbols 2 | `Noto Sans Symbols 2 (vnm)` |
| `JuliaMono-Regular.ttf` | JuliaMono | `JuliaMono (vnm)` |
| `ABeeZee-Regular.ttf` | ABeeZee | `ABeeZee (vnm)` |
| `UbuntuSansMonoDerivativeVnm-Regular.ttf` | Ubuntu Sans Mono derivative vnm | `Ubuntu Sans Mono derivative vnm` |
| `JetBrainsMono-Regular.ttf` | JetBrains Mono | `JetBrains Mono (vnm)` |
| `FiraCode-Regular.ttf` | Fira Code | `Fira Code (vnm)` |

Every file is named after its source PostScript name, and
`THIRD_PARTY/*.toml` records for each one the source provenance and path, any
published artifact URL, its digest and size, and `modifications = "none"`.
The owned Ubuntu Sans Mono derivative also records its local builder identity;
a local build does not claim a published release URL.

## Running the tests

```
python tests/test_font_manifest.py     # standalone, needs no build
ctest --test-dir <build>
```

The CTest suite is available when configured with `VNM_FONTS_BUILD_TESTS=ON`.
The manifest checker needs Python 3.11 or later for `tomllib`, or `tomli` on an
older Python.
