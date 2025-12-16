# Architecture Comparison: pret vs RHH pokeemerald-expansion

## Executive Summary

This document outlines the key architectural differences between pret's pokeemerald (which heyopc's gamecorner-expansion is based on) and RHH's pokeemerald-expansion (our target). Understanding these differences is crucial for successfully porting minigame code.

**Key Takeaway:** RHH expansion is a significant evolution of pret with modern tooling, extensive config systems, and enhanced data structures. Direct code copies will NOT work - systematic adaptation is required.

## 1. Compilation & Toolchain

### pret pokeemerald (heyopc's base)

```makefile
# Uses agbcc (Game Boy Advance C Compiler)
CC := agbcc
CFLAGS := -mthumb-interwork -O2 -fhex-asm
Standard: C99
```

**Characteristics:**
- agbcc is a modified GCC 2.95 from 2001
- Limited C99 support
- No modern C features
- Specific GBA optimizations

### RHH pokeemerald-expansion

```makefile
# Makefile line 147
ASFLAGS := --defsym MODERN=1
CPPFLAGS := -DMODERN=1 -std=gnu17

# Modern GCC (arm-none-eabi-gcc)
CC := arm-none-eabi-gcc
CFLAGS := -mthumb -O2 -std=gnu17
```

**Characteristics:**
- Modern GCC from devkitARM
- Full C17 standard support
- Enhanced optimization
- Modern language features available

### Porting Impact

**What needs to change:**
- Remove agbcc-specific workarounds
- Can use modern C features (inline, designated initializers)
- Update compiler pragmas/attributes
- May need to adjust inline assembly syntax

**Example:**
```c
// pret/agbcc style
__asm__("swi 0x00"); // Old GCC syntax

// RHH/modern style
asm("swi 0x00");     // Modern syntax (both work, prefer asm)
```

**Global header check (include/global.h:33):**
```c
#if MODERN
#define asm __asm__
#else
#define __asm__ asm
#endif
```

## 2. Config System Architecture

### pret pokeemerald (heyopc's base)

**Minimal configs:**
- Few `#define` flags
- Mostly hardcoded behavior
- Gen 3 behavior only
- Feature toggles via `#ifdef`

**Example:**
```c
#ifdef SOME_FEATURE
    DoThing();
#endif
```

### RHH pokeemerald-expansion

**Extensive config system:**
- 18 separate config files (`include/config/*.h`)
- Generation-based behavior (`GEN_1` through `GEN_9`)
- Runtime config checks (NOT preprocessor)
- Feature flags, battle configs, pokemon configs, etc.

**Key config files:**
- `config/general.h` - Core settings, RHH_EXPANSION marker
- `config/battle.h` - 200+ battle mechanics configs
- `config/pokemon.h` - Species, breeding, graphics
- `config/overworld.h` - Overworld features
- `config/item.h` - Item behavior
- `config/save.h` - SaveBlock customization

**Config pattern:**
```c
#define B_CRIT_CHANCE GEN_LATEST
#define B_PHYSICAL_SPECIAL_SPLIT GEN_LATEST
```

Where `GEN_LATEST` = GEN_9 (currently)

**Runtime checking (NOT preprocessor):**
```c
// RHH style - preferred
if (B_CRIT_CHANCE >= GEN_7)
    ModernCritCalc();
else
    LegacyCritCalc();
```

### Porting Impact

**Critical changes required:**
1. Convert `#ifdef` to runtime `if` checks:
   ```c
   // pret style
   #ifdef FEATURE_X
       DoThing();
   #endif

   // Convert to RHH style
   if (B_GAMECORNER_FEATURE_X)
       DoThing();
   ```

2. Add config entries to `include/config/game_corner.h` (new file)

3. Use runtime checks unless struct layout changes

**Why runtime vs preprocessor?**
- RHH philosophy: Keep code paths compiled (catch bugs)
- Compiler optimizes away unreachable code anyway
- Only use preprocessor for struct changes or major features

## 3. Data Structure Evolution

### Pokemon Struct Changes

#### pret pokeemerald (Original Gen 3)

```c
struct Pokemon {
    u32 personality;
    u32 otId;
    u8 nickname[POKEMON_NAME_LENGTH];
    u8 language;
    // ... Gen 3 fields only
};
```

