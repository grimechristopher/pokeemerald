# Write-Through Cache Implementation Plan

**Date:** 2025-12-19
**Status:** Paused for alternative brainstorming
**Approach:** Auto-save on box switch

## Core Design

**Write-Through Single-Box Cache**
- Only 1 box in EWRAM at a time (3 KB total: 640 bytes metadata + 2.4 KB box cache)
- Auto-save silently when switching away from dirty box (~200ms per save)
- **Trade-off**: Player cannot "try and reset" - changes persist after box switch

## Behavior

### PC Operations
```
Deposit to Box 1 → Mark dirty
Switch to Box 2 → Auto-save Box 1 to Flash (~200ms)
                → Load Box 2 from Flash
Switch to Box 3 → Auto-save Box 2 to Flash (~200ms)
                → Load Box 3 from Flash
Exit PC → Auto-save Box 3 if dirty
```

### PokeNav Marking
```
Mark Pokemon in Box 1 → Load → Mark → Save immediately → Unload
Mark Pokemon in Box 5 → Load → Mark → Save immediately → Unload
```

### Auto-Deposit (Catch)
```
Catch with full party → Deposit to Box N → Mark dirty → Continue gameplay
Next PC visit → Auto-save Box N before loading new box
```

## EWRAM Usage

**Constant 3 KB**:
- `gPokemonStorageMetadata`: 640 bytes (currentBox, boxNames, boxWallpapers, fusions)
- `gBoxCache`: 2.4 KB (single box: 30 Pokemon × 80 bytes)

**Savings**: 33.4 KB → 3 KB = **30.4 KB saved (91%)**

## Flash Writes

**Frequency**:
- PC session: 5-10 saves (one per box switch)
- PokeNav session: 1-3 saves (one per box marked)
- Per playthrough: ~1000-5000 saves total

**Flash Wear**:
- GBA Flash rated: >100,000 erase cycles
- Worst case usage: 36,500 cycles over 10 years
- **Safety margin: 3x**

## Player Experience Changes

### ✅ Advantages
- Transparent - saves happen automatically without UI
- Fast - 200ms per box switch barely noticeable
- Simple - no complex behavior to explain

### ⚠️ Disadvantages
- **Cannot "try and reset"** - changes are permanent after box switch
  - Example: Player deposits Pokemon to Box 1, switches to Box 2 → Box 1 auto-saved
  - If player resets, deposit is permanent (cannot undo)
- More frequent Flash writes (wear concern, though within safe limits)
- Implicit save timing (no player control over WHEN saves happen)

## Implementation

### Phase 1: Infrastructure
- Add `struct PokemonStorageMetadata`
- Add `struct BoxCache`
- Implement `ReadBoxFromFlash()`
- Implement `WriteBoxToFlash()`
- Implement `LoadBoxToCache()` (with auto-save on dirty)

### Phase 2: Access Functions
Modify 45+ functions to check:
- If `gPokemonStoragePtr != NULL` → Legacy mode (full storage)
- If `gPokemonStoragePtr == NULL` → Lazy mode (use cache)

Read functions: `LoadBoxToCache()` before accessing
Write functions: `LoadBoxToCache()` + mark dirty after write

### Phase 3: Activation
- Set `gPokemonStoragePtr = NULL` in `SetSaveBlocksPointers()`
- Initialize `gBoxCache.boxId = 0xFF` (no box cached)
- Load metadata only during game load

### Phase 4: Write-Through
- Box switch calls `LoadBoxToCache()` → auto-saves dirty box
- PC exit saves dirty box
- PokeNav marking saves immediately

### Phase 5: Edge Cases
- Multi-box scans (lottery): Read-only from Flash
- Empty box optimization
- Flash read failure handling

### Phase 6: Verification
- Build and check .map file for EWRAM usage
- Verify 30 KB savings achieved

## Files Modified

**Core**:
- `include/pokemon_storage_system.h` - Add structures
- `src/pokemon_storage_system.c` - 45+ function modifications
- `src/load_save.c` - EWRAM allocation

**Related**:
- `src/pokemon.c` - Auto-deposit
- `src/pokenav_conditions.c` - PokeNav marking

## Risks

1. **Player expectations** - "Try and reset" no longer works after box switch
2. **Flash write timing** - 200ms might cause noticeable lag
3. **Flash wear** - Frequent writes (within safe limits but more than normal)
4. **GetBoxedMonPtr()** - Returns pointer to cache that might be invalidated

## Alternative Approaches to Consider

1. **Explicit save prompts** - Ask player "Save changes?" when switching boxes
2. **Deferred writes** - Keep dirty boxes in EWRAM, only save on manual save
3. **Hybrid mode** - Switch to full-load mode on first write
4. **Compressed cache** - Use compression to fit more boxes in EWRAM

---

**Status**: Plan paused - user wants to brainstorm alternatives before proceeding.
