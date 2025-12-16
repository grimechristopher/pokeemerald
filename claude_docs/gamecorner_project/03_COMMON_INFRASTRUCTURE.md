# Common Infrastructure Design

## Purpose

This document describes the shared infrastructure that all Game Corner minigames will use. By centralizing common functionality, we:

- Reduce code duplication across 9 minigames
- Ensure consistent behavior (coin handling, save integration, UI patterns)
- Simplify maintenance (fix once, applies to all games)
- Accelerate development (proven patterns ready to use)

## Files to Create

### Core Infrastructure

**`src/game_corner_common.c`** - Shared minigame utilities
**`include/game_corner_common.h`** - Public API and structures

### Integration Points

**`include/config/game_corner.h`** - Feature flags and configuration
**Modified: `include/global.h`** - Include config, optionally add SaveBlock data
**Modified: `src/field_specials.c`** - Add minigame special functions
**Modified: `include/field_specials.h`** - Declare special functions
**Modified: `data/specials.inc`** - Register special functions

## Common Data Structures

### Minigame Registry

```c
// include/game_corner_common.h

enum MinigameId {
    MINIGAME_SNAKE,
    MINIGAME_FLAPPYBIRD,
    MINIGAME_BLACKJACK,
    MINIGAME_VOLTORB_FLIP,
    MINIGAME_GACHA,
    MINIGAME_PACHINKO,
    MINIGAME_BLOCK_STACKER,
    MINIGAME_PINBALL,
    MINIGAME_DERBY,
    MINIGAME_COUNT
};

struct MinigameMetadata {
    enum MinigameId id;
    const u8 *name;           // "Snake", "Flappy Bird", etc.
    u16 entryCost;            // Coins required to play
    bool8 isUnlocked;         // Runtime check via flag
    u16 unlockFlag;           // FLAG_UNLOCKED_GAMECORNER_*
    void (*initFunc)(void);   // Minigame-specific init function
    void (*mainFunc)(void);   // Minigame main loop
    void (*exitFunc)(void);   // Cleanup function
};

extern const struct MinigameMetadata gMinigameRegistry[MINIGAME_COUNT];
```

### High Score Storage

```c
// Option A: Use existing game stats (no save break)
enum GameStat {
    // ... existing stats ...
    GAME_STAT_SNAKE_HIGH_SCORE,
    GAME_STAT_FLAPPYBIRD_HIGH_SCORE,
    GAME_STAT_BLACKJACK_HIGH_SCORE,
    GAME_STAT_VOLTORB_FLIP_HIGH_SCORE,
    GAME_STAT_GACHA_PLAYS,
    GAME_STAT_PACHINKO_HIGH_SCORE,
    GAME_STAT_BLOCK_STACKER_HIGH_SCORE,
    GAME_STAT_PINBALL_HIGH_SCORE,
    GAME_STAT_DERBY_WINS,
};

// Option B: Dedicated SaveBlock struct (if needed)
struct MinigameData {
    u32 highScores[MINIGAME_COUNT];  // 36 bytes
    u16 playCount[MINIGAME_COUNT];   // 18 bytes
    u8 unlockedMinigames;            // Bitflags (1 byte)
    u8 difficultySettings;           // Per-game difficulty (1 byte)
    u8 reserved[8];                  // Future expansion (8 bytes)
};  // Total: 64 bytes
```

**Recommendation**: Start with Option A (game stats). Only implement Option B if community feedback demands dedicated save space.

## Common Functions

### Initialization & Cleanup

```c
// src/game_corner_common.c

void GameCorner_InitMinigame(enum MinigameId gameId)
{
    // Common setup for all minigames
    SetVBlankCallback(NULL);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetTasks();
    ResetPaletteFade();

    // Call game-specific init
    const struct MinigameMetadata *game = &gMinigameRegistry[gameId];
    if (game->initFunc != NULL)
        game->initFunc();
}

void GameCorner_ExitMinigame(void)
{
    // Common cleanup
    FreeAllWindowBuffers();
    SetVBlankCallback(NULL);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetTasks();
    ResetPaletteFade();

    // Return to field
    SetMainCallback2(CB2_ReturnToField);
}

bool8 GameCorner_CheckUnlocked(enum MinigameId gameId)
{
    const struct MinigameMetadata *game = &gMinigameRegistry[gameId];
    return FlagGet(game->unlockFlag);
}

void GameCorner_UnlockMinigame(enum MinigameId gameId)
{
    const struct MinigameMetadata *game = &gMinigameRegistry[gameId];
    FlagSet(game->unlockFlag);
}
```

