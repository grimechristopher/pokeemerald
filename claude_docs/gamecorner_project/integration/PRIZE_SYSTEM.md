# Prize System Integration

## Purpose

This document describes how to integrate new prizes and update the Prize Counter system for both Mauville and Mossdeep Game Corners.

## Existing Prize System (Mauville)

### Current Implementation

**File:** `src/script_menu.c` (Prize Counter functionality)

**Function:** `ShowPrizeCounterMenu()`

**Pattern:** Uses script commands to display prize menus:
- `multichoice` for prize selection
- `checkcoins` to verify sufficient funds
- `takecoins` to deduct cost
- `giveitem` or `givemon` to award prize

### Existing Prizes (Mauville)

**From:** `data/maps/MauvilleCity_GameCorner/scripts.pory`

**Dolls (Left Counter):**
- Treecko Doll (1,000 Coins)
- Torchic Doll (1,000 Coins)
- Mudkip Doll (1,000 Coins)
- Pikachu Doll (2,500 Coins)
- Meowth Doll (5,000 Coins)

**Items (Right Counter):**
- Smoke Ball (800 Coins)
- Miracle Seed (1,000 Coins)
- Charcoal (1,000 Coins)
- Mystic Water (1,000 Coins)
- Yellow Flute (1,600 Coins)

**Pokemon (Right Counter, post-National Dex):**
- Pikachu holding Light Ball (2,500 Coins)
- Dratini holding Dragon Scale (4,000 Coins)
- Porygon (6,500 Coins)

### Script Pattern

```pory
script MauvilleCity_GameCorner_EventScript_PrizeCounterDolls {
    lock
    faceplayer
    msgbox("Which prize would you like?")
    multichoice(0, 0, MULTI_GAME_CORNER_DOLLS, FALSE)
    switch(var(VAR_RESULT)) {
        case 0: // Treecko Doll
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 1000)
            goto_if_lt(MauvilleCity_GameCorner_EventScript_NotEnoughCoins)
            takecoins(1000)
            giveitem(ITEM_TREECKO_DOLL)
            // ...
        case 1: // Torchic Doll
            // ...
    }
}
```

## New Prize Strategy

### Option A: Shared Prize Pool (RECOMMENDED)

**Approach:** Both Mauville and Mossdeep share the same prize list.

**Pros:**
- ✅ Consistent experience across locations
- ✅ Simpler implementation (one prize script)
- ✅ No confusion about where to get specific items

**Cons:**
- ⚠️ Less variety/uniqueness per location

**Implementation:** Copy Mauville's prize scripts to Mossdeep.

### Option B: Location-Exclusive Prizes

**Approach:** Each Game Corner has unique prizes.

**Mauville Prizes:**
- Keep existing dolls and items
- "Classic" Gen 3 themed rewards

**Mossdeep Prizes:**
- New minigame-themed items
- "Modern" Gen 4+ themed rewards
- Example: Master Ball (999,999 Coins - endgame flex)

**Pros:**
- ✅ Rewards exploration (players visit both locations)
- ✅ Thematic fit (classic vs modern)
- ✅ More total prizes available

**Cons:**
- ⚠️ More complex to balance
- ⚠️ Players may prefer one location's prizes

### Option C: Minigame-Specific Rewards

**Approach:** Each minigame awards unique prizes directly (no counter).

**Example:**
- Snake high score 500+ → Berry farming item
- Flappy Bird high score 100+ → Flying-type held item
- Derby 1st place → Rare Candy

**Pros:**
- ✅ Direct reward incentive
- ✅ Unique achievements per game
- ✅ No coin economy needed

**Cons:**
- ❌ Breaks existing Prize Counter pattern
- ❌ Harder to balance (9 games × prizes)
- ❌ Confusing for players (where do I get X?)

**Verdict:** ❌ Not recommended (too divergent from existing system).

## Recommended Approach: Option A + B Hybrid

**Implementation:**

1. **Mauville:** Keep existing prizes unchanged (preserves vanilla experience)
2. **Mossdeep:** Add new prize tier with modern rewards
3. **Both locations:** Share some common prizes (Rare Candy, evolution items)

