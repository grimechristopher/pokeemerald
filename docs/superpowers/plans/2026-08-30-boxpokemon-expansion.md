# BoxPokemon Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Grow `struct BoxPokemon` from 80 to 128 bytes (Scale, a wider `metLocation`, Shadow Pokémon data, an inline ribbon/mark catalog, a minigame-enrollment flag, and reserved headroom), widen `VARS_COUNT` from 256 to 2,048, and resize the save's PC-storage sector budget to match — all verified against the project's real 512 KB flash target.

**Architecture:** Every new per-mon field lives inline in `BoxPokemon` so it survives the existing trade protocol (`Trade_Memcpy(..., sizeof(struct Pokemon))`) automatically, with zero changes to `src/trade.c`. Nothing is removed from the struct and no existing field changes its stored meaning except `metLocation` (widened) and `unused_0B` (renamed to a real flag, still 1 bit, same position) — so every currently-shipped save-reading code path keeps compiling and behaving the same for fields this plan doesn't touch.

**Tech Stack:** C (arm-none-eabi-gcc via devkitARM), the project's `TEST()` / `EXPECT_EQ` unit-test DSL (`test/pokemon.c`), `make check`.

**Explicitly out of scope** (each needs its own design pass before it can be planned without placeholders):
- Migrating the legacy per-bit ribbons (`coolRibbon`, `championRibbon`, etc.) or their consumers (`src/pokemon_summary_screen.c`, `src/pokenav_ribbons_list.c`, `src/pokenav_ribbons_summary.c`, `src/pokenav.c`) onto the new catalog. This plan adds the new ribbon storage *alongside* the old bits, untouched.
- `PokemonAssignment` (jobs/research/minigame registry), the Ranch, multiple regional `DayCare`, and the expansion-version save stamp. None of these have a decided capacity or exact field layout yet.
- Populating `enum Ribbon` with real ribbon/mark names — that's content work.

---

## File Structure

| File | Responsibility |
|---|---|
| `include/constants/vars.h` | `VARS_START`/`VARS_END` — widen the persisted var count. |
| `include/pokemon.h` | `struct BoxPokemon`, `struct PokemonSubstruct0/3/4`, `enum MonData`, `enum Ribbon`, `MAX_RIBBONS_PER_MON`, new accessor prototypes. |
| `src/pokemon.c` | `GetSubstruct4()`, `GetMonData`/`SetMonData`/`GetBoxMonData`/`SetBoxMonData` cases for every new field, `BoxMonHasRibbon`/`GiveBoxMonRibbon`/`HasMonRibbon`/`GiveMonRibbon`. |
| `include/save.h` | Sector id constants — resized for the new `BoxPokemon` size at 72 boxes. |
| `src/save.c` | `sSaveSlotLayout[]` — extended `SAVEBLOCK_CHUNK(struct PokemonStorage, N)` entries. |
| `test/pokemon.c` | One `TEST()` per new field/behavior, plus one full-struct size assertion. |

---

### Task 0: Fix pre-existing test/pokemon.c breakage (done during planning, documented here for the record)

**Files:**
- Modified: `test/pokemon.c:16-21, 628-633`

Discovered while establishing a clean baseline before Task 1: `test/pokemon.c` did not compile. Two tests called APIs that no longer match the current `include/pokemon.h`:

- `"BoxPokemon raw layout is independent of personality and OT ID"` called `CreateMon(&monA, SPECIES_WOBBUFFET, 50, 0, TRUE, 0x11111111, OT_ID_PRESET, 0x22222222)` — an 8-argument call matching an old `CreateMon` signature. The current one is `void CreateMon(struct Pokemon *mon, enum Species species, u8 level, u32 personality, struct OriginalTrainerId);` (5 args). First fix attempt used plain `CreateMon` with the corrected argument shape — this compiled but then *failed at runtime* (`EXPECT_EQ(-17, 0)` on the `memcmp`), because plain `CreateMon` rolls random IVs per call (confirmed via `USE_RANDOM_IVS` in `src/pokemon.c`), so `monA`/`monB` differed in raw bytes for a reason unrelated to personality/OT ID — exactly the kind of accidental difference this test exists to rule out. Real fix: `CreateMonWithIVs(&monA, SPECIES_WOBBUFFET, 50, 0x11111111, OTID_STRUCT_PRESET(0x22222222), 0);` (fixed IV of 0, same for `monB`) — pins IVs at a shared baseline so only the explicit `SetMonData` calls below it (identical for both mons) can produce a difference, which is what the test is actually checking.
- `"BoxPokemon data round-trips through every field"` called `CreateMonWithNature(&mon, SPECIES_TORCHIC, 20, 0, NATURE_HARDY)` — a function that no longer exists anywhere in the codebase. Fixed by computing the personality for the desired nature via the existing `GetMonPersonality(enum Species, u8 gender, u8 nature, u8 unownLetter)` and passing it to `CreateMon` directly: `u32 personality = GetMonPersonality(SPECIES_TORCHIC, MON_GENDER_RANDOM, NATURE_HARDY, RANDOM_UNOWN_LETTER); CreateMon(&mon, SPECIES_TORCHIC, 20, personality, OTID_STRUCT_PLAYER_ID);` — the rest of that test immediately overwrites nearly every field via `SetMonData` anyway, so only "a valid mon with a specific nature exists" needed preserving, not the exact old call shape.

