# vnm_fonts

The fonts Varinomics products ship, byte-verbatim as their authors published
them, plus the library that registers them under a family name unique to the
running process.

```
fonts/       the shipped files, byte-identical to upstream
LICENSES/    the licence text of every upstream family
THIRD_PARTY/ the provenance manifest for every file
src/         the library that marks a family name at load time
tests/       the gates
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
either. The manifest carries a per-font family override for that case.

**So: do not "simplify" this by registering the files as they are.** The
verbatim bytes are exactly the failure above. The marking is the point.

## How the marking works

The files in `fonts/` are byte-verbatim, so nothing this repository
redistributes differs from what upstream published. The family name is patched
in memory on the way into the font database:

```cpp
const vnm_fonts::Registered_font sans = vnm_fonts::register_marked_font(font_bytes);
if (!sans.is_valid()) {
    // sans.error says why. There is no unmarked fallback.
}
label->setFont(QFont(sans.family));
```

Ask the call for the family name. Do not write `"Roboto Condensed (vnm)"` into
your own code: a hand-written copy is the drift this API exists to prevent.

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

### The two Roboto weights are one family

The Light face carries the shared typographic family in name ID 16 and its own
weight in name ID 1, which is how upstream groups the pair. Marking both records
preserves the grouping: the pair registers as one family, `Roboto Condensed
(vnm)`, with the styles `Regular` and `Light`.

### The non-Qt consumers need none of this

`vnm_msdf_text` and `vnm_plot` bake an atlas directly from the file bytes and
never enter Qt's font database. No two files can merge there, so no patch
applies and none is wanted. They consume the verbatim file as it is. Do not
"fix" that later by routing them through the patcher.

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

Every file is named after its upstream PostScript name, and
`THIRD_PARTY/*.toml` records for each one the upstream repository, revision,
path and URL, its digest and size, and `modifications = "none"`.

## The gates

```
python tests/test_font_manifest.py     # digests, sizes, manifest coverage, notices
ctest --test-dir <build>               # the manifest check and the registration round-trip
```

`tests/test_font_manifest.py` needs Python 3.11 or later for `tomllib`, or
`tomli` on an older one. `tests/vnm_font_namespace_tests.cpp` needs Qt 6 and
runs under the offscreen platform.