### New Prize Tiers (Mossdeep)

**Tier 1: Minigame Tokens (100-500 Coins)**
- Trophy items with no gameplay effect
- Example: "Snake Trophy", "Flappy Bird Medal"
- Collectible for completionists

**Tier 2: Useful Items (1,000-3,000 Coins)**
- Rare evolution items (Upgrade, Dubious Disc, Reaper Cloth)
- Type-enhancing items (Wise Glasses, Muscle Band)
- Battle items (Focus Sash, Air Balloon)

**Tier 3: Pokemon (4,000-10,000 Coins)**
- Eevee holding Soothe Bell (4,000)
- Larvitar holding Hard Stone (5,000)
- Beldum holding Metal Coat (8,000)
- Gible holding Dragon Fang (10,000)

**Tier 4: Ultimate Rewards (50,000+ Coins)**
- Master Ball (99,999 Coins - endgame grind)
- Shiny Charm (100,000 Coins - if not obtained elsewhere)
- Golden Treecko Doll (50,000 Coins - prestige item)

## Implementation Guide

### Step 1: Define New Prize List

**File:** `include/constants/game_corner.h`

```c
// Prize Counter multichoice IDs
enum MossdeepPrizeCounterCategory {
    MOSSDEEP_PRIZES_TROPHIES,
    MOSSDEEP_PRIZES_ITEMS,
    MOSSDEEP_PRIZES_POKEMON,
    MOSSDEEP_PRIZES_ULTIMATE,
    MOSSDEEP_PRIZES_COUNT
};

// Trophy items (new decorations)
#define ITEM_SNAKE_TROPHY        ITEM_TM01  // Placeholder, replace with actual ID
#define ITEM_FLAPPYBIRD_MEDAL    ITEM_TM02
#define ITEM_BLACKJACK_CHIP      ITEM_TM03
// ... etc

// Prize costs
#define PRIZE_COST_SNAKE_TROPHY       100
#define PRIZE_COST_EEVEE              4000
#define PRIZE_COST_GIBLE              10000
#define PRIZE_COST_MASTER_BALL        99999
```

### Step 2: Create Prize Counter Script (Mossdeep)

**File:** `data/maps/MossdeepCity_GameCorner_1F/scripts.pory`

