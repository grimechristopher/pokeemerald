# Config System Design

## Purpose

This document defines the configuration system for Game Corner minigames, following RHH's established config patterns. All minigames will be toggleable at runtime without recompilation.

## RHH Config Philosophy

**Key Principle:** Runtime checks, not preprocessor conditionals.

**Why:**
- Single binary supports multiple configs (no recompilation)
- Easier testing (toggle features at runtime)
- Better user experience (configs in `include/config/` are clear)
- Compiler can optimize away disabled branches (smart compilers)

**Pattern:**

```c
// ❌ OLD WAY (pret style - preprocessor)
#ifdef FEATURE_X
    DoThing();
#endif

// ✅ NEW WAY (RHH style - runtime check)
if (B_FEATURE_X == TRUE)
    DoThing();
```

**Compiler Optimization:**
If `B_FEATURE_X` is `FALSE`, modern compilers eliminate the entire `if` block at compile-time (zero runtime cost).

## File Location

**New File:** `/include/config/game_corner.h`

**Included By:** `include/config.h` (which is included by `include/global.h`)

**Pattern:** Follows existing RHH config files:
- `include/config/battle.h` - Battle system configs
- `include/config/item.h` - Item expansion configs
- `include/config/overworld.h` - Overworld feature configs
- `include/config/pokemon.h` - Pokemon feature configs

## Config File Structure