### Coin Integration

```c
// Coin management helpers

bool8 GameCorner_HasEnoughCoins(u16 cost)
{
    return GetCoins() >= cost;
}

bool8 GameCorner_TryTakeCoins(u16 cost)
{
    if (!GameCorner_HasEnoughCoins(cost))
        return FALSE;

    SetCoins(GetCoins() - cost);
    return TRUE;
}

void GameCorner_AddCoins(u16 amount)
{
    u16 currentCoins = GetCoins();
    u32 newCoins = currentCoins + amount;

    // Cap at 9999 (existing coin system limit)
    if (newCoins > MAX_COINS)
        newCoins = MAX_COINS;

    SetCoins((u16)newCoins);
}

void GameCorner_DisplayCoins(void)
{
    // Show coin count window (reuse existing pattern from slot_machine.c)
    DrawStdWindowFrame(0, FALSE);
    ConvertIntToDecimalStringN(gStringVar1, GetCoins(), STR_CONV_MODE_LEADING_ZEROS, 4);
    AddTextPrinterParameterized(0, FONT_NORMAL, gStringVar1, 0, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(0, COPYWIN_FULL);
}
```

### High Score Management

```c
// High score helpers (using Option A - game stats)

u32 GameCorner_GetHighScore(enum MinigameId gameId)
{
    // Map minigame ID to game stat
    static const enum GameStat sMinigameStatMap[MINIGAME_COUNT] = {
        GAME_STAT_SNAKE_HIGH_SCORE,
        GAME_STAT_FLAPPYBIRD_HIGH_SCORE,
        GAME_STAT_BLACKJACK_HIGH_SCORE,
        GAME_STAT_VOLTORB_FLIP_HIGH_SCORE,
        GAME_STAT_GACHA_PLAYS,
        GAME_STAT_PACHINKO_HIGH_SCORE,
        GAME_STAT_BLOCK_STACKER_HIGH_SCORE,
        GAME_STAT_PINBALL_HIGH_SCORE,
        GAME_STAT_DERBY_WINS,
    };

    return GetGameStat(sMinigameStatMap[gameId]);
}

bool8 GameCorner_IsNewHighScore(enum MinigameId gameId, u32 score)
{
    return score > GameCorner_GetHighScore(gameId);
}

void GameCorner_UpdateHighScore(enum MinigameId gameId, u32 score)
{
    if (GameCorner_IsNewHighScore(gameId, score))
    {
        static const enum GameStat sMinigameStatMap[MINIGAME_COUNT] = {
            GAME_STAT_SNAKE_HIGH_SCORE,
            GAME_STAT_FLAPPYBIRD_HIGH_SCORE,
            GAME_STAT_BLACKJACK_HIGH_SCORE,
            GAME_STAT_VOLTORB_FLIP_HIGH_SCORE,
            GAME_STAT_GACHA_PLAYS,
            GAME_STAT_PACHINKO_HIGH_SCORE,
            GAME_STAT_BLOCK_STACKER_HIGH_SCORE,
            GAME_STAT_PINBALL_HIGH_SCORE,
            GAME_STAT_DERBY_WINS,
        };

        SetGameStat(sMinigameStatMap[gameId], score);
    }
}
```

### UI Helpers

```c
// Common UI patterns used across minigames

void GameCorner_ShowInsufficientCoinsMessage(u16 required)
{
    ConvertIntToDecimalStringN(gStringVar1, required, STR_CONV_MODE_LEADING_ZEROS, 4);
    StringExpandPlaceholders(gStringVar4, gText_NotEnoughCoins);
    ShowFieldMessage(gStringVar4);
}

void GameCorner_ShowNoCoinCaseMessage(void)
{
    ShowFieldMessage(gText_NoCoinCase);
}

void GameCorner_ShowGameOverMessage(u32 score, u32 coinsWon)
{
    // Format: "GAME OVER!\nScore: {score}\nCoins: +{coinsWon}"
    ConvertIntToDecimalStringN(gStringVar1, score, STR_CONV_MODE_LEFT_ALIGN, 5);
    ConvertIntToDecimalStringN(gStringVar2, coinsWon, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringExpandPlaceholders(gStringVar4, gText_GameCornerGameOver);
    ShowFieldMessage(gStringVar4);
}

void GameCorner_ShowHighScoreMessage(u32 newHighScore)
{
    ConvertIntToDecimalStringN(gStringVar1, newHighScore, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringExpandPlaceholders(gStringVar4, gText_GameCornerNewHighScore);
    ShowFieldMessage(gStringVar4);
}

u8 GameCorner_CreateStandardWindow(u8 windowId, u8 x, u8 y, u8 width, u8 height)
{
    struct WindowTemplate template = {
        .bg = 0,
        .tilemapLeft = x,
        .tilemapTop = y,
        .width = width,
        .height = height,
        .paletteNum = 15,
        .baseBlock = 1
    };

    windowId = AddWindow(&template);
    PutWindowTilemap(windowId);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    return windowId;
}
```

