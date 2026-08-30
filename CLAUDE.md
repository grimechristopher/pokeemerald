# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## About This Project

This is **pokeemerald-expansion**, a ROM hack base built on top of pret's pokeemerald decompilation project. It provides a comprehensive toolkit for creating Pokemon ROM hacks for GBA, featuring hundreds of features from modern Pokemon games and quality-of-life enhancements. This is NOT a playable game - it's a development base.

## Build Commands

### Basic Building
```bash
# Build the ROM (creates pokeemerald.gba)
make

# Build with parallel jobs for speed (recommended)
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS

# Clean build artifacts
make clean
make tidy  # Remove build artifacts but keep generated assets
```

### Build Variants
```bash
# Debug build with symbols
make debug

# Release build (optimized, enables LTO)
make release

# Compare to original ROM checksum
make compare
```

### Testing System
```bash
# Run all tests
make check -j

# Run specific tests (e.g., all Spikes-related tests)
make TESTS="Spikes" check

# Build test ROM for viewing in mGBA
make pokeemerald-test.elf TESTS="Spikes"
```

### Build Options (via Make variables)
- `TEST=1` - Build with test runner enabled
- `DEBUG=1` - Add debug info and use -Og optimization
- `RELEASE=1` - Optimized release build
- `LTO=1` - Enable link-time optimization
- `ANALYZE=1` - Enable -fanalyzer for deeper static analysis
- `KEEP_TEMPS=1` - Keep intermediate compilation files
- `NODEP=1` - Skip dependency scanning

## High-Level Architecture

### Code Organization

**Source Code (`src/`)**
- Core game logic in C (compiled with modern GCC, not agbcc)
- Battle system split across many files (`battle_*.c`)
- Battle AI in `battle_ai_*.c` files (main, util, switch_items, field_statuses)
- Pokemon data and mechanics in `pokemon.c`, `daycare.c`
- Overworld and field mechanics spread across many specialized files

**Data Files (`data/`)**
- Battle scripts in assembly (`.s` files)
- Map data in `data/maps/` and `data/layouts/`
- Text data in `data/text/`
- Species/move/item data typically defined in `src/data/` as C source

**Headers (`include/`)**
- Config headers in `include/config/` control features via preprocessor defines
- Constants in `include/constants/` define game enums and values
- Main headers define structs and function prototypes

**Tests (`test/`)**
- Battle mechanic tests in `test/battle/`
- Uses custom testing DSL with GIVEN/WHEN/SCENE/THEN blocks
- Tests compiled separately and run via mgba-rom-test

### Battle System Architecture

The battle system is the most complex subsystem:

**Battle Flow**
- `battle_main.c` - Core battle loop and state machine
- `battle_controller_*.c` - Separate controllers for player, opponent, partner, etc.
- `battle_script_commands.c` - Interprets battle scripts from data/battle_scripts_*.s
- `battle_util.c`, `battle_util2.c` - Shared battle helper functions

**Battle Mechanics**
- Move effects in `battle_script_commands.c` and via battle scripts
- Ability effects scattered across battle files, triggered at specific points
- Items effects in `battle_script_commands.c` and item-specific files
- Status conditions handled in `battle_util.c` and related files

**AI System**
- `battle_ai_main.c` - Core AI decision making with scoring system
- `battle_ai_util.c` - Helper functions for AI calculations
- `battle_ai_switch_items.c` - AI logic for switching and item usage
- AI controlled by flags (see `include/config/ai.h`)
- Tests use `AI_SINGLE_BATTLE_TEST` and `AI_FLAGS` to test AI behavior

**Battle Animations**
- `battle_anim_*.c` files organized by type (fire, water, etc.)
- Scripts in `data/battle_anim_scripts.s`
- Effects in `battle_anim_effects_*.c`

### Configuration System

Features are controlled by defines in `include/config/*.h`:

- **`config/battle.h`** - Battle mechanics, damage calc, generational features
- **`config/overworld.h`** - Overworld features, running, followers
- **`config/pokemon.h`** - Pokemon data, stat calc, evolution
- **`config/item.h`** - Item behavior and features
- **`config/test.h`** - Testing framework configuration
- **`config/general.h`** - General game settings

Configs use pattern: `#define B_FEATURE_NAME GEN_X` where GEN_X indicates which generation's behavior to use.

**Config Philosophy:**
- Features that modify saves MUST be gated by config and OFF by default
- Features that improve developer QoL or emulate modern Pokemon should be ON by default
- All other configs should be OFF

### Data Generation Pipeline

The build system auto-generates several files:

1. **Map Data** (`data/maps/*.inc`, `data/layouts/*.inc`)
   - Generated from JSON via `tools/mapjson/mapjson`
   - Processed via `tools/jsonproc/jsonproc`

2. **Wild Encounters** (`src/data/wild_encounters.h`)
   - Generated from `src/data/wild_encounters.json`
   - Python script: `tools/wild_encounters/wild_encounters_to_header.py`

3. **Learnsets** (`src/data/pokemon/teachable_learnsets.h`)
   - Generated via `tools/learnset_helpers/make_teachables.py`

4. **Script Commands** (`include/constants/script_commands.h`)
   - Generated from `data/script_cmd_table.inc`
   - Python script: `tools/misc/make_scr_cmd_constants.py`

5. **Graphics** (`.1bpp`, `.4bpp`, `.8bpp`, `.gbapal`)
   - Converted from PNG via `tools/gbagfx/gbagfx`
   - Compression via `tools/compresSmol/compresSmol`