```c
// include/config/game_corner.h

#ifndef GUARD_CONFIG_GAME_CORNER_H
#define GUARD_CONFIG_GAME_CORNER_H

// ========================================
// GAME CORNER MINIGAMES
// ========================================
// Enable/disable individual minigames.
// TRUE: Minigame is available (default)
// FALSE: Minigame is disabled (hidden from menus)
// ========================================

// Tier 1 - Simple Games
#define B_GAMECORNER_SNAKE              TRUE    // Snake game (grid-based)
#define B_GAMECORNER_FLAPPYBIRD         TRUE    // Flappy Bird clone
#define B_GAMECORNER_BLACKJACK          TRUE    // Blackjack card game

// Tier 2 - Medium Games
#define B_GAMECORNER_VOLTORB_FLIP       TRUE    // Voltorb Flip (HGSS puzzle)
#define B_GAMECORNER_GACHA              TRUE    // Gacha prize machines
#define B_GAMECORNER_PACHINKO           TRUE    // Pachinko (Japanese pinball)

// Tier 3 - Complex Games
#define B_GAMECORNER_BLOCK_STACKER      TRUE    // Block Stacker (Tetris-like)
#define B_GAMECORNER_PINBALL            TRUE    // Pokemon Pinball adaptation
#define B_GAMECORNER_DERBY              TRUE    // Pokemon racing game

// ========================================
// DIFFICULTY SETTINGS
// ========================================
// Adjust gameplay difficulty for supported games.
// GEN_3: Easy (original Ruby/Sapphire difficulty)
// GEN_4: Normal (balanced)
// GEN_5: Hard (increased challenge)
// GEN_LATEST: Very Hard (expert mode)
// ========================================

#define B_GAMECORNER_DIFFICULTY         GEN_4   // Default: Normal difficulty

// Per-game difficulty overrides (optional)
#define B_SNAKE_DIFFICULTY              GEN_4   // Snake-specific difficulty
#define B_FLAPPYBIRD_DIFFICULTY         GEN_4   // Flappy Bird-specific difficulty
#define B_VOLTORB_FLIP_DIFFICULTY       GEN_4   // Voltorb Flip-specific difficulty

// ========================================
// GAMEPLAY FEATURES
// ========================================

// High score persistence using game stats
#define B_GAMECORNER_HIGH_SCORES        TRUE    // Track high scores across saves

// Show "NEW RECORD!" message when high score is beaten
#define B_GAMECORNER_HIGH_SCORE_FANFARE TRUE    // Play fanfare for new records

// Allow replaying without exiting to map
#define B_GAMECORNER_QUICK_REPLAY       TRUE    // "Play Again?" prompt after game over

// Tutorial messages on first play
#define B_GAMECORNER_TUTORIALS          TRUE    // Show tutorial messages

// Unlock all games from start (useful for testing)
#define B_GAMECORNER_UNLOCK_ALL         FALSE   // Default: games start locked

// ========================================
// COIN REWARDS
// ========================================

// Multiplier for coin rewards
// 10: Original rewards (10 coins for score 100)
// 20: Double rewards (20 coins for score 100)
// 5: Half rewards (5 coins for score 100)
#define B_GAMECORNER_COIN_MULTIPLIER    10      // Default: 1.0x (10 = 100%)

// Minimum coins awarded (even for score 0)
#define B_GAMECORNER_MIN_COINS          1       // Always get at least 1 coin

// Maximum coins per game session
#define B_GAMECORNER_MAX_COINS_PER_GAME 999     // Cap at 999 coins per session

// ========================================
// ENTRY COSTS
// ========================================
// Coins required to play each minigame.
// 0: Free to play (useful for testing)
// ========================================

#define B_SNAKE_ENTRY_COST              10      // 10 coins to play Snake
#define B_FLAPPYBIRD_ENTRY_COST         20      // 20 coins to play Flappy Bird
#define B_BLACKJACK_ENTRY_COST          50      // 50 coins to play Blackjack
#define B_VOLTORB_FLIP_ENTRY_COST       0       // Free (rewards come from gameplay)
#define B_GACHA_ENTRY_COST              20      // 20 coins per pull
#define B_PACHINKO_ENTRY_COST           10      // 10 coins to play Pachinko
#define B_BLOCK_STACKER_ENTRY_COST      30      // 30 coins to play Block Stacker
#define B_PINBALL_ENTRY_COST            50      // 50 coins to play Pinball
#define B_DERBY_ENTRY_COST              100     // 100 coins to enter Derby

// ========================================
// AUDIO SETTINGS
// ========================================

// Use custom minigame BGM (TRUE) or reuse existing Game Corner music (FALSE)
#define B_GAMECORNER_CUSTOM_BGM         TRUE    // Default: use custom music

// Sound effects volume (percentage)
#define B_GAMECORNER_SFX_VOLUME         100     // 100 = full volume, 50 = half volume

// ========================================
// PERFORMANCE SETTINGS
// ========================================

// Target framerate (always 60 for GBA, but useful for debugging)
#define B_GAMECORNER_TARGET_FPS         60      // Do not change

// Enable performance profiling (adds debug overlay)
#define B_GAMECORNER_SHOW_FPS           FALSE   // Show FPS counter (debug only)

// Particle effect quality
// 0: No particles (best performance)
// 1: Minimal particles
// 2: Full particles (default)
#define B_GAMECORNER_PARTICLE_QUALITY   2       // Default: full particles

// ========================================
// DEBUG SETTINGS
// ========================================
// WARNING: These should be FALSE in release builds!
// ========================================

// Enable debug mode (shows collision boxes, state info, etc.)
#define B_GAMECORNER_DEBUG_MODE         FALSE   // Debug overlays

// God mode (invincibility, infinite lives)
#define B_GAMECORNER_GOD_MODE           FALSE   // Invincibility

// Always win (for testing win conditions)
#define B_GAMECORNER_ALWAYS_WIN         FALSE   // Auto-win

// Skip countdown timers (instant start)
#define B_GAMECORNER_SKIP_COUNTDOWN     FALSE   // Skip 3-2-1-GO

// Free entry (ignore coin costs)
#define B_GAMECORNER_FREE_PLAY          FALSE   // No coin costs

// ========================================
// INTEGRATION SETTINGS
// ========================================

// Location for new minigames
// TRUE: Mossdeep Game Corner (default)
// FALSE: Add to Mauville Game Corner
#define B_GAMECORNER_USE_MOSSDEEP       TRUE    // Use new location

// Show minigames in Trainer Card stats
#define B_GAMECORNER_SHOW_IN_TRAINER_CARD TRUE  // Show stats

#endif // GUARD_CONFIG_GAME_CORNER_H
```

## Usage Patterns

### In Minigame Code

**Enable/Disable Feature:**

```c
// src/game_corner_snake.c

void CB2_SnakeMain(void)
{
    // Check if Snake is enabled
    if (!B_GAMECORNER_SNAKE)
    {
        ShowFieldMessage(gText_MinigameNotAvailable);
        SetMainCallback2(CB2_ReturnToField);
        return;
    }

    // Proceed with game
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}
```

