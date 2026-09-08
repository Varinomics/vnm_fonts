# Third-Party Notices

Every font in `fonts/` is a third-party typeface redistributed by Varinomics
**byte-verbatim**, exactly as its authors published it. Nothing in this
repository is modified: every record in `THIRD_PARTY/*.toml` says
`modifications = "none"` and carries the upstream repository, revision, path,
URL, digest and size, and `tests/test_font_manifest.py` checks each file against
its record.

The family name a font registers under is patched in memory at load time by
`src/vnm_font_namespace.cpp`, which appends a mark so the family cannot collide
with a font the user has installed. Two files declaring one family name merge
into a single family entry, after which glyph lookup and rasterisation can be
served from different files and produce systematically wrong glyphs; `README.md`
records the measured case. That patch exists only in the running process, and it
touches only the `name` table.

| Upstream family | File | Upstream project |
|---|---|---|
| Roboto Condensed | `RobotoCondensed-Regular.ttf` | Google Fonts |
| Roboto Condensed Light | `RobotoCondensed-Light.ttf` | Google Fonts |
| FontAwesome | `FontAwesome.otf` | Font Awesome 4.7.0 |
| Font Awesome 7 Brands | `FontAwesome7Brands-Regular.otf` | Font Awesome 7.2.0 |
| Font Awesome 7 Free | `FontAwesome7Free-Regular.otf` | Font Awesome 7.2.0 |
| Font Awesome 7 Free Solid | `FontAwesome7Free-Solid.otf` | Font Awesome 7.2.0 |
| Noto Sans Symbols 2 | `NotoSansSymbols2-Regular.ttf` | Noto |
| JuliaMono | `JuliaMono-Regular.ttf` | JuliaMono |
| ABeeZee | `ABeeZee-Regular.ttf` | ABeeZee |
| Ubuntu Mono - Bront | `UbuntuMono-Bront.ttf` | Bront |

## Roboto Condensed

Shipped files:

- `fonts/RobotoCondensed-Regular.ttf`
- `fonts/RobotoCondensed-Light.ttf`

Both are Roboto Condensed version 2.137 (2017).

License: Apache License 2.0.
Local license text: `LICENSES/RobotoCondensed-Apache-2.0.txt`.

Copyright notice carried in the fonts:

- Copyright 2011 Google Inc. All Rights Reserved.

Source:

- https://github.com/google/fonts
- Revision 55aaf47ca89416e720a2449368943ff87fb6d02a
- https://raw.githubusercontent.com/google/fonts/55aaf47ca89416e720a2449368943ff87fb6d02a/apache/robotocondensed/RobotoCondensed-Regular.ttf
- https://raw.githubusercontent.com/google/fonts/55aaf47ca89416e720a2449368943ff87fb6d02a/apache/robotocondensed/RobotoCondensed-Light.ttf
- https://www.apache.org/licenses/LICENSE-2.0

Details in `THIRD_PARTY/roboto_condensed_fonts.toml`.

## Font Awesome 4.7.0

Shipped file:

- `fonts/FontAwesome.otf`

License: SIL Open Font License 1.1, declared in the project's `README.md` at the
pinned revision. The 4.7.0 tree ships no licence file of its own, so the
unmodified SIL OFL 1.1 text is carried in its place.
Local license text: `LICENSES/OFL-1.1.txt`.

Copyright notice carried in the font:

- Copyright Dave Gandy 2016. All rights reserved.

Source:

- https://github.com/FortAwesome/Font-Awesome
- Revision a8386aae19e200ddb0f6845b5feeee5eb7013687
- https://raw.githubusercontent.com/FortAwesome/Font-Awesome/a8386aae19e200ddb0f6845b5feeee5eb7013687/fonts/FontAwesome.otf
- http://scripts.sil.org/OFL

Details in `THIRD_PARTY/font_awesome_4_font.toml`.

## Font Awesome 7.2.0

Shipped files:

- `fonts/FontAwesome7Brands-Regular.otf`
- `fonts/FontAwesome7Free-Regular.otf`
- `fonts/FontAwesome7Free-Solid.otf`

License: SIL Open Font License 1.1.
Local license text: `LICENSES/FontAwesome-7.2.0-OFL-1.1.txt`, the release's own
licence file.

Copyright notice carried in the fonts:

- Copyright (c) Font Awesome. The release's licence file states the notice as
  `Copyright (c) 2026 Fonticons, Inc. (https://fontawesome.com)`.

Upstream files the Solid face under the typographic family
`Font Awesome 7 Free`, the same family as the Regular face, which puts two
different icon sets in one family entry. The manifest carries a family override
that the load-time patch applies so the two register as separate families. The
shipped files themselves are unmodified.

Source:

- https://github.com/FortAwesome/Font-Awesome
- Revision 337dd2045d5621ce0f8567c33c256f3dedeed55d
- https://github.com/FortAwesome/Font-Awesome/releases/download/7.2.0/fontawesome-free-7.2.0-desktop.zip
- https://fontawesome.com/license/free

Details in `THIRD_PARTY/font_awesome_7_fonts.toml`.

