# Code Style Migration Guide: pret → RHH

## Purpose

This document provides practical before/after examples for converting heyopc's pret-based code to RHH pokeemerald-expansion style. Use this as a reference during porting.

**Reference:** Based on `/home/chris/Documents/Github/pokeemerald/docs/STYLEGUIDE.md`

## Quick Reference Table

| Category | pret Style | RHH Style |
|----------|------------|-----------|
| Functions | `snake_init()` | `SnakeInit()` |
| Variables | `current_score` | `currentScore` |
| Globals | `score` | `gSnakeScore` |
| Statics | `static int counter` | `static u32 sCounter` |
| Constants | `MAX_LENGTH` | `MAX_SNAKE_LENGTH` |
| Loop iterator | `int i; for(i=0; ...)` | `for (u32 i = 0; ...)` |
| Config check | `#ifdef FEATURE` | `if (B_FEATURE)` |
| Spacing | Varies | 4 spaces (no tabs) |
| Braces | `if (x) {` | `if (x)\n{` |

## 1. Function Naming

### Rule: PascalCase for all functions

**Before (pret):**
```c
void snake_init(void)
{
    // ...
}

void update_snake_position(void)
{
    // ...
}

bool check_collision(void)
{
    // ...
}

static void draw_snake(void)
{
    // ...
}
```

**After (RHH):**
```c
void SnakeInit(void)
{
    // ...
}

void UpdateSnakePosition(void)
{
    // ...
}

bool8 CheckCollision(void)  // Note: bool → bool8
{
    // ...
}

static void DrawSnake(void)  // Still PascalCase even if static
{
    // ...
}
```

### Common Patterns

| pret | RHH |
|------|-----|
| `init_game()` | `InitGame()` |
| `start_minigame()` | `StartMinigame()` |
| `get_high_score()` | `GetHighScore()` |
| `set_difficulty()` | `SetDifficulty()` |
| `handle_input()` | `HandleInput()` |
| `update_graphics()` | `UpdateGraphics()` |
| `play_sound_effect()` | `PlaySoundEffect()` |

## 2. Variable Naming

### Rule: camelCase for variables and struct fields

**Before (pret):**
```c
int current_score = 0;
int high_score = 100;
bool game_over = false;
int snake_length = 1;
int food_x, food_y;
```

**After (RHH):**
```c
u32 currentScore = 0;          // Also: int → u32
u32 highScore = 100;
bool8 isGameOver = FALSE;      // Also: bool → bool8, false → FALSE
u32 snakeLength = 1;
s16 foodX, foodY;              // Coordinate variables often s16
```

### Struct Fields

**Before (pret):**
```c
struct SnakeGame {
    int score;
    int lives;
    bool is_paused;
    int current_level;
};
```

**After (RHH):**
```c
struct SnakeGame {
    u32 score;          // camelCase maintained
    u8 lives;           // Use smallest type (u8 sufficient for lives)
    bool8 isPaused;     // bool → bool8, snake_case → camelCase
    u8 currentLevel;    // camelCase
};
```

### Boolean Naming

**Prefer "is/has/can" prefixes for booleans:**

| pret | RHH (preferred) |
|------|-----------------|
| `bool paused` | `bool8 isPaused` |
| `bool active` | `bool8 isActive` |
| `bool won` | `bool8 hasWon` |
| `bool visible` | `bool8 isVisible` |
| `bool loaded` | `bool8 isLoaded` |

## 3. Global and Static Prefixes

### Rule: g for globals, s for statics

**Before (pret):**
```c
// Global variables (in .c file, external linkage)
int highScore = 0;
bool musicEnabled = true;
struct SnakeGame *gameState = NULL;

// Static variables (file-local)
static int frameCount = 0;
static bool initialized = false;
```

**After (RHH):**
```c
// Global variables - add 'g' prefix
u32 gSnakeHighScore = 0;
bool8 gMusicEnabled = TRUE;
struct SnakeGame *gSnakeGameState = NULL;

// Static variables - add 's' prefix
static u32 sFrameCount = 0;
static bool8 sInitialized = FALSE;
```

### Pattern for Module-Scoped Statics

**Before (pret):**
```c
// In snake.c
static struct SnakeGame game;
static int level;
static bool paused;
```

**After (RHH):**
```c
// In game_corner_snake.c
static struct SnakeGame sSnakeGame;
static u8 sCurrentLevel;
static bool8 sIsPaused;
```

**Rationale:** The 's' prefix makes it immediately clear the variable is file-static, not a local variable or global.

## 4. Constants and Macros

### Rule: CAPS_WITH_UNDERSCORES

**Before (pret - likely already correct):**
```c
#define MAX_LENGTH 100
#define GRID_SIZE 20
#define MOVE_SPEED 4
```