### Input Handling

```c
// Common input patterns

bool8 GameCorner_WaitForAnyButton(void)
{
    return JOY_NEW(A_BUTTON | B_BUTTON | START_BUTTON);
}

bool8 GameCorner_WaitForAButton(void)
{
    return JOY_NEW(A_BUTTON);
}

bool8 GameCorner_WaitForBButton(void)
{
    return JOY_NEW(B_BUTTON);
}

bool8 GameCorner_CheckQuitRequested(void)
{
    // Allow B button or START to quit during gameplay
    return JOY_NEW(B_BUTTON | START_BUTTON);
}
```

### Audio Helpers

```c
// Common audio patterns

void GameCorner_PlayDefaultBGM(void)
{
    PlayBGM(MUS_GAME_CORNER);
}

void GameCorner_PlayWinSE(void)
{
    PlaySE(SE_WIN);
}

void GameCorner_PlayLoseSE(void)
{
    PlaySE(SE_FAILURE);
}

void GameCorner_PlayMenuSelectSE(void)
{
    PlaySE(SE_SELECT);
}

void GameCorner_PlayCoinSE(void)
{
    PlaySE(SE_SHOP);
}

void GameCorner_StopBGM(void)
{
    if (IsBGMPlaying())
        FadeOutBGM(4);
}
```

### Graphics Helpers

```c
// Common graphics operations

void GameCorner_LoadStandardPalettes(void)
{
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);
    LoadPalette(GetTextWindowPalette(0), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
}

void GameCorner_ShowCountdown(u8 seconds, void (*callback)(void))
{
    // Common countdown pattern (3...2...1...GO!)
    // Used by Flappy Bird, potentially others
    // Implementation would use task system
}

void GameCorner_FadeInFromBlack(void)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
}

void GameCorner_FadeOutToBlack(void (*callback)(void))
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    // Set callback when fade completes
}
```

## Common Text Strings

```c
// include/game_corner_common.h

extern const u8 gText_NotEnoughCoins[];
extern const u8 gText_NoCoinCase[];
extern const u8 gText_GameCornerGameOver[];
extern const u8 gText_GameCornerNewHighScore[];
extern const u8 gText_GameCornerPlayAgain[];
extern const u8 gText_GameCornerHighScore[];
extern const u8 gText_GameCornerUnlocked[];
```

```c
// src/game_corner_common.c

const u8 gText_NotEnoughCoins[] = _("You need {STR_VAR_1} Coins\nto play this game!");
const u8 gText_NoCoinCase[] = _("You need a COIN CASE\nto play!");
const u8 gText_GameCornerGameOver[] = _("GAME OVER!\nScore: {STR_VAR_1}\nCoins: +{STR_VAR_2}");
const u8 gText_GameCornerNewHighScore[] = _("NEW HIGH SCORE!\n{STR_VAR_1}");
const u8 gText_GameCornerPlayAgain[] = _("Play again?");
const u8 gText_GameCornerHighScore[] = _("High Score: {STR_VAR_1}");
const u8 gText_GameCornerUnlocked[] = _("You unlocked {STR_VAR_1}!");
```

## Integration with Existing Systems

### Field Specials

