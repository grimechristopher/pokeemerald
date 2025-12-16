# Mossdeep Game Corner Integration (RECOMMENDED)

## Purpose

This document describes creating a new Game Corner in Mossdeep City to house all 9 new minigames while preserving Mauville's existing Slot Machines and Roulette tables.

**Recommendation:** ✅ This is the **recommended approach** for integrating new minigames.

## Why Mossdeep?

### Advantages

1. **Preserves Existing Content**
   - Mauville Game Corner remains untouched
   - Zero risk of breaking existing slot/roulette functionality
   - Players still have access to classic games

2. **Clean Separation**
   - Old games (slots, roulette) in Mauville
   - New games (9 minigames) in Mossdeep
   - Clear mental model for players

3. **Ample Space**
   - Can accommodate all 9 minigames comfortably
   - Room for future expansion
   - Two-floor design (1F = simple games, B1F = complex games)

4. **Thematic Fit**
   - Mossdeep is a modern, tech-focused city (Space Center)
   - "Next generation" minigames fit the theme
   - Lore: Space Center sponsors new Game Corner

5. **Future-Proof**
   - Mauville can receive updates independently
   - Mossdeep can receive updates independently
   - No conflicts or dependencies

### Disadvantages

1. **Player Travel**
   - ⚠️ Players must Fly/Surf to Mossdeep
   - ⚠️ Not accessible until mid-game (after Gym 7)
   - Mitigation: Unlocks align with progression

2. **Map Creation Work**
   - ⚠️ Must create 2 new maps (1F and B1F)
   - ⚠️ Moderate implementation effort
   - Mitigation: Clear template provided below

## Map Structure

### Two-Floor Design

**MossdeepCity_GameCorner_1F** (Main Floor)
- Coin Clerk
- Prize Counter
- Simple/quick minigames (Snake, Blackjack, Gacha)
- NPCs with hints/tutorials

**MossdeepCity_GameCorner_B1F** (Basement)
- Complex/long minigames (Flappy Bird, Block Stacker, Pinball, Pachinko, Voltorb Flip, Derby)
- More immersive atmosphere (dimmer lighting, arcade sounds)

### Floor Plans

**1F Layout (10x10 tiles):**

```
+------------------------------------------+
| [Exit to Mossdeep City]                 |
|                                          |
|  [Coin Clerk]           [Prize Counter] |
|                                          |
|  [Snake]     [Blackjack]    [Gacha]     |
|                             [Gacha]     |
|                                          |
|  [Stairs Down to B1F]                   |
|                                          |
|  [NPC: Tutorial]   [NPC: Hints]         |
+------------------------------------------+
```

**B1F Layout (15x10 tiles):**

```
+--------------------------------------------------------+
| [Stairs Up to 1F]                                     |
|                                                        |
|  [Flappy Bird]   [Block Stacker]   [Pinball]         |
|                                                        |
|  [Pachinko]      [Pachinko]        [Voltorb Flip]    |
|                                                        |
|  [Derby Racing Track - 3x3 tiles]                     |
|                                                        |
|  [NPC: Strategy Guide]   [NPC: High Scores]          |
+--------------------------------------------------------+
```

## Implementation Guide

### Step 1: Create Map Files

**Directory Structure:**

```bash
mkdir -p data/maps/MossdeepCity_GameCorner_1F
mkdir -p data/maps/MossdeepCity_GameCorner_B1F
```

**Files to Create:**

```bash
# 1F
touch data/maps/MossdeepCity_GameCorner_1F/map.json
touch data/maps/MossdeepCity_GameCorner_1F/scripts.pory

# B1F
touch data/maps/MossdeepCity_GameCorner_B1F/map.json
touch data/maps/MossdeepCity_GameCorner_B1F/scripts.pory
```

### Step 2: Create 1F Map

**File:** `data/maps/MossdeepCity_GameCorner_1F/map.json`

