# Testing Approach & Quality Assurance

## Purpose

This document defines the comprehensive testing strategy for Game Corner minigames, ensuring quality, performance, and compatibility with RHH's testing framework.

## RHH Testing Framework Overview

**Location:** `test/` directory

**Framework:** Custom DSL built on `test/main.c`

**Pattern:** GIVEN-WHEN-THEN style (similar to Gherkin)

**Example from Battle Tests:**

```c
SINGLE_BATTLE_TEST("Rough Skin inflicts 1/8 damage on contact")
{
    s16 damage;
    GIVEN {
        ASSUME(gMovesInfo[MOVE_TACKLE].makesContact);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_ZIGZAGOON) { Ability(ABILITY_ROUGH_SKIN); }
    } WHEN {
        TURN { MOVE(player, MOVE_TACKLE); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_TACKLE, player);
        HP_BAR(opponent, captureDamage: &damage);
        ABILITY_POPUP(opponent, ABILITY_ROUGH_SKIN);
        HP_BAR(player, damage: damage / 8);
    }
}
```

**Key Features:**
- Declarative test syntax
- Automatic battle simulation
- Heap corruption detection
- RNG determinism for reproducibility
- Parallel test execution

## Testing Layers

### Layer 1: Unit Tests (Isolated Logic)

Test individual functions in isolation.

**File Pattern:** `test/game_corner_<minigame>.c`

**Example - Snake Tests:**

```c
// test/game_corner_snake.c

TEST("Snake initializes with correct defaults")
{
    // Setup
    SnakeInit();

    // Verify
    ASSERT_EQ(3, gSnakeData->length);
    ASSERT_EQ(DIRECTION_RIGHT, gSnakeData->direction);
    ASSERT_EQ(0, gSnakeData->score);
    ASSERT_EQ(STATE_COUNTDOWN, gSnakeData->state);
}

TEST("Snake grows when eating food")
{
    // Setup
    SnakeInit();
    u8 initialLength = gSnakeData->length;

    // Execute
    SnakeEatFood();

    // Verify
    ASSERT_EQ(initialLength + 1, gSnakeData->length);
    ASSERT_EQ(10, gSnakeData->score);  // +10 points per food
}

TEST("Snake dies on wall collision")
{
    // Setup
    SnakeInit();
    gSnakeData->head.x = 0;  // At left wall
    gSnakeData->direction = DIRECTION_LEFT;

    // Execute
    SnakeUpdate();

    // Verify
    ASSERT_EQ(STATE_GAMEOVER, gSnakeData->state);
}

TEST("Snake wraps around screen borders (if enabled)")
{
    // Setup
    SnakeInit();
    gSnakeData->wrapEnabled = TRUE;
    gSnakeData->head.x = 0;
    gSnakeData->direction = DIRECTION_LEFT;

    // Execute
    SnakeUpdate();

    // Verify
    ASSERT_EQ(GRID_WIDTH - 1, gSnakeData->head.x);  // Wrapped to right edge
    ASSERT_NE(STATE_GAMEOVER, gSnakeData->state);   // Not dead
}

TEST("Snake cannot reverse direction")
{
    // Setup
    SnakeInit();
    gSnakeData->direction = DIRECTION_RIGHT;

    // Attempt to reverse (should be ignored)
    SnakeChangeDirection(DIRECTION_LEFT);

    // Verify
    ASSERT_EQ(DIRECTION_RIGHT, gSnakeData->direction);  // Unchanged
}
```

**Example - Common Infrastructure Tests:**

