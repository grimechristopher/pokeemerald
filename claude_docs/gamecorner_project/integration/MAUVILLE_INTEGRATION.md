# Mauville Game Corner Integration

## Purpose

This document describes how to optionally add new minigames to the existing Mauville City Game Corner, which currently has 12 Slot Machines and 2 Roulette tables.

**Note:** The primary plan uses Mossdeep City for new minigames (see `MOSSDEEP_INTEGRATION.md`). This document is for users who prefer to add games to Mauville instead.

## Current Mauville Game Corner State

### Location

**Map:** `data/maps/MauvilleCity_GameCorner/`

**Files:**
- `map.json` - Map layout, warp tiles, object events
- `scripts.inc` - 778 lines of event scripts (Pory compiled to ASM)
- `scripts.pory` - Original Pory source (if available)

### Existing Games

**Slot Machines:** 12 machines across 2 rows
- 6 difficulty tiers × 2 machines per tier
- Implementation: `src/slot_machine.c` (3,916 lines)
- Graphics: `graphics/slot_machine/` (reels, interface, animations)

**Roulette Tables:** 2 tables
- Implementation: `src/roulette.c` (4,158 lines)
- Graphics: `graphics/roulette/` (wheel, board, chips)

**NPCs:**
- Coin Clerk (buys 50 coins for ₽1000)
- Prize Counter Clerk (exchanges coins for prizes)
- Various flavor NPCs (gamblers, spectators)

### Map Layout

```
+------------------------------------------+
|  Entrance (stairs to outside)           |
|                                          |
|  [Slot] [Slot] [Slot] [Slot] [Slot] [Slot]  |
|  [Slot] [Slot] [Slot] [Slot] [Slot] [Slot]  |
|                                          |
|  [Roulette]           [Roulette]        |
|                                          |
|  [Coin Clerk]       [Prize Counter]     |
|                                          |
|  [NPCs talking about gambling]          |
+------------------------------------------+
```

**Available Space:** Limited - most tiles occupied by existing machines/NPCs.

## Integration Options

### Option A: Replace Existing Machines (NOT RECOMMENDED)

**Approach:** Remove some slot machines/roulette tables, add new minigames.

**Pros:**
- ✅ Reuses existing map space
- ✅ Single Game Corner location

**Cons:**
- ❌ Removes existing content (community backlash risk)
- ❌ Existing slot/roulette fans lose features
- ❌ Not consistent with RHH's "add, don't remove" philosophy

**Verdict:** ❌ Avoid unless explicitly desired by user.

### Option B: Expand Mauville Map (MODERATE COMPLEXITY)

**Approach:** Add a second floor or basement to Mauville Game Corner.

**Pros:**
- ✅ Preserves existing content
- ✅ Centralized Game Corner location
- ✅ Maintains Mauville as gambling hub