**Difficulty Adjustment:**

```c
// src/game_corner_snake.c

void SnakeInitDifficulty(void)
{
    // Select difficulty based on config
    switch (B_SNAKE_DIFFICULTY)
    {
    case GEN_3:  // Easy
        gSnakeData->speed = 8;          // 8 frames per move (slow)
        gSnakeData->foodBonus = 20;     // 20 coins per food
        break;
    case GEN_4:  // Normal
        gSnakeData->speed = 6;          // 6 frames per move
        gSnakeData->foodBonus = 10;     // 10 coins per food
        break;
    case GEN_5:  // Hard
        gSnakeData->speed = 4;          // 4 frames per move (fast)
        gSnakeData->foodBonus = 5;      // 5 coins per food
        break;
    case GEN_LATEST:  // Very Hard
        gSnakeData->speed = 2;          // 2 frames per move (very fast)
        gSnakeData->foodBonus = 3;      // 3 coins per food
        break;
    }
}
```

**Entry Cost Check:**

```c
// src/game_corner_snake.c

bool8 SnakeCheckCanPlay(void)
{
    // Check coin case
    if (!CheckBagHasItem(ITEM_COIN_CASE, 1))
    {
        GameCorner_ShowNoCoinCaseMessage();
        return FALSE;
    }

    // Check entry cost (respects B_SNAKE_ENTRY_COST config)
    u16 entryCost = B_SNAKE_ENTRY_COST;

    // Override to 0 if free play enabled
    if (B_GAMECORNER_FREE_PLAY)
        entryCost = 0;

    if (!GameCorner_HasEnoughCoins(entryCost))
    {
        GameCorner_ShowInsufficientCoinsMessage(entryCost);
        return FALSE;
    }

    // Deduct coins
    if (entryCost > 0)
        GameCorner_TryTakeCoins(entryCost);

    return TRUE;
}
```

**High Score Handling:**

```c
// src/game_corner_snake.c

void SnakeGameOver(void)
{
    u16 finalScore = VarGet(VAR_MINIGAME_CURRENT_SCORE);

    // Only track high scores if enabled
    if (B_GAMECORNER_HIGH_SCORES)
    {
        if (GameCorner_IsNewHighScore(MINIGAME_SNAKE, finalScore))
        {
            GameCorner_UpdateHighScore(MINIGAME_SNAKE, finalScore);

            // Play fanfare if enabled
            if (B_GAMECORNER_HIGH_SCORE_FANFARE)
            {
                PlayFanfare(MUS_OBTAIN_ITEM);
                ShowHighScoreMessage(finalScore);
            }
        }
    }

    // Award coins
    u16 coinsWon = CalculateCoinsFromScore(finalScore);
    coinsWon = (coinsWon * B_GAMECORNER_COIN_MULTIPLIER) / 10;

    // Apply min/max caps
    if (coinsWon < B_GAMECORNER_MIN_COINS)
        coinsWon = B_GAMECORNER_MIN_COINS;
    if (coinsWon > B_GAMECORNER_MAX_COINS_PER_GAME)
        coinsWon = B_GAMECORNER_MAX_COINS_PER_GAME;

    GameCorner_AddCoins(coinsWon);
}
```

**Debug Features:**

```c
// src/game_corner_snake.c

void SnakeCheckCollision(void)
{
    // God mode disables collision
    if (B_GAMECORNER_GOD_MODE)
        return;  // No collision damage

    // Normal collision logic
    if (SnakeCollidesWithWall() || SnakeCollidesWithSelf())
    {
        SnakeDie();
    }
}

void SnakeDrawDebugInfo(void)
{
    if (!B_GAMECORNER_DEBUG_MODE)
        return;

    // Draw FPS counter
    if (B_GAMECORNER_SHOW_FPS)
        DrawFPSCounter();

    // Draw collision boxes
    DrawSnakeHitboxes();

    // Draw state info
    ConvertIntToDecimalStringN(gStringVar1, gSnakeData->state, STR_CONV_MODE_LEFT_ALIGN, 2);
    AddTextPrinterParameterized(0, FONT_SMALL, gStringVar1, 0, 0, TEXT_SKIP_DRAW, NULL);
}
```

### In Script Code

**Check If Minigame Enabled:**