```c
// src/field_specials.c

void Special_StartMinigame(void)
{
    // VAR_0x8004 contains MinigameId
    enum MinigameId gameId = VarGet(VAR_0x8004);

    // Validate
    if (gameId >= MINIGAME_COUNT)
        return;

    // Check coin case
    if (!CheckBagHasItem(ITEM_COIN_CASE, 1))
    {
        GameCorner_ShowNoCoinCaseMessage();
        return;
    }

    // Check unlocked
    if (!GameCorner_CheckUnlocked(gameId))
    {
        ShowFieldMessage(gText_GameCornerLocked);
        return;
    }

    // Check coins
    const struct MinigameMetadata *game = &gMinigameRegistry[gameId];
    if (!GameCorner_HasEnoughCoins(game->entryCost))
    {
        GameCorner_ShowInsufficientCoinsMessage(game->entryCost);
        return;
    }

    // Deduct coins and start
    GameCorner_TryTakeCoins(game->entryCost);
    GameCorner_InitMinigame(gameId);
}

u16 Special_GetMinigameHighScore(void)
{
    // VAR_0x8004 contains MinigameId
    enum MinigameId gameId = VarGet(VAR_0x8004);

    if (gameId >= MINIGAME_COUNT)
        return 0;

    return GameCorner_GetHighScore(gameId);
}

bool8 Special_IsMinigameUnlocked(void)
{
    // VAR_0x8004 contains MinigameId
    enum MinigameId gameId = VarGet(VAR_0x8004);

    if (gameId >= MINIGAME_COUNT)
        return FALSE;

    return GameCorner_CheckUnlocked(gameId);
}
```

### Script Integration Pattern

```pory
// data/maps/MossdeepCity_GameCorner_B1F/scripts.pory (example)

script MossdeepCity_GameCorner_B1F_EventScript_SnakeGame {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_SNAKE)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("Play SNAKE for 10 Coins?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_SNAKE)
    special(Special_StartMinigame)
    releaseall
    end
}
```

## Resource Management Patterns

### Sprite Management

```c
// Common sprite loading pattern

u8 GameCorner_LoadSprite(const struct SpriteSheet *spriteSheet,
                          const struct SpritePalette *spritePalette,
                          const struct SpriteTemplate *spriteTemplate,
                          s16 x, s16 y)
{
    LoadSpriteSheet(spriteSheet);
    LoadSpritePalette(spritePalette);
    return CreateSprite(spriteTemplate, x, y, 0);
}

void GameCorner_FreeAllSprites(void)
{
    FreeAllSpritePalettes();
    ResetSpriteData();
}
```

### Background Management

```c
// Common BG loading pattern

void GameCorner_LoadBackground(u8 bgId, const void *tilemap, const void *tileset, const void *palette)
{
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));

    DecompressAndLoadBgGfxUsingHeap(bgId, tileset, 0x2000, 0, 0);
    CopyToBgTilemapBuffer(bgId, tilemap, 0, 0);
    LoadPalette(palette, BG_PLTT_ID(0), PLTT_SIZE_4BPP);

    ShowBg(bgId);
    CopyBgTilemapBufferToVram(bgId);
}

void GameCorner_SetupScrollingBackground(u8 bgId, s16 scrollSpeed)
{
    ChangeBgX(bgId, scrollSpeed, BG_COORD_ADD);
}
```

### Memory Allocation

```c
// Use existing allocation patterns from slot_machine.c

void *GameCorner_AllocZeroed(u32 size)
{
    void *mem = Alloc(size);
    if (mem != NULL)
        memset(mem, 0, size);
    return mem;
}

void GameCorner_Free(void *ptr)
{
    if (ptr != NULL)
        Free(ptr);
}
```

## Task System Integration

```c
// Common task patterns

void GameCorner_CreateMainTask(TaskFunc func, u8 priority)
{
    u8 taskId = CreateTask(func, priority);
    gTasks[taskId].data[0] = 0;  // State
    gTasks[taskId].data[1] = 0;  // Timer
}

void GameCorner_DestroyMainTask(u8 taskId)
{
    DestroyTask(taskId);
}

bool8 GameCorner_IsTaskActive(u8 taskId)
{
    return FuncIsActiveTask(TaskFunc_GetTaskByIdFunc(taskId));
}
```

## Configuration Integration

All minigames check configs at runtime:

```c
// Example usage in minigame code

void StartSnakeGame(void)
{
    if (!B_GAMECORNER_SNAKE)
    {
        ShowFieldMessage(gText_GameNotAvailable);
        return;
    }

    // Proceed with game
    GameCorner_InitMinigame(MINIGAME_SNAKE);
}
```

## Performance Considerations

### Frame Budget

All minigames must maintain 60 FPS:
- Maximum 16.67ms per frame
- Budget collision checks accordingly
- Use scanline effects sparingly
- Profile with `-g` flag during development

### Memory Budget

Reasonable limits per minigame:
- Sprite memory: 64KB available (share wisely)
- BG memory: 96KB for tilemaps/tilesets
- Working RAM: Keep game state structs under 1KB
- Audio: Reuse existing Game Corner music when possible

### Battery Usage