**Cons:**
- ⚠️ Requires map editing (stairs, warps, new floor layout)
- ⚠️ Moderate complexity (map design, NPC placement)
- ⚠️ Cannon consistency (Mauville doesn't have multi-floor Game Corner in vanilla)

**Verdict:** ✅ Viable if user prefers Mauville over Mossdeep.

### Option C: Add Machines to Existing Space (LEAST RECOMMENDED)

**Approach:** Cram 1-2 small machines into corners/empty tiles.

**Pros:**
- ✅ No map expansion needed
- ✅ Preserves existing content

**Cons:**
- ❌ Very limited space (maybe 1-2 games max)
- ❌ Cluttered map layout
- ❌ Doesn't accommodate all 9 new minigames

**Verdict:** ❌ Only suitable for adding 1-2 minigames, not full suite.

### Option D: Use Mossdeep Instead (RECOMMENDED)

**Approach:** Add new minigames to Mossdeep City Game Corner.

**Pros:**
- ✅ Preserves Mauville exactly as-is (zero risk)
- ✅ Clean separation (old vs new games)
- ✅ Plenty of space for all 9 minigames
- ✅ Allows future Mauville updates without conflicts

**Cons:**
- ⚠️ Players must travel to Mossdeep for new games
- ⚠️ Requires creating new map (moderate work)

**Verdict:** ✅ **Recommended approach** - see `MOSSDEEP_INTEGRATION.md`.

## Implementation: Expand Mauville (Option B)

If user chooses to expand Mauville, follow this guide.

### Step 1: Create Second Floor Map

**File:** `data/maps/MauvilleCity_GameCorner_2F/map.json`

**Dimensions:** 10x10 tiles (similar to 1F)

**Layout Pattern:**

```
+------------------------------------------+
|  [Stairs Down to 1F]                    |
|                                          |
|  [Snake]    [Flappy Bird]  [Blackjack]  |
|  [Gacha]                   [Gacha]      |
|                                          |
|  [Block Stacker]    [Pinball]           |
|  [Pachinko]         [Voltorb Flip]      |
|                                          |
|  [Derby Racing]                          |
+------------------------------------------+
```

**Create Files:**

```bash
mkdir -p data/maps/MauvilleCity_GameCorner_2F
touch data/maps/MauvilleCity_GameCorner_2F/map.json
touch data/maps/MauvilleCity_GameCorner_2F/scripts.pory
```

**Tileset:** Reuse `gTileset_MauvilleGameCorner` for consistency.

### Step 2: Add Stairs to 1F

**Edit:** `data/maps/MauvilleCity_GameCorner/map.json`

**Add Warp Tile:**

```json
{
  "warp_events": [
    {
      "dest_map": "MAP_MAUVILLE_CITY",
      "dest_warp_id": 0,
      "elevation": 0,
      "x": 5,
      "y": 12
    },
    {
      "dest_map": "MAP_MAUVILLE_CITY_GAME_CORNER_2F",
      "dest_warp_id": 0,
      "elevation": 0,
      "x": 10,
      "y": 2
    }
  ]
}
```

**Add Stair Tiles:** Place stair metatiles at (10, 2) on 1F map.

### Step 3: Add Warp on 2F

**Edit:** `data/maps/MauvilleCity_GameCorner_2F/map.json`

```json
{
  "warp_events": [
    {
      "dest_map": "MAP_MAUVILLE_CITY_GAME_CORNER",
      "dest_warp_id": 1,
      "elevation": 0,
      "x": 5,
      "y": 12
    }
  ]
}
```

### Step 4: Add Minigame Scripts

**File:** `data/maps/MauvilleCity_GameCorner_2F/scripts.pory`

```pory
mapscripts MauvilleCity_GameCorner_2F_MapScripts {
    MAP_SCRIPT_ON_TRANSITION {
        // Optional: set visited flag
        setflag(FLAG_VISITED_MAUVILLE_GAME_CORNER_2F)
    }
}

script MauvilleCity_GameCorner_2F_EventScript_Snake {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MauvilleCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_SNAKE)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MauvilleCity_GameCorner_EventScript_MinigameLocked)

    msgbox("Play SNAKE for 10 Coins?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MauvilleCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_SNAKE)
    special(Special_StartMinigame)
    releaseall
    end
}

// ... repeat for all 9 minigames ...

script MauvilleCity_GameCorner_EventScript_NoCoinCase {
    msgbox("You need a COIN CASE\nto play!", MSGBOX_DEFAULT)
    releaseall
    end
}

script MauvilleCity_GameCorner_EventScript_MinigameLocked {
    msgbox("This game is currently\nlocked.", MSGBOX_DEFAULT)
    releaseall
    end
}

script MauvilleCity_GameCorner_EventScript_Declined {
    msgbox("Come back anytime!", MSGBOX_DEFAULT)
    releaseall
    end
}
```

### Step 5: Add Object Events

**Edit:** `data/maps/MauvilleCity_GameCorner_2F/map.json`

```json
{
  "object_events": [
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 3,
      "y": 4,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_NONE",
      "script": "MauvilleCity_GameCorner_2F_EventScript_Snake"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 6,
      "y": 4,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_NONE",
      "script": "MauvilleCity_GameCorner_2F_EventScript_FlappyBird"
    }
    // ... add all 9 minigame machines ...
  ]
}
```

**Graphics:** Reuse `OBJ_EVENT_GFX_SLOT_MACHINE` or create custom machine sprites.

### Step 6: Update Map Constants

**Edit:** `include/constants/map_groups.h`

```c
// Mauville City maps
#define MAP_MAUVILLE_CITY (MAP_GROUP_MAUVILLE_CITY << 8 | 0)
#define MAP_MAUVILLE_CITY_GAME_CORNER (MAP_GROUP_MAUVILLE_CITY << 8 | 1)
#define MAP_MAUVILLE_CITY_GAME_CORNER_2F (MAP_GROUP_MAUVILLE_CITY << 8 | 2)  // ADD THIS
```

### Step 7: Register Map

**Edit:** `data/maps/map_groups.json`

```json
{
  "group_name": "gMapGroup_MauvilleCity",
  "maps": [
    "MauvilleCity",
    "MauvilleCity_GameCorner",
    "MauvilleCity_GameCorner_2F"  // ADD THIS
  ]
}
```

### Step 8: Add NPC Dialogue

**Optional:** Add NPCs on 2F to guide players:

```pory
script MauvilleCity_GameCorner_2F_EventScript_Guide {
    msgbox("Welcome to the second floor!\p"
           "We have all the latest minigames\n"
           "from across the regions!", MSGBOX_NPC)
    end
}
```

### Step 9: Update Existing NPCs

**Edit:** `data/maps/MauvilleCity_GameCorner/scripts.pory`

Add dialogue hinting at 2F:

```pory
script MauvilleCity_GameCorner_EventScript_Gambler {
    msgbox("I heard they opened a second\n"
           "floor with new games!\p"
           "Check out the stairs in the\n"
           "back!", MSGBOX_NPC)
    end
}
```

## Preserving Existing Content

### Do NOT Modify These Files

**Critical - Leave Untouched:**
- `src/slot_machine.c` - Existing slot machine logic
- `src/roulette.c` - Existing roulette logic
- `graphics/slot_machine/` - Slot machine graphics
- `graphics/roulette/` - Roulette graphics

### Safe to Modify

**Can Edit (Carefully):**
- `data/maps/MauvilleCity_GameCorner/map.json` - Add stairs only
- `data/maps/MauvilleCity_GameCorner/scripts.pory` - Add NPC hints

## Testing Checklist

- [ ] Stairs on 1F warp to 2F correctly
- [ ] Stairs on 2F warp back to 1F correctly
- [ ] All existing slot machines still work
- [ ] All existing roulette tables still work
- [ ] Coin Clerk still functions
- [ ] Prize Counter still functions
- [ ] New minigames on 2F are accessible
- [ ] NPC dialogue updated (if applicable)

## Comparison: Mauville vs Mossdeep

| Factor | Mauville Expansion | Mossdeep New Hub |
|--------|-------------------|------------------|
| Preserves Existing | ✅ Yes (2F separate) | ✅ Yes (different city) |
| Implementation Work | ⚠️ Medium (map expansion) | ⚠️ Medium (new map) |
| Canon Consistency | ⚠️ Moderate (2F not in vanilla) | ✅ High (new location) |
| Space for 9 Games | ✅ Yes (2F can fit all) | ✅ Yes (ample space) |
| Player Travel | ✅ Already in Mauville | ⚠️ Must Fly to Mossdeep |
| Future Flexibility | ⚠️ Limited | ✅ High |
| **Recommendation** | ✅ If user prefers Mauville | ✅ **Default choice** |

## Alternatives Within Mauville

### Add Small Arcade Room

**Approach:** Create `MauvilleCity_GameCorner_Arcade` as side room.

**Layout:** Single-screen room with 3-4 minigames (Snake, Flappy Bird, Blackjack).

**Access:** Doorway on left/right side of main Game Corner.

**Pros:**
- ✅ Less invasive than full 2F
- ✅ Fits "arcade" theme well

**Cons:**
- ⚠️ Only fits 3-4 games (not all 9)
- ⚠️ Requires map editor work

## Documentation Cross-References

**Related Documents:**
- `MOSSDEEP_INTEGRATION.md` - Recommended alternative location
- `PRIZE_SYSTEM.md` - Prize counter updates for new minigames
- `03_COMMON_INFRASTRUCTURE.md` - Shared minigame code

**Map Editing Resources:**
- Porymap: https://github.com/huderlem/porymap
- RHH Map Tutorial: (if available in docs/)

## Summary

**Mauville Integration Approaches:**

1. **Option B (Expand to 2F):** Best if user wants centralized Game Corner
   - Moderate work: Add stairs, create 2F map, add minigame scripts
   - Preserves all existing content
   - Fits all 9 minigames

2. **Option D (Use Mossdeep):** **Recommended**
   - Clean separation of old vs new content
   - Zero risk to existing Mauville
   - See `MOSSDEEP_INTEGRATION.md` for details

**User Choice:** Decide based on preferences (centralized vs separated Game Corners).

---

**Last Updated:** 2025-12-16
**Status:** Design Complete - Optional Integration Path
**Recommendation:** Use Mossdeep (see MOSSDEEP_INTEGRATION.md) unless user specifically wants Mauville expansion