This is unrelated to `BoxPokemon`/`VARS_COUNT`/the ribbon catalog — it's stale test code from an earlier API, not something introduced by this plan. It blocked establishing any baseline at all (a compile error anywhere in `test/pokemon.c` fails the whole file, including every test Tasks 1-8 add to it), so it had to be resolved before Task 1 could start. Committed on its own, separate from the feature work:

```bash
git add test/pokemon.c
git commit -m "fix: update test/pokemon.c to the current CreateMon API"
```

Every `CreateMon(...)` call written into the test snippets below already uses the corrected, current calling convention.

---

### Task 1: Widen VARS_COUNT

**Files:**
- Modify: `include/constants/vars.h:292`

- [ ] **Step 1: Change the bound**

`include/constants/vars.h` currently has:

```c
#define VARS_END                                         0x40FF
```

Change it to:

```c
#define VARS_END                                         0x47FF
```

This widens `VARS_COUNT` (`(VARS_END - VARS_START + 1)`, computed automatically) from 256 to 2,048 — `u16 vars[VARS_COUNT]` in `struct SaveBlock1` grows from 512 to 4,096 bytes, a +3,584 byte increase.

- [ ] **Step 2: Build to confirm nothing hardcodes the old value**

Run: `make -j$(nproc)`
Expected: builds clean. (`VARS_COUNT` is used exclusively via the macro throughout the codebase — no other file hardcodes `256` or `0x40FF` for this purpose.)

- [ ] **Step 3: Commit**

```bash
git add include/constants/vars.h
git commit -m "expand: widen VARS_COUNT from 256 to 2048"
```

---

### Task 2: Add Scale (individual size variance)

**Files:**
- Modify: `include/pokemon.h` (`struct PokemonSubstruct0`, `enum MonData`, prototypes)
- Modify: `src/pokemon.c` (`GetMonData`/`SetMonData`/`GetBoxMonData`/`SetBoxMonData`)
- Test: `test/pokemon.c`

- [ ] **Step 1: Write the failing test**

Add to `test/pokemon.c`:

```c
TEST("Scale is stored and round-trips through Get/SetMonData")
{
    struct Pokemon mon;
    u32 scale = 200;
    CreateMon(&mon, SPECIES_WOBBUFFET, 50, 0x11111111, OTID_STRUCT_PRESET(0x22222222));
    SetMonData(&mon, MON_DATA_SCALE, &scale);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_SCALE), 200);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="Scale is stored" check`
Expected: build FAILS — `MON_DATA_SCALE` is not declared.

- [ ] **Step 3: Add the field to PokemonSubstruct0**

`include/pokemon.h` — `struct PokemonSubstruct0` currently ends with:

```c
    u16 pokeball:6; // 63 balls.
    u16 nickname12:8; // 12th character of nickname.
    u16 unused_0A:2;
};
```

Change to:

```c
    u16 pokeball:6; // 63 balls.
    u16 nickname12:8; // 12th character of nickname.
    u16 unused_0A:2;
    u8 scale; // Individual size variance, 0-255. Drives both height and weight
              // display against the species' base figures; 0 and 255 are the
              // Mini/Jumbo Mark thresholds.
};
```

- [ ] **Step 4: Add the enum value**

`include/pokemon.h` — `enum MonData` currently ends with:

```c
    MON_DATA_TERA_TYPE,
    MON_DATA_EVOLUTION_TRACKER,
};
```

Change to:

```c
    MON_DATA_TERA_TYPE,
    MON_DATA_EVOLUTION_TRACKER,
    MON_DATA_SCALE,
};
```

- [ ] **Step 5: Wire the getter**

In `src/pokemon.c`, `GetBoxMonData3`/`GetBoxMonData2`'s big switch has this case near the other substruct0 fields:

```c
        case MON_DATA_POKEBALL:
            retVal = GetSubstruct0(boxMon)->pokeball;
            break;
```

Add immediately after it:

```c
        case MON_DATA_SCALE:
            retVal = GetSubstruct0(boxMon)->scale;
            break;
```

- [ ] **Step 6: Wire the setter**

In `SetBoxMonData`'s switch, find:

```c
        case MON_DATA_POKEBALL:
            SET8(GetSubstruct0(boxMon)->pokeball);
            break;
```

Add immediately after it:

```c
        case MON_DATA_SCALE:
            SET8(GetSubstruct0(boxMon)->scale);
            break;
```

- [ ] **Step 7: Run the test to verify it passes**

Run: `make TESTS="Scale is stored" check`
Expected: PASS

- [ ] **Step 8: Commit**

```bash
git add include/pokemon.h src/pokemon.c test/pokemon.c
git commit -m "expand: add Scale to PokemonSubstruct0"
```

---

### Task 3: Widen metLocation to u16

**Files:**
- Modify: `include/pokemon.h` (`struct PokemonSubstruct3`)
- Modify: `src/pokemon.c`
- Test: `test/pokemon.c`

- [ ] **Step 1: Write the failing test**

Add to `test/pokemon.c`:

