# Save Strategy & Data Persistence

## Purpose

This document defines how Game Corner minigame data (high scores, play counts, unlocks) will be persisted across save/load cycles while preserving save compatibility with existing saves.

## Save Compatibility Constraint

**Critical Requirement:** Must preserve existing saves by default.

**Why This Matters:**
- Users have hundreds of hours invested in existing saves
- Breaking saves = community backlash
- RHH expansion prioritizes backwards compatibility
- Save breaks require MAJOR justification

## SaveBlock Architecture Overview

RHH uses three SaveBlocks defined in `include/global.h`:

### SaveBlock1 (0x3C00 bytes)

**Purpose:** Core game state that changes frequently

**Contents:**
- Player data (pos, name, gender, ID)
- Pokemon party & boxes
- Item bag
- Pokedex
- Map state
- Event flags & vars
- Game stats

**FREE Space Available:**
```c
// From include/global.h analysis
u8 filler_xyzABC[FREE_SAVEBLOCK1_SPACE];  // Check actual value
```

### SaveBlock2 (0x1000 bytes)

**Purpose:** Data that rarely changes

**Contents:**
- Hall of Fame records
- Battle Records
- Link Battle records
- Berry growth data
- Battle Tower streak
- TV shows

**FREE Space:** Limited, likely fully utilized.

### SaveBlock3 (Infrared data, unused in modern builds)

**Not Recommended** for minigame data.

### PokemonStorage (Boxes)

**Not Suitable** for minigame data.

## Strategy Options Analysis

### Option A: Use Existing Game Stats (RECOMMENDED)

**Approach:** Leverage existing `gGameStats[]` array for high scores.

**Pros:**
- ✅ **No save breaks** - game stats already in SaveBlock1
- ✅ **Zero risk** - proven persistence mechanism
- ✅ **Simple implementation** - just add new enum values
- ✅ **Automatic UI integration** - Trainer Card already shows stats
- ✅ **No memory overhead** - uses existing allocation

**Cons:**
- ❌ Limited to u32 values (acceptable for high scores)
- ❌ Game stats are visible in Trainer Card (minor cosmetic issue)
- ❌ Less semantic clarity (GAME_STAT_SNAKE_HIGH_SCORE vs dedicated struct)

**Implementation:**

```c
// include/constants/game_stats.h

enum {
    // ... existing game stats ...
    GAME_STAT_STARTED_TRENDS,
    GAME_STAT_PLANTED_BERRIES,
    GAME_STAT_TRADED_BIKES,

    // Game Corner minigame stats (add here)
    GAME_STAT_SNAKE_HIGH_SCORE,
    GAME_STAT_SNAKE_PLAYS,
    GAME_STAT_FLAPPYBIRD_HIGH_SCORE,
    GAME_STAT_FLAPPYBIRD_PLAYS,
    GAME_STAT_BLACKJACK_HIGH_SCORE,
    GAME_STAT_BLACKJACK_WINS,
    GAME_STAT_VOLTORB_FLIP_HIGH_SCORE,
    GAME_STAT_VOLTORB_FLIP_PLAYS,
    GAME_STAT_GACHA_PLAYS,
    GAME_STAT_GACHA_RARE_WINS,
    GAME_STAT_PACHINKO_HIGH_SCORE,
    GAME_STAT_PACHINKO_PLAYS,
    GAME_STAT_BLOCK_STACKER_HIGH_SCORE,
    GAME_STAT_BLOCK_STACKER_LINES_CLEARED,
    GAME_STAT_PINBALL_HIGH_SCORE,
    GAME_STAT_PINBALL_PLAYS,
    GAME_STAT_DERBY_WINS,
    GAME_STAT_DERBY_RACES,

    NUM_GAME_STATS
};
```

**Usage:**

```c
// Get high score
u32 highScore = GetGameStat(GAME_STAT_SNAKE_HIGH_SCORE);

// Update high score
if (currentScore > highScore)
    SetGameStat(GAME_STAT_SNAKE_HIGH_SCORE, currentScore);

// Increment play count
IncrementGameStat(GAME_STAT_SNAKE_PLAYS);
```

**Save Compatibility:** ✅ **PERFECT** - no save format changes required.

### Option B: Add Dedicated MinigameData Struct

**Approach:** Add `struct MinigameData` to SaveBlock1's FREE space.

**Pros:**
- ✅ Semantic clarity (dedicated data structure)
- ✅ Can store complex data (difficulty settings, config per game)
- ✅ Not visible in Trainer Card
- ✅ Easy to extend in future