#### RHH pokeemerald-expansion (Gen 3-9)

```c
// include/pokemon.h - Enhanced structures
struct PokemonSubstruct0 {
    // Original Gen 3 fields
    u16 species;
    u16 heldItem;
    u32 experience;
    u8 ppBonuses;
    u8 friendship;

    // NEW Gen 4-9 fields
    u8 teraType:5;           // Gen 9: Terastallization
    u8 unused_04:3;
    // ... more additions
};

struct PokemonSubstruct1 {
    // ... Gen 3 fields ...

    // NEW: Evolution tracking (Gen 8+)
    u8 evolutionTracker1:5;
    u8 evolutionTracker2:5;

    // NEW: Hyper Training (Gen 7+)
    u8 hyperTrainedHP:1;
    u8 hyperTrainedAtk:1;
    // ... etc
};

struct PokemonSubstruct3 {
    // ... Gen 3 fields ...

    // NEW Gen 8+ Dynamax
    u8 dynamaxLevel:4;
    u8 gigantamaxFactor:1;
};
```

**Access Pattern:**
```c
// DON'T use direct access for new fields
mon->teraType = TYPE_FIRE; // May break

// DO use getter/setter
SetMonData(&mon, MON_DATA_TERA_TYPE, &teraType);
u8 type = GetMonData(&mon, MON_DATA_TERA_TYPE, NULL);
```

### Move Struct Changes

#### pret (Gen 3)

```c
struct BattleMove {
    u8 effect;
    u8 power;
    u8 type;
    u8 accuracy;
    u8 pp;
    u8 secondaryEffectChance;
    u8 target;
    s8 priority;
    u32 flags;
};
```

#### RHH (Gen 3-9)

```c
// include/move.h:63-150 - Massive expansion
struct BattleMove {
    // Original fields enhanced
    u16 effect;              // More effects
    u8 power;
    u8 type;
    u8 accuracy;
    u8 pp;
    u8 secondaryEffectChance;
    u8 target;
    s8 priority;

    // NEW: Type categorization flags
    u16 soundMove:1;
    u16 ballisticMove:1;
    u16 powderMove:1;
    u16 danceMove:1;
    u16 windMove:1;
    u16 slicingMove:1;
    u16 healingMove:1;

    // NEW: Effect enhancements
    u16 alwaysCriticalHit:1;
    u8 criticalHitStage;
    u8 strikeCount;          // Multi-hit support

    // NEW: Ban flags
    u16 forbiddenGravity:1;
    u16 forbiddenMirrorMove:1;
    u16 forbiddenMetronome:1;
    // ... 20+ ban flags

    // NEW: Situation-specific
    u16 minimizeDoubleDamage:1;
    u16 damagesUnderground:1;
    u16 damagesUnderwater:1;
    u16 damagesAirborne:1;

    // NEW: Union for move-specific data
    union {
        struct TwoTurnAttack twoTurnAttack;
        u8 protectMethod;
        u32 status;
        u16 fixedDamage;
        // ... many more
    };
};
```

### Item Struct Changes

#### pret (Gen 3)

```c
struct Item {
    const u8 *name;
    u16 itemId;
    u16 price;
    u8 holdEffect;
    u8 holdEffectParam;
    const u8 *description;
    u8 importance;
    u8 pocket;
    u8 type;
    ItemUseFunc fieldUseFunc;
    u8 battleUsage;
    u8 battleUseFunc;
    u32 flingPower;
};
```

#### RHH (Gen 3-9)

```c
// include/item.h:89-109
struct Item {
    const u8 *name;
    u16 itemId;
    u16 price;
    u8 holdEffect;
    u8 holdEffectParam;
    const u8 *description;
    u16 secondaryId;         // NEW: Categorization
    u8 importance;

    // NEW: Item sorting
    ItemSortType sortType;   // 35+ categories

    u8 pocket;
    u8 type;
    ItemUseFunc fieldUseFunc;
    u8 battleUsage;
    u8 battleUseFunc;
    u32 flingPower;
};
```

**Dynamic TM/HM mapping:**
```c
// pret: Hardcoded TM numbers
#define ITEM_TM01 ITEM_FOCUS_PUNCH

// RHH: Dynamic lookup via gTMHMItemMoveIds table
u16 GetTMHMMove(u16 itemId);
```