### Testing System Details

Tests use a domain-specific language (see `docs/tutorials/how_to_testing_system.md`):

**Test Structure:**
```c
SINGLE_BATTLE_TEST("Test name")
{
    GIVEN { /* Setup parties */ }
    WHEN { /* Specify turns */ }
    SCENE { /* Check UI output */ }
    THEN { /* Check internal state */ }
}
```

**Key Test Concepts:**
- Tests verify UI output (messages, animations, HP bars) not just internal state
- `ASSUME` documents prerequisites; test skipped if assumptions fail
- `PARAMETRIZE` runs test multiple times with different parameters
- `PASSES_RANDOMLY` tests probabilistic mechanics
- RNG rigged so moves always hit, never crit (unless specified)
- Tests in `test/battle/` organized by mechanic

**Running Single Test File:**
The test binary includes all tests but can filter by name prefix:
```bash
make TESTS="Stun Spore" check  # Runs all tests starting with "Stun Spore"
```

## Code Style (CRITICAL)

See `docs/STYLEGUIDE.md` for full details. Key points:

**Naming:**
- Functions/structs: `PascalCase`
- Variables/fields: `camelCase`
- Globals: `gVariableName`
- Statics: `sVariableName`
- Macros/constants: `CAPS_WITH_UNDERSCORES`
- Loop iterators: Declare in loop (`for (u32 i = 0; ...)`)

**Formatting:**
- 4 spaces (no tabs) for C/H files
- Tabs for assembly (.s) and scripts (.inc)
- Opening braces on next line for control structures
- Switch cases align with switch statement
- One empty line after blocks

**Data Types:**
- Default to `u32`/`s32` for most variables
- Use smallest type for: saveblock fields, EWRAM vars, global vars
- Use enums for sequential constants (not in saveblock)
- Always use enum types in function signatures

**Inline Configs:**
Check configs in normal control flow, not preprocessor (unless data structure changes):
```c
// Correct
if (!B_VAR_DIFFICULTY)
    return;

// Incorrect
#ifdef B_VAR_DIFFICULTY
    return;
#endif
```

**Principles:**
- Minimally invasive - isolate new code in its own files where possible
- Avoid magic numbers - use constants or enums
- Mark unused functions with `UNUSED` prefix
- Prefer explicit over implicit (e.g., enum types in parameters)

## Important Development Notes

1. **Never use GitHub "Download Zip"** - Clone with git to preserve history for merging updates

2. **Toolchain**: Modern GCC (arm-none-eabi) via devkitARM. agbcc deprecated as of 1.9.

3. **Saveblock Philosophy**: No save-breaking changes merged until save migration implemented. Save-breaking features batched for major version increments.

4. **Version Updates**: When updating, do so incrementally following guide in INSTALL.md (1.6.2 → 1.7.4 → 1.8.3 → 1.9.4 → 1.10.3).

5. **Multiplayer**: Compatible with other pokeemerald-expansion games, NOT official Pokemon games.

6. **External Tools** (not in repo):
   - porymap - Map editor
   - poryscript - Modern scripting language
   - porytiles - Metatile management

## Common Patterns

### Adding a New Move
1. Add constant to `include/constants/moves.h`
2. Add move data to `src/data/moves.inc.c`
3. Implement effect in battle scripts or C code
4. Add tests in `test/battle/move_effect/` or `test/battle/move/`

### Adding a New Ability
1. Add constant to `include/constants/abilities.h`
2. Add ability data to `src/data/abilities.h`
3. Hook effect in appropriate battle files
4. Add tests in `test/battle/ability/`

### Adding a New Pokemon
1. Add species constant to `include/constants/species.h`
2. Enable in `include/config/species_enabled.h`
3. Add data in `src/data/pokemon/` files (base stats, level up moves, etc.)
4. Add graphics in `graphics/pokemon/`
5. See `docs/tutorials/how_to_new_pokemon.md` for details

### Modifying Battle Behavior
1. Check if behavior controlled by `include/config/battle.h` define
2. Locate relevant `battle_*.c` file
3. Add test in `test/battle/` FIRST (TDD approach recommended)
4. Implement change, verify test passes
5. Consider if change should be config-gated

## File Locations Quick Reference

- Battle scripts: `data/battle_scripts_1.s`, `data/battle_scripts_2.s`
- Text strings: `src/data/text/` and various C files with `COMPOUND_STRING`
- Pokemon base stats: `src/data/pokemon/base_stats.h`
- Move data: `src/data/moves.inc.c`
- Item data: `src/data/items.h`
- Ability data: `src/data/abilities.h`
- Evolution data: `src/data/pokemon/evolution.h`
- Learnsets: `src/data/pokemon/level_up_learnsets.h`
- TM learnsets: `src/data/pokemon/tmhm_learnsets.h`
- Wild encounters: `src/data/wild_encounters.json`
- Trainer parties: `src/data/trainer_parties.h`
- Map scripts: `data/maps/*/scripts.inc`

## Documentation Resources

- Full documentation: https://rh-hideout.github.io/pokeemerald-expansion/
- Features list: `FEATURES.md`
- Contributing guide: `CONTRIBUTING.md`
- Style guide: `docs/STYLEGUIDE.md`
- Testing guide: `docs/tutorials/how_to_testing_system.md`
- Tutorials: `docs/tutorials/`
- Discord: https://discord.gg/6CzjAG6GZk
