# True 8-Directional Movement Design

**Branch:** `feature/diagonal-movement`

**Goal:** Let the player (and wandering NPCs) move diagonally on the overworld, matching modern (Gen 6+) mainline Pokémon games, gated by a config flag that's ON by default.

**Background:** This fork already has most of the low-level infrastructure diagonal movement needs, discovered while scoping this feature:

- `enum Direction` (`include/constants/global.h`) already has `DIR_NORTHEAST`/`DIR_NORTHWEST`/`DIR_SOUTHEAST`/`DIR_SOUTHWEST` alongside the four cardinal directions.
- A working "sideways stairs" feature (`src/field_player_avatar.c`'s `GetRightSideStairsDirection`/`GetLeftSideStairsDirection`, `ObjectMovingOnRockStairs`) already moves the player diagonally on specific stair metatiles, proving the movement primitives (`InitMovementNormal`, `InitWalkSlow`), sprite-facing tables, and animation-selection tables in `src/event_object_movement.c` all already handle every one of the 8 directions correctly.
- The single chokepoint that prevents general diagonal input: `FieldGetPlayerInput` in `src/field_control_avatar.c` reads the D-pad as an if/else-if chain (UP, else DOWN, else LEFT, else RIGHT) that collapses any simultaneous press down to one cardinal direction, discarding the second one even though the GBA's real D-pad hardware reports both.

So this is substantially a matter of generalizing what already works for stairs into a general-purpose input path, plus the corner-cutting collision rule and (optionally) real diagonal sprite art support - not building diagonal movement from scratch.

**Note:** this section originally scoped Bikes/Surf/Dive and diagonal ledge-hopping out for a later pass. Both were brought in scope afterward - see "Addendum 2" at the end of the companion plan doc (`docs/superpowers/plans/2026-09-02-diagonal-movement.md`) for what changed and why.

**Explicitly out of scope for this pass:**
- Per-map opt-out. One global config flag, no map-level override.
- Autotile/terrain-tile changes. Diagonal walking doesn't require new terrain art - the sprite just moves across existing tiles regardless of which of the 8 directions it came from.
- Inventing a genuinely new "diagonal stairs" mechanic. Sideways stairs (see below) keep working through their existing, unmodified logic; diagonal input is decomposed back to a single cardinal component before it ever reaches that code.

---

## Config

`include/config/overworld.h`:
```c
#define OW_DIAGONAL_MOVEMENT GEN_LATEST // In Gen6+, the player can move diagonally by holding two adjacent D-pad directions.
```
Matches this file's existing `GEN_LATEST`-style convention for generation-gated features. Checked in normal control flow (`if (GetConfig(OW_DIAGONAL_MOVEMENT) >= GEN_6)` or equivalent), not `#ifdef`, per project style.

## Architecture

### Input layer (`src/field_control_avatar.c`)

Replace `FieldGetPlayerInput`'s if/else-if cardinal chain with two independent checks - one for the vertical axis (`DPAD_UP`/`DPAD_DOWN`), one for the horizontal axis (`DPAD_LEFT`/`DPAD_RIGHT`). When the config is on and both a vertical and a horizontal key are held, combine them into the matching `DIR_NORTHEAST`/`DIR_NORTHWEST`/`DIR_SOUTHEAST`/`DIR_SOUTHWEST`. When the config is off, or only one axis is held, or both keys on one axis are held (cancelling out), behavior is byte-for-byte identical to today.

### Collision: centralized resolver

New function, `CanObjectEventMoveInDirection(struct ObjectEvent *objectEvent, enum Direction direction)`, added near the existing collision-check functions in `src/event_object_movement.c`. For a cardinal direction it's a thin wrapper around today's existing check (no behavior change). For a diagonal direction:

1. Decompose into its two cardinal components (e.g. `DIR_NORTHEAST` → `DIR_NORTH` + `DIR_EAST`).
2. Check the actual diagonal destination tile's collision (walkability, encounter/warp/etc. behavior) the same way a cardinal move would.
3. Enforce no-corner-cutting: at least one of the two flanking cardinal tiles must also be passable. If both flanks are blocked, the move is rejected even if the diagonal destination tile itself is open.

Both the player-input path and the NPC-wander path (below) call this same function, so the corner-cutting rule and collision semantics can't drift apart between them.

### Player movement wiring

`PlayerStep` / `MovePlayerNotOnBike` in `src/field_player_avatar.c` receive whatever direction `FieldGetPlayerInput` resolved (cardinal or diagonal) exactly the way they already do today for sideways-stairs movement. Given the movement primitives and animation tables already handle all 8 directions, this is expected to work with the existing code path once fed a diagonal direction outside the stairs-specific context - verified per call site during implementation rather than assumed, since `MovePlayerNotOnBike`'s dispatch table (`sPlayerNotOnBikeFuncs`) and related tables need to be checked for any place that implicitly assumes only 4 possible directions (e.g. array sized `[4]` instead of indexed by the full `enum Direction` range).

### NPC wander (`src/event_object_movement.c`)

The random-direction table used by wander-type movement behaviors (e.g. `MOVEMENT_TYPE_WANDER_AROUND`) extends from picking among 4 directions to picking among 8 when `OW_DIAGONAL_MOVEMENT` is on, validating the chosen direction through the same `CanObjectEventMoveInDirection` used by the player. When the config is off, behavior is unchanged.

### Sideways stairs interaction

Confirmed these are two independent mechanisms today: stairs are driven entirely by `GetCollisionAtCoords` checking the player's raw cardinal `dir` against explicit `DIR_EAST`/`DIR_WEST`/`DIR_NORTH`/`DIR_SOUTH` equality, then translating it into a diagonal `directionOverwrite` via `GetLeftSideStairsDirection`/`GetRightSideStairsDirection`. A genuinely diagonal `dir` value matches none of those equality checks, so it would fall straight through to `GetVanillaCollision`/our new `CanObjectEventMoveInDirection` instead - two diagonal systems that were never designed to meet.

Resolution: when the player is on or moving onto a sideways-stairs tile and the resolved input is diagonal, decompose it into its horizontal and vertical components before it reaches the stairs logic at all, and feed the **horizontal component first** (stairs are fundamentally a left/right-driven mechanic per `GetLeftSideStairsDirection`/`GetRightSideStairsDirection`'s explicit West/East-only switch cases). If the horizontal component isn't actually held, or the existing stairs checks reject it, fall back to the vertical component. `GetCollisionAtCoords` and the stairs translation functions themselves are not modified at all - this is purely a pre-processing decomposition step ahead of that existing, untouched code.

### Follower interaction

`FollowablePlayerMovement_Step` doesn't replay the player's direction directly - it moves the follower to `gObjectEvents[player].previousCoords` and calls `GetDirectionToFace(followerX, followerY, targetX, targetY)` to pick which way to step there. That function is cardinal-only by design (checks X first, returns `DIR_WEST`/`DIR_EAST` on any horizontal delta, only falls back to `DIR_NORTH`/`DIR_SOUTH` when X matches exactly) - it can never return a diagonal direction, even when the follower ends up diagonally offset from the player's last tile, which will now happen on every diagonal player step.

`GetDirectionToFace` is also used elsewhere, including script-exposed via `GetDirectionToFaceScript` for other "face toward" scripted behavior that should keep its current cardinal-only behavior. So the fix is a new function, not a behavior change to the shared one: add a diagonal-aware direction resolver used only by `FollowablePlayerMovement_Step` - when the follower's position differs from the player's previous tile in both axes by exactly one tile (which it always will be here, since the player only ever moves one tile per step), it returns the matching diagonal direction instead of collapsing to a cardinal one.

## Diagonal sprite frame support

Existing precedent (sideways stairs, diagonal bike animations) reuses the East/West cardinal sprite for every diagonal direction (`DIR_NORTHEAST`/`DIR_SOUTHEAST` → face-East frames, `DIR_NORTHWEST`/`DIR_SOUTHWEST` → face-West frames) - this remains the default and the fallback for every sprite that doesn't opt into more.

**Capability declaration:** add an optional field to `struct ObjectEventGraphicsInfo` (`include/global.fieldmap.h` or wherever that struct lives) - `bool8 hasDiagonalFrames` - defaulting to unset/false for every existing sprite entry. Nothing about an existing sprite sheet changes unless its entry explicitly opts in.

**Sprite layout:** a sheet that sets this flag is expected to carry the standard cardinal frame set plus 4 more (NE/NW/SE/SW), in a documented layout extension. New OAM/frame-index tables parallel to the existing cardinal ones.

**Animation tables:** new `ANIM_STD_FACE_NORTHEAST`-style entries (face, walk, run) that only get referenced for sprites with the capability flag set.

**Fallback logic:** every table lookup that currently resolves a diagonal direction via the E/W-substitution tables (`sFaceDirectionAnimNums` and its siblings in `event_object_movement.c`) gets a branch: if the active object event's graphics info has `hasDiagonalFrames` set, use the new diagonal table; otherwise, fall back to exactly today's behavior.

## Testing

- New tests for `CanObjectEventMoveInDirection`'s corner-cutting rule: blocked when both flanks are walls, allowed when both flanks are open, allowed when exactly one flank is open, and the plain-cardinal path is unaffected.
- New tests for `FieldGetPlayerInput`'s diagonal resolution: both-axis-held combinations produce the right diagonal direction with the config on, and produce today's single-cardinal-direction behavior with it off or with only one axis held.
- Confirm existing cardinal-movement, ledge, and wild-encounter tests still pass unchanged with `OW_DIAGONAL_MOVEMENT` on (encounter/tile-behavior triggers fire on arrival regardless of which direction the step came from, so these should be unaffected, but verified rather than assumed).
- NPC wander test: with the config on, a wandering object event picks and executes a diagonal step over enough trials, respecting the same corner-cutting rule as the player.
- Follower test: after a diagonal player step, the follower's new diagonal-aware direction resolver returns the correct diagonal direction and the follower ends up on the player's previous tile in one step (not two, not a wrong cardinal detour).
- Sideways-stairs test: diagonal input on a stairs tile resolves to the correct single cardinal component (horizontal preferred, vertical fallback) and produces byte-for-byte the same result as pressing that one cardinal key today; the existing stairs test coverage keeps passing unmodified.