Save operations are expensive:
- Batch save writes (don't save every frame)
- Use game stats for high scores (1 write at game end)
- Avoid unnecessary SaveBlock modifications

## Testing Infrastructure

### Common Test Patterns

```c
// test/game_corner_common.c (example)

static void TestCoinsystem(void)
{
    // Setup
    SetCoins(100);

    // Test taking coins
    TEST_ASSERT(GameCorner_HasEnoughCoins(50));
    TEST_ASSERT(GameCorner_TryTakeCoins(50));
    TEST_ASSERT_EQUAL(50, GetCoins());

    // Test insufficient coins
    TEST_ASSERT_FALSE(GameCorner_TryTakeCoins(100));
    TEST_ASSERT_EQUAL(50, GetCoins());

    // Test adding coins
    GameCorner_AddCoins(50);
    TEST_ASSERT_EQUAL(100, GetCoins());

    // Test coin cap
    GameCorner_AddCoins(10000);
    TEST_ASSERT_EQUAL(MAX_COINS, GetCoins());
}

static void TestHighScores(void)
{
    // Setup
    SetGameStat(GAME_STAT_SNAKE_HIGH_SCORE, 100);

    // Test retrieval
    TEST_ASSERT_EQUAL(100, GameCorner_GetHighScore(MINIGAME_SNAKE));

    // Test new high score
    TEST_ASSERT_TRUE(GameCorner_IsNewHighScore(MINIGAME_SNAKE, 150));
    GameCorner_UpdateHighScore(MINIGAME_SNAKE, 150);
    TEST_ASSERT_EQUAL(150, GameCorner_GetHighScore(MINIGAME_SNAKE));

    // Test non-high score
    TEST_ASSERT_FALSE(GameCorner_IsNewHighScore(MINIGAME_SNAKE, 100));
    GameCorner_UpdateHighScore(MINIGAME_SNAKE, 100);
    TEST_ASSERT_EQUAL(150, GameCorner_GetHighScore(MINIGAME_SNAKE)); // Unchanged
}
```

## Documentation Cross-References

**Related Documents:**
- `01_ARCHITECTURE_COMPARISON.md` - Understanding RHH vs pret patterns
- `02_CODE_STYLE_MIGRATION.md` - Applying naming conventions
- `04_SAVE_STRATEGY.md` - High score persistence details
- `05_CONFIG_DESIGN.md` - Feature flag integration
- `06_TESTING_APPROACH.md` - Test coverage requirements

## Implementation Checklist

When implementing common infrastructure:

- [ ] Create `include/config/game_corner.h` with all feature flags
- [ ] Create `src/game_corner_common.c` with helper functions
- [ ] Create `include/game_corner_common.h` with public API
- [ ] Add `#include "config/game_corner.h"` to `include/global.h`
- [ ] Implement minigame registry in `game_corner_common.c`
- [ ] Implement coin helpers (take, add, check, display)
- [ ] Implement high score helpers (get, update, check new)
- [ ] Implement UI helpers (windows, messages, countdowns)
- [ ] Implement input helpers (button checks, quit detection)
- [ ] Implement audio helpers (BGM, SFX playback)
- [ ] Implement graphics helpers (palettes, backgrounds, sprites)
- [ ] Add common text strings with localization support
- [ ] Add field special functions to `src/field_specials.c`
- [ ] Register specials in `data/specials.inc`
- [ ] Write unit tests for all helper functions
- [ ] Verify no memory leaks (100+ game cycles)
- [ ] Profile performance (maintain 60 FPS)
- [ ] Document all public APIs with comments

## Summary

The common infrastructure provides:

1. **Unified entry point** - All minigames launch consistently
2. **Shared coin management** - No duplicate coin code across 9 games
3. **High score persistence** - Single implementation, reusable everywhere
4. **UI consistency** - Messages and windows follow RHH patterns
5. **Audio management** - Standard BGM/SFX playback
6. **Resource helpers** - Sprite/BG/palette loading patterns
7. **Testing foundation** - Reusable test utilities

**Benefits:**
- Faster development (proven patterns ready to use)
- Easier maintenance (fix once, applies to all)
- Better quality (shared code is well-tested)
- Consistent UX (all games feel cohesive)

**Next Steps:**
1. Implement `game_corner_common.c` per this spec
2. Write unit tests for all helpers
3. Use common infrastructure in Snake (first minigame)
4. Refine based on real-world usage

---

**Last Updated:** 2025-12-16
**Status:** Design Complete - Ready for Implementation