## Noto Sans Symbols 2

Shipped file:

- `fonts/NotoSansSymbols2-Regular.ttf` — version 2.008.

License: SIL Open Font License 1.1.
Local license text: `LICENSES/NotoSansSymbols2-OFL-1.1.txt`, the family's own
OFL.

Copyright notice carried in the font:

- Copyright 2022 The Noto Project Authors (https://github.com/notofonts/symbols)

Source:

- https://github.com/notofonts/symbols
- https://fonts.gstatic.com/s/notosanssymbols2/v25/I_uyMoGduATTei9eI8daxVHDyfisHr71ypPqfX71-AI.ttf
- https://openfontlicense.org

The bytes are a Google Fonts CDN build rather than a checked-in artifact.
`THIRD_PARTY/noto_sans_symbols_2_font.toml` records the CDN URL, how it was
found, the google/fonts revision the build was compiled from, and the candidates
that were ruled out.

## JuliaMono

Shipped file:

- `fonts/JuliaMono-Regular.ttf` — v0.63.2.

License: SIL Open Font License 1.1.
Local license text: `LICENSES/JuliaMono-OFL-1.1.txt`.

Copyright notice carried in the font:

- Copyright 2020 - 2026 The JuliaMono Project Authors
  (https://github.com/cormullion/juliamono)

Source:

- https://github.com/cormullion/juliamono
- Revision 0dfc749503778730d6af79183a17fcde668ae626
- https://raw.githubusercontent.com/cormullion/juliamono/0dfc749503778730d6af79183a17fcde668ae626/JuliaMono-Regular.ttf
- http://scripts.sil.org/OFL

Details in `THIRD_PARTY/juliamono_font.toml`.

## ABeeZee

Shipped file:

- `fonts/ABeeZee-Regular.ttf` — version 1.003.

License: SIL Open Font License 1.1.
Local license text: `LICENSES/ABeeZee-OFL-1.1.txt`.

Copyright notice carried in the font:

- Copyright 2011 The ABeeZee Project Authors
  (https://github.com/googlefonts/abeezee) with Reserved Font Name ABeeZee

Source:

- https://github.com/google/fonts
- Revision a8bc01c6dcb2933dca0f37180df6c516e226b346
- https://raw.githubusercontent.com/google/fonts/a8bc01c6dcb2933dca0f37180df6c516e226b346/ofl/abeezee/ABeeZee-Regular.ttf
- https://scripts.sil.org/OFL

Details in `THIRD_PARTY/abeezee_font.toml`.

## Ubuntu Mono - Bront

Shipped file:

- `fonts/UbuntuMono-Bront.ttf` — version 0.1.

The typeface is Chris Wendt's derivative of Canonical's Ubuntu Mono, distributed
by him as `Ubuntu Mono - Bront`. The file here is his, unmodified.

License: Ubuntu Font Licence 1.0.
Local license text: `LICENSES/Ubuntu-Font-Licence-1.0.txt`.

Copyright notice carried in the font:

- Copyright 2011 Canonical Ltd. Licensed under the Ubuntu Font Licence 1.0.

Upstream contributor:

- Chris Wendt (`chrismwendt`), the author of the pinned upstream commit. The
  pinned upstream files contain no separate contributor copyright statement.

Source:

- https://github.com/chrismwendt/bront
- Revision aef23d9a11416655a8351230edb3c2377061c077
- https://raw.githubusercontent.com/chrismwendt/bront/aef23d9a11416655a8351230edb3c2377061c077/UbuntuMono-Bront.ttf
- https://ubuntu.com/legal/font-licence

Details in `THIRD_PARTY/ubuntu_mono_bront_font.toml`.

## JetBrains Mono

Shipped file: `fonts/JetBrainsMono-Regular.ttf`, byte-verbatim upstream release.

License: OFL-1.1. Local license: `LICENSES/JetBrainsMono-OFL-1.1.txt`.

Copyright 2020 The JetBrains Mono Project Authors (https://github.com/JetBrains/JetBrainsMono)

Source: https://github.com/JetBrains/JetBrainsMono

Revision: cd5227bd1f61dff3bbd6c814ceaf7ffd95e947d9

https://raw.githubusercontent.com/JetBrains/JetBrainsMono/cd5227bd1f61dff3bbd6c814ceaf7ffd95e947d9/fonts/ttf/JetBrainsMono-Regular.ttf

Details in `THIRD_PARTY/jetbrains_mono_fonts.toml`.

## Fira Code

Shipped file: `fonts/FiraCode-Regular.ttf`, byte-verbatim upstream release.

License: OFL-1.1. Local license: `LICENSES/FiraCode-OFL-1.1.txt`.

Copyright (c) 2014, The Fira Code Project Authors (https://github.com/tonsky/FiraCode)

Source: https://github.com/tonsky/FiraCode

Revision: eee6db993696aba61ff4eef03698e2987d79910c

https://github.com/tonsky/FiraCode/releases/download/6.2/Fira_Code_v6.2.zip

Details in `THIRD_PARTY/fira_code_fonts.toml`.
