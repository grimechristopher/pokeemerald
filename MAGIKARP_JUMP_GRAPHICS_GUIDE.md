# Magikarp Jump Forms - Graphics Creation Guide

This guide lists all the Magikarp Jump pattern forms that have been implemented and what graphics you need to create for each.

## Status

✅ Code Implementation Complete:
- 62 new species constants added to `include/constants/species.h`
- Form species tables created in `src/data/pokemon/form_species_tables.h`
- 62 species info entries added to `src/data/pokemon/species_info/gen_1_families.h`

📝 Graphics Needed:
- All forms currently use standard Magikarp/Gyarados graphics as placeholders
- You need to create pattern-specific graphics for visual variety

## Pattern List

Based on Pokémon: Magikarp Jump, here are the 31 new patterns (plus Standard and Gold which already exist):

### Existing Forms (Already Have Graphics)
1. **Standard** - Base Magikarp (already exists)
2. **Gold** - Shiny Magikarp (already exists)

### New Patterns to Create Graphics For

#### Pattern #2: Skelly
- **Species Constants:** `SPECIES_MAGIKARP_SKELLY`, `SPECIES_GYARADOS_SKELLY`
- **Description:** Skeleton pattern
- **Graphics Needed:** Yes (all listed below)

#### Pattern #3: Calico (Orange & White)
- **Species Constants:** `SPECIES_MAGIKARP_CALICO_ORANGE_WHITE`, `SPECIES_GYARADOS_CALICO_ORANGE_WHITE`
- **Description:** Calico pattern with orange and white coloring
- **Graphics Needed:** Yes

#### Pattern #4: Calico (Orange, White, Black)
- **Species Constants:** `SPECIES_MAGIKARP_CALICO_ORANGE_WHITE_BLACK`, `SPECIES_GYARADOS_CALICO_ORANGE_WHITE_BLACK`
- **Description:** Calico pattern with orange, white, and black coloring
- **Graphics Needed:** Yes