```pory
script MossdeepCity_GameCorner_1F_EventScript_PrizeCounter {
    lock
    faceplayer
    msgbox("Welcome to the Prize Counter!\p"
           "You have {STR_VAR_1} Coins.\p"
           "Which category interests you?")

    // Display current coins
    checkcoins(VAR_TEMP_0)
    buffernumberstring(STR_VAR_1, VAR_TEMP_0)

    // Category selection
    multichoicegrid(0, 0, MULTI_MOSSDEEP_PRIZE_CATEGORIES, 2, FALSE)
    switch(var(VAR_RESULT)) {
        case 0: // Trophies
            call(MossdeepCity_GameCorner_EventScript_TrophyPrizes)
        case 1: // Items
            call(MossdeepCity_GameCorner_EventScript_ItemPrizes)
        case 2: // Pokemon
            call(MossdeepCity_GameCorner_EventScript_PokemonPrizes)
        case 3: // Ultimate
            call(MossdeepCity_GameCorner_EventScript_UltimatePrizes)
        case MULTI_B_PRESSED:
            goto(MossdeepCity_GameCorner_EventScript_PrizeCounterDeclined)
    }
    release
    end
}

// Trophy Prizes (100-500 Coins)
script MossdeepCity_GameCorner_EventScript_TrophyPrizes {
    msgbox("Trophies commemorate your\nachievements!\p"
           "Which trophy would you like?")

    multichoice(0, 0, MULTI_MOSSDEEP_TROPHIES, FALSE)
    switch(var(VAR_RESULT)) {
        case 0: // Snake Trophy - 100 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 100)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)
            takecoins(100)
            giveitem(ITEM_SNAKE_TROPHY)
            msgbox("Obtained a SNAKE TROPHY!")

        case 1: // Flappy Bird Medal - 200 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 200)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)
            takecoins(200)
            giveitem(ITEM_FLAPPYBIRD_MEDAL)

        // ... etc for all 9 minigames
    }
    return
}

// Item Prizes (1,000-3,000 Coins)
script MossdeepCity_GameCorner_EventScript_ItemPrizes {
    msgbox("We have rare items available!\p"
           "Which item would you like?")

    multichoice(0, 0, MULTI_MOSSDEEP_ITEMS, FALSE)
    switch(var(VAR_RESULT)) {
        case 0: // Upgrade - 1,500 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 1500)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)
            takecoins(1500)
            giveitem(ITEM_UPGRADE)
            msgbox("Obtained an UPGRADE!\p"
                   "Trade to a PORYGON while\nholding this item!")

        case 1: // Dubious Disc - 2,000 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 2000)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)
            takecoins(2000)
            giveitem(ITEM_DUBIOUS_DISC)

        case 2: // Reaper Cloth - 2,000 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 2000)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)
            takecoins(2000)
            giveitem(ITEM_REAPER_CLOTH)

        // ... etc
    }
    return
}

// Pokemon Prizes (4,000-10,000 Coins)
script MossdeepCity_GameCorner_EventScript_PokemonPrizes {
    msgbox("We have rare POKéMON!\p"
           "Which POKéMON would you like?")

    multichoice(0, 0, MULTI_MOSSDEEP_POKEMON, FALSE)
    switch(var(VAR_RESULT)) {
        case 0: // Eevee - 4,000 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 4000)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)

            // Check party/box space
            specialvar(VAR_RESULT, CalculatePlayerPartyCount)
            compare(VAR_RESULT, PARTY_SIZE)
            goto_if_eq(MossdeepCity_GameCorner_EventScript_NoRoomForPokemon)

            takecoins(4000)
            givemon(SPECIES_EEVEE, 25, ITEM_SOOTHE_BELL)
            setflag(FLAG_RECEIVED_EEVEE_FROM_GAME_CORNER)
            msgbox("Obtained an EEVEE!")

        case 1: // Larvitar - 5,000 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 5000)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)

            specialvar(VAR_RESULT, CalculatePlayerPartyCount)
            compare(VAR_RESULT, PARTY_SIZE)
            goto_if_eq(MossdeepCity_GameCorner_EventScript_NoRoomForPokemon)

            takecoins(5000)
            givemon(SPECIES_LARVITAR, 25, ITEM_HARD_STONE)
            setflag(FLAG_RECEIVED_LARVITAR_FROM_GAME_CORNER)

        case 2: // Beldum - 8,000 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 8000)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)

            specialvar(VAR_RESULT, CalculatePlayerPartyCount)
            compare(VAR_RESULT, PARTY_SIZE)
            goto_if_eq(MossdeepCity_GameCorner_EventScript_NoRoomForPokemon)

            takecoins(8000)
            givemon(SPECIES_BELDUM, 25, ITEM_METAL_COAT)
            setflag(FLAG_RECEIVED_BELDUM_FROM_GAME_CORNER)

        case 3: // Gible - 10,000 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 10000)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)

            specialvar(VAR_RESULT, CalculatePlayerPartyCount)
            compare(VAR_RESULT, PARTY_SIZE)
            goto_if_eq(MossdeepCity_GameCorner_EventScript_NoRoomForPokemon)

            takecoins(10000)
            givemon(SPECIES_GIBLE, 25, ITEM_DRAGON_FANG)
            setflag(FLAG_RECEIVED_GIBLE_FROM_GAME_CORNER)
    }
    return
}

// Ultimate Prizes (50,000+ Coins)
script MossdeepCity_GameCorner_EventScript_UltimatePrizes {
    msgbox("These are our most prestigious\nrewards!\p"
           "They require exceptional skill\nand dedication!")

    multichoice(0, 0, MULTI_MOSSDEEP_ULTIMATE, FALSE)
    switch(var(VAR_RESULT)) {
        case 0: // Master Ball - 99,999 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 99999)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)

            takecoins(99999)
            giveitem(ITEM_MASTER_BALL)
            msgbox("Obtained a MASTER BALL!\p"
                   "The ultimate POKé BALL that\nnever fails!")

        case 1: // Golden Treecko Doll - 50,000 Coins
            checkcoins(VAR_TEMP_0)
            compare(VAR_TEMP_0, 50000)
            goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)

            takecoins(50000)
            giveitem(ITEM_GOLDEN_TREECKO_DOLL)
            setflag(FLAG_RECEIVED_GOLDEN_DOLL)
            msgbox("Obtained a GOLDEN TREECKO\nDOLL!\p"
                   "A symbol of your mastery!")
    }
    return
}

// Error messages
script MossdeepCity_GameCorner_EventScript_NotEnoughCoins {
    msgbox("You don't have enough Coins\nfor that prize.")
    return
}

script MossdeepCity_GameCorner_EventScript_NoRoomForPokemon {
    msgbox("Your party is full!")
    return
}

script MossdeepCity_GameCorner_EventScript_PrizeCounterDeclined {
    msgbox("Come back anytime!")
    return
}
```

