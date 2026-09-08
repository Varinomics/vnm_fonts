# vnm_fonts

The fonts Varinomics products ship, byte-verbatim as their authors published
them, plus the library that registers them under a family name unique to the
running process.

```
fonts/         the shipped files, byte-identical to upstream
LICENSES/      the licence text of every upstream family
THIRD_PARTY/   the provenance manifest for every file
src/           the library that marks a family name at load time (needs Qt)
vnm_fonts.qrc  the resource that carries the twelve files into a process
tests/         the gates
```

## Why the family names are marked at load time

Two font files that declare the same family name do not shadow one another.
They **merge** into a single family entry, after which glyph lookup and
rasterisation can be served from different files, and the result is
systematically wrong glyphs.

This was measured, not theorised. `vnm_framework/rc/RobotoCondensed-Regular.ttf`
has 1294 glyphs and a `cmap` that maps `B` to glyph 38. A
`RobotoCondensed-Regular.ttf` installed system-wide on the same machine has 1042
glyphs, and its glyph 38 is `D`. Both files declared name ID 1
`Roboto Condensed`. The two merged into one family entry: the `cmap` lookup
resolved in the embedded file while the outline came from the installed one, so
every character in the application rendered two positions along the alphabet.
The word `Branches` appeared on screen as `Dtcpej`.

Nothing in the application was wrong. Nothing in either font was wrong. The
collision was the family name.

The same failure occurs **between two files from one upstream release**. Font
Awesome 7 files its Solid face under the typographic family
`Font Awesome 7 Free`, which is the name the Regular face also declares, so two
different icon sets land in one family entry and the host is free to resolve
either. The library carries a family override for that case.

**So: do not "simplify" this by registering the files as they are.** The
verbatim bytes are exactly the failure above. The marking is the point.

## Two contracts, and only one needs Qt

| Contract | What it gives you | Needs Qt |
|---|---|---|
| File | `VNM_FONTS_DIRECTORY`, the manifest, the licences | no |
| Library | `vnm::fonts`, which registers a font under a marked family | yes |

`find_package(Qt6 ...)` here is **QUIET, not REQUIRED**, so adding this
repository to a project on a machine with no Qt configures normally: you get
the file contract, and `vnm::fonts` is simply not defined. A consumer that
links `vnm::fonts` anyway fails on its own `target_link_libraries` line naming
that target, with the explanation directly above it in the configure log —
rather than failing inside a repository it only wanted a file path from.
`VNM_FONTS_LIBRARY_AVAILABLE` says which contract you have.

Do not restore `REQUIRED`. `vnm_msdf_text` and `vnm_plot` configure on Linux,
macOS, FreeBSD and Windows with no Qt installed, and `REQUIRED` breaks every one
of those jobs. The `vnm_fonts_without_qt` gate fails if anyone tries.

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
yourself — a real font engine reports several families for one face, and picking
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

It also costs nothing when Qt *is* present. The library target is
`EXCLUDE_FROM_ALL`, so a consumer that links nothing from it builds neither the
library nor the generated resource — which is around 29 MB of source, all twelve
fonts embedded, and would otherwise be compiled on every build to be linked by
nobody. `vnm::fonts` is still built on demand for anything that links it, so Qt
consumers need no change. Measured: a file-only consumer's build tree contains
its own objects and nothing of this repository's.

Do not "fix" that later by routing those two through the patcher.

### Redistributing the fonts means redistributing the licences

All twelve faces are OFL-1.1, Apache-2.0 or the Ubuntu Font Licence, and every one
of those requires the licence and copyright notice to accompany copies. This
repository installs them, so a consumer that installs gets them:

```
share/doc/vnm_fonts/*.txt                  the seven upstream licence texts
share/doc/vnm_fonts/THIRD_PARTY_NOTICES.md what points a reader at them
```

The rules are outside the Qt branch, because a file-only consumer redistributes
the bytes just as much as a Qt one does.

Two things a consumer may have to do:

- **Packaging from named CPack components.** `CPACK_COMPONENTS_ALL` silently
  drops every component not listed, so set `VNM_FONTS_INSTALL_COMPONENT` to your
  own runtime component — `vnm_terminal`, for one, packages only
  `vnm_terminal_runtime`. The default is `vnm_fonts_licenses`.
- **Packaging that copies files itself** rather than running `cmake --install`.
  `VNM_FONTS_LICENSE_FILES`, `VNM_FONTS_NOTICES_FILE` and
  `VNM_FONTS_LICENSES_DIR` name the sources; `VNM_FONTS_INSTALL_DOCDIR` names
  the destination this repository uses.

**Do not add this repository with `add_subdirectory(... EXCLUDE_FROM_ALL)`.**
That form discards a subdirectory's install rules outright — measured on CMake
3.30 — so the fonts ship and the licences do not, with nothing in the output to
say anything was dropped. The configure warns if you do it anyway. This is
unrelated to the `EXCLUDE_FROM_ALL` on the library *target*, which is a
different property and affects no install rule.

### Being included more than once