```c
// test/game_corner_common.c

TEST("Coin system adds coins correctly")
{
    // Setup
    SetCoins(100);

    // Execute
    GameCorner_AddCoins(50);

    // Verify
    ASSERT_EQ(150, GetCoins());
}

TEST("Coin system caps at MAX_COINS")
{
    // Setup
    SetCoins(9990);

    // Execute
    GameCorner_AddCoins(100);  // Would exceed 9999

    // Verify
    ASSERT_EQ(MAX_COINS, GetCoins());  // Capped at 9999
}

TEST("High score updates only when beaten")
{
    // Setup
    SetGameStat(GAME_STAT_SNAKE_HIGH_SCORE, 100);

    // Execute - score below high score
    GameCorner_UpdateHighScore(MINIGAME_SNAKE, 50);

    // Verify
    ASSERT_EQ(100, GameCorner_GetHighScore(MINIGAME_SNAKE));  // Unchanged

    // Execute - score above high score
    GameCorner_UpdateHighScore(MINIGAME_SNAKE, 150);

    // Verify
    ASSERT_EQ(150, GameCorner_GetHighScore(MINIGAME_SNAKE));  // Updated
}

TEST("Unlock flags persist correctly")
{
    // Setup
    FlagClear(FLAG_UNLOCKED_GAMECORNER_SNAKE);

    // Verify initial state
    ASSERT_FALSE(GameCorner_IsUnlocked(MINIGAME_SNAKE));

    // Execute
    GameCorner_Unlock(MINIGAME_SNAKE);

    // Verify
    ASSERT_TRUE(GameCorner_IsUnlocked(MINIGAME_SNAKE));
    ASSERT_TRUE(FlagGet(FLAG_UNLOCKED_GAMECORNER_SNAKE));
}
```

### Layer 2: Integration Tests (System Interactions)

Test interactions between components.

**Example - Coin Integration:**

```c
TEST("Playing Snake deducts entry cost")
{
    // Setup
    SetCoins(100);
    GameCorner_Unlock(MINIGAME_SNAKE);

    // Execute - start game (costs 10 coins)
    Special_StartMinigame();
    VarSet(VAR_0x8004, MINIGAME_SNAKE);

    // Verify
    ASSERT_EQ(90, GetCoins());  // 100 - 10
}

TEST("Game over awards coins based on score")
{
    // Setup
    SetCoins(0);
    VarSet(VAR_MINIGAME_CURRENT_SCORE, 100);

    // Execute
    SnakeGameOver();

    // Verify
    u16 expectedCoins = (100 * B_GAMECORNER_COIN_MULTIPLIER) / 10;
    ASSERT_EQ(expectedCoins, GetCoins());
}

TEST("High score persists across game sessions")
{
    // Session 1
    SnakeInit();
    VarSet(VAR_MINIGAME_CURRENT_SCORE, 100);
    SnakeGameOver();
    ASSERT_EQ(100, GameCorner_GetHighScore(MINIGAME_SNAKE));

    // Session 2
    SnakeInit();
    VarSet(VAR_MINIGAME_CURRENT_SCORE, 50);  // Lower score
    SnakeGameOver();
    ASSERT_EQ(100, GameCorner_GetHighScore(MINIGAME_SNAKE));  // Unchanged

    // Session 3
    SnakeInit();
    VarSet(VAR_MINIGAME_CURRENT_SCORE, 150);  // Higher score
    SnakeGameOver();
    ASSERT_EQ(150, GameCorner_GetHighScore(MINIGAME_SNAKE));  // Updated
}
```

**Example - Save Persistence:**

```c
TEST("High scores persist across save/load")
{
    // Setup
    SetGameStat(GAME_STAT_SNAKE_HIGH_SCORE, 0);
    GameCorner_UpdateHighScore(MINIGAME_SNAKE, 100);

    // Simulate save
    TrySavingData(SAVE_NORMAL);

    // Corrupt memory (simulate power loss)
    memset(gSaveBlock1Ptr, 0xFF, sizeof(struct SaveBlock1));

    // Load save
    LoadSaveblockData();

    // Verify persistence
    ASSERT_EQ(100, GameCorner_GetHighScore(MINIGAME_SNAKE));
}

TEST("Unlock flags persist across save/load")
{
    // Setup
    FlagClear(FLAG_UNLOCKED_GAMECORNER_SNAKE);
    GameCorner_Unlock(MINIGAME_SNAKE);

    // Save
    TrySavingData(SAVE_NORMAL);

    // Corrupt
    FlagClear(FLAG_UNLOCKED_GAMECORNER_SNAKE);

    // Load
    LoadSaveblockData();

    // Verify
    ASSERT_TRUE(GameCorner_IsUnlocked(MINIGAME_SNAKE));
}
```

