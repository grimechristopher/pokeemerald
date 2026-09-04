# True 8-Directional Movement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the player and wandering NPCs move diagonally on the overworld (Gen 6+ style), gated by a config flag ON by default, reusing this fork's existing (but currently stairs-only) diagonal movement primitives.

**Architecture:** Generalize `FieldGetPlayerInput`'s D-pad reading to detect diagonal combinations, add one centralized `CanObjectEventMoveInDirection` collision resolver (used by both player input and NPC wander) that enforces no-corner-cutting, fix one real out-of-bounds risk in the existing directional-block check that diagonal directions would trigger, and add a small, isolated fix for followers (whose "walk to the player's last tile" logic can't currently resolve a diagonal step) and for sideways stairs (which must keep working exactly as today by having diagonal input decomposed back to a single cardinal component before it reaches the existing stairs code).

**Tech Stack:** C (arm-none-eabi-gcc via devkitARM), this project's `TEST()` unit-test DSL (plain `TEST()`, not the battle-scenario DSL, for these overworld-engine functions), `make check`.

**Spec:** `docs/superpowers/specs/2026-09-02-diagonal-movement-design.md`

**Branch:** `feature/diagonal-movement` (already created and checked out from `expanded/base`)

---

## File Structure

| File | Change |
|---|---|
| `include/config/overworld.h` | New `OW_DIAGONAL_MOVEMENT` config flag |
| `src/event_object_movement.c` | Fix `IsMetatileDirectionallyImpassable` diagonal guard; new `CanObjectEventMoveInDirection`; new `GetDiagonalMoveDirection` helper; new diagonal-aware follower direction resolver; `MovementType_WanderAround_Step4` diagonal wander; new `hasDiagonalFrames`-aware animation lookup for face-direction |
| `src/data/object_events/movement_type_func_tables.h` | New `gStandardDirectionsWithDiagonals[]` array |
| `include/global.fieldmap.h` | New `hasDiagonalFrames` field on `struct ObjectEventGraphicsInfo` |
| `src/field_control_avatar.c` | `FieldGetPlayerInput`'s D-pad reading generalized to detect diagonals |
| `src/field_player_avatar.c` | Sideways-stairs diagonal-to-cardinal decomposition |
| `test/event_object_movement.c` | New tests for all of the above |

---

### Task 1: Config flag

**Files:**
- Modify: `include/config/overworld.h`

- [ ] **Step 1: Add the config define**

Add this line right after the existing `SLOW_MOVEMENT_ON_STAIRS` line (in the "Movement config" section):

```c
#define OW_DIAGONAL_MOVEMENT        GEN_LATEST  // In Gen6+, the player can move diagonally by holding two adjacent D-pad directions.
```

- [ ] **Step 2: Verify it builds**

Run: `make -j$(nproc)`
Expected: builds clean, no warnings about `OW_DIAGONAL_MOVEMENT`.

- [ ] **Step 3: Commit**

```bash
git add include/config/overworld.h
git commit -m "expand: add OW_DIAGONAL_MOVEMENT config flag"
```

---

### Task 2: Fix the out-of-bounds risk in `IsMetatileDirectionallyImpassable`

This is a prerequisite safety fix. `IsMetatileDirectionallyImpassable` (`src/event_object_movement.c:10029`) indexes two 4-entry function-pointer tables via `direction - 1`:

```c
bool8 IsMetatileDirectionallyImpassable(struct ObjectEvent *objectEvent, s16 x, s16 y, enum Direction direction)
{
    if (gOppositeDirectionBlockedMetatileFuncs[direction - 1](objectEvent->currentMetatileBehavior)
        || gDirectionBlockedMetatileFuncs[direction - 1](MapGridGetMetatileBehaviorAt(x, y)))
        return TRUE;

    return FALSE;
}
```

`gOppositeDirectionBlockedMetatileFuncs`/`gDirectionBlockedMetatileFuncs` (`src/event_object_movement.c:975-987`) each have exactly 4 entries (indices 0-3, matching `DIR_SOUTH-1`..`DIR_EAST-1`). Once `CanObjectEventMoveInDirection` (Task 3) starts calling into this same collision chain with a genuine diagonal `direction` value (5-8 per `enum Direction` in `include/constants/global.h`), `direction - 1` becomes 4-7 - an out-of-bounds function-pointer read. Directionally-blocked metatile behaviors (one-way ledges, currents that block entry from one side) are an inherently cardinal concept, so for a diagonal move this check is skipped entirely - the corner-cutting flanking checks in `CanObjectEventMoveInDirection` already call this same function with each cardinal *component*, so the direction-blocking semantics are still enforced correctly per axis.

**Files:**
- Modify: `src/event_object_movement.c:10029` (search for `bool8 IsMetatileDirectionallyImpassable`)
- Test: `test/event_object_movement.c`

- [ ] **Step 1: Write the failing test**

Add to `test/event_object_movement.c` (after the existing `#include`s and test, so it can reuse nothing but its own setup):

```c
TEST("IsMetatileDirectionallyImpassable returns FALSE for diagonal directions instead of reading out of bounds")
{
    struct ObjectEvent objectEvent = {0};
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_NORTHEAST), FALSE);
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_NORTHWEST), FALSE);
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_SOUTHEAST), FALSE);
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_SOUTHWEST), FALSE);
}
```

- [ ] **Step 2: Run it to verify it fails (or crashes)**

