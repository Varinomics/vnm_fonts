#!/usr/bin/env python3

"""Prepare the shipped Roboto Condensed outlines for distance-field baking."""

from __future__ import annotations

import argparse
from pathlib import Path

from fontTools.ttLib import TTFont
from fontTools.ttLib.removeOverlaps import removeOverlaps


FONT_DIRECTORY = Path(__file__).resolve().parents[1] / "fonts"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-directory", type=Path, default=FONT_DIRECTORY / "msdf")
    arguments = parser.parse_args()
    arguments.output_directory.mkdir(parents=True, exist_ok=True)

    for weight in ("Regular", "Light"):
        name = "RobotoCondensed-{}.ttf".format(weight)
        with TTFont(FONT_DIRECTORY / name, recalcTimestamp=False) as font:
            # An MSDF needs the boundary of the filled shape: internal overlaps
            # otherwise become false edges. Nonzero-winding path simplification
            # preserves the fill while invalidating point-indexed instructions.
            removeOverlaps(font, removeHinting=True)
            for table in ("fpgm", "prep", "cvt ", "hdmx", "LTSH", "VDMX"):
                if table in font:
                    del font[table]
            font.save(arguments.output_directory / name)


if __name__ == "__main__":
    main()