**After (RHH - usually same, but add context):**
```c
#define MAX_SNAKE_LENGTH 100      // Add context to constant names
#define SNAKE_GRID_SIZE 20
#define SNAKE_MOVE_SPEED 4

// OR use enums for related constants
enum SnakeConstants {
    MAX_SNAKE_LENGTH = 100,
    SNAKE_GRID_SIZE = 20,
    SNAKE_MOVE_SPEED = 4
};
```

### Enums (Sequential Constants)

**Before (pret):**
```c
#define DIR_UP 0
#define DIR_DOWN 1
#define DIR_LEFT 2
#define DIR_RIGHT 3
```

**After (RHH - use enum):**
```c
enum SnakeDirection {
    SNAKE_DIR_UP,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_RIGHT,
    SNAKE_DIR_COUNT  // Useful for array sizing
};
```

**Then use enum type in function signatures:**
```c
// Before (pret)
void SetDirection(int dir);

// After (RHH)
void SetSnakeDirection(enum SnakeDirection dir);
```

**Why?** Type safety - compiler catches invalid values.

## 5. Loop Iterator Declaration

### Rule: Declare iterator in loop

**Before (pret):**
```c
void UpdateSnake(void)
{
    int i;
    int j;

    for (i = 0; i < snakeLength; i++)
    {
        // Update segment i
    }

    for (j = 0; j < GRID_SIZE; j++)
    {
        // Process grid j
    }
}
```

**After (RHH):**
```c
void UpdateSnake(void)
{
    for (u32 i = 0; i < sSnakeLength; i++)
    {
        // Update segment i
    }

    for (u32 j = 0; j < SNAKE_GRID_SIZE; j++)
    {
        // Process grid j
    }
}
```

**Benefits:**
- Tighter scope (iterator not accessible outside loop)
- Clearer intent
- Matches modern C standards

**Exception:** If iterator needs to persist after loop:
```c
u32 i;
for (i = 0; i < count; i++)
{
    if (SomeCondition())
        break;
}
// Use i here - valid because declared outside
```

## 6. Data Types

### Rule: Use GBA-specific types

**Before (pret):**
```c
int score;
unsigned int coins;
short x, y;
unsigned char color;
char *message;
bool active;
```

**After (RHH):**
```c
s32 score;           // int → s32 (signed 32-bit)
u32 coins;           // unsigned int → u32
s16 x, y;            // short → s16 (signed 16-bit)
u8 color;            // unsigned char → u8
const u8 *message;   // char* → const u8* (strings are u8 arrays)
bool8 isActive;      // bool → bool8 (GBA-compatible boolean)
```

### Type Selection Guidelines

| Size Needed | Signed | Unsigned |
|-------------|--------|----------|
| 8-bit | `s8` | `u8` |
| 16-bit | `s16` | `u16` |
| 32-bit | `s32` | `u32` |
| Boolean | - | `bool8` |
| Pointer | - | `void*`, `u8*`, etc. |

**Default to u32/s32 unless:**
- SaveBlock field (use smallest type)
- EWRAM variable (use smallest type)
- Global variable (use smallest type)
- Coordinates (typically s16)
- Counters 0-255 (u8)
- Flags/booleans (bool8 or u8 for bitfields)

## 7. Preprocessor to Runtime Checks

### Rule: Use runtime checks for configs

**Before (pret):**
```c
void StartGame(void)
{
    #ifdef HARD_MODE
        SetDifficulty(DIFFICULTY_HARD);
    #else
        SetDifficulty(DIFFICULTY_NORMAL);
    #endif

    #ifdef ENABLE_MUSIC
        PlayBGM(MUS_GAME_CORNER);
    #endif
}
```

**After (RHH):**
```c
void StartGame(void)
{
    if (B_GAMECORNER_HARD_MODE)
        SetDifficulty(DIFFICULTY_HARD);
    else
        SetDifficulty(DIFFICULTY_NORMAL);

    if (B_GAMECORNER_ENABLE_MUSIC)
        PlayBGM(MUS_GAME_CORNER);
}
```

**Why?**
- Both code paths remain compiled (catches bugs in disabled paths)
- Compiler optimizes away unreachable code
- More flexible (can change at runtime if config is a variable)

**Exception:** Use preprocessor only when:
- Struct layout changes based on config
- Including/excluding entire functions
- Major feature differences

```c
// Preprocessor OK for struct changes
struct SnakeGame {
    u32 score;
    #if B_GAMECORNER_HIGH_SCORES
    u32 highScore;  // Field only exists if config enabled
    #endif
};
```

## 8. Spacing and Indentation

### Rule: 4 spaces, NO TABS (for C files)