Run: `make TESTS="IsMetatileDirectionallyImpassable returns FALSE" check -j$(nproc)`
Expected: crash, hang, or a wrong result - not a clean `PASS`. (An out-of-bounds function-pointer call can do any of these depending on what happens to be at that memory address; if it happens to produce a clean pass on this run, that's still evidence of undefined behavior, not correctness - proceed to the fix regardless.)

- [ ] **Step 3: Add the guard**

In `src/event_object_movement.c`, change:

```c
bool8 IsMetatileDirectionallyImpassable(struct ObjectEvent *objectEvent, s16 x, s16 y, enum Direction direction)
{
    if (gOppositeDirectionBlockedMetatileFuncs[direction - 1](objectEvent->currentMetatileBehavior)
        || gDirectionBlockedMetatileFuncs[direction - 1](MapGridGetMetatileBehaviorAt(x, y)))
        return TRUE;

    return FALSE;
}
```

to:

```c
bool8 IsMetatileDirectionallyImpassable(struct ObjectEvent *objectEvent, s16 x, s16 y, enum Direction direction)
{
    // Directionally-blocked metatile behaviors (one-way ledges, currents) are a cardinal-only
    // concept - a diagonal move is validated per cardinal component by the caller instead.
    if (direction >= CARDINAL_DIRECTION_COUNT)
        return FALSE;

    if (gOppositeDirectionBlockedMetatileFuncs[direction - 1](objectEvent->currentMetatileBehavior)
        || gDirectionBlockedMetatileFuncs[direction - 1](MapGridGetMetatileBehaviorAt(x, y)))
        return TRUE;

    return FALSE;
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `make TESTS="IsMetatileDirectionallyImpassable returns FALSE" check -j$(nproc)`
Expected: `PASS`

- [ ] **Step 5: Commit**

```bash
git add src/event_object_movement.c test/event_object_movement.c
git commit -m "fix: guard IsMetatileDirectionallyImpassable against diagonal directions

gOppositeDirectionBlockedMetatileFuncs/gDirectionBlockedMetatileFuncs
are 4-entry tables indexed by direction-1; a diagonal direction value
would read out of bounds. Diagonal moves are validated per cardinal
component by the caller instead."
```

---

### Task 3: `CanObjectEventMoveInDirection` - centralized collision resolver

**Files:**
- Modify: `src/event_object_movement.c` (add near `GetCollisionInDirection`, currently at line 6409)
- Modify: `include/event_object_movement.h` (add prototype)
- Test: `test/event_object_movement.c`

This is the shared corner-cutting resolver both the player-input path (Task 4) and NPC wander (Task 7) will call.

- [ ] **Step 1: Write the failing tests**

Add to `test/event_object_movement.c`:

```c
static void PlaceTestObjectEvent(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    memset(objectEvent, 0, sizeof(*objectEvent));
    objectEvent->currentCoords.x = x;
    objectEvent->currentCoords.y = y;
    objectEvent->previousCoords.x = x;
    objectEvent->previousCoords.y = y;
    objectEvent->currentElevation = ELEVATION_DEFAULT;
}

// GetMapBorderIdAt (src/fieldmap.c) treats any coordinate within MAP_OFFSET (7) tiles of the
// grid edge as a border/connection check, returning CONNECTION_INVALID (blocking every move)
// unless sMapConnectionFlags says otherwise - which is unset in a bare test context. The grid
// has to be big enough that the whole test area sits strictly inside that margin: valid
// interior coordinates are 7 <= x < (width - 8) and 7 <= y < (height - 7). A 21x21 grid with
// the object event at (10, 10) gives a comfortable interior neighborhood (valid x: 7-12,
// valid y: 7-13) for every direction this task's tests move in.
#define TEST_MAP_SIZE 21
#define TEST_MAP_ORIGIN 10
static u16 sTestMapGrid[TEST_MAP_SIZE * TEST_MAP_SIZE];

static void SetUpTestMap(void)
{
    s32 i;
    for (i = 0; i < ARRAY_COUNT(sTestMapGrid); i++)
        sTestMapGrid[i] = PACK_ELEVATION(ELEVATION_DEFAULT);
    gBackupMapLayout.width = TEST_MAP_SIZE;
    gBackupMapLayout.height = TEST_MAP_SIZE;
    gBackupMapLayout.map = sTestMapGrid;
    // DoesObjectCollideWithObjectAt scans the global gObjectEvents[] array - clear it so a
    // stale .active entry left over from an unrelated earlier test can't cause a spurious
    // collision at one of this test's coordinates.
    memset(gObjectEvents, 0, sizeof(gObjectEvents));
}

static void BlockTestMapTile(s16 x, s16 y)
{
    sTestMapGrid[x + gBackupMapLayout.width * y] = PACK_ELEVATION(ELEVATION_DEFAULT) | MAPGRID_IMPASSABLE;
}

TEST("CanObjectEventMoveInDirection allows a plain cardinal move onto an open tile")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTH), TRUE);
}

TEST("CanObjectEventMoveInDirection blocks a plain cardinal move onto a blocked tile")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // directly north
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTH), FALSE);
}

TEST("CanObjectEventMoveInDirection allows a diagonal move when both flanks are open")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), TRUE);
}

TEST("CanObjectEventMoveInDirection allows a diagonal move when exactly one flank is open")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // north flank blocked
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), TRUE); // east flank still open
}

TEST("CanObjectEventMoveInDirection blocks corner-cutting when both flanks are blocked")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // north flank blocked
    BlockTestMapTile(TEST_MAP_ORIGIN + 1, TEST_MAP_ORIGIN); // east flank blocked
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), FALSE);
}

TEST("CanObjectEventMoveInDirection blocks a diagonal move onto a blocked destination even with both flanks open")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN + 1, TEST_MAP_ORIGIN - 1); // the actual NE destination tile
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), FALSE);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="CanObjectEventMoveInDirection" check -j$(nproc)`
Expected: build error (`CanObjectEventMoveInDirection` undeclared).

- [ ] **Step 3: Add the prototype**

In `include/event_object_movement.h`, add near the other collision-related prototypes (alongside `GetCollisionInDirection`):

```c
bool8 CanObjectEventMoveInDirection(struct ObjectEvent *objectEvent, enum Direction direction);
```

- [ ] **Step 4: Implement it**

In `src/event_object_movement.c`, add right after `GetCollisionInDirection` (currently ending at line 6414):

```c
bool8 CanObjectEventMoveInDirection(struct ObjectEvent *objectEvent, enum Direction direction)
{
    enum Direction vertical, horizontal;

    if (direction < CARDINAL_DIRECTION_COUNT)
        return GetCollisionInDirection(objectEvent, direction) == COLLISION_NONE;

    vertical = (direction == DIR_NORTHEAST || direction == DIR_NORTHWEST) ? DIR_NORTH : DIR_SOUTH;
    horizontal = (direction == DIR_NORTHEAST || direction == DIR_SOUTHEAST) ? DIR_EAST : DIR_WEST;

    // No corner-cutting: at least one of the two flanking cardinal tiles must be passable.
    if (GetCollisionInDirection(objectEvent, vertical) != COLLISION_NONE
     && GetCollisionInDirection(objectEvent, horizontal) != COLLISION_NONE)
        return FALSE;

    return GetCollisionInDirection(objectEvent, direction) == COLLISION_NONE;
}
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `make TESTS="CanObjectEventMoveInDirection" check -j$(nproc)`
Expected: all 6 tests `PASS`

- [ ] **Step 6: Commit**

```bash
git add include/event_object_movement.h src/event_object_movement.c test/event_object_movement.c
git commit -m "expand: add CanObjectEventMoveInDirection, a diagonal-aware collision resolver

Cardinal directions are a thin wrapper around the existing
GetCollisionInDirection with no behavior change. Diagonal directions
enforce no-corner-cutting (at least one flanking cardinal tile must
be passable) in addition to checking the actual destination tile.
Shared by the player-input path and NPC wander so the rule can't
drift between them."
```

---

### Task 4: Diagonal input in `FieldGetPlayerInput`

**Files:**
- Modify: `src/event_object_movement.c` (new `GetDiagonalMoveDirection` helper)
- Modify: `include/event_object_movement.h` (prototype)
- Modify: `src/field_control_avatar.c:142-149`
- Test: `test/event_object_movement.c`

- [ ] **Step 1: Write the failing tests for the helper**

Add to `test/event_object_movement.c`:

```c
TEST("GetDiagonalMoveDirection combines a held vertical and horizontal direction into a diagonal")
{
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NORTH, DIR_EAST), DIR_NORTHEAST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NORTH, DIR_WEST), DIR_NORTHWEST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_SOUTH, DIR_EAST), DIR_SOUTHEAST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_SOUTH, DIR_WEST), DIR_SOUTHWEST);
}

TEST("GetDiagonalMoveDirection falls back to whichever single axis is held")
{
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NORTH, DIR_NONE), DIR_NORTH);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NONE, DIR_EAST), DIR_EAST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NONE, DIR_NONE), DIR_NONE);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="GetDiagonalMoveDirection" check -j$(nproc)`
Expected: build error (`GetDiagonalMoveDirection` undeclared).

- [ ] **Step 3: Add the prototype and implementation**

In `include/event_object_movement.h`:

```c
enum Direction GetDiagonalMoveDirection(enum Direction vertical, enum Direction horizontal);
```

In `src/event_object_movement.c`, add near `MoveCoords` (currently line 6677):