### Porting Impact for Minigames

**Minigames rarely interact with Pokemon/Move structs directly**, but:

1. If minigame references species constants, use `SPECIES_*` (unchanged)
2. If minigame plays Pokemon cries: Use `PlayCry_Normal(species, ...)`
3. If minigame awards items: Use `AddBagItem(itemId, quantity)`
4. If minigame needs random Pokemon: Use existing party functions

**Most relevant:** SaveBlock changes (see Section 5)

## 4. SaveBlock Architecture

### pret pokeemerald

**Fixed SaveBlock:**
```c
// Three save blocks with fixed sizes
struct SaveBlock1 { ... };  // Player/party data
struct SaveBlock2 { ... };  // Pokedex/link data
struct SaveBlock3 { ... };  // Runtime data
```

**Characteristics:**
- Fixed structure (can't add fields without breaking saves)
- No reclaimed space from unused features
- Direct struct access

### RHH pokeemerald-expansion

**Modular SaveBlock with FREE_* space:**

```c
// include/config/save.h
#define FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK1 TRUE   // +52 bytes
#define FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK2 TRUE   // +108 bytes
#define FREE_MYSTERY_GIFT TRUE                  // +876 bytes
#define FREE_UNION_ROOM_CHAT TRUE               // +212 bytes
// ... more FREE_* options

// include/global.h - SaveBlock1 structure
struct SaveBlock1 {
    // Original Gen 3 fields
    struct Coords16 pos;
    struct WarpData warp[WARP_BUFFERS_COUNT];
    // ...

    // Conditional additions based on FREE_*
    #if FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK1
    u8 reclaimedSpace1[52];
    #endif
};

// SaveBlock3 - Runtime data with conditional features
struct SaveBlock3 {
    // Core runtime data
    struct MapPosition mapPosition;

    // NEW: Conditional features
    #if FNPC_ENABLE_NPC_FOLLOWERS
    struct NPCFollower npcFollower;
    #endif

    #if OW_USE_FAKE_RTC
    struct FakeRTC fakeRTC;
    #endif

    #if OW_SHOW_ITEM_DESCRIPTIONS == OW_ITEM_DESCRIPTIONS_FIRST_TIME
    u8 itemDescriptionFlags[128];
    #endif

    // NEW: DexNav, Apricorn trees, etc.
    u16 dexNavSearchLevels[NATIONAL_DEX_COUNT];
    u8 apricornTreeFlags[NUM_APRICORN_TREES];
};
```

**ASLR wrappers (include/load_save.h):**
```c
// Address space layout randomization support
struct SaveBlock1ASLR { ... };
struct SaveBlock2ASLR { ... };
```

### Porting Impact

**For Game Corner minigames:**

**Option A: No SaveBlock changes (recommended)**
- Use existing game stats array for high scores
- Use `VAR_TEMP_*` for runtime state
- Avoids save compatibility issues

**Option B: Add minigame data (if needed)**
```c
// Add to SaveBlock1 (if FREE_* space available)
struct MinigameData {
    u32 highScores[9];      // One per minigame
    u16 playCount[9];
    u8 unlockedMinigames;   // Bitflags
    u8 difficultySettings;
}; // 56 bytes total
```

**Accessing SaveBlock:**
```c
// pret style (direct)
gSaveBlock1Ptr->coins += 100;

// RHH style (still direct for coins, but check for new fields)
if (gSaveBlock1Ptr->coins < MAX_COINS)
    gSaveBlock1Ptr->coins += 100;
```

**Game Stats for high scores:**
```c
// Use existing game stats (no save changes needed)
// include/constants/game_stat.h
enum {
    GAME_STAT_SAVED_GAME,
    GAME_STAT_FIRST_HOF_PLAY_TIME,
    // ... existing stats ...
    GAME_STAT_SLOT_JACKPOTS,         // Existing
    GAME_STAT_ROULETTE_WINS,         // Existing
    // Add new stats here (if slots available)
};

// Access
IncrementGameStat(GAME_STAT_CUSTOM_MINIGAME);
u32 score = GetGameStat(GAME_STAT_CUSTOM_MINIGAME);
```

## 5. Script System Differences

### pret pokeemerald

**Basic script system:**
```c
// Simple script commands
struct ScriptContext {
    u8 stackDepth;
    u8 mode;
    u8 comparisonResult;
    const u8 *scriptPtr;
    const u8 *stack[20];
    ScrCmdFunc *cmdTable;
    u32 data[4];
};
```

**Script execution:**
- Linear execution
- Basic flow control
- Limited error handling

### RHH pokeemerald-expansion

**Enhanced script system:**
```c
// include/script.h - Enhanced context
struct ScriptContext {
    u8 stackDepth;
    u8 mode;
    u8 comparisonResult;
    bool8 breakOnTrainerBattle;  // NEW: Trainer battle awareness
    u8 (*nativePtr)(void);
    const u8 *scriptPtr;
    const u8 *stack[20];
    ScrCmdFunc *cmdTable;
    ScrCmdFunc *cmdTableEnd;
    u32 data[4];
};
```

**NEW features:**
- Effect tracking system (`SCREFF_SAVE`, `SCREFF_HARDWARE`)
- Script validation (`ValidateSavedRamScript`)
- Effect-based execution (`RunScriptImmediatelyUntilEffect`)
- Trainer battle flow control

### Porting Impact

**For minigame scripts:**

**Script command pattern (unchanged):**
```assembly
MossdeepCity_GameCorner_EventScript_Snake::
    lockall
    checkitem ITEM_COIN_CASE
    goto_if_eq VAR_RESULT, FALSE, EventScript_NoCoinCase
    setvar VAR_0x8004, MINIGAME_ID_SNAKE
    specialvar VAR_RESULT, GetMinigameId
    playminigame VAR_RESULT  # Custom command to add
    releaseall
    end
```

**New special function registration:**
```c
// data/specials.inc
def_special GetMinigameId
def_special StartSnakeGame
def_special StartFlappyBirdGame
// ... etc
```

**Implementation in field_specials.c:**
```c
u16 GetMinigameId(void)
{
    u16 machineId = gSpecialVar_0x8004;
    // Map machine ID to minigame enum
    return CalculateMinigameFromCoords(machineId);
}
```

## 6. Graphics System Differences

### pret pokeemerald

**Manual graphics:**
- Hand-crafted .s files
- Manual palette management
- Direct VRAM manipulation

**File format:**
```
graphics/pokemon/pikachu/
├── anim_front.png
├── back.png
├── front.png
├── normal.pal
└── shiny.pal
```

### RHH pokeemerald-expansion

**Automated graphics pipeline:**
- PNG → 4bpp conversion via build system
- Automatic palette generation
- Sprite compression support
- Gender variant support

**Config-driven (include/config/pokemon.h):**
```c
#define P_GBA_STYLE_SPECIES_GFX FALSE         // Use Gen4/5 sprites
#define P_GBA_STYLE_SPECIES_ICONS FALSE       // Updated icons
#define P_GENDER_DIFFERENCES TRUE             // Female variants
#define OW_POKEMON_OBJECT_EVENTS TRUE         // OW sprites
#define OW_GFX_COMPRESS TRUE                  // Compression
```

**Build system (graphics_file_rules.mk):**
```makefile
%.4bpp: %.png ; $(GFX) $< $@
%.gbapal: %.pal ; $(GFX) $< $@
%.lz: % ; $(GFX) $< $@
%.smol: % ; $(SMOL) -w $< $@
```

**Auto-generated header (include/graphics.h):**
- 152,228 bytes
- Contains all sprite/icon data
- Tool-generated, DO NOT EDIT MANUALLY

### Porting Impact

**For minigame graphics:**

1. **Create graphics directory:**
   ```
   graphics/game_corner_snake/
   ├── snake_tiles.png         # Source image
   ├── snake_tiles.4bpp        # Generated by build
   ├── snake_tiles.gbapal      # Generated by build
   ├── food.png
   └── background.png
   ```

2. **Build system handles conversion automatically**

3. **Reference in code:**
   ```c
   // Load graphics (pattern from slot_machine.c)
   const u32 gSnakeTiles[] = INCBIN_U32("graphics/game_corner_snake/snake_tiles.4bpp.lz");
   const u16 gSnakePalette[] = INCBIN_U16("graphics/game_corner_snake/snake_tiles.gbapal");
   ```

4. **Compression options:**
   - `.lz` - LZ77 compression
   - `.smol` - Custom compression (smaller)
   - `.fastSmol` - Fast compression
   - `.smolTM` - Tilemap compression

## 7. Testing Framework

### pret pokeemerald

**No built-in testing:**
- Manual testing only
- No test framework
- No automated battle tests

### RHH pokeemerald-expansion

**Comprehensive testing system:**

**Test DSL (GIVEN/WHEN/SCENE/THEN):**
```c
// test/battle/move_effect/poison_hit.c
SINGLE_BATTLE_TEST("Poison Sting inflicts poison")
{
    GIVEN {
        ASSUME(GetMoveEffect(MOVE_POISON_STING) == EFFECT_POISON_HIT);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WOBBUFFET);
    }
    WHEN {
        TURN { MOVE(player, MOVE_POISON_STING); }
    }
    SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_POISON_STING, player);
        MESSAGE("The opposing Wobbuffet is poisoned!");
        STATUS_ICON(opponent, poison: TRUE);
    }
}
```

**Test execution:**
```bash
# Run all tests
make check -j

# Run specific tests
make check TESTS="Poison Sting"

# Build test ROM
make pokeemerald-test.elf TESTS="Poison Sting"
```

**mgba-rom-test integration:**
- Automated test runner
- Headless testing
- CI/CD support

### Porting Impact

**For minigame testing:**

1. **Create test file:**
   ```
   test/game_corner_snake.c
   ```

2. **Write unit tests:**
   ```c
   SINGLE_BATTLE_TEST("Snake: Self-collision ends game")
   {
       GIVEN {
           InitSnakeGame();
           // Setup snake in collision state
       }
       WHEN {
           SimulateSnakeMovement();
       }
       THEN {
           EXPECT_EQ(GetSnakeGameState(), SNAKE_STATE_GAME_OVER);
       }
   }
   ```

3. **Run tests:**
   ```bash
   make check TESTS="Snake"
   ```

**Test categories for minigames:**
- Initialization (resource allocation)
- Gameplay logic (collision, scoring)
- Edge cases (overflow, underflow)
- Coin system integration
- Save persistence

## 8. Code Style & Conventions

### pret pokeemerald

**Varied styles:**
- Mix of snake_case and camelCase
- Inconsistent prefix usage
- Variable scoping varies

### RHH pokeemerald-expansion

**Strict style guide (docs/STYLEGUIDE.md):**

**Naming:**
```c
// Functions and structs: PascalCase
void SnakeGameInit(void);
struct SnakeGameState { ... };

// Variables and fields: camelCase
u32 currentScore;
bool8 isGameOver;

// Globals: g prefix
u32 gSnakeHighScore;

// Statics: s prefix
static u32 sCurrentLevel;

// Constants and macros: CAPS_WITH_UNDERSCORES
#define MAX_SNAKE_LENGTH 100
#define SNAKE_TILE_SIZE 8

// Enums: CAPS_WITH_UNDERSCORES
enum SnakeDirection {
    SNAKE_DIR_UP,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_RIGHT
};
```

**Loop iterators:**
```c
// pret style
int i;
for (i = 0; i < 10; i++)

// RHH style (declare in loop)
for (u32 i = 0; i < 10; i++)
```

**Whitespace:**
```c
// 4 spaces (NO TABS) for C files
void MyFunction(void)
{
    if (condition)
    {
        DoThing();
    }
}

// Tabs for assembly (.s) and scripts (.inc)
```

**Control structures:**
```c
// Opening brace on next line
if (condition)
{
    DoThing();
}

// Switch cases align with switch
switch (value)
{
case 0:
    HandleZero();
    break;
case 1:
    HandleOne();
    break;
}

// Empty line after blocks
if (condition)
    DoThing();

return result;  // Empty line before return
```

**Comments:**
```c
// Single-line comments with space after //
// This explains the following code

/*
 * Multi-line comments for detailed explanations
 * Format like this for block comments
 */

// Comments on same line for data
const u8 gData[] = {
    1,  // First value
    2,  // Second value
};
```

### Porting Impact

**Systematic renaming required:**

1. **Search and replace patterns:**
   ```
   snake_init → SnakeInit
   current_score → currentScore
   static int level → static u32 sLevel
   ```

2. **Add prefixes:**
   ```c
   // Find all globals
   int score;  // Global
   → u32 gSnakeScore;

   // Find all statics
   static int level;
   → static u32 sLevel;
   ```

3. **Reformat code:**
   - Use 4 spaces throughout
   - Move braces to next line
   - Add empty lines between blocks
   - Declare iterators in loops

**Tools to help:**
- `clang-format` with RHH config (if available)
- Search/replace in editor
- Manual review pass

## 9. Build System Differences

### pret pokeemerald

**Basic Makefile:**
- Simple build rules
- Limited options
- Manual dependency tracking

### RHH pokeemerald-expansion

**Advanced Makefile:**

**Build options:**
```makefile
COMPARE ?= 0     # Compare to original ROM
TEST ?= 0        # Build test ROM
ANALYZE ?= 0     # Enable -fanalyzer
DEBUG ?= 0       # Debug build (-Og -g)
LTO ?= 0         # Link-time optimization
RELEASE ?= 0     # Release build (LTO + NDEBUG)
KEEP_TEMPS ?= 1  # Keep intermediate files
```

**Usage:**
```bash
# Basic build
make -j$(nproc)

# Debug build
make debug -j$(nproc)

# Release build
make release -j$(nproc)

# Run tests
make check -j$(nproc)

# Test specific minigame
make check TESTS="Snake" -j$(nproc)
```

**Parallel builds:**
```bash
# Linux
make -j$(nproc)

# macOS
make -j$(sysctl -n hw.ncpu)
```

**Auto-generated files:**
- Graphics conversion (PNG → 4bpp)
- Map data (JSON → .inc)
- Wild encounters (JSON → .h)
- Learnsets (scripts generate)
- Script commands (auto-generate constants)

### Porting Impact

**Build integration:**

1. Minigame code compiles with existing Makefile
2. Graphics auto-convert via graphics_file_rules.mk
3. No manual build steps needed
4. Can test individual minigames: `make check TESTS="Snake"`

## 10. Summary: Critical Differences

### Top 10 Differences That Impact Porting

1. **Compilation:** agbcc (C99) → Modern GCC (C17)
2. **Configs:** Preprocessor → Runtime checks
3. **Naming:** Various → Strict PascalCase/camelCase/g/s
4. **Loops:** Separate declaration → Declare in loop
5. **SaveBlock:** Fixed → Modular with FREE_* space
6. **Graphics:** Manual → Automated build pipeline
7. **Testing:** None → Comprehensive DSL framework
8. **Data Structures:** Gen 3 only → Gen 3-9 enhanced
9. **Script System:** Basic → Effect tracking + validation
10. **Build System:** Simple → Advanced with options

### Porting Checklist

For each minigame from heyopc:

- [ ] Update function names to PascalCase
- [ ] Update variables to camelCase
- [ ] Add g/s prefixes to globals/statics
- [ ] Convert loop iterators to in-loop declaration
- [ ] Replace `#ifdef` with `if (B_CONFIG)`
- [ ] Update includes (global.h, coins.h, etc.)
- [ ] Convert graphics to PNG → 4bpp pipeline
- [ ] Add audio to sound/ directory
- [ ] Write unit tests
- [ ] Add script integration
- [ ] Verify save compatibility
- [ ] Performance test (60 FPS)

### Files to Reference

**Style guide:**
- `/home/chris/Documents/Github/pokeemerald/docs/STYLEGUIDE.md`

**Existing minigames (patterns):**
- `/home/chris/Documents/Github/pokeemerald/src/slot_machine.c`
- `/home/chris/Documents/Github/pokeemerald/src/roulette.c`

**Config examples:**
- `/home/chris/Documents/Github/pokeemerald/include/config/battle.h`
- `/home/chris/Documents/Github/pokeemerald/include/config/pokemon.h`

**Test examples:**
- `/home/chris/Documents/Github/pokeemerald/test/battle/move_effect/*.c`

---

**Document Version:** 1.0
**Last Updated:** 2025-12-16
**Purpose:** Guide heyopc → RHH minigame porting