**Before (pret - may use tabs):**
```c
void UpdateGame(void)
{
→   if (isGameActive)
→   {
→   →   ProcessInput();
→   →   UpdatePhysics();
→   }
}
```

**After (RHH - 4 spaces):**
```c
void UpdateGame(void)
{
    if (isGameActive)
    {
        ProcessInput();
        UpdatePhysics();
    }
}
```

**Note:** Assembly (.s) and scripts (.inc) still use TABS.

### Empty Lines

**Rule: One empty line after blocks**

**Before (pret - may be inconsistent):**
```c
void MyFunction(void)
{
    DoSetup();
    if (condition)
    {
        DoThing();
    }
    return;
}
```

**After (RHH - consistent spacing):**
```c
void MyFunction(void)
{
    DoSetup();

    if (condition)
    {
        DoThing();
    }

    return;  // Empty line before return
}
```

## 9. Control Structure Formatting

### Rule: Opening brace on next line

**Before (pret - K&R style):**
```c
if (condition) {
    DoThing();
}

for (int i = 0; i < 10; i++) {
    Process(i);
}

while (running) {
    Update();
}
```

**After (RHH - Allman style):**
```c
if (condition)
{
    DoThing();
}

for (u32 i = 0; i < 10; i++)
{
    Process(i);
}

while (isRunning)
{
    Update();
}
```

### Switch Statement Alignment

**Rule: Cases align with switch**

**Before (pret - may be indented):**
```c
switch (state)
{
    case STATE_INIT:
        Initialize();
        break;
    case STATE_PLAY:
        Update();
        break;
    default:
        HandleError();
        break;
}
```

**After (RHH - cases at same level):**
```c
switch (state)
{
case STATE_INIT:
    Initialize();
    break;
case STATE_PLAY:
    Update();
    break;
default:
    HandleError();
    break;
}
```

### Single-Line Conditionals

**Rule: Omit braces only if block is one line**

**Before (pret):**
```c
if (x > 0)
    Increment();
else {
    Decrement();
    Log();
}
```

**After (RHH - consistent brace usage):**
```c
if (x > 0)
{
    Increment();
}
else
{
    Decrement();
    Log();
}
```

**OR if all blocks are single-line:**
```c
if (x > 0)
    Increment();
else if (x < 0)
    Decrement();
else
    Reset();
```

## 10. Comments

### Single-Line Comments

**Rule: Space after //**

**Before (pret - may be inconsistent):**
```c
//Initialize game state
int score = 0;//starting score
```

**After (RHH - consistent spacing):**
```c
// Initialize game state
u32 score = 0; // Starting score
```

**Capitalization and punctuation:**
```c
// Correct: Capitalized, ends with period.
// incorrect: lowercase, no period
```

### Block Comments

**Rule: Use for multi-line explanations**

**Before (pret):**
```c
// This function handles the snake movement logic
// It checks for collisions with walls and itself
// Returns true if game should continue
bool UpdateSnake(void) { ... }
```

**After (RHH):**
```c
/*
 * This function handles the snake movement logic.
 * It checks for collisions with walls and itself.
 * Returns TRUE if game should continue.
 */
bool8 UpdateSnake(void) { ... }
```

### Inline Data Comments

**Rule: Comment on same line for data arrays**

**Before (pret):**
```c
const u8 difficultySettings[] = {
    1,
    2,
    3,
    4
};
```

**After (RHH):**
```c
const u8 sDifficultySettings[] = {
    1, // Easy
    2, // Normal
    3, // Hard
    4, // Expert
};
```

## 11. Complete Example: Before and After

### Before (pret style):
```c
#include <stdio.h>
#include <stdbool.h>

#define MAX_LEN 100
#define DIR_UP 0
#define DIR_DOWN 1

// Global state
int score = 0;
bool game_running = false;

// File-local state
static int snake_length = 1;
static int direction = DIR_UP;

void init_game(void) {
    int i;
    score = 0;
    snake_length = 1;
    direction = DIR_UP;
    game_running = true;

    #ifdef ENABLE_SOUND
    play_music();
    #endif

    for(i = 0; i < MAX_LEN; i++) {
        reset_cell(i);
    }
}

bool update_game(void) {
    if(!game_running) {
        return false;
    }

    move_snake();

    if(check_collision()) {
        game_running = false;
        return false;
    }
    return true;
}

void set_direction(int dir) {
    if(dir >= DIR_UP && dir <= DIR_DOWN) {
        direction = dir;
    }
}
```