```c
enum Direction GetDiagonalMoveDirection(enum Direction vertical, enum Direction horizontal)
{
    if (vertical == DIR_NORTH && horizontal == DIR_WEST)
        return DIR_NORTHWEST;
    if (vertical == DIR_NORTH && horizontal == DIR_EAST)
        return DIR_NORTHEAST;
    if (vertical == DIR_SOUTH && horizontal == DIR_WEST)
        return DIR_SOUTHWEST;
    if (vertical == DIR_SOUTH && horizontal == DIR_EAST)
        return DIR_SOUTHEAST;
    if (vertical != DIR_NONE)
        return vertical;
    return horizontal;
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `make TESTS="GetDiagonalMoveDirection" check -j$(nproc)`
Expected: both tests `PASS`

- [ ] **Step 5: Wire it into `FieldGetPlayerInput`**

In `src/field_control_avatar.c`, replace:

```c
    if (heldKeys & DPAD_UP)
        input->dpadDirection = DIR_NORTH;
    else if (heldKeys & DPAD_DOWN)
        input->dpadDirection = DIR_SOUTH;
    else if (heldKeys & DPAD_LEFT)
        input->dpadDirection = DIR_WEST;
    else if (heldKeys & DPAD_RIGHT)
        input->dpadDirection = DIR_EAST;
```

with:

```c
    if (OW_DIAGONAL_MOVEMENT >= GEN_6)
    {
        enum Direction vertical = DIR_NONE;
        enum Direction horizontal = DIR_NONE;

        if (heldKeys & DPAD_UP)
            vertical = DIR_NORTH;
        else if (heldKeys & DPAD_DOWN)
            vertical = DIR_SOUTH;

        if (heldKeys & DPAD_LEFT)
            horizontal = DIR_WEST;
        else if (heldKeys & DPAD_RIGHT)
            horizontal = DIR_EAST;

        input->dpadDirection = GetDiagonalMoveDirection(vertical, horizontal);
    }
    else
    {
        if (heldKeys & DPAD_UP)
            input->dpadDirection = DIR_NORTH;
        else if (heldKeys & DPAD_DOWN)
            input->dpadDirection = DIR_SOUTH;
        else if (heldKeys & DPAD_LEFT)
            input->dpadDirection = DIR_WEST;
        else if (heldKeys & DPAD_RIGHT)
            input->dpadDirection = DIR_EAST;
    }
```

- [ ] **Step 6: Verify the build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 7: Commit**

```bash
git add include/event_object_movement.h src/event_object_movement.c src/field_control_avatar.c test/event_object_movement.c
git commit -m "expand: detect diagonal D-pad input in FieldGetPlayerInput

Behind OW_DIAGONAL_MOVEMENT (GEN_6+, on by default). With it off, or
with only one axis held, or with opposite keys on one axis
cancelling out, behavior is identical to today."
```

---

### Task 5: Sideways stairs - decompose diagonal input to a single cardinal component

**Files:**
- Modify: `src/field_player_avatar.c` (wherever `PlayerStep`/`MovePlayerAvatarUsingKeypadInput` first receives the resolved direction, before it reaches stairs-aware collision code)
- Test: `test/event_object_movement.c` (testing the decomposition helper directly, not the full player-avatar pipeline)

Rather than touch `GetCollisionAtCoords`'s existing stairs logic (which checks `dir == DIR_EAST`/`DIR_WEST`/`DIR_NORTH`/`DIR_SOUTH` via exact equality and would silently skip all of its branches for a genuine diagonal `dir`), add a small decomposition step that runs specifically when the player is on or moving onto a sideways-stairs tile, turning a diagonal input into whichever single cardinal component the existing code already knows how to handle.

- [ ] **Step 1: Write the failing tests for the decomposition helper**

Add to `test/event_object_movement.c`:

```c
TEST("ResolveStairsMoveDirection prefers the horizontal component of a diagonal input")
{
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NORTHEAST), DIR_EAST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NORTHWEST), DIR_WEST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_SOUTHEAST), DIR_EAST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_SOUTHWEST), DIR_WEST);
}