### Layer 3: Performance Tests

Verify 60 FPS stability and no memory leaks.

**Example - Memory Leak Detection:**

```c
TEST("Snake has no memory leaks over 100 sessions")
{
    // Baseline memory
    void *baselineHeap = GetHeapBase();
    u32 baselineSize = GetHeapSize();

    // Run 100 complete game sessions
    for (u32 i = 0; i < 100; i++)
    {
        SnakeInit();

        // Simulate gameplay
        for (u32 frame = 0; frame < 3600; frame++)  // 60 seconds at 60 FPS
        {
            SnakeUpdate();
        }

        // Game over and cleanup
        SnakeGameOver();
        SnakeFree();
    }

    // Verify no heap growth
    ASSERT_EQ(baselineHeap, GetHeapBase());
    ASSERT_EQ(baselineSize, GetHeapSize());
}
```

**Example - Frame Budget:**

```c
TEST("Snake maintains 60 FPS under max load")
{
    // Setup worst-case scenario
    SnakeInit();
    gSnakeData->length = MAX_SNAKE_LENGTH;  // Maximum segments
    gSnakeData->particleCount = MAX_PARTICLES;  // Maximum particles

    // Measure frame time
    u32 startCycles = GetCpuCycleCount();

    // Execute one full frame
    SnakeUpdate();
    SnakeRender();

    u32 endCycles = GetCpuCycleCount();
    u32 frameCycles = endCycles - startCycles;

    // Verify within budget (16.67ms @ 16.78 MHz = ~280,300 cycles)
    ASSERT_LT(frameCycles, 280300);  // Must complete within one frame
}
```

### Layer 4: Config Tests

Verify all config combinations work correctly.

**Example - Config Variations:**

```c
TEST("Snake respects difficulty config")
{
    // GEN_3 (Easy)
    #undef B_SNAKE_DIFFICULTY
    #define B_SNAKE_DIFFICULTY GEN_3
    SnakeInitDifficulty();
    ASSERT_EQ(8, gSnakeData->speed);

    // GEN_4 (Normal)
    #undef B_SNAKE_DIFFICULTY
    #define B_SNAKE_DIFFICULTY GEN_4
    SnakeInitDifficulty();
    ASSERT_EQ(6, gSnakeData->speed);

    // GEN_LATEST (Very Hard)
    #undef B_SNAKE_DIFFICULTY
    #define B_SNAKE_DIFFICULTY GEN_LATEST
    SnakeInitDifficulty();
    ASSERT_EQ(2, gSnakeData->speed);
}

TEST("Disabled minigames are not accessible")
{
    // Disable Snake
    #undef B_GAMECORNER_SNAKE
    #define B_GAMECORNER_SNAKE FALSE

    // Attempt to start
    VarSet(VAR_0x8004, MINIGAME_SNAKE);
    Special_StartMinigame();

    // Verify rejection
    ASSERT_EQ(STATE_IDLE, gSnakeData->state);  // Never started
}

TEST("Free play mode ignores coin costs")
{
    // Enable free play
    #undef B_GAMECORNER_FREE_PLAY
    #define B_GAMECORNER_FREE_PLAY TRUE

    // Setup with 0 coins
    SetCoins(0);

    // Start game (normally costs 10 coins)
    ASSERT_TRUE(SnakeCheckCanPlay());  // Should succeed

    // Verify coins unchanged
    ASSERT_EQ(0, GetCoins());
}
```

### Layer 5: Manual Testing

**Gameplay Testing Checklist (per minigame):**