### After (RHH style):
```c
#include "global.h"
#include "game_corner_snake.h"
#include "coins.h"
#include "sound.h"

#define MAX_SNAKE_LENGTH 100

enum SnakeDirection {
    SNAKE_DIR_UP,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_RIGHT
};

// Global state
u32 gSnakeScore = 0;
bool8 gSnakeGameRunning = FALSE;

// File-local state
static u32 sSnakeLength = 1;
static enum SnakeDirection sDirection = SNAKE_DIR_UP;

void InitSnakeGame(void)
{
    gSnakeScore = 0;
    sSnakeLength = 1;
    sDirection = SNAKE_DIR_UP;
    gSnakeGameRunning = TRUE;

    if (B_GAMECORNER_ENABLE_SOUND)
        PlayBGM(MUS_GAME_CORNER);

    for (u32 i = 0; i < MAX_SNAKE_LENGTH; i++)
    {
        ResetCell(i);
    }
}

bool8 UpdateSnakeGame(void)
{
    if (!gSnakeGameRunning)
        return FALSE;

    MoveSnake();

    if (CheckSnakeCollision())
    {
        gSnakeGameRunning = FALSE;
        return FALSE;
    }

    return TRUE;
}

void SetSnakeDirection(enum SnakeDirection dir)
{
    if (dir >= SNAKE_DIR_UP && dir <= SNAKE_DIR_RIGHT)
        sDirection = dir;
}
```

## 12. Automated Tools and Workflow

### Search and Replace Patterns

**1. Function names (use regex in editor):**
```regex
Find:    ([a-z]+)_([a-z]+)_([a-z]+)\(
Replace: \u$1\u$2\u$3(

Example: snake_init_game() → SnakeInitGame()
```

**2. Variables (more manual, context-dependent):**
```regex
Find:    ([a-z]+)_([a-z]+)
Replace: $1\u$2

Example: current_score → currentScore
```

**3. Add global prefix:**
```regex
Find:    ^(extern |)([su]\d+|bool8) ([a-z])
Replace: $1$2 g\u$3

Example: u32 score → u32 gScore
```

**4. Add static prefix:**
```regex
Find:    ^static ([su]\d+|bool8) ([a-z])
Replace: static $1 s\u$2

Example: static u32 counter → static u32 sCounter
```

### Manual Review Checklist

After automated transformations:

- [ ] All function names PascalCase
- [ ] All variables camelCase
- [ ] Globals have g prefix
- [ ] Statics have s prefix
- [ ] Loop iterators declared in loop
- [ ] `#ifdef` converted to `if (B_CONFIG)`
- [ ] `int` → `s32/u32` as appropriate
- [ ] `bool` → `bool8`
- [ ] TRUE/FALSE (not true/false)
- [ ] 4 spaces throughout (no tabs)
- [ ] Braces on next line
- [ ] Empty lines after blocks
- [ ] Comments have space after //
- [ ] Switch cases align with switch
- [ ] Enum types in function signatures

## 13. Common Gotchas

### Boolean Constants

**Before (pret):**
```c
bool active = true;
if (active == false) { ... }
```

**After (RHH):**
```c
bool8 isActive = TRUE;   // TRUE not true
if (isActive == FALSE) { ... }  // FALSE not false
// Or better:
if (!isActive) { ... }
```

### NULL vs 0

**Before (pret - may use 0):**
```c
void *ptr = 0;
if (ptr == 0) { ... }
```

**After (RHH - use NULL for pointers):**
```c
void *ptr = NULL;
if (ptr == NULL) { ... }
// Or:
if (ptr != NULL) { ... }
```

### Implicit Comparisons

**Rule: Explicit comparisons for clarity (except booleans)**

**Before (pret):**
```c
if (count) { ... }      // Implicit != 0
if (!ptr) { ... }       // Implicit == NULL
```

**After (RHH - be explicit for non-booleans):**
```c
if (count != 0) { ... }
if (ptr == NULL) { ... }

// But for booleans, implicit is OK:
if (isActive) { ... }
if (!isGameOver) { ... }
```

### Array Initialization

**Before (pret):**
```c
int values[10] = {0};
```

**After (RHH - designated initializers OK):**
```c
u32 values[10] = {[0] = 0}; // Explicitly zero first element
// Or just:
u32 values[10] = {0}; // Also works
```

## 14. Reference Files in RHH Codebase

**Style guide:**
- `/home/chris/Documents/Github/pokeemerald/docs/STYLEGUIDE.md`

**Good examples to follow:**
- `/home/chris/Documents/Github/pokeemerald/src/slot_machine.c`
- `/home/chris/Documents/Github/pokeemerald/src/roulette.c`
- `/home/chris/Documents/Github/pokeemerald/src/pokemon.c`

**Test style examples:**
- `/home/chris/Documents/Github/pokeemerald/test/battle/move_effect/poison_hit.c`

---

**Document Version:** 1.0
**Last Updated:** 2025-12-16
**Purpose:** Practical guide for pret → RHH code conversion