TEST("ResolveStairsMoveDirection passes cardinal input through unchanged")
{
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NORTH), DIR_NORTH);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_SOUTH), DIR_SOUTH);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_EAST), DIR_EAST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_WEST), DIR_WEST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NONE), DIR_NONE);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="ResolveStairsMoveDirection" check -j$(nproc)`
Expected: build error (`ResolveStairsMoveDirection` undeclared).

- [ ] **Step 3: Implement it**

In `include/event_object_movement.h`:

```c
enum Direction ResolveStairsMoveDirection(enum Direction direction);
```

In `src/event_object_movement.c`, add next to `GetDiagonalMoveDirection`:

```c
// Sideways stairs are driven entirely by cardinal input (GetLeftSideStairsDirection/
// GetRightSideStairsDirection translate a plain West/East press into the correct diagonal
// walk). A genuinely diagonal input needs to be decomposed back into a single cardinal
// component before it reaches that existing, unmodified logic - horizontal preferred,
// since stairs are fundamentally a left/right-driven mechanic.
enum Direction ResolveStairsMoveDirection(enum Direction direction)
{
    switch (direction)
    {
    case DIR_NORTHEAST:
    case DIR_SOUTHEAST:
        return DIR_EAST;
    case DIR_NORTHWEST:
    case DIR_SOUTHWEST:
        return DIR_WEST;
    default:
        return direction;
    }
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `make TESTS="ResolveStairsMoveDirection" check -j$(nproc)`
Expected: both tests `PASS`

- [ ] **Step 5: Wire it into the player-avatar movement path**

In `src/field_player_avatar.c`, find `MovePlayerNotOnBike`:

```c
static void MovePlayerNotOnBike(enum Direction direction, u16 heldKeys)
{
    sPlayerNotOnBikeFuncs[CheckMovementInputNotOnBike(direction)](direction, heldKeys);
}
```

Replace it with:

```c
static bool8 IsOnSidewaysStairsTile(u8 metatileBehavior)
{
    return MetatileBehavior_IsSidewaysStairsLeftSideAny(metatileBehavior)
        || MetatileBehavior_IsSidewaysStairsRightSideAny(metatileBehavior);
}

static void MovePlayerNotOnBike(enum Direction direction, u16 heldKeys)
{
    if (direction >= CARDINAL_DIRECTION_COUNT
     && IsOnSidewaysStairsTile(GetPlayerCurMetatileBehavior(gPlayerAvatar.runningState)))
        direction = ResolveStairsMoveDirection(direction);

    sPlayerNotOnBikeFuncs[CheckMovementInputNotOnBike(direction)](direction, heldKeys);
}
```

`MetatileBehavior_IsSidewaysStairsLeftSideAny`/`MetatileBehavior_IsSidewaysStairsRightSideAny` (declared in `include/metatile_behavior.h`) are the existing general "is this any kind of left/right sideways-stairs tile" predicates - `GetCollisionAtCoords`'s own stairs logic uses the more specific `...Top`/`...Bottom` variants of the same predicates. This only touches the decomposition when the player is actually standing on a sideways-stairs tile; everywhere else, a diagonal `direction` value flows through unchanged.

- [ ] **Step 6: Verify the build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 7: Commit**

```bash
git add include/event_object_movement.h src/event_object_movement.c src/field_player_avatar.c test/event_object_movement.c
git commit -m "expand: decompose diagonal input to one cardinal component on sideways stairs

Sideways stairs' existing collision/translation logic checks the
player's dir via exact cardinal equality and would silently ignore a
genuine diagonal value. Rather than modify that logic, diagonal input
is decomposed back to its horizontal component (falling back to
vertical) before it ever reaches the stairs-aware code path, which
stays completely untouched."
```

---

### Task 6: Verify diagonal movement actually moves the player

**Files:**
- Test: `test/event_object_movement.c`

This task doesn't add new production code - it verifies the existing movement primitives (`InitMovementNormal`, sprite-facing/animation tables) really do handle a diagonal direction outside the stairs-specific context, since the spec flagged this as "expected to work, verify rather than assume."

- [ ] **Step 1: Write the test**

Add to `test/event_object_movement.c`:

```c
TEST("A diagonal MoveCoords step moves both axes by exactly one tile")
{
    s16 x = 5, y = 5;
    MoveCoords(DIR_NORTHEAST, &x, &y);
    EXPECT_EQ(x, 6);
    EXPECT_EQ(y, 4);

    x = 5; y = 5;
    MoveCoords(DIR_SOUTHWEST, &x, &y);
    EXPECT_EQ(x, 4);
    EXPECT_EQ(y, 6);
}

TEST("GetFaceDirectionMovementAction resolves every diagonal direction to a valid movement action")
{
    EXPECT(GetFaceDirectionMovementAction(DIR_NORTHEAST) != MOVEMENT_ACTION_NONE);
    EXPECT(GetFaceDirectionMovementAction(DIR_NORTHWEST) != MOVEMENT_ACTION_NONE);
    EXPECT(GetFaceDirectionMovementAction(DIR_SOUTHEAST) != MOVEMENT_ACTION_NONE);
    EXPECT(GetFaceDirectionMovementAction(DIR_SOUTHWEST) != MOVEMENT_ACTION_NONE);
}
```

- [ ] **Step 2: Run it**

Run: `make TESTS="A diagonal MoveCoords step" check -j$(nproc)` and `make TESTS="GetFaceDirectionMovementAction resolves" check -j$(nproc)`
Expected: both `PASS` with no production code changes - this task exists to prove the claim in writing, not to fix anything. If either fails, stop and report it rather than proceeding to Task 7 - it means an assumption this whole plan depends on is wrong.

- [ ] **Step 3: Commit**

```bash
git add test/event_object_movement.c
git commit -m "test: confirm diagonal movement primitives already work outside the stairs context"
```

---

### Task 7: NPC diagonal wandering

**Files:**
- Modify: `src/data/object_events/movement_type_func_tables.h` (new array)
- Modify: `src/event_object_movement.c:3931-3938` (`MovementType_WanderAround_Step4`)
- Test: `test/event_object_movement.c`

`MovementType_WanderAround_Step4` is shared by both `MOVEMENT_TYPE_WANDER_AROUND` and `MOVEMENT_TYPE_WANDER_AROUND_SLOWER` (they share every step function except `Step5`). A new, separate array is used instead of extending the existing `gStandardDirections[]`, since that array is also used by `MovementType_LookAround_Step4` and `wild_encounter_ow.c`'s pre-encounter turn - both facing-only mechanics that are out of scope here and shouldn't start facing diagonally.

- [ ] **Step 1: Write the failing test**

Add to `test/event_object_movement.c`:

```c
TEST("gStandardDirectionsWithDiagonals has exactly the 4 cardinal and 4 diagonal directions")
{
    s32 i;
    bool8 seen[CARDINAL_DIRECTION_COUNT + 4] = {0};

    EXPECT_EQ(ARRAY_COUNT(gStandardDirectionsWithDiagonals), 8);
    for (i = 0; i < ARRAY_COUNT(gStandardDirectionsWithDiagonals); i++)
    {
        enum Direction direction = gStandardDirectionsWithDiagonals[i];
        EXPECT(direction >= DIR_SOUTH && direction <= DIR_NORTHEAST);
        EXPECT(!seen[direction]);
        seen[direction] = TRUE;
    }
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="gStandardDirectionsWithDiagonals" check -j$(nproc)`
Expected: build error (`gStandardDirectionsWithDiagonals` undeclared).

- [ ] **Step 3: Add the array**

In `include/event_object_movement.h`, add near `extern const enum Direction gStandardDirections[];`:

```c
extern const enum Direction gStandardDirectionsWithDiagonals[];
```

In `src/data/object_events/movement_type_func_tables.h`, add right after the existing `gStandardDirections[]` line:

```c
const enum Direction gStandardDirectionsWithDiagonals[] = {DIR_SOUTH, DIR_NORTH, DIR_WEST, DIR_EAST, DIR_SOUTHWEST, DIR_SOUTHEAST, DIR_NORTHWEST, DIR_NORTHEAST};
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `make TESTS="gStandardDirectionsWithDiagonals" check -j$(nproc)`
Expected: `PASS`

- [ ] **Step 5: Wire it into wander movement, gated and collision-checked**

In `src/event_object_movement.c`, replace `MovementType_WanderAround_Step4`:

```c
bool8 MovementType_WanderAround_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    enum Direction chosenDirection = gStandardDirections[Random() & 3];
    SetObjectEventDirection(objectEvent, chosenDirection);
    sprite->sTypeFuncId = 5;
    if (GetCollisionInDirection(objectEvent, chosenDirection))
        sprite->sTypeFuncId = 1;

    return TRUE;
}
```

with:

```c
bool8 MovementType_WanderAround_Step4(struct ObjectEvent *objectEvent, struct Sprite *sprite)
{
    enum Direction chosenDirection;

    if (OW_DIAGONAL_MOVEMENT >= GEN_6)
        chosenDirection = gStandardDirectionsWithDiagonals[Random() % ARRAY_COUNT(gStandardDirectionsWithDiagonals)];
    else
        chosenDirection = gStandardDirections[Random() & 3];

    SetObjectEventDirection(objectEvent, chosenDirection);
    sprite->sTypeFuncId = 5;
    if (!CanObjectEventMoveInDirection(objectEvent, chosenDirection))
        sprite->sTypeFuncId = 1;

    return TRUE;
}
```

(This also switches the collision check itself from the raw `GetCollisionInDirection` to the new `CanObjectEventMoveInDirection`, so a diagonal choice gets the corner-cutting rule instead of only a destination-tile check; cardinal choices behave exactly as before since `CanObjectEventMoveInDirection` is a thin wrapper for them.)

- [ ] **Step 6: Verify the build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 7: Commit**

```bash
git add include/event_object_movement.h src/event_object_movement.c src/data/object_events/movement_type_func_tables.h test/event_object_movement.c
git commit -m "expand: let wandering NPCs pick diagonal directions

Behind OW_DIAGONAL_MOVEMENT. Uses a new gStandardDirectionsWithDiagonals
array rather than extending the shared gStandardDirections, since that
array is also used by MovementType_LookAround_Step4 (a facing-only,
not movement, mechanic) and wild_encounter_ow.c's pre-encounter turn -
both out of scope and left cardinal-only."
```

---

### Task 8: Fix followers for diagonal player movement

**Files:**
- Modify: `src/event_object_movement.c` (new diagonal-aware direction resolver; wire into `FollowablePlayerMovement_Step`)
- Test: `test/event_object_movement.c`

`FollowablePlayerMovement_Step` (`src/event_object_movement.c:5859`) moves the follower toward `gObjectEvents[player].previousCoords`, picking its direction via `GetDirectionToFace(x, y, targetX, targetY)` - a function that's cardinal-only by construction (checks X first, only ever returns `DIR_WEST`/`DIR_EAST` on any horizontal delta) and is also script-exposed via `GetDirectionToFaceScript` for unrelated "face toward" scripted behavior that must keep working exactly as it does today. So this adds a new, follower-specific resolver rather than changing the shared one.

- [ ] **Step 1: Write the failing tests**

Add to `test/event_object_movement.c`:

```c
TEST("GetFollowerStepDirection returns a diagonal direction when both axes differ by one tile")
{
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 6, 4), DIR_NORTHEAST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 4, 4), DIR_NORTHWEST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 6, 6), DIR_SOUTHEAST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 4, 6), DIR_SOUTHWEST);
}