```pory
// data/maps/MossdeepCity_GameCorner_B1F/scripts.pory

script MossdeepCity_GameCorner_B1F_EventScript_SnakeGame {
    lockall

    // Check if Snake is enabled via config
    compare(B_GAMECORNER_SNAKE, TRUE)
    goto_if_eq(MossdeepCity_GameCorner_EventScript_SnakeStart)

    // Snake disabled
    msgbox("This machine is out of\norder.", MSGBOX_DEFAULT)
    releaseall
    end
}

script MossdeepCity_GameCorner_EventScript_SnakeStart {
    // Check unlock status
    setvar(VAR_0x8004, MINIGAME_SNAKE)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    // Proceed with game...
}
```

## Config Validation

**Compile-Time Checks:**

Add to `include/config/game_corner.h`:

```c
// Validate configs at compile time
#if B_GAMECORNER_COIN_MULTIPLIER < 1
    #error "B_GAMECORNER_COIN_MULTIPLIER must be at least 1"
#endif

#if B_GAMECORNER_MAX_COINS_PER_GAME > 9999
    #error "B_GAMECORNER_MAX_COINS_PER_GAME cannot exceed coin system limit (9999)"
#endif

#if B_GAMECORNER_TARGET_FPS != 60
    #warning "B_GAMECORNER_TARGET_FPS should be 60 for GBA hardware"
#endif

// Ensure debug features are disabled in release builds
#ifndef DEBUG
    #if B_GAMECORNER_DEBUG_MODE == TRUE
        #error "B_GAMECORNER_DEBUG_MODE must be FALSE in release builds"
    #endif
    #if B_GAMECORNER_GOD_MODE == TRUE
        #error "B_GAMECORNER_GOD_MODE must be FALSE in release builds"
    #endif
    #if B_GAMECORNER_FREE_PLAY == TRUE
        #error "B_GAMECORNER_FREE_PLAY must be FALSE in release builds"
    #endif
#endif
```

## Testing Different Configs

### Test Cases

**Test 1: Disable Specific Minigame**
```c
#define B_GAMECORNER_SNAKE FALSE
```
- Expected: Snake machine shows "Out of order" message
- Expected: Snake high score NOT tracked in game stats
- Expected: Snake menu option hidden/grayed out

**Test 2: Free Play Mode**
```c
#define B_GAMECORNER_FREE_PLAY TRUE
```
- Expected: All minigames cost 0 coins
- Expected: Can play without Coin Case
- Expected: Coin rewards still awarded

**Test 3: Difficulty Changes**
```c
#define B_SNAKE_DIFFICULTY GEN_LATEST  // Very Hard
```
- Expected: Snake moves faster
- Expected: Coin rewards reduced
- Expected: High score achievement harder

**Test 4: Coin Multiplier**
```c
#define B_GAMECORNER_COIN_MULTIPLIER 20  // 2x rewards
```
- Expected: Score 100 gives 20 coins (instead of 10)
- Expected: Capped at B_GAMECORNER_MAX_COINS_PER_GAME

**Test 5: Unlock All**
```c
#define B_GAMECORNER_UNLOCK_ALL TRUE
```
- Expected: All minigames playable from start
- Expected: No unlock flags needed

## Integration with Existing RHH Configs

### Generation-Based Configs

Follow RHH's generation constant pattern:

```c
// include/constants/config.h (existing)
#define GEN_3  0
#define GEN_4  1
#define GEN_5  2
#define GEN_6  3
#define GEN_7  4
#define GEN_8  5
#define GEN_9  6
#define GEN_LATEST GEN_9
```

**Usage:**

```c
#define B_GAMECORNER_DIFFICULTY GEN_4  // Normal difficulty
```

**Benefit:** Familiar pattern for RHH users, semantic clarity.

### Boolean Configs

```c
#define TRUE  1
#define FALSE 0
```

**Usage:**

```c
#define B_GAMECORNER_SNAKE TRUE
#define B_GAMECORNER_TUTORIALS FALSE
```

### Numeric Configs

```c
#define B_GAMECORNER_COIN_MULTIPLIER 10  // Integer (10 = 100%)
#define B_SNAKE_ENTRY_COST 10            // Coins
```

## Config Documentation