```c
TEST("Met location supports values beyond one region's 256")
{
    struct Pokemon mon;
    u32 metLocation = 40000; // out of u8 range, must survive as u16
    CreateMon(&mon, SPECIES_WOBBUFFET, 50, 0x11111111, OTID_STRUCT_PRESET(0x22222222));
    SetMonData(&mon, MON_DATA_MET_LOCATION, &metLocation);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_MET_LOCATION), 40000);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="Met location supports" check`
Expected: FAIL — `SetMonData`/`GetMonData` currently truncate `metLocation` to a `u8`, so the stored value comes back as `40000 & 0xFF = 64`, not `40000`.

- [ ] **Step 3: Widen the field**

`include/pokemon.h` — `struct PokemonSubstruct3` currently starts with:

```c
struct PokemonSubstruct3
{
    u8 pokerus;
    u8 metLocation;
    u16 metLevel:7;
```

Change to:

```c
struct PokemonSubstruct3
{
    u8 pokerus;
    u16 metLocation; // Widened from u8 (256) - one region alone was already tight;
                      // a multi-region world needs real headroom here.
    u16 metLevel:7;
```

- [ ] **Step 4: Update the getter's width**

In `src/pokemon.c`, `GetBoxMonData3`/`GetBoxMonData2`, the existing case reads:

```c
        case MON_DATA_MET_LOCATION:
            retVal = GetSubstruct3(boxMon)->metLocation;
            break;
```

No change needed here — `retVal` is already a wide type and a plain member read picks up the new `u16` automatically.

- [ ] **Step 5: Update the setter's width**

In `SetBoxMonData`, find:

```c
        case MON_DATA_MET_LOCATION:
            SET8(GetSubstruct3(boxMon)->metLocation);
            break;
```

Change to:

```c
        case MON_DATA_MET_LOCATION:
            SET16(GetSubstruct3(boxMon)->metLocation);
            break;
```

(`SET16` is already defined at the top of this function block, used elsewhere in the same switch — e.g. `MON_DATA_HP_LOST`.)

- [ ] **Step 6: Run the test to verify it passes**

Run: `make TESTS="Met location supports" check`
Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add include/pokemon.h src/pokemon.c test/pokemon.c
git commit -m "expand: widen metLocation from u8 to u16"
```

---

### Task 4: Minigame-enrollment flag

**Files:**
- Modify: `include/pokemon.h`
- Modify: `src/pokemon.c`
- Test: `test/pokemon.c`

- [ ] **Step 1: Write the failing test**

Add to `test/pokemon.c`:

```c
TEST("Minigame enrollment flag round-trips and defaults false")
{
    struct Pokemon mon;
    u32 enrolled = TRUE;
    CreateMon(&mon, SPECIES_WOBBUFFET, 50, 0x11111111, OTID_STRUCT_PRESET(0x22222222));
    EXPECT_EQ(GetMonData(&mon, MON_DATA_IS_ENROLLED_IN_MINIGAME), FALSE);
    SetMonData(&mon, MON_DATA_IS_ENROLLED_IN_MINIGAME, &enrolled);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_IS_ENROLLED_IN_MINIGAME), TRUE);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="Minigame enrollment" check`
Expected: build FAILS — `MON_DATA_IS_ENROLLED_IN_MINIGAME` not declared.

- [ ] **Step 3: Rename the bit in place**

`include/pokemon.h` — `struct PokemonSubstruct3` currently has, in its ribbons/flags `u32`:

```c
    u32 isShadow:1;
    u32 unused_0B:1;
    u32 abilityNum:2;
```

Change to:

```c
    u32 isShadow:1;
    u32 isEnrolledInMinigame:1; // was unused_0B - marks a mon "look me up in the
                                // PokemonAssignment registry" without a table scan
    u32 abilityNum:2;
```

This is a rename of an already-reserved bit, not a resize — no layout change.

- [ ] **Step 4: Add the enum value**

`include/pokemon.h` — append to `enum MonData` (after the `MON_DATA_SCALE` added in Task 2):

```c
    MON_DATA_SCALE,
    MON_DATA_IS_ENROLLED_IN_MINIGAME,
};
```

- [ ] **Step 5: Wire the getter**

In `src/pokemon.c`, find the existing case:

```c
        case MON_DATA_IS_SHADOW:
            retVal = GetSubstruct3(boxMon)->isShadow;
            break;
```

Add immediately after it:

```c
        case MON_DATA_IS_ENROLLED_IN_MINIGAME:
            retVal = GetSubstruct3(boxMon)->isEnrolledInMinigame;
            break;
```

- [ ] **Step 6: Wire the setter**

In `SetBoxMonData`, find:

```c
        case MON_DATA_IS_SHADOW:
            SET8(GetSubstruct3(boxMon)->isShadow);
            break;
```

Add immediately after it:

```c
        case MON_DATA_IS_ENROLLED_IN_MINIGAME:
            SET8(GetSubstruct3(boxMon)->isEnrolledInMinigame);
            break;