```markdown
## Snake Manual Testing

### Core Gameplay
- [ ] Snake moves in all 4 directions
- [ ] Snake grows when eating food
- [ ] Food spawns in random valid positions
- [ ] Score increments correctly (+10 per food)
- [ ] Game speed increases as snake grows

### Collisions
- [ ] Snake dies on wall collision
- [ ] Snake dies on self-collision
- [ ] Food does not spawn on snake body
- [ ] Food does not spawn on walls

### Controls
- [ ] D-Pad changes direction
- [ ] Cannot reverse 180° (e.g., right → left while moving right)
- [ ] B button quits to map
- [ ] START button pauses game

### UI
- [ ] Score displays correctly
- [ ] High score displays correctly
- [ ] "NEW RECORD!" message shows when appropriate
- [ ] Countdown timer (3-2-1-GO) works
- [ ] Game over message shows final score

### Audio
- [ ] BGM plays during gameplay
- [ ] SFX plays when eating food
- [ ] SFX plays on death
- [ ] Music stops on game over

### Coin System
- [ ] Entry cost deducted (10 coins)
- [ ] Insufficient coins message shows if < 10 coins
- [ ] Coins awarded at game over (score / 10)
- [ ] Coins capped at 9999

### Save Integration
- [ ] High score persists after save/load
- [ ] Play count increments
- [ ] Unlock flag persists

### Performance
- [ ] 60 FPS throughout gameplay
- [ ] No slowdown with max snake length
- [ ] No memory leaks (play 10+ times)
- [ ] No graphical glitches

### Edge Cases
- [ ] Game handles 0 coins gracefully
- [ ] Game handles no COIN_CASE gracefully
- [ ] Game handles locked state gracefully
- [ ] Score cap at 999,999 works correctly
```

## Automated Test Execution

### Running Tests

```bash
# Run all tests
make test

# Run specific test file
make test/game_corner_snake.o

# Run tests with verbose output
make test VERBOSE=1

# Run tests with heap debugging
make test DEBUG_HEAP=1
```

### CI/CD Integration

**GitHub Actions Workflow (example):**

```yaml
# .github/workflows/test.yml

name: Game Corner Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-arm-none-eabi
      - name: Run tests
        run: make test
      - name: Check for memory leaks
        run: make test DEBUG_HEAP=1
```

## Test Coverage Goals

**Target Coverage:**
- Unit tests: 80%+ code coverage
- Integration tests: All critical paths
- Manual tests: 100% gameplay features

**Per-Minigame Coverage:**
- [ ] All state transitions tested
- [ ] All collision scenarios tested
- [ ] All scoring logic tested
- [ ] All config variations tested
- [ ] All error conditions tested
- [ ] Memory leak test (100+ sessions)
- [ ] Performance test (60 FPS worst-case)

## Regression Testing

**When to Run Full Regression:**
- Before committing any code
- Before creating a pull request
- After modifying common infrastructure
- After changing config system

**Regression Test Suite:**
1. All unit tests pass
2. All integration tests pass
3. All performance tests pass
4. Manual smoke test (play each game once)
5. Save compatibility test (load old save)

## Test Data Management

**Deterministic RNG:**

```c
// Use fixed seed for reproducible tests
void SnakeTestSetup(void)
{
    SeedRng(12345);  // Fixed seed
    SnakeInit();
}

TEST("Food spawns in deterministic positions")
{
    SnakeTestSetup();

    // First food always at (10, 10) with seed 12345
    ASSERT_EQ(10, gSnakeData->food.x);
    ASSERT_EQ(10, gSnakeData->food.y);

    SnakeEatFood();

    // Second food always at (15, 7) with seed 12345
    ASSERT_EQ(15, gSnakeData->food.x);
    ASSERT_EQ(7, gSnakeData->food.y);
}
```

**Mock Data:**

```c
// Create test fixtures for complex scenarios
struct SnakeTestFixture {
    u8 length;
    struct Pos segments[100];
    enum Direction direction;
    u16 score;
};

static const struct SnakeTestFixture sLongSnakeFixture = {
    .length = 50,
    .segments = { {0, 0}, {1, 0}, {2, 0}, /* ... */ },
    .direction = DIRECTION_RIGHT,
    .score = 500
};

void LoadSnakeFixture(const struct SnakeTestFixture *fixture)
{
    gSnakeData->length = fixture->length;
    memcpy(gSnakeData->segments, fixture->segments, fixture->length * sizeof(struct Pos));
    gSnakeData->direction = fixture->direction;
    gSnakeData->score = fixture->score;
}

TEST("Long snake handles turns correctly")
{
    LoadSnakeFixture(&sLongSnakeFixture);
    SnakeChangeDirection(DIRECTION_DOWN);
    SnakeUpdate();

    // Verify all segments follow head
    // ...
}
```