Add to `/docs/CONFIG.md` (or create if doesn't exist):

```markdown
## Game Corner Configs

### Minigame Toggles

- `B_GAMECORNER_SNAKE` - Enable/disable Snake minigame
- `B_GAMECORNER_FLAPPYBIRD` - Enable/disable Flappy Bird
- ... (etc for all 9 games)

### Difficulty

- `B_GAMECORNER_DIFFICULTY` - Global difficulty (GEN_3 to GEN_LATEST)
- `B_SNAKE_DIFFICULTY` - Snake-specific difficulty override

### Coin System

- `B_GAMECORNER_COIN_MULTIPLIER` - Multiplier for coin rewards (10 = 1.0x)
- `B_GAMECORNER_MIN_COINS` - Minimum coins per game
- `B_GAMECORNER_MAX_COINS_PER_GAME` - Maximum coins per session

### Entry Costs

- `B_SNAKE_ENTRY_COST` - Coins to play Snake (default: 10)
- `B_FLAPPYBIRD_ENTRY_COST` - Coins to play Flappy Bird (default: 20)
- ... (etc)

### Debug Settings (WARNING: Disable in release!)

- `B_GAMECORNER_DEBUG_MODE` - Show debug overlays
- `B_GAMECORNER_GOD_MODE` - Invincibility
- `B_GAMECORNER_FREE_PLAY` - Ignore coin costs
```

## Default Config Recommendations

**For Release Builds:**

```c
// All minigames enabled
#define B_GAMECORNER_SNAKE              TRUE
#define B_GAMECORNER_FLAPPYBIRD         TRUE
// ... (all TRUE)

// Normal difficulty
#define B_GAMECORNER_DIFFICULTY         GEN_4

// Standard rewards
#define B_GAMECORNER_COIN_MULTIPLIER    10  // 1.0x

// Moderate entry costs
#define B_SNAKE_ENTRY_COST              10
#define B_FLAPPYBIRD_ENTRY_COST         20
// ... (balanced costs)

// All debug features OFF
#define B_GAMECORNER_DEBUG_MODE         FALSE
#define B_GAMECORNER_GOD_MODE           FALSE
#define B_GAMECORNER_FREE_PLAY          FALSE
```

**For Testing Builds:**

```c
// All minigames enabled
#define B_GAMECORNER_SNAKE              TRUE
// ... (all TRUE)

// Easy difficulty for rapid testing
#define B_GAMECORNER_DIFFICULTY         GEN_3

// Free play for testing
#define B_GAMECORNER_FREE_PLAY          TRUE

// Unlock all from start
#define B_GAMECORNER_UNLOCK_ALL         TRUE

// Debug features ON
#define B_GAMECORNER_DEBUG_MODE         TRUE
#define B_GAMECORNER_SHOW_FPS           TRUE
```

## Documentation Cross-References

**Related Documents:**
- `01_ARCHITECTURE_COMPARISON.md` - pret preprocessor vs RHH runtime checks
- `03_COMMON_INFRASTRUCTURE.md` - How configs are used in helper functions
- `04_SAVE_STRATEGY.md` - High score persistence (affected by B_GAMECORNER_HIGH_SCORES)
- `06_TESTING_APPROACH.md` - Testing different config combinations

**RHH Config Files to Reference:**
- `include/config/battle.h` - Battle system config patterns
- `include/config/item.h` - Item expansion config patterns
- `include/config/pokemon.h` - Pokemon feature toggles

## Implementation Checklist

- [ ] Create `include/config/game_corner.h`
- [ ] Add `#include "config/game_corner.h"` to `include/config.h`
- [ ] Add compile-time validation checks
- [ ] Document all configs in `/docs/CONFIG.md`
- [ ] Implement config checks in all 9 minigames
- [ ] Test with all minigames disabled (should compile)
- [ ] Test with all minigames enabled
- [ ] Test debug mode builds vs release builds
- [ ] Test difficulty variations (GEN_3 through GEN_LATEST)
- [ ] Test coin multiplier edge cases (0, 1, 100)
- [ ] Test free play mode
- [ ] Verify compiler optimizations (disabled code eliminated)
- [ ] Code review for consistent config usage

---

**Last Updated:** 2025-12-16
**Status:** Design Complete - Ready for Implementation
**Pattern:** Follows RHH's established runtime config system