```json
{
  "id": "MAP_MOSSDEEP_CITY_GAME_CORNER_1F",
  "name": "MossdeepCity_GameCorner_1F",
  "layout": "LAYOUT_MOSSDEEP_CITY_GAME_CORNER_1F",
  "music": "MUS_GAME_CORNER",
  "region_map_section": "MAPSEC_MOSSDEEP_CITY",
  "requires_flash": false,
  "weather": "WEATHER_NONE",
  "map_type": "MAP_TYPE_INDOOR",
  "allow_cycling": false,
  "allow_escaping": false,
  "allow_running": true,
  "show_map_name": true,
  "battle_scene": "MAP_BATTLE_SCENE_NORMAL",
  "connections": null,
  "object_events": [
    {
      "graphics_id": "OBJ_EVENT_GFX_MAN_3",
      "x": 3,
      "y": 4,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_FACE_DOWN",
      "movement_range_x": 0,
      "movement_range_y": 0,
      "trainer_type": "TRAINER_TYPE_NONE",
      "trainer_sight_or_berry_tree_id": 0,
      "script": "MossdeepCity_GameCorner_1F_EventScript_CoinClerk",
      "flag": "0"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_WOMAN_1",
      "x": 8,
      "y": 4,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_FACE_DOWN",
      "script": "MossdeepCity_GameCorner_1F_EventScript_PrizeCounter"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 3,
      "y": 6,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_NONE",
      "script": "MossdeepCity_GameCorner_1F_EventScript_Snake"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 6,
      "y": 6,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_NONE",
      "script": "MossdeepCity_GameCorner_1F_EventScript_Blackjack"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 9,
      "y": 6,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_NONE",
      "script": "MossdeepCity_GameCorner_1F_EventScript_Gacha1"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 9,
      "y": 7,
      "elevation": 0,
      "movement_type": "MOVEMENT_TYPE_NONE",
      "script": "MossdeepCity_GameCorner_1F_EventScript_Gacha2"
    }
  ],
  "warp_events": [
    {
      "x": 5,
      "y": 12,
      "elevation": 0,
      "dest_map": "MAP_MOSSDEEP_CITY",
      "dest_warp_id": "0"
    },
    {
      "x": 5,
      "y": 9,
      "elevation": 0,
      "dest_map": "MAP_MOSSDEEP_CITY_GAME_CORNER_B1F",
      "dest_warp_id": "0"
    }
  ],
  "coord_events": [],
  "bg_events": []
}
```

### Step 3: Create 1F Scripts

**File:** `data/maps/MossdeepCity_GameCorner_1F/scripts.pory`

```pory
mapscripts MossdeepCity_GameCorner_1F_MapScripts {
    MAP_SCRIPT_ON_TRANSITION {
        setflag(FLAG_VISITED_MOSSDEEP_GAME_CORNER)
    }
}

// Coin Clerk (reuse Mauville pattern)
script MossdeepCity_GameCorner_1F_EventScript_CoinClerk {
    lock
    faceplayer
    msgbox("Welcome to the Mossdeep\nGame Corner!\p"
           "I can exchange money for\nCoins!", MSGBOX_DEFAULT)
    special(MossdeepGameCornerCoinClerk)
    waitstate
    release
    end
}

// Prize Counter
script MossdeepCity_GameCorner_1F_EventScript_PrizeCounter {
    lock
    faceplayer
    msgbox("Welcome! You can exchange\nyour Coins for prizes here!", MSGBOX_DEFAULT)
    special(ShowPrizeCounterMenu)
    waitstate
    release
    end
}

// Snake Minigame
script MossdeepCity_GameCorner_1F_EventScript_Snake {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_SNAKE)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("SNAKE\p"
           "Entry Cost: 10 Coins\p"
           "Play?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_SNAKE)
    special(Special_StartMinigame)
    releaseall
    end
}

// Blackjack Minigame
script MossdeepCity_GameCorner_1F_EventScript_Blackjack {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_BLACKJACK)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("BLACKJACK\p"
           "Entry Cost: 50 Coins\p"
           "Play?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_BLACKJACK)
    special(Special_StartMinigame)
    releaseall
    end
}

// Gacha Machines (1 & 2)
script MossdeepCity_GameCorner_1F_EventScript_Gacha1 {
    lockall
    setvar(VAR_0x8004, MINIGAME_GACHA)
    special(Special_StartMinigame)
    releaseall
    end
}

script MossdeepCity_GameCorner_1F_EventScript_Gacha2 {
    goto(MossdeepCity_GameCorner_1F_EventScript_Gacha1)
}

// Common messages
script MossdeepCity_GameCorner_EventScript_NoCoinCase {
    msgbox("You need a COIN CASE to\nplay minigames!", MSGBOX_DEFAULT)
    releaseall
    end
}

script MossdeepCity_GameCorner_EventScript_Locked {
    msgbox("This minigame is locked.\p"
           "Complete challenges to unlock\nnew games!", MSGBOX_DEFAULT)
    releaseall
    end
}

script MossdeepCity_GameCorner_EventScript_Declined {
    msgbox("Come back anytime!", MSGBOX_DEFAULT)
    releaseall
    end
}
```