#### Pattern #5: Calico (White & Orange)
- **Species Constants:** `SPECIES_MAGIKARP_CALICO_WHITE_ORANGE`, `SPECIES_GYARADOS_CALICO_WHITE_ORANGE`
- **Description:** Calico pattern with white and orange (inverted from #3)
- **Graphics Needed:** Yes

#### Pattern #6: Calico (Orange & Gold)
- **Species Constants:** `SPECIES_MAGIKARP_CALICO_ORANGE_GOLD`, `SPECIES_GYARADOS_CALICO_ORANGE_GOLD`
- **Description:** Calico pattern with orange and gold coloring
- **Graphics Needed:** Yes

#### Pattern #7: Orange Two-Tone
- **Species Constants:** `SPECIES_MAGIKARP_ORANGE_TWO_TONE`, `SPECIES_GYARADOS_ORANGE_TWO_TONE`
- **Description:** Two-tone orange pattern
- **Graphics Needed:** Yes

#### Pattern #8: Orange Orca
- **Species Constants:** `SPECIES_MAGIKARP_ORANGE_ORCA`, `SPECIES_GYARADOS_ORANGE_ORCA`
- **Description:** Orca-style orange pattern
- **Graphics Needed:** Yes

#### Pattern #9: Orange Dapples
- **Species Constants:** `SPECIES_MAGIKARP_ORANGE_DAPPLES`, `SPECIES_GYARADOS_ORANGE_DAPPLES`
- **Description:** Dappled orange pattern
- **Graphics Needed:** Yes

#### Pattern #10: Pink Two-Tone
- **Species Constants:** `SPECIES_MAGIKARP_PINK_TWO_TONE`, `SPECIES_GYARADOS_PINK_TWO_TONE`
- **Description:** Two-tone pink pattern
- **Graphics Needed:** Yes

#### Pattern #11: Pink Orca
- **Species Constants:** `SPECIES_MAGIKARP_PINK_ORCA`, `SPECIES_GYARADOS_PINK_ORCA`
- **Description:** Orca-style pink pattern
- **Graphics Needed:** Yes

#### Pattern #12: Pink Dapples
- **Species Constants:** `SPECIES_MAGIKARP_PINK_DAPPLES`, `SPECIES_GYARADOS_PINK_DAPPLES`
- **Description:** Dappled pink pattern
- **Graphics Needed:** Yes

#### Pattern #13: Gray Bubbles
- **Species Constants:** `SPECIES_MAGIKARP_GRAY_BUBBLES`, `SPECIES_GYARADOS_GRAY_BUBBLES`
- **Description:** Gray with bubble pattern
- **Graphics Needed:** Yes

#### Pattern #14: Gray Diamonds
- **Species Constants:** `SPECIES_MAGIKARP_GRAY_DIAMONDS`, `SPECIES_GYARADOS_GRAY_DIAMONDS`
- **Description:** Gray with diamond pattern
- **Graphics Needed:** Yes

#### Pattern #15: Gray Patches
- **Species Constants:** `SPECIES_MAGIKARP_GRAY_PATCHES`, `SPECIES_GYARADOS_GRAY_PATCHES`
- **Description:** Gray with patches
- **Graphics Needed:** Yes

#### Pattern #16: Purple Bubbles
- **Species Constants:** `SPECIES_MAGIKARP_PURPLE_BUBBLES`, `SPECIES_GYARADOS_PURPLE_BUBBLES`
- **Description:** Purple with bubble pattern
- **Graphics Needed:** Yes

#### Pattern #17: Purple Diamonds
- **Species Constants:** `SPECIES_MAGIKARP_PURPLE_DIAMONDS`, `SPECIES_GYARADOS_PURPLE_DIAMONDS`
- **Description:** Purple with diamond pattern
- **Graphics Needed:** Yes

#### Pattern #18: Purple Patches
- **Species Constants:** `SPECIES_MAGIKARP_PURPLE_PATCHES`, `SPECIES_GYARADOS_PURPLE_PATCHES`
- **Description:** Purple with patches
- **Graphics Needed:** Yes

#### Pattern #19: Apricot Tiger
- **Species Constants:** `SPECIES_MAGIKARP_APRICOT_TIGER`, `SPECIES_GYARADOS_APRICOT_TIGER`
- **Description:** Apricot color with tiger stripes
- **Graphics Needed:** Yes

#### Pattern #20: Apricot Zebra
- **Species Constants:** `SPECIES_MAGIKARP_APRICOT_ZEBRA`, `SPECIES_GYARADOS_APRICOT_ZEBRA`
- **Description:** Apricot color with zebra stripes
- **Graphics Needed:** Yes

#### Pattern #21: Apricot Stripes
- **Species Constants:** `SPECIES_MAGIKARP_APRICOT_STRIPES`, `SPECIES_GYARADOS_APRICOT_STRIPES`
- **Description:** Apricot color with stripes
- **Graphics Needed:** Yes

#### Pattern #22: Brown Tiger
- **Species Constants:** `SPECIES_MAGIKARP_BROWN_TIGER`, `SPECIES_GYARADOS_BROWN_TIGER`
- **Description:** Brown with tiger stripes
- **Graphics Needed:** Yes

#### Pattern #23: Brown Zebra
- **Species Constants:** `SPECIES_MAGIKARP_BROWN_ZEBRA`, `SPECIES_GYARADOS_BROWN_ZEBRA`
- **Description:** Brown with zebra stripes
- **Graphics Needed:** Yes

#### Pattern #24: Brown Stripes
- **Species Constants:** `SPECIES_MAGIKARP_BROWN_STRIPES`, `SPECIES_GYARADOS_BROWN_STRIPES`
- **Description:** Brown with stripes
- **Graphics Needed:** Yes

#### Pattern #25: Orange Forehead
- **Species Constants:** `SPECIES_MAGIKARP_ORANGE_FOREHEAD`, `SPECIES_GYARADOS_ORANGE_FOREHEAD`
- **Description:** White with orange forehead marking
- **Graphics Needed:** Yes

#### Pattern #26: Orange Mask
- **Species Constants:** `SPECIES_MAGIKARP_ORANGE_MASK`, `SPECIES_GYARADOS_ORANGE_MASK`
- **Description:** White with orange mask marking
- **Graphics Needed:** Yes

#### Pattern #27: Black Forehead
- **Species Constants:** `SPECIES_MAGIKARP_BLACK_FOREHEAD`, `SPECIES_GYARADOS_BLACK_FOREHEAD`
- **Description:** White with black forehead marking
- **Graphics Needed:** Yes

#### Pattern #28: Black Mask
- **Species Constants:** `SPECIES_MAGIKARP_BLACK_MASK`, `SPECIES_GYARADOS_BLACK_MASK`
- **Description:** White with black mask marking
- **Graphics Needed:** Yes

#### Pattern #29: Saucy Blue
- **Species Constants:** `SPECIES_MAGIKARP_SAUCY_BLUE`, `SPECIES_GYARADOS_SAUCY_BLUE`
- **Description:** Saucy blue coloring
- **Graphics Needed:** Yes

#### Pattern #30: Blue Raindrop
- **Species Constants:** `SPECIES_MAGIKARP_BLUE_RAINDROP`, `SPECIES_GYARADOS_BLUE_RAINDROP`
- **Description:** Blue with raindrop pattern
- **Graphics Needed:** Yes

#### Pattern #31: Violet Blue
- **Species Constants:** `SPECIES_MAGIKARP_VIOLET_BLUE`, `SPECIES_GYARADOS_VIOLET_BLUE`
- **Description:** Violet-blue coloring
- **Graphics Needed:** Yes

#### Pattern #32: Violet Raindrop
- **Species Constants:** `SPECIES_MAGIKARP_VIOLET_RAINDROP`, `SPECIES_GYARADOS_VIOLET_RAINDROP`
- **Description:** Violet with raindrop pattern
- **Graphics Needed:** Yes

## Graphics Requirements Per Pattern

For EACH of the 31 patterns above, you need to create graphics for BOTH Magikarp and Gyarados variants (62 total form sets).

### Currently (Placeholder State)
- All forms use standard Magikarp/Gyarados graphics
- Code compiles and runs, but all patterns look identical
- You can test forms in debug mode before creating custom graphics

### What You Need to Create

The graphics system in pokeemerald-expansion will auto-generate pointers based on directory names. Since the code currently uses standard Magikarp/Gyarados pointers, you have two options:

#### Option 1: Keep Using Standard Graphics (Current State)
- **Pros:** Code already works, can test evolution/forms immediately
- **Cons:** All patterns look identical visually
- **When to use:** If you want to test the form system first, or implement wild encounter mechanics before graphics

#### Option 2: Create Pattern-Specific Graphics
To create unique graphics for each pattern, you would need to:

**For each Magikarp pattern:**
- Create graphics directory (e.g., for Skelly pattern, the build system doesn't require a specific directory structure, but you could organize them)
- Modify the species info entry to point to pattern-specific graphics pointers
- Create sprites:
  - Front battle sprite (48x56 px)
  - Back battle sprite (64x56 px)
  - Animated front sprite
  - Party menu icon (32x32 or 64x64 px)
  - Overworld sprite
  - Footprint (can reuse standard)
- Create palettes:
  - Normal palette (16 colors, .pal format)
  - Shiny palette (only for Gold, others use standard shiny)
  - Overworld palette

**For each Gyarados pattern:**
- Same as above but with Gyarados dimensions:
  - Front battle sprite (64x64 px)
  - Back battle sprite (64x64 px)
  - etc.

### Recommended Approach

1. **Phase 1: Test with placeholders (CURRENT)**
   - All forms use standard graphics
   - Test evolution (Calico Magikarp → Calico Gyarados)
   - Test debug mode access to all forms
   - Verify build compiles

2. **Phase 2: Create graphics incrementally**
   - Start with 1-2 patterns (e.g., Skelly, Calico Orange & White)
   - Create full graphics set for those patterns
   - Update species info entries to use pattern-specific pointers
   - Test in-game
   - Repeat for remaining patterns as desired

3. **Phase 3: Implement wild encounters**
   - Add rod attachment system
   - Make patterns available in wild
   - Test encounter mechanics

## Reference Images

For pattern references, see:
- **Serebii:** https://www.serebii.net/magikarpjump/magikarp.shtml
- **Bulbapedia:** https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon:_Magikarp_Jump
- **Magikarp Jump Wiki:** https://magikarpjump.fandom.com/wiki/Magikarp_Patterns
- **Gyarados Patterns (Fan Art):** https://www.deviantart.com/zorathetwilightdrake/art/Magikarp-Jump-Gyarados-Patterns-879727614

## Testing the Forms

Once the build compiles, you can test forms in debug mode by:
1. Opening the debug menu (varies by pokeemerald-expansion version, usually R+START or similar)
2. Navigating to "Give Pokemon" or "Util" → "PC" → "Give Pokemon"
3. Selecting the species constant (e.g., `SPECIES_MAGIKARP_SKELLY`)
4. Testing evolution to confirm it evolves to matching Gyarados pattern

## Current Implementation Status

✅ **Code Complete:**
- 62 species constants defined
- 62 species info entries created
- Form tables linking patterns together
- Evolution chains (each Magikarp pattern → matching Gyarados pattern)
- Mega Evolution (all Gyarados patterns can Mega Evolve to standard Mega Gyarados)

⏳ **Pending:**
- Pattern-specific graphics (currently using placeholders)
- Wild encounter system (rod attachments)
- Testing/verification

📝 **Next Steps:**
1. Run `make -j$(nproc)` to compile and check for errors
2. Test forms in debug mode with placeholder graphics
3. Create pattern graphics incrementally
4. Implement wild encounter mechanics (future task)

## File Locations Summary

- **Species Constants:** `include/constants/species.h` (lines 1669-1734)
- **Form Tables:** `src/data/pokemon/form_species_tables.h` (lines 523-597)
- **Species Info:** `src/data/pokemon/species_info/gen_1_families.h` (after Mega Gyarados entry)
- **Graphics:** `graphics/pokemon/magikarp/` and `graphics/pokemon/gyarados/` (placeholders)

## Notes

- Gold Magikarp is the shiny variant (no separate Gold species constant needed)
- All patterns share the same stats (purely visual differences)
- Mega Gyarados has no pattern variants (all patterns Mega Evolve to standard Mega)
- Gender differences supported via existing `P_GENDER_DIFFERENCES` system