**Cons:**
- ❌ **Breaks existing saves** (adds new data to SaveBlock1)
- ❌ Requires FREE_SAVEBLOCK1_SPACE verification
- ❌ Must coordinate with other features using FREE space
- ❌ More complex save migration logic

**Implementation:**

```c
// include/global.h

struct MinigameData {
    u32 highScores[9];         // 36 bytes (one per minigame)
    u16 playCount[9];          // 18 bytes
    u8 unlockedMinigames;      // 1 byte (bitflags)
    u8 difficultySettings;     // 1 byte (4 bits per setting for 2 games)
    u8 reserved[8];            // 8 bytes (future expansion)
};  // Total: 64 bytes

struct SaveBlock1 {
    // ... existing fields ...
    struct MinigameData minigameData;
    u8 filler_xyz[FREE_SAVEBLOCK1_SPACE - 64];  // Adjust FREE space
};
```

**FREE Space Check:**

Before implementing, **must verify** FREE_SAVEBLOCK1_SPACE >= 64 bytes:

```bash
grep "FREE_SAVEBLOCK1_SPACE" include/global.h
```

**Save Compatibility:** ❌ **BREAKS SAVES** - existing saves won't have this struct.

**Migration Strategy (if chosen):**

```c
// src/load_save.c (hypothetical)

void MigrateSaveBlock1_MinigameData(struct SaveBlock1 *saveBlock)
{
    // Check if save is from older version (lacks minigame data)
    if (saveBlock->minigameData.reserved[0] != MINIGAME_DATA_MAGIC)
    {
        // Initialize to defaults
        memset(&saveBlock->minigameData, 0, sizeof(struct MinigameData));
        saveBlock->minigameData.reserved[0] = MINIGAME_DATA_MAGIC;
        saveBlock->minigameData.unlockedMinigames = 0xFF;  // All unlocked
    }
}
```

### Option C: Use Flags and Vars Only

**Approach:** Store data in VAR_* and FLAG_* constants.

**Pros:**
- ✅ No save breaks (flags/vars already in SaveBlock1)
- ✅ Simple implementation

**Cons:**
- ❌ Limited capacity (16-bit vars, boolean flags)
- ❌ Inefficient storage (1 var = 2 bytes for potentially 1 bit of info)
- ❌ Difficult to manage (scattered across var space)
- ❌ Collision risk with other features

**Not Recommended** - Option A is superior in every way.

## Recommended Strategy: Hybrid Approach

**Primary:** Use Option A (Game Stats) for high scores and play counts.

**Secondary:** Use existing FLAGS for unlock states.

**Rationale:**
- Game stats handle persistence perfectly
- Flags are already used for unlock gates (e.g., FLAG_BADGE_01_GET)
- Zero save breaks
- Proven patterns
- Simple to implement

### Flag Allocation

**Pattern:** Use FLAG_UNUSED_* range for minigame unlocks.

```c
// include/constants/flags.h

// Find unused flag range (coordinate with other features)
#define FLAG_UNLOCKED_GAMECORNER_SNAKE       FLAG_UNUSED_0x020
#define FLAG_UNLOCKED_GAMECORNER_FLAPPYBIRD  FLAG_UNUSED_0x021
#define FLAG_UNLOCKED_GAMECORNER_BLACKJACK   FLAG_UNUSED_0x022
#define FLAG_UNLOCKED_GAMECORNER_VOLTORB     FLAG_UNUSED_0x023
#define FLAG_UNLOCKED_GAMECORNER_GACHA       FLAG_UNUSED_0x024
#define FLAG_UNLOCKED_GAMECORNER_PACHINKO    FLAG_UNUSED_0x025
#define FLAG_UNLOCKED_GAMECORNER_STACKER     FLAG_UNUSED_0x026
#define FLAG_UNLOCKED_GAMECORNER_PINBALL     FLAG_UNUSED_0x027
#define FLAG_UNLOCKED_GAMECORNER_DERBY       FLAG_UNUSED_0x028
```

**Usage:**

```c
// Check if unlocked
if (FlagGet(FLAG_UNLOCKED_GAMECORNER_SNAKE))
{
    // Allow play
}

// Unlock minigame
FlagSet(FLAG_UNLOCKED_GAMECORNER_SNAKE);
```

### Variable Allocation

**Avoid permanent vars** - use TEMP_VARS for runtime state only.