TEST("GetFollowerStepDirection falls back to a cardinal direction when only one axis differs")
{
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 5, 4), DIR_NORTH);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 5, 6), DIR_SOUTH);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 4, 5), DIR_WEST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 6, 5), DIR_EAST);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="GetFollowerStepDirection" check -j$(nproc)`
Expected: build error (`GetFollowerStepDirection` undeclared).

- [ ] **Step 3: Implement it**

In `include/event_object_movement.h`:

```c
enum Direction GetFollowerStepDirection(s16 x, s16 y, s16 targetX, s16 targetY);
```

In `src/event_object_movement.c`, add next to `GetDirectionToFace` (currently line 6335):

```c
// Like GetDirectionToFace, but resolves a diagonal direction when the target differs in
// both axes - used only for the follower's step toward the player's previous tile, which
// can now be diagonally offset. GetDirectionToFace itself stays cardinal-only since it's
// also script-exposed (GetDirectionToFaceScript) for unrelated "face toward" behavior.
enum Direction GetFollowerStepDirection(s16 x, s16 y, s16 targetX, s16 targetY)
{
    enum Direction vertical = DIR_NONE;
    enum Direction horizontal = DIR_NONE;

    if (y > targetY)
        vertical = DIR_NORTH;
    else if (y < targetY)
        vertical = DIR_SOUTH;

    if (x > targetX)
        horizontal = DIR_WEST;
    else if (x < targetX)
        horizontal = DIR_EAST;