```

- [ ] **Step 7: Run the test to verify it passes**

Run: `make TESTS="Minigame enrollment" check`
Expected: PASS

- [ ] **Step 8: Commit**

```bash
git add include/pokemon.h src/pokemon.c test/pokemon.c
git commit -m "expand: rename unused_0B to isEnrolledInMinigame"
```

---

### Task 5: Shadow Pokémon data (nickname union)

**Files:**
- Modify: `include/pokemon.h`
- Modify: `src/pokemon.c`
- Test: `test/pokemon.c`

`isShadow` itself already exists and is already wired (`MON_DATA_IS_SHADOW`, `src/pokemon.c:2347,2790`) — this task only adds the data a Shadow Pokémon needs once that flag is set.

- [ ] **Step 1: Write the failing test**

Add to `test/pokemon.c`:

```c
TEST("Shadow Pokemon data shares nickname's storage and round-trips")
{
    struct Pokemon mon;
    u32 isShadow = TRUE;
    u32 isReverse = TRUE;
    u32 heartValue = 3000;
    u32 heartMax = 8000;
    CreateMon(&mon, SPECIES_WOBBUFFET, 50, 0x11111111, OTID_STRUCT_PRESET(0x22222222));
    SetMonData(&mon, MON_DATA_IS_SHADOW, &isShadow);
    SetMonData(&mon, MON_DATA_IS_REVERSE, &isReverse);
    SetMonData(&mon, MON_DATA_HEART_VALUE, &heartValue);
    SetMonData(&mon, MON_DATA_HEART_MAX, &heartMax);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_IS_REVERSE), TRUE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HEART_VALUE), 3000);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_HEART_MAX), 8000);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="Shadow Pokemon data" check`
Expected: build FAILS — `MON_DATA_IS_REVERSE`/`MON_DATA_HEART_VALUE`/`MON_DATA_HEART_MAX` not declared.

- [ ] **Step 3: Turn nickname into an anonymous union**

`include/pokemon.h` — `struct BoxPokemon` currently starts:

```c
struct BoxPokemon
{
    u32 personality;
    u32 otId;
    u8 nickname[min(10, POKEMON_NAME_LENGTH)];
    u8 language:3;
```

Change to:

```c
struct BoxPokemon
{
    u32 personality;
    u32 otId;
    // A Shadow Pokemon can't be nicknamed until purified, so nickname's 10 bytes
    // double as Shadow Pokemon data when substruct3's isShadow bit is set - an
    // anonymous union, so every existing `boxMon->nickname` access is unchanged.
    union
    {
        u8 nickname[min(10, POKEMON_NAME_LENGTH)];
        struct
        {
            u8 isReverse;
            u16 heartValue;
            u16 heartMax;
        } shadowData;
    };
    u8 language:3;
```

- [ ] **Step 4: Add the enum values**

`include/pokemon.h` — append to `enum MonData`:

```c
    MON_DATA_IS_ENROLLED_IN_MINIGAME,
    MON_DATA_IS_REVERSE,
    MON_DATA_HEART_VALUE,
    MON_DATA_HEART_MAX,
};
```

- [ ] **Step 5: Wire the getters**

In `src/pokemon.c`, `GetBoxMonData3`/`GetBoxMonData2`'s switch, add after the `MON_DATA_IS_ENROLLED_IN_MINIGAME` case from Task 4:

```c
        case MON_DATA_IS_REVERSE:
            retVal = boxMon->shadowData.isReverse;
            break;
        case MON_DATA_HEART_VALUE:
            retVal = boxMon->shadowData.heartValue;
            break;
        case MON_DATA_HEART_MAX:
            retVal = boxMon->shadowData.heartMax;
            break;
```

- [ ] **Step 6: Wire the setters**

In `SetBoxMonData`, add after the `MON_DATA_IS_ENROLLED_IN_MINIGAME` case from Task 4:

```c
        case MON_DATA_IS_REVERSE:
            SET8(boxMon->shadowData.isReverse);
            break;
        case MON_DATA_HEART_VALUE:
            SET16(boxMon->shadowData.heartValue);
            break;
        case MON_DATA_HEART_MAX:
            SET16(boxMon->shadowData.heartMax);
            break;
```

- [ ] **Step 7: Run the test to verify it passes**

Run: `make TESTS="Shadow Pokemon data" check`
Expected: PASS

- [ ] **Step 8: Commit**

```bash
git add include/pokemon.h src/pokemon.c test/pokemon.c
git commit -m "expand: add Shadow Pokemon data via nickname union"
```

---

### Task 6: Ribbon & mark catalog (PokemonSubstruct4)

**Files:**
- Modify: `include/pokemon.h`
- Modify: `src/pokemon.c`
- Test: `test/pokemon.c`

This adds new, additive storage. It does not touch the existing per-bit ribbons (`coolRibbon`, `championRibbon`, ...) or `MON_DATA_RIBBON_COUNT`/`MON_DATA_RIBBONS` — those keep working exactly as they do today. Migrating them onto this catalog is separate, deferred work (see plan header).

- [ ] **Step 1: Write the failing test**

Add to `test/pokemon.c`:

```c
TEST("A mon can hold up to MAX_RIBBONS_PER_MON ribbons, no duplicates, then no more")
{
    struct Pokemon mon;
    CreateMon(&mon, SPECIES_WOBBUFFET, 50, 0x11111111, OTID_STRUCT_PRESET(0x22222222));

    EXPECT_EQ(HasMonRibbon(&mon, 5), FALSE);
    EXPECT_EQ(GiveMonRibbon(&mon, 5), TRUE);
    EXPECT_EQ(HasMonRibbon(&mon, 5), TRUE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_RIBBON_TALLY), 1);