```c
// Runtime state during gameplay (does NOT persist)
#define VAR_MINIGAME_CURRENT_SCORE    VAR_TEMP_0
#define VAR_MINIGAME_CURRENT_LEVEL    VAR_TEMP_1
#define VAR_MINIGAME_LIVES_REMAINING  VAR_TEMP_2
```

**Why TEMP only:**
- TEMP vars are overwritten frequently (safe)
- Persistent vars are scarce (save for critical features)
- Game stats cover persistence needs

## Implementation Details

### Game Stat Helper Functions

```c
// src/game_corner_common.c

static const enum GameStat sMinigameHighScoreStats[] = {
    [MINIGAME_SNAKE]         = GAME_STAT_SNAKE_HIGH_SCORE,
    [MINIGAME_FLAPPYBIRD]    = GAME_STAT_FLAPPYBIRD_HIGH_SCORE,
    [MINIGAME_BLACKJACK]     = GAME_STAT_BLACKJACK_HIGH_SCORE,
    [MINIGAME_VOLTORB_FLIP]  = GAME_STAT_VOLTORB_FLIP_HIGH_SCORE,
    [MINIGAME_GACHA]         = GAME_STAT_GACHA_RARE_WINS,
    [MINIGAME_PACHINKO]      = GAME_STAT_PACHINKO_HIGH_SCORE,
    [MINIGAME_BLOCK_STACKER] = GAME_STAT_BLOCK_STACKER_HIGH_SCORE,
    [MINIGAME_PINBALL]       = GAME_STAT_PINBALL_HIGH_SCORE,
    [MINIGAME_DERBY]         = GAME_STAT_DERBY_WINS,
};

static const enum GameStat sMinigamePlayCountStats[] = {
    [MINIGAME_SNAKE]         = GAME_STAT_SNAKE_PLAYS,
    [MINIGAME_FLAPPYBIRD]    = GAME_STAT_FLAPPYBIRD_PLAYS,
    [MINIGAME_BLACKJACK]     = GAME_STAT_BLACKJACK_WINS,
    [MINIGAME_VOLTORB_FLIP]  = GAME_STAT_VOLTORB_FLIP_PLAYS,
    [MINIGAME_GACHA]         = GAME_STAT_GACHA_PLAYS,
    [MINIGAME_PACHINKO]      = GAME_STAT_PACHINKO_PLAYS,
    [MINIGAME_BLOCK_STACKER] = GAME_STAT_BLOCK_STACKER_LINES_CLEARED,
    [MINIGAME_PINBALL]       = GAME_STAT_PINBALL_PLAYS,
    [MINIGAME_DERBY]         = GAME_STAT_DERBY_RACES,
};

u32 GameCorner_GetHighScore(enum MinigameId gameId)
{
    if (gameId >= MINIGAME_COUNT)
        return 0;

    return GetGameStat(sMinigameHighScoreStats[gameId]);
}

void GameCorner_UpdateHighScore(enum MinigameId gameId, u32 score)
{
    if (gameId >= MINIGAME_COUNT)
        return;

    u32 currentHigh = GetGameStat(sMinigameHighScoreStats[gameId]);
    if (score > currentHigh)
        SetGameStat(sMinigameHighScoreStats[gameId], score);
}

void GameCorner_IncrementPlayCount(enum MinigameId gameId)
{
    if (gameId >= MINIGAME_COUNT)
        return;

    IncrementGameStat(sMinigamePlayCountStats[gameId]);
}

u32 GameCorner_GetPlayCount(enum MinigameId gameId)
{
    if (gameId >= MINIGAME_COUNT)
        return 0;

    return GetGameStat(sMinigamePlayCountStats[gameId]);
}
```

### Flag Helper Functions

```c
// src/game_corner_common.c

static const u16 sMinigameUnlockFlags[] = {
    [MINIGAME_SNAKE]         = FLAG_UNLOCKED_GAMECORNER_SNAKE,
    [MINIGAME_FLAPPYBIRD]    = FLAG_UNLOCKED_GAMECORNER_FLAPPYBIRD,
    [MINIGAME_BLACKJACK]     = FLAG_UNLOCKED_GAMECORNER_BLACKJACK,
    [MINIGAME_VOLTORB_FLIP]  = FLAG_UNLOCKED_GAMECORNER_VOLTORB,
    [MINIGAME_GACHA]         = FLAG_UNLOCKED_GAMECORNER_GACHA,
    [MINIGAME_PACHINKO]      = FLAG_UNLOCKED_GAMECORNER_PACHINKO,
    [MINIGAME_BLOCK_STACKER] = FLAG_UNLOCKED_GAMECORNER_STACKER,
    [MINIGAME_PINBALL]       = FLAG_UNLOCKED_GAMECORNER_PINBALL,
    [MINIGAME_DERBY]         = FLAG_UNLOCKED_GAMECORNER_DERBY,
};

bool8 GameCorner_IsUnlocked(enum MinigameId gameId)
{
    if (gameId >= MINIGAME_COUNT)
        return FALSE;

    return FlagGet(sMinigameUnlockFlags[gameId]);
}

void GameCorner_Unlock(enum MinigameId gameId)
{
    if (gameId >= MINIGAME_COUNT)
        return;

    FlagSet(sMinigameUnlockFlags[gameId]);
}

void GameCorner_UnlockAll(void)
{
    for (u32 i = 0; i < MINIGAME_COUNT; i++)
        FlagSet(sMinigameUnlockFlags[i]);
}
```