### Step 3: Add Multichoice Definitions

**File:** `include/constants/game_corner.h`

```c
// Add to multichoice constants
#define MULTI_MOSSDEEP_PRIZE_CATEGORIES  100
#define MULTI_MOSSDEEP_TROPHIES          101
#define MULTI_MOSSDEEP_ITEMS             102
#define MULTI_MOSSDEEP_POKEMON           103
#define MULTI_MOSSDEEP_ULTIMATE          104
```

**File:** `src/data/script_menu.c`

```c
// Add multichoice text arrays
static const u8 *const sMossdeepPrizeCategoryNames[] = {
    gText_Trophies,
    gText_Items,
    gText_Pokemon,
    gText_Ultimate,
};

static const u8 *const sMossdeepTrophyNames[] = {
    gText_SnakeTrophy,
    gText_FlappyBirdMedal,
    gText_BlackjackChip,
    gText_VoltorbFlipMedal,
    gText_GachaToken,
    gText_PachinkoBall,
    gText_StackerTrophy,
    gText_PinballFlipperMedal,
    gText_DerbyRibbon,
};

static const u8 *const sMossdeepItemNames[] = {
    gText_Upgrade,
    gText_DubiousDisc,
    gText_ReaperCloth,
    gText_Protector,
    gText_Electirizer,
    gText_Magmarizer,
};

static const u8 *const sMossdeepPokemonNames[] = {
    gText_Eevee4000Coins,
    gText_Larvitar5000Coins,
    gText_Beldum8000Coins,
    gText_Gible10000Coins,
};

static const u8 *const sMossdeepUltimateNames[] = {
    gText_MasterBall99999Coins,
    gText_GoldenTreeckoDoll50000Coins,
};
```

### Step 4: Add Text Strings

**File:** `src/strings.c` or `data/text/game_corner.inc`

```c
const u8 gText_Trophies[] = _("TROPHIES");
const u8 gText_Items[] = _("ITEMS");
const u8 gText_Pokemon[] = _("POKéMON");
const u8 gText_Ultimate[] = _("ULTIMATE");

const u8 gText_SnakeTrophy[] = _("SNAKE TROPHY - 100C");
const u8 gText_FlappyBirdMedal[] = _("FLAPPY BIRD MEDAL - 200C");
const u8 gText_Eevee4000Coins[] = _("EEVEE - 4000C");
const u8 gText_MasterBall99999Coins[] = _("MASTER BALL - 99999C");
// ... etc
```

## Prize Balance Guidelines

### Coin Earning Rates

**Assumptions (per minigame):**
- Beginner: 50-100 coins per 5 minutes
- Intermediate: 100-200 coins per 5 minutes
- Advanced: 200-500 coins per 5 minutes