    // Giving the same ribbon twice does not add a second entry.
    EXPECT_EQ(GiveMonRibbon(&mon, 5), FALSE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_RIBBON_TALLY), 1);

    // Fill every remaining slot with distinct ribbons.
    for (u32 i = 6; i < 6 + (MAX_RIBBONS_PER_MON - 1); i++)
        EXPECT_EQ(GiveMonRibbon(&mon, i), TRUE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_RIBBON_TALLY), MAX_RIBBONS_PER_MON);

    // The catalog is full - one more ribbon is rejected, not silently dropped.
    EXPECT_EQ(GiveMonRibbon(&mon, 200), FALSE);
    EXPECT_EQ(GetMonData(&mon, MON_DATA_RIBBON_TALLY), MAX_RIBBONS_PER_MON);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="A mon can hold up" check`
Expected: build FAILS — `HasMonRibbon`, `GiveMonRibbon`, `MON_DATA_RIBBON_TALLY`, `MAX_RIBBONS_PER_MON` not declared.

- [ ] **Step 3: Add the catalog type and the new substruct**

`include/pokemon.h` — add just above `struct BoxPokemon` (after the `STATIC_ASSERT(SECURE_REGION_BYTES % 4 == 0, ...)` line):

```c
// The full catalog (real ribbons + marks) is populated by content work, not this
// layer - this just reserves the id space. RIBBON_NONE (0) is the empty-slot value,
// matching this codebase's SPECIES_NONE/MOVE_NONE/ITEM_NONE convention.
enum Ribbon
{
    RIBBON_NONE,
};
#define RIBBON_CATALOG_CAP 256 // enum Ribbon values must fit in a u8 (ribbonIds' element type)
#define MAX_RIBBONS_PER_MON 32 // slots per mon, not the catalog size - see docs/superpowers/plans/2026-08-30-boxpokemon-expansion.md

struct PokemonSubstruct4
{
    u8 ribbonCount;
    u8 ribbonIds[MAX_RIBBONS_PER_MON];
};
```

- [ ] **Step 4: Add the field to BoxPokemon, outside the secure union**

`include/pokemon.h` — `struct BoxPokemon` currently ends:

```c
    } secure;
};
```

Change to:

```c
    } secure;
    // Not inside `secure`: ribbons aren't anti-cheat-sensitive, and keeping this out
    // of the checksummed region means the checksum calculation never needs to change
    // as the ribbon catalog grows.
    struct PokemonSubstruct4 substruct4;
};
```

- [ ] **Step 5: Add the enum values**

`include/pokemon.h` — append to `enum MonData`:

```c
    MON_DATA_HEART_MAX,
    MON_DATA_RIBBON_TALLY,
};
```

(Named `_TALLY`, not `_COUNT`, because `MON_DATA_RIBBON_COUNT` already exists for the legacy per-bit system.)

- [ ] **Step 6: Add the accessor function prototypes**

`include/pokemon.h` — near the other `Mon`/`BoxMon` pair prototypes (by `IsMonShiny`/`GetMonFriendshipScore` etc.), add:

```c
bool32 BoxMonHasRibbon(struct BoxPokemon *boxMon, enum Ribbon ribbon);
bool32 GiveBoxMonRibbon(struct BoxPokemon *boxMon, enum Ribbon ribbon);
bool32 HasMonRibbon(struct Pokemon *mon, enum Ribbon ribbon);
bool32 GiveMonRibbon(struct Pokemon *mon, enum Ribbon ribbon);
```

- [ ] **Step 7: Add GetSubstruct4 alongside the other substruct getters**

`src/pokemon.c` — after `GetSubstruct3` (see `GetSubstruct0`/`1`/`2`/`3` together near line 1941), add:

```c
static ALWAYS_INLINE struct PokemonSubstruct4 *GetSubstruct4(struct BoxPokemon *boxMon)
{
    return &boxMon->substruct4;
}
```

- [ ] **Step 8: Implement the ribbon functions**

`src/pokemon.c` — add near `IsMonShiny`/other small `struct Pokemon *` / `struct BoxPokemon *` pair-of-functions utilities:

```c
bool32 BoxMonHasRibbon(struct BoxPokemon *boxMon, enum Ribbon ribbon)
{
    struct PokemonSubstruct4 *substruct4 = GetSubstruct4(boxMon);
    u32 i;

    for (i = 0; i < substruct4->ribbonCount; i++)
    {
        if (substruct4->ribbonIds[i] == ribbon)
            return TRUE;
    }
    return FALSE;
}

bool32 GiveBoxMonRibbon(struct BoxPokemon *boxMon, enum Ribbon ribbon)
{
    struct PokemonSubstruct4 *substruct4 = GetSubstruct4(boxMon);

    if (BoxMonHasRibbon(boxMon, ribbon))
        return FALSE;
    if (substruct4->ribbonCount >= MAX_RIBBONS_PER_MON)
        return FALSE;

    substruct4->ribbonIds[substruct4->ribbonCount] = ribbon;
    substruct4->ribbonCount++;
    return TRUE;
}

bool32 HasMonRibbon(struct Pokemon *mon, enum Ribbon ribbon)
{
    return BoxMonHasRibbon(&mon->box, ribbon);
}