logonomic reaches this repository through several dependency paths in one
configure — the framework, the terminal, the terminal surface, the plot, the
MSDF text renderer and the keyboard. Adding it twice is ordinary: the CMake
returns early on a global property — not on the library target, which does not
exist in a Qt-free configure — and it never `FORCE`s `VNM_FONTS_DIRECTORY`, so a
consumer that set that variable itself keeps its value.
`tests/repeated_inclusion` is the gate on both.

## What the marking does

The files in `fonts/` are byte-verbatim, so nothing this repository
redistributes differs from what upstream published. The family name is patched
in memory on the way into the font database.

| Name ID | Treatment |
|---|---|
| 1, 4, 16 | `" (vnm)"` appended, in every platform, encoding and language record. |
| 6 | `-VNM` appended. A PostScript name must stay printable ASCII with no spaces or parentheses. |
| 2, 17 | Untouched. A font database groups weights by them. |
| 0, 5, 7, 9-14 | Untouched. Copyright, version, designer, vendor, description, licence and licence URL. |

Nothing else in the font changes. The patcher rebuilds the `name` table, repacks
the table directory, and recomputes the table checksums and
`head.checkSumAdjustment`; every other table comes through byte-identical, and
`tests/vnm_font_namespace_tests.cpp` asserts it. Both sfnt flavours are handled,
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
  record of the file — it is `Roboto Condensed (vnm)` with the width token
  removed. Verified by searching the marked bytes: the string does not occur
  there. Nothing in the name table can prevent this.
- A family is reported once per name record, so a face carrying name ID 16 on
  both the Macintosh and Windows platforms is reported twice.

So the count carries no information and neither does position. The library
returns the family the name table declares — name ID 16 where the face has one,
otherwise name ID 1 — after asserting it is among those reported, and that every
reported family carries the mark. An unmarked one is still a hard failure,
because it could collide with a font the user has installed.

The offscreen platform reports exactly one family for every font, which is why
the ordinary gates could not see any of this until a consumer hit it on a real
desktop. The `real-platform` gates run the same tests under the platform plugin
the product uses.

### The two Roboto weights are one family

The Light face carries the shared typographic family in name ID 16 and its own
weight in name ID 1, which is how upstream groups the pair. Marking both records
preserves the grouping: the pair registers as one family, `Roboto Condensed
(vnm)`, with the styles `Regular` and `Light`. Twelve fonts therefore make eleven
families, and the consumer gate asserts that count.

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
| `UbuntuMono-Bront.ttf` | Ubuntu Mono - Bront | `Ubuntu Mono - Bront (vnm)` |
| `JetBrainsMono-Regular.ttf` | JetBrains Mono | `JetBrains Mono (vnm)` |
| `FiraCode-Regular.ttf` | Fira Code | `Fira Code (vnm)` |

Every file is named after its upstream PostScript name, and
`THIRD_PARTY/*.toml` records for each one the upstream repository, revision,
path and URL, its digest and size, and `modifications = "none"`.

## The gates

```
python tests/test_font_manifest.py     # standalone, needs no build
ctest --test-dir <build>               # all four
```

| Gate | What it proves |
|---|---|
| `vnm_font_namespace` | Marking changes only the `name` table; each font registers under one marked family that the database resolves to itself and that appears exactly once; the Roboto pair is one family, the Font Awesome 7 Free pair is two. |
| `vnm_fonts_consumer` | The resource is reachable from a target that merely links the library; all twelve register by id into eleven families; a second registration returns the first. |
| `vnm_fonts_repeated_inclusion` | Adding this repository twice configures, and `VNM_FONTS_DIRECTORY` is not overwritten. |
| `vnm_fonts_file_only_consumer` | A project that adds this repository and links nothing from it builds neither the library nor its resource — the library target stays `EXCLUDE_FROM_ALL`. |
| `vnm_fonts_without_qt` | The file contract survives a configure with Qt disabled: it succeeds, `VNM_FONTS_DIRECTORY` points at the shipped set, the manifest test is registered and passes, and no Qt-dependent test is registered. |
| `vnm_fonts_manifest` | Every shipped file matches its recorded digest and size, every file is described by exactly one record, and the notices cover every revision, URL and licence. |
| `vnm_fonts_installed_licenses` | `cmake --install` puts every licence text the manifests name into `share/doc/vnm_fonts/` with its recorded digest, along with the notices. |
| `vnm_font_namespace_real_platform`, `vnm_fonts_consumer_real_platform` | The same two C++ gates under the real platform plugin, where a font engine reports several families per face. Labelled `real-platform`; registered on Windows. |
| `vnm_fonts_installed_package` | A project outside this build finds the installed package and links `vnm::fonts`, resolving a call into the library. |

`vnm_fonts_manifest` and `vnm_fonts_installed_licenses` are the two a Qt-free
configure registers, and together they are the whole of what a file-only
consumer relies on. They need Python 3.11 or later for `tomllib`, or `tomli` on
an older one. The other four need Qt 6; the two C++ ones run under the offscreen
platform.
