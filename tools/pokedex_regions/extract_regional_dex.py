#!/usr/bin/env python3
"""Extracts each region's species list straight from the National Dex enum
in include/constants/pokedex.h, instead of hand-deriving it by reading
(expensive - this is what burned a lot of tokens for the Hoenn-only pass
that came before this script existed).

include/constants/pokedex.h's `enum NationalDexOrder` is laid out as one
contiguous block per region, each introduced by a `// RegionName` comment
(`// Kanto`, `// Johto`, `// Hoenn`, `// Sinnoh`, `// Unova`, `// Kalos`,
`// Alola`, `// Galar`, plus an `// Unknown` block of placeholder slots in
between that gets skipped). A region's species are just "every
NATIONAL_DEX_ constant between this region's comment and the next one" -
no reordering needed (unlike the actual in-game Hoenn dex, which reorders
around cross-gen evolutions via sHoennToNationalOrder in src/pokemon.c -
see LoadHoennDexSpecies in tools/encounter_editor/model.go for that one).

Usage:
    python3 tools/pokedex_regions/extract_regional_dex.py [--json out.json]

Run from the repo root. Prints a per-region species count to stdout and
(optionally) writes a JSON file shaped like:
    {"Kanto": ["SPECIES_BULBASAUR", ...], "Johto": [...], ...}

A couple of National Dex entries (Deoxys, Castform, ...) only exist in
species.h as a `#define SPECIES_FOO SPECIES_FOO_NORMAL` alias rather than
their own numbered constant - those resolve to the _NORMAL form here too,
same as the Hoenn-side fallback in the Go tool.
"""
import argparse
import json
import re
import sys

POKEDEX_H = "include/constants/pokedex.h"
SPECIES_H = "include/constants/species.h"

REGION_COMMENT_RE = re.compile(r"^\s*//\s*([A-Za-z]+)\s*$")
NATIONAL_DEX_RE = re.compile(r"NATIONAL_DEX_([A-Z0-9_]+)")

# Not real regions - skip these comment headers if they ever appear.
SKIP_HEADERS = {"National", "Pokédex", "order", "Unknown"}


def load_species_defines(path):
    """Returns {SPECIES_NAME: dex_number} for every #define SPECIES_X <int>
    line - mirrors LoadSpeciesList in tools/encounter_editor/model.go, but
    keeps the number too so resolve_species_const can pick the lowest-ID
    form as the "base" one below."""
    define_re = re.compile(r"^#define\s+(SPECIES_[A-Z0-9_]+)\s+(\d+)\s*$")
    species = {}
    with open(path) as f:
        for line in f:
            m = define_re.match(line.rstrip("\r\n"))
            if m:
                species[m.group(1)] = int(m.group(2))
    return species


def extract_regions(pokedex_path):
    with open(pokedex_path) as f:
        lines = f.readlines()

    # Only look inside `enum NationalDexOrder { ... }`.
    start = next(i for i, l in enumerate(lines) if "enum NationalDexOrder" in l)
    end = next(i for i in range(start, len(lines)) if lines[i].strip() == "};")
    body = lines[start:end]

    regions = {}
    current = None
    for line in body:
        m = REGION_COMMENT_RE.match(line)
        if m:
            name = m.group(1)
            if name not in SKIP_HEADERS:
                current = name
                regions.setdefault(current, [])
            else:
                current = None
            continue
        m = NATIONAL_DEX_RE.search(line)
        if m and current:
            regions[current].append(m.group(1))
    return regions


def resolve_species_const(dex_name, known_species):
    """NATIONAL_DEX_FOO -> SPECIES_FOO. Multi-form species (Deoxys, Burmy,
    Giratina, Shellos, Basculin, ...) have no bare SPECIES_FOO constant of
    their own in this fork - just per-form ones (SPECIES_FOO_NORMAL,
    SPECIES_FOO_PLANT, SPECIES_FOO_ALTERED, ...). In every case the form
    that keeps the species' original contiguous dex-block number is its
    lowest-numbered SPECIES_FOO_* constant (later forms get appended way
    out past NATIONAL_DEX_COUNT), so fall back to that instead of hardcoding
    a suffix list that would need updating for every new species."""
    direct = "SPECIES_" + dex_name
    if direct in known_species:
        return direct
    prefix = direct + "_"
    forms = [(num, name) for name, num in known_species.items() if name.startswith(prefix)]
    if forms:
        return min(forms)[1]
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--json", help="write the region -> species-list mapping to this file")
    ap.add_argument("--pokedex-h", default=POKEDEX_H)
    ap.add_argument("--species-h", default=SPECIES_H)
    args = ap.parse_args()

    known_species = load_species_defines(args.species_h)
    regions = extract_regions(args.pokedex_h)

    out = {}
    total_unresolved = 0
    for region, dex_names in regions.items():
        resolved = []
        for dex_name in dex_names:
            const = resolve_species_const(dex_name, known_species)
            if const is None:
                print(f"WARNING: {region}: NATIONAL_DEX_{dex_name} has no matching SPECIES_ constant "
                      f"(and no _NORMAL fallback) - skipping", file=sys.stderr)
                total_unresolved += 1
                continue
            resolved.append(const)
        out[region] = resolved
        print(f"{region:10s} {len(resolved):4d} species")

    print(f"\n{sum(len(v) for v in out.values())} species total across {len(out)} regions"
          + (f" ({total_unresolved} unresolved)" if total_unresolved else ""))

    if args.json:
        with open(args.json, "w") as f:
            json.dump(out, f, indent=2)
            f.write("\n")
        print(f"wrote {args.json}")


if __name__ == "__main__":
    main()