### Step 4: Create B1F Map

**File:** `data/maps/MossdeepCity_GameCorner_B1F/map.json`

```json
{
  "id": "MAP_MOSSDEEP_CITY_GAME_CORNER_B1F",
  "name": "MossdeepCity_GameCorner_B1F",
  "layout": "LAYOUT_MOSSDEEP_CITY_GAME_CORNER_B1F",
  "music": "MUS_GAME_CORNER",
  "region_map_section": "MAPSEC_MOSSDEEP_CITY",
  "object_events": [
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 3,
      "y": 4,
      "script": "MossdeepCity_GameCorner_B1F_EventScript_FlappyBird"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 7,
      "y": 4,
      "script": "MossdeepCity_GameCorner_B1F_EventScript_BlockStacker"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 11,
      "y": 4,
      "script": "MossdeepCity_GameCorner_B1F_EventScript_Pinball"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 3,
      "y": 7,
      "script": "MossdeepCity_GameCorner_B1F_EventScript_Pachinko1"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 6,
      "y": 7,
      "script": "MossdeepCity_GameCorner_B1F_EventScript_Pachinko2"
    },
    {
      "graphics_id": "OBJ_EVENT_GFX_SLOT_MACHINE",
      "x": 11,
      "y": 7,
      "script": "MossdeepCity_GameCorner_B1F_EventScript_VoltorbFlip"
    }
  ],
  "warp_events": [
    {
      "x": 5,
      "y": 2,
      "dest_map": "MAP_MOSSDEEP_CITY_GAME_CORNER_1F",
      "dest_warp_id": "1"
    }
  ]
}
```

### Step 5: Create B1F Scripts

**File:** `data/maps/MossdeepCity_GameCorner_B1F/scripts.pory`

```pory
mapscripts MossdeepCity_GameCorner_B1F_MapScripts {
}

// Flappy Bird
script MossdeepCity_GameCorner_B1F_EventScript_FlappyBird {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_FLAPPYBIRD)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("FLAPPY BIRD\p"
           "Entry Cost: 20 Coins\p"
           "Play?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_FLAPPYBIRD)
    special(Special_StartMinigame)
    releaseall
    end
}

// Block Stacker
script MossdeepCity_GameCorner_B1F_EventScript_BlockStacker {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_BLOCK_STACKER)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("BLOCK STACKER\p"
           "Entry Cost: 30 Coins\p"
           "Play?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_BLOCK_STACKER)
    special(Special_StartMinigame)
    releaseall
    end
}

// Pinball
script MossdeepCity_GameCorner_B1F_EventScript_Pinball {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_PINBALL)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("PINBALL\p"
           "Entry Cost: 50 Coins\p"
           "Play?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_PINBALL)
    special(Special_StartMinigame)
    releaseall
    end
}

// Pachinko (2 machines)
script MossdeepCity_GameCorner_B1F_EventScript_Pachinko1 {
    lockall
    setvar(VAR_0x8004, MINIGAME_PACHINKO)
    special(Special_StartMinigame)
    releaseall
    end
}

script MossdeepCity_GameCorner_B1F_EventScript_Pachinko2 {
    goto(MossdeepCity_GameCorner_B1F_EventScript_Pachinko1)
}

// Voltorb Flip
script MossdeepCity_GameCorner_B1F_EventScript_VoltorbFlip {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_VOLTORB_FLIP)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("VOLTORB FLIP\p"
           "Free to play!\p"
           "Start?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_VOLTORB_FLIP)
    special(Special_StartMinigame)
    releaseall
    end
}

// Derby (large 3x3 object)
script MossdeepCity_GameCorner_B1F_EventScript_Derby {
    lockall
    checkitem(ITEM_COIN_CASE)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase)

    setvar(VAR_0x8004, MINIGAME_DERBY)
    special(Special_IsMinigameUnlocked)
    goto_if_eq(VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_Locked)

    msgbox("POKEMON DERBY\p"
           "Entry Cost: 100 Coins\p"
           "Enter race?", MSGBOX_YESNO)
    goto_if_eq(VAR_RESULT, NO, MossdeepCity_GameCorner_EventScript_Declined)

    setvar(VAR_0x8004, MINIGAME_DERBY)
    special(Special_StartMinigame)
    releaseall
    end
}
```