## Bug Tracking Integration

**Test Failure Report Template:**

```markdown
## Test Failure: Snake dies incorrectly on valid move

**Test:** `test/game_corner_snake.c::SnakeHandlesTurnsAtWall`
**Status:** FAILED
**Expected:** Snake turns down at right wall without dying
**Actual:** Snake dies (STATE_GAMEOVER)

**Repro Steps:**
1. SnakeInit()
2. Move snake to x=GRID_WIDTH-1
3. Call SnakeChangeDirection(DIRECTION_DOWN)
4. Call SnakeUpdate()

**Root Cause:** Collision check runs before position update

**Fix:** Reorder collision check to after position update
```

## Performance Benchmarks

**Target Metrics:**
- Frame time: < 16.67ms (60 FPS)
- Memory usage: < 10 KB per minigame
- Load time: < 0.5 seconds
- Save time: < 1 second (existing constraint)

**Benchmark Suite:**

```c
BENCHMARK("Snake update performance")
{
    SnakeInit();
    gSnakeData->length = MAX_SNAKE_LENGTH;

    u32 totalCycles = 0;
    for (u32 i = 0; i < 1000; i++)
    {
        u32 start = GetCpuCycleCount();
        SnakeUpdate();
        u32 end = GetCpuCycleCount();
        totalCycles += (end - start);
    }

    u32 averageCycles = totalCycles / 1000;
    printf("Average cycles per update: %u\n", averageCycles);
    ASSERT_LT(averageCycles, 50000);  // Must be fast
}
```

## Test Documentation

**Per-Test Documentation:**

```c
// Document WHY, not WHAT
TEST("Snake does not die when wrapping around screen borders")
{
    // WHY: In wrap mode, moving off-screen should teleport, not kill
    // This was a bug in v1.0 - snake died on wrap
    // Regression test ensures it stays fixed

    SnakeInit();
    gSnakeData->wrapEnabled = TRUE;
    // ... test implementation
}
```

## Quality Gates

**Code Review Checklist:**
- [ ] All new code has unit tests
- [ ] All tests pass locally
- [ ] No new compiler warnings
- [ ] Code coverage did not decrease
- [ ] Performance benchmarks pass
- [ ] Manual smoke test completed

**Pull Request Requirements:**
- [ ] All CI tests pass
- [ ] Code review approved
- [ ] Documentation updated
- [ ] Changelog entry added

## Documentation Cross-References

**Related Documents:**
- `03_COMMON_INFRASTRUCTURE.md` - Functions to test
- `04_SAVE_STRATEGY.md` - Save persistence testing
- `05_CONFIG_DESIGN.md` - Config variation testing

**RHH Testing Examples:**
- `test/battle/` - Battle test DSL patterns
- `test/main.c` - Test framework implementation
- `docs/tutorials/how_to_testing_system.md` - Testing guide

## Summary

**Testing Strategy:**
1. **Unit tests** - Isolated function testing (80%+ coverage)
2. **Integration tests** - Component interaction testing
3. **Performance tests** - 60 FPS and memory leak detection
4. **Config tests** - All configuration variations
5. **Manual tests** - Comprehensive gameplay verification

**Quality Assurance:**
- Automated CI/CD pipeline
- Regression test suite
- Performance benchmarks
- Code review requirements

**Success Criteria:**
- All automated tests pass
- 60 FPS maintained under load
- No memory leaks (100+ sessions)
- High scores persist correctly
- Save compatibility preserved

---

**Last Updated:** 2025-12-16
**Status:** Testing Strategy Defined
**Framework:** RHH's GIVEN-WHEN-THEN DSL