**Grind Times:**
| Prize | Cost | Beginner Time | Advanced Time |
|-------|------|---------------|---------------|
| Snake Trophy | 100 | 5-10 min | 1-2 min |
| Eevee | 4,000 | 3-4 hours | 40-80 min |
| Gible | 10,000 | 8-10 hours | 2-3 hours |
| Master Ball | 99,999 | 80-100 hours | 16-33 hours |

**Balance Goals:**
- Trophies: Quick rewards (5-30 min)
- Items: Moderate grind (1-3 hours)
- Pokemon: Significant grind (3-10 hours)
- Ultimate: Endgame flex (20-100 hours)

### Rarity Considerations

**Common Items (1,000-2,000 Coins):**
- Items available elsewhere in-game
- Convenience option for players

**Rare Items (2,000-5,000 Coins):**
- Hard to find or limited quantity in-game
- Valuable but obtainable

**Exclusive Items (5,000+ Coins):**
- Not available elsewhere (or very limited)
- True incentive for Game Corner engagement

**Ultimate Items (50,000+ Coins):**
- Prestige/bragging rights
- Not necessary for completion

## Config Integration

**File:** `include/config/game_corner.h`

```c
// Prize system configs
#define B_GAMECORNER_PRIZE_TROPHIES       TRUE   // Enable trophy prizes
#define B_GAMECORNER_PRIZE_POKEMON        TRUE   // Enable Pokemon prizes
#define B_GAMECORNER_PRIZE_MASTER_BALL    FALSE  // Enable Master Ball prize (default: too powerful)

// Prize cost multiplier (balancing)
#define B_GAMECORNER_PRIZE_COST_MULT      10     // 10 = 100%, 20 = 200% (double cost)
```

**Usage in Scripts:**

```pory
// Adjust prize cost based on config
setvar(VAR_TEMP_1, 4000)  // Base Eevee cost
setvar(VAR_TEMP_2, B_GAMECORNER_PRIZE_COST_MULT)
special(MultiplyVars)  // VAR_TEMP_1 *= VAR_TEMP_2 / 10
checkcoins(VAR_TEMP_0)
compare(VAR_TEMP_0, VAR_TEMP_1)
goto_if_lt(MossdeepCity_GameCorner_EventScript_NotEnoughCoins)
```

## Testing Checklist

- [ ] Prize counter displays correctly
- [ ] Category multichoice works
- [ ] Prize multichoice works (all categories)
- [ ] Coin check prevents insufficient coin purchases
- [ ] Coin deduction works correctly
- [ ] Item prizes given successfully
- [ ] Pokemon prizes given successfully (party not full)
- [ ] Pokemon prizes blocked if party full
- [ ] Held items on Pokemon work correctly
- [ ] Flags set for one-time prizes (if applicable)
- [ ] Master Ball prize respects config flag
- [ ] Prize cost multiplier works correctly
- [ ] Text strings display correctly
- [ ] No softlocks or crashes

## Documentation Cross-References

**Related Documents:**
- `MOSSDEEP_INTEGRATION.md` - Where prize counter is located
- `MAUVILLE_INTEGRATION.md` - Existing prize counter reference
- `05_CONFIG_DESIGN.md` - Prize system config flags

**RHH Code References:**
- `src/script_menu.c` - Prize counter implementation
- `data/maps/MauvilleCity_GameCorner/scripts.pory` - Existing prize scripts

## Summary

**Prize System Strategy:**
1. **Preserve Mauville prizes** - Keep existing dolls/items untouched
2. **Add Mossdeep prizes** - New 4-tier system (Trophies, Items, Pokemon, Ultimate)
3. **Balance grind times** - Trophies (minutes), Items (hours), Pokemon (hours), Ultimate (days)
4. **Config-driven** - Prize costs and availability configurable

**Implementation:**
- Define new prize constants and costs
- Create prize counter script (4 categories)
- Add multichoice definitions and text strings
- Test all prize exchanges
- Balance coin earning vs prize costs

**Estimated Work:** ~4-6 hours (scripting, testing, balancing)

---

**Last Updated:** 2025-12-16
**Status:** Design Complete - Prize System Defined
**Integration:** Works with both Mauville (existing) and Mossdeep (new) Game Corners