### Step 6: Add Warp to Mossdeep City

**Edit:** `data/maps/MossdeepCity/map.json`

Add warp event to Game Corner entrance:

```json
{
  "object_events": [
    {
      "graphics_id": "OBJ_EVENT_GFX_BUILDING",
      "x": 20,
      "y": 15,
      "script": "NULL",
      "flag": "0"
    }
  ],
  "warp_events": [
    {
      "x": 20,
      "y": 16,
      "dest_map": "MAP_MOSSDEEP_CITY_GAME_CORNER_1F",
      "dest_warp_id": "0"
    }
  ]
}
```

### Step 7: Update Map Constants

**Edit:** `include/constants/map_groups.h`

```c
// Mossdeep City maps
#define MAP_MOSSDEEP_CITY (MAP_GROUP_MOSSDEEP << 8 | 0)
#define MAP_MOSSDEEP_CITY_GAME_CORNER_1F (MAP_GROUP_MOSSDEEP << 8 | 10)   // ADD
#define MAP_MOSSDEEP_CITY_GAME_CORNER_B1F (MAP_GROUP_MOSSDEEP << 8 | 11)  // ADD
```

### Step 8: Register in Map Groups

**Edit:** `data/maps/map_groups.json`

```json
{
  "group_name": "gMapGroup_Mossdeep",
  "maps": [
    "MossdeepCity",
    "MossdeepCity_Gym",
    "MossdeepCity_House1",
    "MossdeepCity_SpaceCenter_1F",
    "MossdeepCity_SpaceCenter_2F",
    "MossdeepCity_GameCorner_1F",    // ADD
    "MossdeepCity_GameCorner_B1F"    // ADD
  ]
}
```

### Step 9: Create Tileset/Layout

**Option A: Reuse Mauville Tileset**

```c
// data/layouts/layouts.json (add)
{
  "id": "LAYOUT_MOSSDEEP_CITY_GAME_CORNER_1F",
  "name": "MossdeepCity_GameCorner_1F",
  "width": 10,
  "height": 12,
  "primary_tileset": "gTileset_Building",
  "secondary_tileset": "gTileset_MauvilleGameCorner",  // Reuse
  "border_filepath": "data/layouts/MossdeepCity_GameCorner_1F/border.bin",
  "blockdata_filepath": "data/layouts/MossdeepCity_GameCorner_1F/map.bin"
}
```

**Option B: Custom Tileset (Advanced)**

Create custom tileset with modern/tech aesthetic to match Mossdeep's theme.

## NPC Dialogue

### Flavor NPCs (1F)

```pory
script MossdeepCity_GameCorner_1F_EventScript_Visitor1 {
    msgbox("This Game Corner is sponsored\n"
           "by the Mossdeep Space Center!\p"
           "The latest technology in\n"
           "entertainment!", MSGBOX_NPC)
    end
}

script MossdeepCity_GameCorner_1F_EventScript_Visitor2 {
    msgbox("I just got the high score on\n"
           "SNAKE! 999 points!\p"
           "Check the leaderboard upstairs!", MSGBOX_NPC)
    end
}
```

### Tutorial NPC

