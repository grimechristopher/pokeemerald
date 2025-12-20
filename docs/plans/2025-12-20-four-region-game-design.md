# Four-Region Pokemon Game - Design Document

**Date:** 2025-12-20
**Status:** Design Approved, Ready for Implementation

---

## Project Vision

Create a massive four-region Pokemon game combining Hoenn, Johto, Kanto, and Sinnoh into one interconnected ROM hack.

### Core Goals
- **Traditional Pokemon gameplay** with modern quality-of-life improvements
- **Four fully interconnected regions** (similar to HGSS's Johto + Kanto approach, but bigger)
- **Expanded Pokemon data structures** - more fields/stats than standard GBA format
- **Instant box access** - all boxes in RAM simultaneously, zero loading
- **4th gen aesthetic** - DPPt/HGSS graphical style across all regions
- **Reimagined regions** - inspired by originals but remapped to be grander and more exciting

---

## Platform Decision

**pokeemerald-expansion + custom mGBA**

### Why This Platform?
- pokeemerald-expansion provides all Pokemon systems (battle engine, abilities, moves, items)
- Active community constantly improving the codebase
- Custom mGBA expansion provides necessary hardware capabilities
- Building this in Godot would require 2+ years of engine work before content creation

### Custom mGBA Requirements
- ✅ **1 MB EWRAM** (4x standard) - All boxes in RAM, complex multi-region scripts
- ✅ **512 KB Flash** (4x standard) - Save data spanning four regions
- ✅ **Expanded ROM size** - No artificial limits (can expand as needed)
- ✅ **Expanded Pokemon data structure** - Custom fields beyond standard 80-byte format

**Status:** Custom mGBA already built (see `claude_docs/MGBA_EXPANSION_GUIDE.md`)

---

## Development Strategy

### Phase 1: Prototype with Built-in Tilesets ⭐ START HERE

**Goal:** Get all four regions roughed out and playable ASAP

**Approach:**
- Use pokeemerald's existing Hoenn tilesets for all regions
- Focus on map layout, flow, connections, and gameplay
- Don't worry about graphics looking authentic yet
- Prove the four-region concept works

**Timeline:** Weeks 1-12
- Weeks 1-4: Rough out Johto (cities, routes, caves)
- Weeks 5-8: Rough out Kanto
- Weeks 9-12: Rough out Sinnoh
- (Hoenn already exists from base pokeemerald)

**Deliverable:** Playable four-region game with placeholder graphics

---

### Phase 2: Polish with 4th Gen Tilesets (Later)

**Goal:** Replace placeholder tilesets with authentic 4th gen style

**Approach:**
- Extract tilesets from pokediamond/pokeheartgold decomp projects
- Deconstruct and rebuild for GBA format (downscale, palette reduction)
- Replace one area at a time incrementally

**Tileset Workflow:**
1. Extract DPPt/HGSS tileset PNGs from decomp repos
2. Downscale to GBA resolution (GIMP)
3. Reduce to 256 color palette (GIMP)
4. Clean up downscaled tiles (Aseprite)
5. Import to pokeemerald `data/tilesets/`
6. Rebuild metatiles in Porymap
7. Swap into existing maps

**Time Estimate:** ~4-6 hours per tileset, 20-30 tilesets total

**Deliverable:** Four-region game with authentic 4th gen aesthetic

---

## Technical Architecture

### Region Structure

**Four regions in single ROM:**
- **Hoenn** (already exists) - Starting region
- **Johto** - Accessible after [milestone]
- **Kanto** - Accessible after Johto
- **Sinnoh** - Post-game region

**Region Connections:**
- Sea routes between regions (like HGSS's ship between Johto/Kanto)
- Or direct land connections (design TBD during implementation)

### Memory Layout

**With 1 MB EWRAM:**
- All 72+ boxes in RAM (no bank swapping)
- Estimated: 72 boxes × 30 Pokemon × 80+ bytes = ~170 KB
- Leaves 850+ KB for scripts, events, region data

**With 512 KB Flash:**
- Save data includes: Player progress across 4 regions, 72+ boxes, expanded Pokemon data
- Plenty of headroom for future expansion

### Expanded Pokemon Data Structure

**Standard GBA Pokemon:** 80 bytes
**Custom expanded Pokemon:** TBD (design during implementation)

**Potential additions:**
- Additional stats/attributes
- Custom data for new mechanics
- Extended move slots
- Region-specific data

---

## Tools and Resources

### Required Tools
- **Porymap** (included with pokeemerald) - Map editor, tileset assembly
- **GIMP** (free) - Tileset downscaling, palette reduction
- **Aseprite** ($20 or compile free) - Tile cleanup, custom tile creation
- **Tilemap Studio** (optional) - Extract tiles from NDS ROMs

### Tileset Resources
- **pokediamond decomp** - Authentic DPPt tilesets
- **pokeheartgold decomp** - Authentic HGSS tilesets
- **Relic Castle** - Community-made tilesets
- **pokecommunity** - Additional custom resources

### Custom mGBA Distribution
- Include compiled mGBA builds with ROM releases (Windows/Linux/Android)
- Document emulator requirement clearly for players

---

## Development Workflow

### Phase 1 Workflow (Prototype)
1. Design region layout on paper/digital sketch
2. Open Porymap
3. Create new maps using existing Hoenn tilesets
4. Connect maps together
5. Add basic events/NPCs
6. Playtest and iterate

### Phase 2 Workflow (Polish)
1. Identify area to polish
2. Extract/create 4th gen tileset for that area
3. Import to pokeemerald
4. Rebuild metatiles in Porymap
5. Repaint maps with new tileset
6. Move to next area

---

## Success Criteria

**Minimum Viable Product (Phase 1):**
- [ ] All four regions mapped out and connected
- [ ] Can walk between all regions
- [ ] Basic NPCs and events in place
- [ ] Core Pokemon gameplay works (catch, battle, store)
- [ ] Instant box access functioning
- [ ] Save/load works across regions

**Polished Release (Phase 2):**
- [ ] All regions have authentic 4th gen aesthetic
- [ ] Expanded Pokemon data implemented
- [ ] Full story/events implemented
- [ ] All gyms/elite four/champions in place
- [ ] Post-game content in Sinnoh

---

## Risks and Mitigations

### Risk: ROM size limitations
**Mitigation:** Custom mGBA allows unlimited ROM size expansion

### Risk: Four regions is too ambitious
**Mitigation:** Phase 1 proves concept early; can scale back if needed

### Risk: Custom emulator limits distribution
**Mitigation:** Include emulator with releases; ROM hack community is used to this

### Risk: Tileset work takes forever
**Mitigation:** Phase 1 decouples tileset work from gameplay development

---

## Next Steps

1. ✅ Design approved
2. ⬜ Set up git worktree for isolated development
3. ⬜ Create detailed implementation plan for Phase 1
4. ⬜ Begin mapping Johto region

---

## Notes

- This is an ambitious project - pace yourself
- Prioritize getting something playable over perfection
- Community feedback during Phase 1 will guide Phase 2 priorities
- Consider releasing Phase 1 as "beta" to generate interest