bool32 GiveMonRibbon(struct Pokemon *mon, enum Ribbon ribbon)
{
    return GiveBoxMonRibbon(&mon->box, ribbon);
}
```

- [ ] **Step 9: Wire MON_DATA_RIBBON_TALLY**

In `src/pokemon.c`, `GetBoxMonData3`/`GetBoxMonData2`'s switch, add after the existing `MON_DATA_RIBBONS` case:

```c
        case MON_DATA_RIBBON_TALLY:
            retVal = GetSubstruct4(boxMon)->ribbonCount;
            break;
```

(Read-only by design — mutation goes through `GiveBoxMonRibbon`/`GiveMonRibbon`, not `SetMonData`, so a caller can't desync `ribbonCount` from the actual id list. No case is added to `SetBoxMonData` for this value.)

- [ ] **Step 10: Run the test to verify it passes**

Run: `make TESTS="A mon can hold up" check`
Expected: PASS

- [ ] **Step 11: Commit**

```bash
git add include/pokemon.h src/pokemon.c test/pokemon.c
git commit -m "expand: add PokemonSubstruct4 ribbon/mark catalog"
```

---

### Task 7: True up BoxPokemon to exactly 128 bytes

**Files:**
- Modify: `include/pokemon.h`
- Test: `test/pokemon.c`

The reserved byte count below (7) was verified empirically against the real target compiler before this plan was written — `arm-none-eabi-gcc -std=c11 -mthumb -mcpu=arm7tdmi` on the exact field layout from Tasks 2-6 reports `sizeof(struct PokemonSubstruct0) == 16`, `sizeof(struct PokemonSubstruct3) == 16`, `sizeof(struct PokemonSubstruct4) == 33` (compiler-inserted alignment padding included), giving `32 (header) + 56 (secure region) + 33 (substruct4) + 7 (reserved) = 128`.

- [ ] **Step 1: Write the failing test**

Add to `test/pokemon.c`:

```c
TEST("BoxPokemon is exactly 128 bytes")
{
    EXPECT_EQ(sizeof(struct BoxPokemon), 128);
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `make TESTS="BoxPokemon is exactly" check`
Expected: FAIL — without `reserved`, `sizeof(struct BoxPokemon)` is 121.

- [ ] **Step 3: Add the reserved tail**

`include/pokemon.h` — `struct BoxPokemon` now ends (after Task 6's Step 4):

```c
    struct PokemonSubstruct4 substruct4;
};
```

Change to:

```c
    struct PokemonSubstruct4 substruct4;
    // Deliberate, zero-initialized, documented - not compiler padding. A future
    // inline field that fits here is a safe migration; pair with a save-wide
    // expansion-version stamp (tracked separately, not yet added) so old saves
    // can tell which layout wrote them.
    u8 reserved[7];
};

STATIC_ASSERT(sizeof(struct BoxPokemon) == 128, BoxPokemonIsExactly128Bytes);
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `make TESTS="BoxPokemon is exactly" check`
Expected: PASS

- [ ] **Step 5: Run the full existing pokemon.c test file to confirm nothing regressed**

Run: `make TESTS="pokemon" check`
Expected: PASS, including "BoxPokemon raw layout is independent of personality and OT ID" (the no-shuffle/no-encryption invariant test) — unaffected, since every new field is a plain member or plain array, not permuted by personality/OT ID.

- [ ] **Step 6: Commit**

```bash
git add include/pokemon.h test/pokemon.c
git commit -m "expand: reserve 7 bytes, lock BoxPokemon at 128 bytes"
```

---

### Task 8: Resize the PC-storage sector budget — and fix a pre-existing gap found while planning this

**Files:**
- Modify: `include/save.h`
- Modify: `src/save.c:62-110` (`sSaveSlotLayout[]`)

`struct PokemonStorage` (`boxes[TOTAL_BOXES_COUNT][IN_BOX_COUNT]`, i.e. `boxes[72][30]`) needs no code change — it's just an array of `struct BoxPokemon`, so it already grows to the new size automatically. Only the manually-maintained sector-count constants and chunk list need updating to match.

**Pre-existing issue found while writing this plan, not introduced by it:** `include/save.h` declares `SECTOR_ID_SAVEBLOCK1_END 17` (17 sectors reserved for `SaveBlock1`), but `src/save.c`'s `sSaveSlotLayout[]` currently lists only 4 explicit `SAVEBLOCK_CHUNK(struct SaveBlock1, N)` entries (`N = 0..3`, lines 62-65) — enough for `4 * 3968 = 15,872` bytes. A minimal standalone compile against this project's real `include/global.h` (`arm-none-eabi-gcc -std=c11 -mthumb -mcpu=arm7tdmi`) confirms `sizeof(struct SaveBlock1) > 15,872` — i.e. `struct SaveBlock1` is already bigger than the 4 chunks that exist to serialize it. Any of its fields living past byte 15,872 have no sector chunk carrying them to or from flash. This is independent of everything else in this plan (it doesn't touch `BoxPokemon` at all) — worth fixing here only because Task 8 is already the place that touches this exact array, so leaving it unfixed while extending the array right next to it would be leaving a known gap in place on purpose.

- [ ] **Step 1: Confirm the exact gap with the project's real build config**

The sandbox used to write this plan couldn't complete a full `make` build (two unrelated bitfield-width errors appeared under a minimal, hand-assembled set of compiler flags, not the project's real config), so the exact byte count isn't pinned down — only that it exceeds 15,872. Confirm it precisely and safely, with zero risk of clobbering anything, via a throwaway build using the project's real Makefile-driven flags:

```bash
cat > /tmp/sb1_gap_check.c << 'CHECKEOF'
#include "global.h"
_Static_assert(sizeof(struct SaveBlock1) <= 15872, "SaveBlock1 exceeds its 4 existing chunk entries");
int dummy;
CHECKEOF
```

Run: `make build/modern/src/save.o 2>&1 | grep -A2 "sb1_gap_check\|static assert"` won't work directly since this file isn't part of the source tree — instead, temporarily add the same `_Static_assert(...)` line to the top of `src/save.c` itself (after the `#include`s), run `make build/modern/src/save.o`, read the exact byte count the compiler error reports (GCC's `_Static_assert` failure message doesn't include the actual size — if it doesn't, temporarily change the assert to `_Static_assert(sizeof(struct SaveBlock1) <= N, "x")` for a few different `N` values via binary search, or add `#pragma message` / a deliberate divide-by-zero-style trick like `char (*p)[sizeof(struct SaveBlock1)] = 1;` and read the real size out of the resulting type-mismatch error), then remove the temporary line.

Expected: a concrete number, call it `realSize`. Compute `neededChunks = ceil(realSize / 3968)`.

- [ ] **Step 2: Extend the SaveBlock1 chunk entries to cover the real size**

`src/save.c` currently has, at lines 62-65:

```c
    SAVEBLOCK_CHUNK(struct SaveBlock1, 0), // SECTOR_ID_SAVEBLOCK1_START
    SAVEBLOCK_CHUNK(struct SaveBlock1, 1),
    SAVEBLOCK_CHUNK(struct SaveBlock1, 2),
    SAVEBLOCK_CHUNK(struct SaveBlock1, 3), // SECTOR_ID_SAVEBLOCK1_END
```

Extend this list so it runs `N = 0` through `N = neededChunks - 1` (from Step 1), following the identical existing pattern, moving the `// SECTOR_ID_SAVEBLOCK1_END` comment to the new last line. If `neededChunks` comes out to 17 or fewer, this fits inside the already-reserved budget and `SECTOR_ID_SAVEBLOCK1_END` in `include/save.h` does not need to change. If it's more than 17, `SECTOR_ID_SAVEBLOCK1_END` (and every constant after it, including the ones this task's Step 3 is about to set) needs to shift by the difference — resolve that before continuing to Step 3.

Let `sb1Chunks` = `neededChunks` from Step 1 (the corrected `SaveBlock1` chunk count — use 17 if Step 1 confirmed the existing reservation already covers it).

`include/save.h` currently has:

```c
#define SECTOR_ID_SAVEBLOCK2          0      // 1 sector
#define SECTOR_ID_SAVEBLOCK1_START    1
#define SECTOR_ID_SAVEBLOCK1_END      17   // 17 sectors = 67 KB for multi-region data
#define SECTOR_ID_PKMN_STORAGE_START  18
#define SECTOR_ID_PKMN_STORAGE_END   61    // 44 sectors for 72 boxes (~174 KB)
#define NUM_SECTORS_PER_SLOT         62    // 1 + 17 + 44 sectors; the only save slot
#define SECTOR_ID_HOF_1              62
#define SECTOR_ID_HOF_2              63
#define SECTOR_ID_TRAINER_HILL       64
#define SECTOR_ID_RECORDED_BATTLE    65
#define SECTORS_COUNT                66    // 62 save + 4 special sectors (62 sectors/~248 KB reclaimed from the dropped backup slot)
```

`72 boxes * 30 slots/box * 128 bytes = 276,480 bytes`, and each sector carries `SECTOR_DATA_SIZE` (3,968) usable bytes for this chunk, so `ceil(276480 / 3968) = 70` PC-storage sectors (up from 44). Combined with `sb1Chunks` from Step 1:

```c
#define SECTOR_ID_SAVEBLOCK2          0      // 1 sector
#define SECTOR_ID_SAVEBLOCK1_START    1
#define SECTOR_ID_SAVEBLOCK1_END      (SECTOR_ID_SAVEBLOCK1_START + sb1Chunks - 1)
#define SECTOR_ID_PKMN_STORAGE_START  (SECTOR_ID_SAVEBLOCK1_END + 1)
#define SECTOR_ID_PKMN_STORAGE_END    (SECTOR_ID_PKMN_STORAGE_START + 70 - 1)    // 70 sectors for 72 boxes at 128 B/mon (~276 KB)
#define NUM_SECTORS_PER_SLOT          (SECTOR_ID_PKMN_STORAGE_END + 1)          // 1 + sb1Chunks + 70 sectors; the only save slot
#define SECTOR_ID_HOF_1               NUM_SECTORS_PER_SLOT
#define SECTOR_ID_HOF_2               (SECTOR_ID_HOF_1 + 1)
#define SECTOR_ID_TRAINER_HILL        (SECTOR_ID_HOF_1 + 2)
#define SECTOR_ID_RECORDED_BATTLE     (SECTOR_ID_HOF_1 + 3)
#define SECTORS_COUNT                 (SECTOR_ID_HOF_1 + 4)    // save slot + 4 special sectors
```

Replace `sb1Chunks` with the literal number Step 1 found before committing — plain `#define` integer constants, not the symbolic form above (written symbolically here only so the arithmetic is traceable; e.g. with `sb1Chunks = 17` this reproduces exactly `SECTOR_ID_SAVEBLOCK1_END 17`, `SECTOR_ID_PKMN_STORAGE_START 18`, `SECTOR_ID_PKMN_STORAGE_END 87`, `NUM_SECTORS_PER_SLOT 88`, `SECTOR_ID_HOF_1 88`, `SECTOR_ID_HOF_2 89`, `SECTOR_ID_TRAINER_HILL 90`, `SECTOR_ID_RECORDED_BATTLE 91`, `SECTORS_COUNT 92` — 92 of the 128 sectors available in this project's 512 KB flash target (`claude_docs/MGBA_EXPANSION_GUIDE.md`), 36 spare). Keep the descriptive comments from the original block, updated to match.

- [ ] **Step 4: Extend the PokemonStorage chunk list**

`src/save.c` — the `sSaveSlotLayout[]` array currently lists `SAVEBLOCK_CHUNK(struct PokemonStorage, N)` for `N = 0` through `N = 43`, ending:

```c
    SAVEBLOCK_CHUNK(struct PokemonStorage, 43), // SECTOR_ID_PKMN_STORAGE_END
};
```

Extend the list so it runs `N = 0` through `N = 69` (70 total entries), ending:

```c
    SAVEBLOCK_CHUNK(struct PokemonStorage, 68),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 69), // SECTOR_ID_PKMN_STORAGE_END
};
```

Every line in between follows the identical pattern already present for entries 0-43 — `SAVEBLOCK_CHUNK(struct PokemonStorage, N),` for each `N`, one per line, in order. No other content changes.

- [ ] **Step 5: Build and confirm the layout is fully covered**

Run: `make -j$(nproc)`
Expected: builds clean, and `1 (SaveBlock2) + sb1Chunks (SaveBlock1) + 70 (PokemonStorage) == NUM_SECTORS_PER_SLOT` holds by construction (Step 3 defines `NUM_SECTORS_PER_SLOT` from the same `sb1Chunks` and `70`, so this can't drift out of sync the way it did before Step 1-2 fixed it).

- [ ] **Step 6: Run the full test suite**

Run: `make check -j`
Expected: PASS across the board — this task changes save-layout bookkeeping, not any struct or accessor, so no test from Tasks 1-7 should be affected.

- [ ] **Step 7: Commit**

```bash
git add include/save.h src/save.c
git commit -m "expand: fix SaveBlock1 chunk gap, resize PC storage for 128-byte BoxPokemon"
```

---

### Task 9: Full verification

**Files:** none (verification only)

- [ ] **Step 1: Full clean build**

Run: `make clean && make -j$(nproc)`
Expected: builds clean, no warnings about `struct BoxPokemon`, `PokemonSubstruct0/3/4`, or `sSaveSlotLayout`.

- [ ] **Step 2: Full test suite**

Run: `make check -j`
Expected: all tests PASS, including every `TEST(...)` added in Tasks 2-7 and the pre-existing `test/pokemon.c` suite.

- [ ] **Step 3: Confirm the final numbers**

Run:
```bash
grep -n "SECTORS_COUNT\|NUM_SECTORS_PER_SLOT\|SECTOR_ID_PKMN_STORAGE_END" include/save.h
```
Expected: `SECTORS_COUNT` = `sb1Chunks + 75` (1 SaveBlock2 + `sb1Chunks` SaveBlock1 + 70 PokemonStorage + 4 special), using whatever `sb1Chunks` Task 8 Step 1 found. If `sb1Chunks` came out to 17 (the value the pre-existing constant already reserved, most likely if that reservation was sized correctly in the first place), this is `SECTORS_COUNT 92`, `NUM_SECTORS_PER_SLOT 88`, `SECTOR_ID_PKMN_STORAGE_END 87` — 92 of the 128 sectors available in the 512 KB target, 36 spare. Confirm `SECTORS_COUNT <= 128` regardless of the exact `sb1Chunks` value — that's the real target-hardware ceiling this whole plan is sized against.

- [ ] **Step 4: Final commit (if any working-tree changes remain)**

```bash
git status
# if clean, nothing to do - Tasks 1-8 already committed everything
```

---

## What this plan deliberately leaves for later

- **Legacy ribbon migration** — retiring `coolRibbon`/`championRibbon`/etc. and repointing `MON_DATA_RIBBON_COUNT`/`MON_DATA_RIBBONS` at `PokemonSubstruct4` requires auditing and updating `pokemon_summary_screen.c`, `pokenav_ribbons_list.c`, `pokenav_ribbons_summary.c`, and `pokenav.c` — a UI-layer plan of its own.
- **`PokemonAssignment` registry, Ranch, regional `DayCare`, expansion-version stamp** — each needs a capacity/field-layout decision before it can be planned without placeholders.
- **Populating `enum Ribbon`** with real ribbon and mark names — content work, not struct work.