## Data Persistence Workflow

### Game Start

```c
// Minigame initialization
void SnakeInit(void)
{
    // Load high score from game stats
    u32 highScore = GameCorner_GetHighScore(MINIGAME_SNAKE);

    // Display high score in UI
    DisplayHighScore(highScore);

    // Initialize current score to 0
    VarSet(VAR_MINIGAME_CURRENT_SCORE, 0);
}
```

### During Gameplay

```c
// Score update (runtime only, no save)
void SnakeEatFood(void)
{
    u16 currentScore = VarGet(VAR_MINIGAME_CURRENT_SCORE);
    currentScore += 10;
    VarSet(VAR_MINIGAME_CURRENT_SCORE, currentScore);
}
```

### Game End

```c
// Save high score if new record
void SnakeGameOver(void)
{
    u16 finalScore = VarGet(VAR_MINIGAME_CURRENT_SCORE);

    // Check and update high score
    if (GameCorner_IsNewHighScore(MINIGAME_SNAKE, finalScore))
    {
        GameCorner_UpdateHighScore(MINIGAME_SNAKE, finalScore);
        ShowHighScoreMessage(finalScore);
    }

    // Increment play count
    GameCorner_IncrementPlayCount(MINIGAME_SNAKE);

    // Award coins based on score
    u16 coinsWon = CalculateCoinsFromScore(finalScore);
    GameCorner_AddCoins(coinsWon);

    // Game stats are auto-saved when player saves game
}
```

## Save Compatibility Testing

### Test Cases

**Test 1: New Save**
- Create new save file
- Play Snake, get high score of 100
- Save game
- Load game
- Verify high score persists (100)

**Test 2: Existing Save (Pre-Minigame)**
- Load save created before minigame implementation
- Verify game loads successfully ✅
- Play Snake, get high score of 50
- Save game
- Load game
- Verify high score persists (50)

**Test 3: Multiple Minigames**
- Play Snake (high score 100)
- Play Flappy Bird (high score 200)
- Save game
- Load game
- Verify both high scores persist

**Test 4: High Score Update**
- Existing high score: 100
- Play game, get 150
- Verify high score updates to 150
- Play game, get 75
- Verify high score remains 150 (not overwritten)

**Test 5: Unlock Persistence**
- Unlock Snake via script
- Save game
- Load game
- Verify Snake remains unlocked

### Automated Tests

```c
// test/game_corner_save.c

TEST("High score persists across save/load")
{
    // Setup
    SetGameStat(GAME_STAT_SNAKE_HIGH_SCORE, 0);

    // Play game and set high score
    GameCorner_UpdateHighScore(MINIGAME_SNAKE, 100);
    ASSERT_EQ(100, GameCorner_GetHighScore(MINIGAME_SNAKE));

    // Simulate save/load (actual save system integration)
    // ... TrySavingData(), LoadSaveblockData() ...

    // Verify persistence
    ASSERT_EQ(100, GameCorner_GetHighScore(MINIGAME_SNAKE));
}

TEST("Unlocks persist across save/load")
{
    // Setup
    FlagClear(FLAG_UNLOCKED_GAMECORNER_SNAKE);
    ASSERT_FALSE(GameCorner_IsUnlocked(MINIGAME_SNAKE));

    // Unlock
    GameCorner_Unlock(MINIGAME_SNAKE);
    ASSERT_TRUE(GameCorner_IsUnlocked(MINIGAME_SNAKE));

    // Simulate save/load
    // ...

    // Verify persistence
    ASSERT_TRUE(GameCorner_IsUnlocked(MINIGAME_SNAKE));
}
```

## Edge Cases & Safeguards