    return GetDiagonalMoveDirection(vertical, horizontal);
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `make TESTS="GetFollowerStepDirection" check -j$(nproc)`
Expected: both tests `PASS`

- [ ] **Step 5: Wire it into `FollowablePlayerMovement_Step`**

In `src/event_object_movement.c`, find this line inside `FollowablePlayerMovement_Step` (currently line 5904):

```c
    direction = GetDirectionToFace(x, y, targetX, targetY);
```

Replace it with:

```c
    direction = GetFollowerStepDirection(x, y, targetX, targetY);
```

- [ ] **Step 6: Verify the build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 7: Commit**

```bash
git add include/event_object_movement.h src/event_object_movement.c test/event_object_movement.c
git commit -m "fix: followers can now step diagonally to reach the player's previous tile

FollowablePlayerMovement_Step used the cardinal-only GetDirectionToFace,
which could never resolve a diagonal step - needed on every diagonal
player move, since the follower's target (the player's previous tile)
is now frequently diagonally offset from the follower's own position.
New GetFollowerStepDirection is used only here; GetDirectionToFace
itself is unchanged since it's also script-exposed elsewhere."
```

---

### Task 9: Diagonal sprite frame capability (opt-in, falls back to E/W substitution)

**Files:**
- Modify: `include/global.fieldmap.h` (`struct ObjectEventGraphicsInfo`)
- Modify: `src/event_object_movement.c` (`sFaceDirectionAnimNums` lookup)
- Test: `test/event_object_movement.c`

This task establishes the pattern for one animation table (`sFaceDirectionAnimNums`, used for facing/idle). Every other diagonal-substitution table mentioned in the design (walk, run, etc.) follows the identical shape and is intentionally left for a follow-up pass, since none of it is visible without real diagonal sprite art to go with it - the goal here is proving the capability flag and fallback mechanism work end to end for one concrete case.

- [ ] **Step 1: Add the capability field**

In `include/global.fieldmap.h`, add a new field at the end of `struct ObjectEventGraphicsInfo` (after `affineAnims`):

```c
struct ObjectEventGraphicsInfo
{
    /*0x00*/ u16 tileTag;
    /*0x02*/ u16 paletteTag;
    /*0x04*/ u16 reflectionPaletteTag;
    /*0x06*/ u16 size;
    /*0x08*/ s16 width;
    /*0x0A*/ s16 height;
    /*0x0C*/ u8 paletteSlot:4;
             u8 shadowSize:2;
             u8 inanimate:1;
             u8 compressed:1;
    /*0x0D*/ u8 tracks;
    /*0x10*/ const struct OamData *oam;
    /*0x14*/ const struct SubspriteTable *subspriteTables;
    /*0x18*/ const union AnimCmd *const *anims;
    /*0x1C*/ const struct SpriteFrameImage *images;
    /*0x20*/ const union AffineAnimCmd *const *affineAnims;
    bool8 hasDiagonalFrames; // If TRUE, `anims` includes real diagonal-facing frames after the 4 cardinal ones instead of falling back to the East/West substitution.
};
```

Every existing `struct ObjectEventGraphicsInfo` initializer in the codebase uses designated initializers (`.tileTag = ..., .size = ...`), so this is purely additive - no existing table needs to change, and every existing entry defaults `hasDiagonalFrames` to `FALSE`.

- [ ] **Step 2: Write the failing tests**

Add to `test/event_object_movement.c`. This reuses `sGraphicsInfo32x32` already defined at the top of the file, plus a second copy with the new flag set:

```c
static const struct ObjectEventGraphicsInfo sGraphicsInfo32x32WithDiagonals = {
    .tileTag = TAG_NONE,
    .size = sizeof(sFrame32x32),
    .oam = &sOam32x32,
    .images = sImages32x32,
    .hasDiagonalFrames = TRUE,
};

TEST("GetFaceDirectionAnimNum falls back to the East/West substitution when hasDiagonalFrames is unset")
{
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32, DIR_NORTHEAST), ANIM_STD_FACE_EAST);
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32, DIR_NORTHWEST), ANIM_STD_FACE_WEST);
}

TEST("GetFaceDirectionAnimNum uses the real diagonal anim when hasDiagonalFrames is set")
{
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32WithDiagonals, DIR_NORTHEAST), ANIM_STD_FACE_NORTHEAST);
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32WithDiagonals, DIR_NORTHWEST), ANIM_STD_FACE_NORTHWEST);
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32WithDiagonals, DIR_SOUTHEAST), ANIM_STD_FACE_SOUTHEAST);
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32WithDiagonals, DIR_SOUTHWEST), ANIM_STD_FACE_SOUTHWEST);
}

TEST("GetFaceDirectionAnimNum is unaffected for cardinal directions either way")
{
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32, DIR_NORTH), ANIM_STD_FACE_NORTH);
    EXPECT_EQ(GetFaceDirectionAnimNum(&sGraphicsInfo32x32WithDiagonals, DIR_NORTH), ANIM_STD_FACE_NORTH);
}
```

- [ ] **Step 3: Run it to verify it fails**

Run: `make TESTS="GetFaceDirectionAnimNum" check -j$(nproc)`
Expected: build error (`GetFaceDirectionAnimNum`/`ANIM_STD_FACE_NORTHEAST`/etc. undeclared).

- [ ] **Step 4: Add the new anim constants**

`include/constants/event_object_movement.h` defines these as sequential `#define`s ending with `ANIM_STD_FASTEST_EAST 19` then `ANIM_STD_COUNT 20` (currently unreferenced anywhere else in the codebase - safe to leave untouched, since it accurately describes the count of the *standard* non-diagonal set). Add the four new diagonal constants right after `ANIM_STD_COUNT`:

```c
#define ANIM_STD_FACE_NORTHEAST   20
#define ANIM_STD_FACE_NORTHWEST   21
#define ANIM_STD_FACE_SOUTHEAST   22
#define ANIM_STD_FACE_SOUTHWEST   23
```

These are placeholders for real animation *data* if/when a sprite sheet with true diagonal frames is authored - this task proves the selection logic, not the art.

- [ ] **Step 5: Add `GetFaceDirectionAnimNum` and wire it in**

In `include/event_object_movement.h`:

```c
u8 GetFaceDirectionAnimNum(const struct ObjectEventGraphicsInfo *graphicsInfo, enum Direction direction);
```

In `src/event_object_movement.c`, add right after the `sFaceDirectionAnimNums[]` table (currently ending around line 795):

```c
u8 GetFaceDirectionAnimNum(const struct ObjectEventGraphicsInfo *graphicsInfo, enum Direction direction)
{
    if (graphicsInfo->hasDiagonalFrames)
    {
        switch (direction)
        {
        case DIR_NORTHEAST:
            return ANIM_STD_FACE_NORTHEAST;
        case DIR_NORTHWEST:
            return ANIM_STD_FACE_NORTHWEST;
        case DIR_SOUTHEAST:
            return ANIM_STD_FACE_SOUTHEAST;
        case DIR_SOUTHWEST:
            return ANIM_STD_FACE_SOUTHWEST;
        default:
            break;
        }
    }

    return sFaceDirectionAnimNums[direction];
}
```

Every existing call site that currently does `sFaceDirectionAnimNums[objectEvent->facingDirection]` (or similar) for a *player or NPC-facing* lookup should be migrated to `GetFaceDirectionAnimNum(graphicsInfo, direction)` as a follow-up once a real sprite sheet with `hasDiagonalFrames` exists to exercise it - out of scope for this task, which only needs the selection function itself to exist and be correct.

- [ ] **Step 6: Run the tests to verify they pass**

Run: `make TESTS="GetFaceDirectionAnimNum" check -j$(nproc)`
Expected: all 3 tests `PASS`

- [ ] **Step 7: Verify the full build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 8: Commit**

```bash
git add include/global.fieldmap.h include/event_object_movement.h src/event_object_movement.c test/event_object_movement.c
git commit -m "expand: add opt-in true-diagonal-sprite-frame capability

hasDiagonalFrames defaults FALSE on every existing
ObjectEventGraphicsInfo entry (designated initializers, purely
additive). GetFaceDirectionAnimNum establishes the selection pattern
for one table (face-direction); the remaining walk/run/etc tables
follow the identical shape once real diagonal art exists to justify
wiring them up. SE/SW currently still fall back to E/W pending that
art, matching the existing substitution."
```

---

### Task 10: Full regression pass

**Files:** none (verification only)

- [ ] **Step 1: Full clean build**

Run: `make clean && make -j$(nproc)`
Expected: builds clean, no new warnings.

- [ ] **Step 2: Full test suite**

Run: `make check -j$(nproc)`
Expected: every test from Tasks 1-9 passes, and the full suite's pass/fail/known-failing/TO_DO counts match what they were on `expanded/base` before this branch (no regressions introduced). If any existing test now fails, stop and root-cause it before continuing - do not assume it's unrelated.

- [ ] **Step 3: Final commit (if any working-tree changes remain)**

```bash
git status
# if clean, nothing to do - Tasks 1-9 already committed everything
```

---

## Addendum: fixes from the final whole-branch review

Task 10 passed clean (4970 tests, 0 regressions), but the required final holistic code-reviewer subagent (per `subagent-driven-development`'s own process, run over the *entire* `expanded/base..feature/diagonal-movement` diff rather than task-by-task) found two **Critical**, silent, default-on integration gaps that no individual task's tests caught, because each task's tests exercised its own new function directly rather than the full player-input-to-collision path. It also found one **Important** gap in a second, separate follower system. Tasks 11-13 below close these before merge.

**Not turned into tasks, by design:**
- **Arrow warps** (`IsArrowWarpMetatileBehavior`, `src/field_control_avatar.c`): a diagonal approach already falls through its cardinal-only `switch`'s `default: return FALSE`, so the warp simply doesn't trigger - the player is not blocked, corrupted, or desynced, just not force-warped until they step in from a cardinal direction. This is already consistent with this feature's established "diagonal approach to a directional special-case tile does not get special-case behavior" precedent (see the ledge-jump fix in Task 8), so it's left as-is rather than turned into a new task.
- **`GetJumpInPlaceMovementActions`/spin-timer edge cases** (`src/field_player_avatar.c`): cosmetic-only (wrong-facing animation on secret base mat jump; one missed spin-evolution turn count), no desync or corruption risk. Deferred.

### Task 11: Enforce no-corner-cutting for the player's own movement

**Problem:** `CanObjectEventMoveInDirection` (Task 3) is only ever called from NPC wander (`MovementType_WanderAround_Step4`). The player's collision path - `CheckForPlayerAvatarCollision` (`src/field_player_avatar.c`) - calls `CheckForObjectEventCollision`, a completely separate function that only checks the diagonal destination tile, never the two flanking cardinal tiles. **The player can freely cut corners between two blocking tiles**, the exact rule this branch built and unit-tested for NPCs, but never wired up for the character the feature is actually for.

**Files:**
- Modify: `src/event_object_movement.c` (extract the corner-cut check out of `CanObjectEventMoveInDirection` into its own reusable function)
- Modify: `include/event_object_movement.h` (prototype)
- Modify: `src/field_player_avatar.c:945-961` (`CheckForPlayerAvatarCollision`)
- Test: `test/event_object_movement.c`

- [ ] **Step 1: Write the failing test**

Add near the existing `CanObjectEventMoveInDirection` corner-cutting tests in `test/event_object_movement.c`:

```c
TEST("IsDiagonalMoveBlockedByCorner returns FALSE for a cardinal direction")
{
    struct ObjectEvent objectEvent = {0};
    objectEvent.currentCoords.x = 10;
    objectEvent.currentCoords.y = 10;
    EXPECT_FALSE(IsDiagonalMoveBlockedByCorner(&objectEvent, DIR_NORTH));
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make TESTS="IsDiagonalMoveBlockedByCorner" check -j$(nproc)`
Expected: FAIL to compile - `IsDiagonalMoveBlockedByCorner` undeclared.

- [ ] **Step 3: Extract the shared corner-cut check**

In `src/event_object_movement.c`, replace the existing `CanObjectEventMoveInDirection`:

```c
bool8 CanObjectEventMoveInDirection(struct ObjectEvent *objectEvent, enum Direction direction)
{
    enum Direction vertical, horizontal;

    if (direction < CARDINAL_DIRECTION_COUNT)
        return GetCollisionInDirection(objectEvent, direction) == COLLISION_NONE;

    vertical = (direction == DIR_NORTHEAST || direction == DIR_NORTHWEST) ? DIR_NORTH : DIR_SOUTH;
    horizontal = (direction == DIR_NORTHEAST || direction == DIR_SOUTHEAST) ? DIR_EAST : DIR_WEST;

    // No corner-cutting: at least one of the two flanking cardinal tiles must be passable.
    if (GetCollisionInDirection(objectEvent, vertical) != COLLISION_NONE
     && GetCollisionInDirection(objectEvent, horizontal) != COLLISION_NONE)
        return FALSE;

    return GetCollisionInDirection(objectEvent, direction) == COLLISION_NONE;
}
```

with:

```c
// No corner-cutting: for a diagonal move, at least one of the two flanking cardinal
// tiles must be passable, or the move is rejected even if the diagonal destination
// tile itself is open. Shared by the player's own collision path
// (CheckForPlayerAvatarCollision, src/field_player_avatar.c) and the NPC-wander path
// (CanObjectEventMoveInDirection below) so the rule can't drift apart between them.
bool8 IsDiagonalMoveBlockedByCorner(struct ObjectEvent *objectEvent, enum Direction direction)
{
    enum Direction vertical, horizontal;

    if (direction < CARDINAL_DIRECTION_COUNT)
        return FALSE;

    vertical = (direction == DIR_NORTHEAST || direction == DIR_NORTHWEST) ? DIR_NORTH : DIR_SOUTH;
    horizontal = (direction == DIR_NORTHEAST || direction == DIR_SOUTHEAST) ? DIR_EAST : DIR_WEST;

    return (GetCollisionInDirection(objectEvent, vertical) != COLLISION_NONE
         && GetCollisionInDirection(objectEvent, horizontal) != COLLISION_NONE);
}

bool8 CanObjectEventMoveInDirection(struct ObjectEvent *objectEvent, enum Direction direction)
{
    if (IsDiagonalMoveBlockedByCorner(objectEvent, direction))
        return FALSE;

    return GetCollisionInDirection(objectEvent, direction) == COLLISION_NONE;
}
```

Add the prototype to `include/event_object_movement.h` next to `CanObjectEventMoveInDirection`'s existing prototype:

```c
bool8 IsDiagonalMoveBlockedByCorner(struct ObjectEvent *, enum Direction);
```

- [ ] **Step 4: Run test to verify it passes**

Run: `make TESTS="IsDiagonalMoveBlockedByCorner" check -j$(nproc)`
Expected: PASS

- [ ] **Step 5: Wire the check into the player's own collision path**

In `src/field_player_avatar.c`, modify `CheckForPlayerAvatarCollision`:

```c
static enum Collision CheckForPlayerAvatarCollision(enum Direction direction)
{
    s16 x, y;
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    x = playerObjEvent->currentCoords.x;
    y = playerObjEvent->currentCoords.y;
    if (IsDirectionalStairWarpMetatileBehavior(MapGridGetMetatileBehaviorAt(x, y), direction))
        return COLLISION_STAIR_WARP;

    if (IsDiagonalMoveBlockedByCorner(playerObjEvent, direction))
        return COLLISION_IMPASSABLE;

    MoveCoords(direction, &x, &y);
    return CheckForObjectEventCollision(playerObjEvent, x, y, direction, MapGridGetMetatileBehaviorAt(x, y));
}
```

(By the time a diagonal `direction` reaches this function on a sideways-stairs tile, Task 5's `ResolveStairsMoveDirection` has already reduced it to a single cardinal component, so `IsDiagonalMoveBlockedByCorner` is a no-op there - it only ever fires for a genuine, non-stairs diagonal move.)

- [ ] **Step 6: Add an integration-level regression test**

Add to `test/event_object_movement.c`, reusing the existing 21x21 test-map fixture from the `CanObjectEventMoveInDirection` tests (place the player object event instead of a generic one, and drive the same corner-blocked/corner-open coordinates through `CheckForPlayerAvatarCollision` if it's reachable from the test binary, or - if it is `static` and not exposed - through `CanObjectEventMoveInDirection` called with `&gObjectEvents[gPlayerAvatar.objectEventId]` after `TryInitLocalPlayerAvatar`/`ObjectEventSetGraphicsId` set up a real player object event, confirming the corner-cut result matches for the player's own object event, not just an arbitrary one). Confirm with the implementer subagent which is reachable and use it - do not skip this step if `CheckForPlayerAvatarCollision` turns out to be unreachable; expose it (drop `static`) rather than leave the fix untested at the integration level.

- [ ] **Step 7: Run the tests to verify they pass**

Run: `make TESTS="IsDiagonalMoveBlockedByCorner CanObjectEventMoveInDirection" check -j$(nproc)`
Expected: all PASS

- [ ] **Step 8: Verify the full build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 9: Commit**

```bash
git add include/event_object_movement.h src/event_object_movement.c src/field_player_avatar.c test/event_object_movement.c
git commit -m "fix: enforce no-corner-cutting for the player's own diagonal movement

CanObjectEventMoveInDirection was only ever called from NPC wander -
the player's own collision path (CheckForPlayerAvatarCollision) never
checked the two flanking cardinal tiles on a diagonal move, so the
player could freely cut corners between two blocking tiles. Extracted
the shared check into IsDiagonalMoveBlockedByCorner and wired it into
both call sites so the rule can't drift apart between them again."
```

---

### Task 12: Restrict diagonal input to walking/running, excluding bikes and surf

**Problem:** `FieldGetPlayerInput` combines the D-pad into a diagonal direction whenever `OW_DIAGONAL_MOVEMENT` is on, with no check of the player's current bike/surf state. The design spec explicitly scopes this pass to "walking + running only... Bikes (Mach/Acro), Surf, and Dive stay cardinal-only" - that boundary isn't enforced anywhere in code, so a player who is biking or surfing and holds a diagonal D-pad combo gets a diagonal *position* update while the bike/surf-specific movement and animation code (never audited for 8-direction input) is fed a value outside the range it was written for.

**Files:**
- Modify: `src/field_control_avatar.c:141-171` (`FieldGetPlayerInput`)
- Test: `test/event_object_movement.c` (or a new `test/field_control_avatar.c` if the implementer subagent determines `FieldGetPlayerInput` is better tested from its own file - check for precedent first)

- [ ] **Step 1: Write the failing test**

`FieldGetPlayerInput` takes a `struct FieldInput *`, `heldKeys`, and reads the global `gPlayerAvatar.flags`. Add a test that sets `gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_SURFING`, calls `FieldGetPlayerInput` with `DPAD_UP | DPAD_RIGHT` held, and asserts `input.dpadDirection == DIR_NORTH` (i.e. collapses to the single most-recent/priority cardinal, exactly like today, not `DIR_NORTHEAST`) - then repeats with `gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT` and asserts `input.dpadDirection == DIR_NORTHEAST`. Restore `gPlayerAvatar.flags` to its prior value at the end of the test (or confirm the test harness resets globals between tests - check existing tests in the same file for the established pattern before assuming).

```c
TEST("FieldGetPlayerInput does not combine diagonal input while surfing")
{
    struct FieldInput input = {0};
    u8 savedFlags = gPlayerAvatar.flags;

    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_SURFING;
    FieldGetPlayerInput(&input, 0, DPAD_UP | DPAD_RIGHT);
    EXPECT_EQ(input.dpadDirection, DIR_NORTH);

    gPlayerAvatar.flags = savedFlags;
}

TEST("FieldGetPlayerInput combines diagonal input while on foot")
{
    struct FieldInput input = {0};
    u8 savedFlags = gPlayerAvatar.flags;

    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
    FieldGetPlayerInput(&input, 0, DPAD_UP | DPAD_RIGHT);
    EXPECT_EQ(input.dpadDirection, DIR_NORTHEAST);

    gPlayerAvatar.flags = savedFlags;
}
```

- [ ] **Step 2: Run test to verify the surf test fails**

Run: `make TESTS="FieldGetPlayerInput" check -j$(nproc)`
Expected: the surfing test FAILs (`input.dpadDirection` is `DIR_NORTHEAST`, not `DIR_NORTH`); the on-foot test already PASSes.

- [ ] **Step 3: Gate the diagonal-combine branch on foot travel**

In `src/field_control_avatar.c`, change:

```c
    if (OW_DIAGONAL_MOVEMENT >= GEN_6)
    {
```

to:

```c
    if (OW_DIAGONAL_MOVEMENT >= GEN_6 && (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ON_FOOT))
    {
```

leaving the rest of that `if` block and its `else` (the existing single-cardinal if/else-if chain) unchanged - biking, surfing, and underwater all fall to the same `else` branch that config-off already uses, so their behavior is byte-for-byte what it is on `expanded/base` today.

- [ ] **Step 4: Run test to verify both pass**

Run: `make TESTS="FieldGetPlayerInput" check -j$(nproc)`
Expected: both PASS

- [ ] **Step 5: Verify the full build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 6: Commit**

```bash
git add src/field_control_avatar.c test/event_object_movement.c
git commit -m "fix: restrict diagonal input combining to on-foot travel

FieldGetPlayerInput combined the D-pad into a diagonal direction
regardless of the player's bike/surf state, violating this feature's
explicit walking+running-only scope - bike and surf movement/animation
code was never audited for 8-direction input. Gate the diagonal-combine
branch on PLAYER_AVATAR_FLAG_ON_FOOT; biking, surfing, and underwater
now fall through to the same cardinal-only chain config-off already
uses."
```

---

### Task 13: Diagonal-aware direction for the follower NPC system

**Problem:** Task 8 fixed the Pokémon follower (`FollowablePlayerMovement_Step`) to step diagonally when the player does, by adding `GetFollowerStepDirection` rather than changing the shared `GetDirectionToFace`. The separate "follower NPC" system (`src/follower_npc.c` - a trailing human NPC, not a Pokémon) has the exact same shape of bug via its own cardinal-only resolver, `DetermineObjectEventDirectionFromObject` (`src/event_object_movement.c`), called from `DetermineFollowerNPCDirection`. `DetermineObjectEventDirectionFromObject` is also used by `ObjectEventsTurnToEachOther` for general scripted "face each other" behavior, so - same as Task 8 - it must not change; only the follower-NPC call site gets diagonal-aware.

Unlike the Pokémon follower (always exactly one tile behind the player by construction), the follower NPC's offset from the player is not guaranteed to be exactly one tile (it can lag and catch up), so this needs a resolver based on the *sign* of the coordinate delta rather than `GetFollowerStepDirection`'s one-tile-offset assumption.

**Files:**
- Modify: `src/follower_npc.c` (`DetermineFollowerNPCDirection`)
- Test: `test/event_object_movement.c` or a follower-NPC-specific test file if one already exists - check `test/` for `follower_npc` precedent first.

- [ ] **Step 1: Write the failing test**

```c
TEST("DetermineFollowerNPCDirection returns a diagonal direction when both axes differ")
{
    struct ObjectEvent player = {0};
    struct ObjectEvent follower = {0};

    player.currentCoords.x = 12;
    player.currentCoords.y = 8;
    follower.currentCoords.x = 10;
    follower.currentCoords.y = 10;

    EXPECT_EQ(DetermineFollowerNPCDirection(&player, &follower), DIR_NORTHEAST);
}
```

(Confirm the expected direction empirically against the existing, unchanged `DetermineObjectEventDirectionFromObject`'s per-axis sign convention - Step 3 below derives it from that function's current behavior; if the derivation and this expectation disagree, trust the derivation and fix this test's expected value rather than the implementation.)

- [ ] **Step 2: Run test to verify it fails**

Run: `make TESTS="DetermineFollowerNPCDirection" check -j$(nproc)`
Expected: FAIL (returns a single cardinal direction today, not `DIR_NORTHEAST`).

- [ ] **Step 3: Make the follower-NPC call site diagonal-aware**

In `src/follower_npc.c`, replace:

```c
enum Direction DetermineFollowerNPCDirection(struct ObjectEvent *player, struct ObjectEvent *follower)
{
    if (player->currentCoords.x == follower->currentCoords.x
     && player->currentCoords.y == follower->currentCoords.y)
        return DIR_NONE;
        
    return DetermineObjectEventDirectionFromObject(player, follower);
}
```

with:

```c
enum Direction DetermineFollowerNPCDirection(struct ObjectEvent *player, struct ObjectEvent *follower)
{
    s32 deltaX, deltaY;
    enum Direction vertical, horizontal;

    if (player->currentCoords.x == follower->currentCoords.x
     && player->currentCoords.y == follower->currentCoords.y)
        return DIR_NONE;

    if (OW_DIAGONAL_MOVEMENT < GEN_6)
        return DetermineObjectEventDirectionFromObject(player, follower);

    // Sign-based, not one-tile-offset-based (unlike GetFollowerStepDirection): the
    // follower NPC can lag and catch up across more than one tile, unlike the
    // Pokemon follower's always-exactly-one-tile trail.
    deltaX = player->currentCoords.x - follower->currentCoords.x;
    deltaY = player->currentCoords.y - follower->currentCoords.y;
    horizontal = (deltaX > 0) ? DIR_EAST : (deltaX < 0) ? DIR_WEST : DIR_NONE;
    vertical = (deltaY > 0) ? DIR_SOUTH : (deltaY < 0) ? DIR_NORTH : DIR_NONE;

    return GetDiagonalMoveDirection(vertical, horizontal);
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `make TESTS="DetermineFollowerNPCDirection" check -j$(nproc)`
Expected: PASS

- [ ] **Step 5: Add a config-off regression test**

```c
TEST("DetermineFollowerNPCDirection falls back to cardinal-only with the config off")
{
    // Only meaningful if OW_DIAGONAL_MOVEMENT can be overridden per-test in this
    // codebase's test harness (check existing tests in this file for the established
    // pattern, e.g. how the CanObjectEventMoveInDirection or FieldGetPlayerInput tests
    // above handle the config-off case, and mirror it). If the config is a compile-time
    // constant with no test-time override available, skip this step and note it in the
    // commit message instead - do not fabricate a test that can't actually flip the config.
}
```

- [ ] **Step 6: Verify the full build**

Run: `make -j$(nproc)`
Expected: builds clean.

- [ ] **Step 7: Commit**

```bash
git add src/follower_npc.c test/event_object_movement.c
git commit -m "fix: make the follower-NPC system step diagonally too

Task 8 fixed this for the Pokemon follower (FollowablePlayerMovement_Step)
but the separate follower-NPC system (src/follower_npc.c) had the same
cardinal-only zigzag via DetermineObjectEventDirectionFromObject, left
unaudited because it's a structurally distinct feature. Added a
sign-based (not one-tile-offset-based, since this follower can lag by
more than one tile) diagonal resolver at the follower-NPC call site only
- DetermineObjectEventDirectionFromObject itself is untouched, since
ObjectEventsTurnToEachOther's scripted face-each-other behavior must
stay cardinal-only."
```

---

### Task 14: Second full regression pass and final re-review

**Files:** none (verification only)

- [ ] **Step 1: Full clean build**

Run: `make clean && make -j$(nproc)`
Expected: builds clean, no new warnings.

- [ ] **Step 2: Full test suite**

Run: `make check -j$(nproc)`
Expected: no regressions vs. Task 10's numbers, plus the new Task 11-13 tests passing.

- [ ] **Step 3: Dispatch a final holistic code-reviewer subagent** over the full `expanded/base..feature/diagonal-movement` diff (now including Tasks 11-13), specifically re-checking: is `CanObjectEventMoveInDirection`/`IsDiagonalMoveBlockedByCorner` now reachable from every diagonal-move-producing path (player, NPC wander, and confirm follower/follower-NPC don't need it since they only ever target the player's own already-validated tile)? Is there still any diagonal-input path that bypasses the `PLAYER_AVATAR_FLAG_ON_FOOT` gate? Confirm before declaring the branch ready to merge.

- [ ] **Step 4: Final commit (if any working-tree changes remain)**

```bash
git status
```
