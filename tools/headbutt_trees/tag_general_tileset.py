#!/usr/bin/env python3
"""Tags the "general" primary tileset's ambient forest-canopy metatiles with
MB_HEADBUTT_TREE, so every outdoor route that uses the general tileset's
treeline (essentially all of them) gets Headbutt-able trees for free -
no per-map or per-object placement needed.

The 16 metatile IDs below are the general tileset's full autotile set for
its solid, impassable forest/canopy blob (the big tree masses that wall in
routes - not the small walkable "tuft of grass near a tree" corner tiles
like METATILE_General_Grass_TreeLeft/Right, which stay untouched since the
player can stand on those). They were identified by:

  1. Rendering every general-tileset metatile from metatiles.bin/tiles.png
     and visually confirming which ones are solid canopy foliage.
  2. Cross-checking against real map data (data/layouts/*/map.bin) for
     which of those IDs are actually placed with impassable collision
     across a sample of outdoor routes (Route101/102/103/104/110/111/
     116/117/119/120/121) - i.e. proven-in-use forest tiles, not just
     visually similar ones.

Before this script runs, all 16 have behavior 0x00 (MB_NORMAL) with no
other meaning attached, so this is a safe, additive change - see the dump
in this directory's README for the verification transcript.

Usage:
    python3 tools/headbutt_trees/tag_general_tileset.py [--dry-run]

Run from the repo root. Idempotent - re-running after the tag is already
applied is a no-op.
"""
import argparse
import struct
import sys

TILESET_DIR = "data/tilesets/primary/general"
ATTR_PATH = f"{TILESET_DIR}/metatile_attributes.bin"

# Values must match include/constants/metatile_behaviors.h. MB_HEADBUTT_TREE
# was appended at the very end of the enum (right after MB_ROCK_CLIMB), so
# its value is whatever NUM_METATILE_BEHAVIORS - 2 currently is - re-derive
# this if the enum ever changes rather than trusting the literal below.
MB_NORMAL = 0x00
MB_HEADBUTT_TREE = 0xF0

FOREST_CANOPY_METATILE_IDS = [
    0x1D4, 0x1D5, 0x1D6, 0x1D7,
    0x1DC, 0x1DD,
    0x1E4, 0x1E5, 0x1E6, 0x1E7,
    0x1EC, 0x1ED,
    0x1F2, 0x1F3, 0x1F4, 0x1F5,
]

BEHAVIOR_MASK = 0x00FF
LAYER_MASK = 0xF000


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dry-run", action="store_true", help="Report what would change without writing.")
    args = ap.parse_args()

    with open(ATTR_PATH, "rb") as f:
        data = f.read()
    n = len(data) // 2
    vals = list(struct.unpack("<%dH" % n, data))

    changed = 0
    for mt in FOREST_CANOPY_METATILE_IDS:
        raw = vals[mt]
        behavior = raw & BEHAVIOR_MASK
        layer = raw & LAYER_MASK
        if behavior == MB_HEADBUTT_TREE:
            print(f"0x{mt:03X}: already tagged, skipping")
            continue
        if behavior != MB_NORMAL:
            print(f"WARNING: 0x{mt:03X} has non-default behavior 0x{behavior:02X} - "
                  f"leaving it alone, check by hand", file=sys.stderr)
            continue
        vals[mt] = layer | MB_HEADBUTT_TREE
        print(f"0x{mt:03X}: behavior 0x{behavior:02X} -> 0x{MB_HEADBUTT_TREE:02X}")
        changed += 1

    print(f"\n{changed} metatile(s) {'would be' if args.dry_run else ''} tagged MB_HEADBUTT_TREE")

    if not args.dry_run and changed:
        out = struct.pack("<%dH" % n, *vals)
        with open(ATTR_PATH, "wb") as f:
            f.write(out)
        print(f"wrote {ATTR_PATH}")


if __name__ == "__main__":
    main()