```pory
script MossdeepCity_GameCorner_1F_EventScript_Tutorial {
    msgbox("New to the Game Corner?\p"
           "Buy Coins from the clerk, then\n"
           "play minigames to win more!\p"
           "Exchange Coins for prizes at\n"
           "the counter!", MSGBOX_NPC)
    end
}
```

## Unlock Progression

### Default State (All Locked)

```pory
// On first visit, all minigames locked except one starter

mapscripts MossdeepCity_GameCorner_1F_MapScripts {
    MAP_SCRIPT_ON_TRANSITION {
        call(MossdeepCity_GameCorner_UnlockStarter)
    }
}

script MossdeepCity_GameCorner_UnlockStarter {
    goto_if_set(FLAG_VISITED_MOSSDEEP_GAME_CORNER, Common_EventScript_NopReturn)
    setflag(FLAG_VISITED_MOSSDEEP_GAME_CORNER)

    // Unlock Snake as starter minigame
    setflag(FLAG_UNLOCKED_GAMECORNER_SNAKE)
    msgbox("Welcome to the Mossdeep Game\n"
           "Corner!\p"
           "Try playing SNAKE for free!", MSGBOX_SIGN)
    return
}
```

### Unlock Triggers

```pory
// Unlock Flappy Bird after beating Gym 7
script MossdeepCity_Gym_EventScript_UnlockFlappyBird {
    // ... after Tate & Liza defeated ...
    setflag(FLAG_UNLOCKED_GAMECORNER_FLAPPYBIRD)
    msgbox("Congratulations on defeating\n"
           "Tate & Liza!\p"
           "FLAPPY BIRD has been unlocked\n"
           "at the Game Corner!", MSGBOX_SIGN)
}

// Unlock Blackjack after defeating Elite Four
script EventScript_HallOfFame_UnlockBlackjack {
    // ... after entering Hall of Fame ...
    setflag(FLAG_UNLOCKED_GAMECORNER_BLACKJACK)
}

// Unlock all minigames after becoming Champion
script EventScript_BecomeChampion_UnlockAll {
    // ... after becoming Champion ...
    special(GameCorner_UnlockAll)
    msgbox("All minigames unlocked!", MSGBOX_SIGN)
}
```

## Testing Checklist

- [ ] Game Corner entrance warps correctly from Mossdeep City
- [ ] 1F exits correctly back to Mossdeep City
- [ ] Stairs from 1F to B1F work
- [ ] Stairs from B1F to 1F work
- [ ] Coin Clerk functions (buy coins)
- [ ] Prize Counter functions (exchange prizes)
- [ ] All 9 minigame scripts trigger correctly
- [ ] Locked minigames show proper message
- [ ] Unlocked minigames launch correctly
- [ ] NPC dialogue works
- [ ] Music plays (MUS_GAME_CORNER)
- [ ] No graphical glitches
- [ ] No collision errors (can walk freely)

## Documentation Cross-References

**Related Documents:**
- `MAUVILLE_INTEGRATION.md` - Alternative integration option
- `PRIZE_SYSTEM.md` - Prize counter setup
- `03_COMMON_INFRASTRUCTURE.md` - Special functions used in scripts

**Map Editing Tools:**
- Porymap: https://github.com/huderlem/porymap
- Poryscript: https://github.com/huderlem/poryscript

## Summary

**Mossdeep Game Corner provides:**
- ✅ Clean separation from Mauville (preserves existing content)
- ✅ Ample space for all 9 minigames
- ✅ Two-floor design (1F = simple, B1F = complex)
- ✅ Thematic fit (modern city, tech theme)
- ✅ Room for future expansion
- ✅ Progressive unlock system aligned with game progression

**Implementation Steps:**
1. Create 2 maps (1F and B1F)
2. Add scripts for all 9 minigames
3. Add warp from Mossdeep City
4. Add Coin Clerk and Prize Counter
5. Implement unlock progression
6. Test all warps and interactions

**Estimated Work:** ~8-12 hours (map creation, scripting, testing)

---

**Last Updated:** 2025-12-16
**Status:** Design Complete - Recommended Integration Path
**Recommendation:** ✅ Use this approach for implementing all 9 minigames
