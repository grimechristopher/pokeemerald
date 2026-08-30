# Magikarp Jump Forms - Graphics Setup Complete!

## ✅ What Was Done (CORRECTED)

After fixing the directory structure to match pokeemerald-expansion conventions, all 62 Magikarp Jump forms now have proper graphics directories with placeholder files.

### Correct Directory Structure

Forms use **subdirectories WITHIN the Pokemon folder** (not separate top-level directories):

```
graphics/pokemon/magikarp/
├── (standard files)
├── skelly/
│   ├── anim_front.png
│   ├── back.png
│   ├── icon.png
│   ├── normal.pal
│   ├── shiny.pal
│   └── (all other graphics files)
├── calico_orange_white/
├── calico_orange_white_black/
├── ... (29 more patterns)

graphics/pokemon/gyarados/
├── (standard files)
├── skelly/
├── calico_orange_white/
└── ... (31 patterns total)
```

This matches how Pikachu forms work:
- `graphics/pokemon/pikachu/belle/` ✅
- `graphics/pokemon/pikachu/cosplay/` ✅

NOT:
- `graphics/pokemon/pikachu_belle/` ❌ (what I initially created)

### Files Modified

1. **`src/data/graphics/pokemon.h`** (~2000 lines added)
   - Added graphics pointer declarations for all 62 forms
   - Each form has: front, back, icon, palettes, overworld
   - Follows pokeemerald-expansion pattern for form graphics

2. **`src/data/pokemon/species_info/gen_1_families.h`** (updated)
   - All 62 species entries now reference pattern-specific graphics pointers
   - Example: `gMonFrontPic_MagikarpSkelly` instead of `gMonFrontPic_Magikarp`

3. **Graphics directories** (62 subdirectories created)
   - 31 Magikarp pattern subdirectories
   - 31 Gyarados pattern subdirectories
   - All populated with placeholder graphics (copies of standard)

### Current State

✅ **Code compiles successfully**
✅ **ROM builds** (pokeemerald.gba created)
✅ **All 62 forms functional** (can test in debug mode)
⚠️ **Graphics are placeholders** - all patterns look identical (use standard Magikarp/Gyarados graphics)

### What You Can Do Now

1. **Test the forms in debug mode:**
   - Load pokeemerald.gba in mGBA
   - Open debug menu (R+START)
   - Util → PC → Give Pokemon
   - Select `SPECIES_MAGIKARP_SKELLY` (or any pattern)
   - Level to 20 to test evolution
   - Confirm it evolves to `SPECIES_GYARADOS_SKELLY`

2. **Start creating pattern graphics:**
   - Pick a pattern (e.g., Skelly)
   - Edit files in `graphics/pokemon/magikarp/skelly/`
   - Required files to edit:
     - `anim_front.png` - Front battle sprite
     - `back.png` - Back battle sprite
     - `icon.png` - Party menu icon
     - `normal.pal` - Normal palette (16 colors)
     - `overworld.png` - Overworld sprite
   - Rebuild with `make -j20`
   - Test in game!

3. **Batch edit multiple patterns:**
   - Use image editing software with batch processing
   - Apply different palettes/patterns to each subdirectory
   - Each pattern can have completely unique sprites

### Graphics File Reference

Each pattern subdirectory contains:

**Battle Graphics:**
- `anim_front.png` / `anim_front_gba.png` - Animated front sprite
- `back.png` / `back_gba.png` - Back sprite
- `anim_frontf.png` / `backf.png` - Female variants (if different)

**UI Graphics:**
- `icon.png` / `icon_gba.png` - Party menu icon (32x32 or 64x64 px)

**Palettes:**
- `normal.pal` / `normal.gbapal` - Normal coloring
- `shiny.pal` / `shiny.gbapal` - Shiny coloring (Gold for Magikarp, Red for Gyarados)

**Overworld:**
- `overworld.png` - Overworld/following sprite
- `overworld_normal.pal` / `overworld_shiny.pal` - Overworld palettes

### Recommended Workflow

**Option 1: Edit Palettes Only (Easiest)**
If patterns differ only by color:
1. Keep the standard sprites (don't edit .png files)
2. Only edit `.pal` files to change colors
3. Much faster than redrawing sprites

**Option 2: Edit Full Sprites (Most Flexible)**
If patterns have different shapes/markings:
1. Edit both `.png` files AND `.pal` files
2. Allows for pattern variations beyond just recoloring
3. More work but more unique results

**Start Small:**
- Pick 1-2 patterns to test workflow
- Edit their graphics
- Build and test
- Once satisfied, batch-process the rest

### Pattern Reference Images

For visual reference of what each pattern should look like:
- **Serebii:** https://www.serebii.net/magikarpjump/magikarp.shtml
- **Magikarp Jump Wiki:** https://magikarpjump.fandom.com/wiki/Magikarp_Patterns
- **Gyarados Fan Art:** https://www.deviantart.com/zorathetwilightdrake/art/Magikarp-Jump-Gyarados-Patterns-879727614

### Build Commands

```bash
# Clean build
make clean
make -j20

# Quick rebuild (after graphics changes)
make -j20

# If you get errors about missing files, rebuild clean:
make clean && make -j20
```

### Important Notes

1. **Graphics are duplicates now:** All pattern directories contain copies of the standard sprites. This lets the game compile and run while you create unique graphics.

2. **Shiny behavior:** All patterns share the same shiny palette (Gold for Magikarp, Red for Gyarados). This is intentional - see main design docs.

3. **Gender differences:** Female sprite files exist but are copies. You can make them unique if desired.

4. **GBA vs Modern:** The `*_gba.png` files are for GBA-style graphics. If you're using modern sprites, you only need to edit the non-GBA versions.

### Next Steps

1. ✅ Build compiles
2. ✅ Graphics directories set up
3. ✅ Placeholder graphics in place
4. 📝 **YOU ARE HERE** → Start editing pattern graphics
5. ⏳ Test each pattern as you create it
6. ⏳ Implement wild encounter mechanics (future task)

---

**You're all set!** The infrastructure is complete. Now you can focus on creating the actual pattern graphics. Every subdirectory is ready for your edits!
