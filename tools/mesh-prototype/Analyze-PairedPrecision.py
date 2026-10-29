#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline evidence analysis; never closes the paired runtime acceptance gate.

Usage: python Analyze-PairedPrecision.py IMAGE.ppm [FLOAT_CAPTURE.csv]
Uses exact rational geometry, independent of runtime implementation.
"""
import collections
import csv
from fractions import Fraction as F
import json
import math
from pathlib import Path
import sys


def covered(x, y):
    return x >= 16 and y >= 16 and 63*(2*x-31)+64*(2*y-31) < 4032


def ideal(x, y):
    return [F(2*x-15, 64), F(8*y-61, 252)]


def analyze(image, capture=None):
    header, width, maximum, data = Path(image).read_bytes().split(b"\n", 3)
    if (header, width, maximum, len(data)) != (b"P6", b"64 64", b"255", 12288):
        raise ValueError("expected complete 64x64 RGB8 PPM")
    delta = collections.Counter()
    outside_bracket = 0
    differences = []
    for y in range(64):
        for x in range(64):
            active = covered(x, y)
            v = ideal(x, y) if active else [F(0), F(0)]
            scaled = [min(F(1), max(F(0), z))*255 for z in [v[0], v[1], v[0]]]
            expected = [math.floor(z+F(1, 2)) for z in scaled]
            actual = list(data[(y*64+x)*3:(y*64+x)*3+3])
            d = tuple(a-e for a, e in zip(actual, expected))
            if any(d):
                delta[str(d)] += 1
                differences.append(dict(x=x, y=y, actual=actual, nearest=expected))
            outside_bracket += any(a not in {math.floor(z), math.ceil(z)} for a, z in zip(actual, scaled))
    result = dict(nearest_rgb_mismatches=len(differences), deltas=dict(delta),
                  outside_ideal_floor_ceil=outside_bracket, differences=differences,
                  interpretation="Ideal-value bracket is descriptive; interpolation precision and actual pre-conversion values remain separate. Not runtime acceptance.")
    if capture:
        rows = list(csv.DictReader(Path(capture).open(newline="", encoding="utf-8")))
        if len(rows) != 4096:
            raise ValueError("incomplete float capture")
        errors = []
        conversion_differences = 0
        for i, row in enumerate(rows):
            x, y = int(row["x"]), int(row["y"])
            if (x, y) != (i % 64, i // 64):
                raise ValueError("duplicate/unordered float pixel")
            v = [float(row[c]) for c in ("r", "g", "b", "a")]
            if not all(map(math.isfinite, v)):
                raise ValueError("nonfinite capture")
            ref = ideal(x, y) if covered(x, y) else [F(0), F(0)]
            errors.extend(abs(z-float(t)) for z,t in zip(v[:2], ref))
            nearest = [math.floor(min(1,max(0,z))*255+.5) for z in v[:3]]
            conversion_differences += nearest != list(data[i*3:i*3+3])
        result.update(float_max_ideal_error=max(errors),
                      pixels_different_from_float_capture_nearest=conversion_differences,
                      note="Separate draws may have different interpolation; correlation does not prove identical shader values in the RGBA8 draw.")
    return result


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    print(json.dumps(analyze(*sys.argv[1:]), indent=2))