### Integer Overflow Protection

```c
void GameCorner_UpdateHighScore(enum MinigameId gameId, u32 score)
{
    // Game stats are u32, no overflow risk up to 4,294,967,295
    // But sanity check anyway
    if (score > 999999)  // Reasonable cap for minigames
        score = 999999;

    u32 currentHigh = GetGameStat(sMinigameHighScoreStats[gameId]);
    if (score > currentHigh)
        SetGameStat(sMinigameHighScoreStats[gameId], score);
}
```

### Corrupted Data Detection

```c
u32 GameCorner_GetHighScore(enum MinigameId gameId)
{
    if (gameId >= MINIGAME_COUNT)
        return 0;

    u32 score = GetGameStat(sMinigameHighScoreStats[gameId]);

    // Sanity check: unreasonably high scores = corruption
    if (score > 999999)
    {
        // Reset to 0 and log error
        SetGameStat(sMinigameHighScoreStats[gameId], 0);
        return 0;
    }

    return score;
}
```

### Save Failure Handling

Game stats are part of SaveBlock1, which is saved atomically with the rest of the save file. If save fails, **all data** (including game stats) fails together. No special handling needed.

## Memory Footprint

### Option A (Recommended): Game Stats + Flags

**Game Stats Added:** 18 new u32 values (18 × 4 = 72 bytes)
**Flags Added:** 9 new flags (9 bits ≈ 2 bytes)
**Total Overhead:** ~74 bytes

**Fits In:** Existing gGameStats[] expansion (no SaveBlock restructure)

### Option B: Dedicated Struct

**MinigameData Struct:** 64 bytes
**Must Fit In:** FREE_SAVEBLOCK1_SPACE (verify availability)

## Comparison Summary

| Feature | Option A (Game Stats) | Option B (Dedicated Struct) | Option C (Vars Only) |
|---------|----------------------|---------------------------|---------------------|
| Save Breaks | ✅ No | ❌ Yes | ✅ No |
| Implementation Complexity | ✅ Low | ⚠️ Medium | ✅ Low |
| Extensibility | ⚠️ Limited | ✅ High | ❌ Very Limited |
| Memory Efficiency | ✅ Good | ✅ Good | ❌ Poor |
| Collision Risk | ✅ Low | ⚠️ Medium | ❌ High |
| **Recommendation** | ✅ **USE THIS** | ⚠️ Only if community demands | ❌ Avoid |

## Final Recommendation

**Implement Option A: Game Stats + Flags**

**Justification:**
1. **Zero save breaks** - highest priority constraint
2. **Proven pattern** - game stats already used throughout codebase
3. **Simple implementation** - minimal code complexity
4. **Sufficient capacity** - u32 handles any reasonable high score
5. **Automatic UI integration** - Trainer Card shows stats for free

**When to Reconsider:**
- Community explicitly requests dedicated save struct
- Need to store complex per-game config (difficulty, unlockables)
- FREE_SAVEBLOCK1_SPACE verification shows ample room (100+ bytes)

**For Now:** Start with Option A. Can always migrate to Option B in a later update if needed (with save migration logic).

## Documentation Cross-References

**Related Documents:**
- `03_COMMON_INFRASTRUCTURE.md` - High score helper functions
- `05_CONFIG_DESIGN.md` - Runtime feature toggles
- `06_TESTING_APPROACH.md` - Save persistence testing

**RHH Documentation:**
- `include/global.h` - SaveBlock definitions
- `include/constants/game_stats.h` - Game stat enums
- `include/constants/flags.h` - Flag allocation
- `src/load_save.c` - Save/load implementation

## Implementation Checklist

- [ ] Verify FREE_SAVEBLOCK1_SPACE capacity (if using Option B)
- [ ] Add GAME_STAT_* constants to `include/constants/game_stats.h`
- [ ] Add FLAG_UNLOCKED_* constants to `include/constants/flags.h`
- [ ] Implement helper functions in `src/game_corner_common.c`
- [ ] Write unit tests for save/load persistence
- [ ] Test with existing save files (no corruption)
- [ ] Test with new save files (proper initialization)
- [ ] Test high score updates (only increase, never decrease)
- [ ] Test unlock persistence across sessions
- [ ] Document any flag/var allocation in CLAUDE.md
- [ ] Code review for integer overflow safeguards
- [ ] Performance test (save time < 1 second)

---

**Last Updated:** 2025-12-16
**Status:** Design Complete - Recommends Option A
**Decision:** Use Game Stats + Flags (no save breaks)
