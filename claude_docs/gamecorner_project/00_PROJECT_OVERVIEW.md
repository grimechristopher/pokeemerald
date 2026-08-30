# Game Corner Minigames Port - Project Overview

## Project Goal

Port 9 new minigames from heyopc's pokeemerald-gamecorner-expansion (pret-based) to RHH's pokeemerald-expansion codebase, adapting the code to meet RHH's modern coding standards and architectural patterns.

## Source Repository

**heyopc's pokeemerald-gamecorner-expansion:**
- URL: https://github.com/heyopc/pokeemerald-gamecorner-expansion
- Base: pret's pokeemerald (vanilla decompilation)
- Compatibility Note: NOT compatible with pokeemerald-expansion (different architectures)

## Target Repository

**RHH's pokeemerald-expansion:**
- Location: `/home/chris/Documents/Github/pokeemerald/`
- Base: pret's pokeemerald + extensive RHH enhancements
- Features: Modern GCC, config system, enhanced data structures, testing framework

## Minigames to Port

| # | Game | Complexity | Status | Estimated Hours | Actual Hours |
|---|------|------------|--------|-----------------|--------------|
| 1 | Snake | Simple | Not Started | 15-25 | - |
| 2 | Flappy Bird | Medium | Not Started | 20-35 | - |
| 3 | Blackjack | Medium | Not Started | 20-35 | - |
| 4 | Voltorb Flip | Medium | Not Started | 20-35 | - |
| 5 | Gacha | Medium | Not Started | 20-35 | - |
| 6 | Pachinko | Medium | Not Started | 20-35 | - |
| 7 | Block Stacker | High | Not Started | 30-50 | - |
| 8 | Pinball | High | Not Started | 30-50 | - |
| 9 | Derby | Unknown | Deferred | 30-50 | - |

**Total Estimated:** 200-300 hours

## Implementation Strategy

### Tier System

**Tier 1 - Establish Workflow (Start Here):**
1. Snake - Simplest logic, proves complete workflow
2. Flappy Bird - Physics/parallax, tests complex systems
3. Blackjack - Card game, tests UI framework

**Tier 2 - Build Confidence:**
4. Voltorb Flip - Puzzle logic, save persistence
5. Gacha - Prize integration
6. Pachinko - Physics-lite

**Tier 3 - Advanced (After Patterns Proven):**
7. Block Stacker - Large file, complex logic
8. Pinball - Advanced physics
9. Derby - Unknown complexity, defer to last

### Rationale

- Start with **simplest** (Snake) to establish workflow
- Each game builds on lessons from previous
- Defer most complex until patterns proven
- Derby last due to unknown requirements

## Current Game Corner State

### Existing in RHH Expansion (Mauville)

**Slot Machines:**
- File: `src/slot_machine.c`
- Count: 12 machines (6 difficulty levels × 2)
- Graphics: `graphics/slot_machine/`
- Status: ✅ Complete, keep as-is

**Roulette:**
- File: `src/roulette.c`
- Count: 2 tables
- Graphics: `graphics/roulette/`
- Status: ✅ Complete, keep as-is

### New Games Location (Mossdeep)

**1F - Simple Games & Services:**
- Coin Clerk
- Prize Counter
- Snake
- Blackjack
- Gacha

**B1F - Complex Games:**
- Flappy Bird
- Block Stacker
- Pinball
- Pachinko
- Voltorb Flip
- Derby

## Progress Tracking

### Phase 1: Setup & Infrastructure ✅ IN PROGRESS

- [x] Create documentation folder structure
- [ ] Write architecture comparison document
- [ ] Write code style migration guide
- [ ] Write Flappy Bird analysis (reference implementation)
- [ ] Create `include/config/game_corner.h` config system
- [ ] Implement `src/game_corner_common.c` shared utilities
- [ ] Allocate flags/vars in constants files

### Phase 2: Snake Implementation (Milestone 1)

- [ ] Documentation
  - [ ] Fetch snake.c from heyopc repo
  - [ ] Write ANALYSIS.md
  - [ ] Write ADAPTATION.md
  - [ ] Write IMPLEMENTATION.md
- [ ] Code Port
  - [ ] Create src/game_corner_snake.c
  - [ ] Create include/game_corner_snake.h
  - [ ] Apply naming conventions
  - [ ] Add proper includes
  - [ ] Convert loop iterators
- [ ] Graphics
  - [ ] Create graphics/game_corner_snake/
  - [ ] Port graphics to 4bpp/gbapal
  - [ ] Update code references
- [ ] Audio
  - [ ] Extract audio files
  - [ ] Add to sound/ directory
  - [ ] Add constants
- [ ] Integration
  - [ ] Add script to MossdeepCity_GameCorner_1F
  - [ ] Register special function
  - [ ] Test trigger
- [ ] Testing
  - [ ] Write unit tests
  - [ ] Gameplay testing
  - [ ] Edge case verification
- [ ] Completion
  - [ ] Code review
  - [ ] Documentation update
  - [ ] Checklist verification

### Phase 3-11: Remaining Games

Similar checklists will be created for each remaining game as Snake's workflow is proven.

## Key Deliverables

### Documentation

1. **Architecture Documents:**
   - 01_ARCHITECTURE_COMPARISON.md - pret vs RHH differences
   - 02_CODE_STYLE_MIGRATION.md - Conversion guide
   - 03_COMMON_INFRASTRUCTURE.md - Shared systems
   - 04_SAVE_STRATEGY.md - SaveBlock approach
   - 05_CONFIG_DESIGN.md - Config integration
   - 06_TESTING_APPROACH.md - Test strategy

2. **Per-Minigame Documentation:**
   - minigames/<game>/ANALYSIS.md - Original code breakdown
   - minigames/<game>/ADAPTATION.md - RHH-specific changes
   - minigames/<game>/IMPLEMENTATION.md - Step-by-step guide
   - minigames/<game>/TESTING.md - Test cases

3. **Integration Documentation:**
   - integration/MAUVILLE_INTEGRATION.md
   - integration/MOSSDEEP_INTEGRATION.md
   - integration/PRIZE_SYSTEM.md

### Code

**Config System:**
- include/config/game_corner.h

**Common Infrastructure:**
- src/game_corner_common.c
- include/game_corner_common.h

**Per-Game Files (×9):**
- src/game_corner_<game>.c
- include/game_corner_<game>.h
- graphics/game_corner_<game>/
- test/game_corner_<game>.c

**Integration:**
- Modified: include/global.h
- Modified: src/field_specials.c
- Modified: include/field_specials.h
- Modified: data/specials.inc
- Modified: data/maps/MossdeepCity_GameCorner_1F/scripts.inc
- Modified: data/maps/MossdeepCity_GameCorner_B1F/scripts.inc
- Modified: include/constants/flags.h
- Modified: include/constants/vars.h
- Modified: include/constants/songs.h

## Known Challenges

### Architectural Differences

1. **pret vs RHH Compilation:**
   - pret: agbcc (C99)
   - RHH: Modern GCC (C17, MODERN=1)
   - Impact: May need syntax updates

2. **Config System:**
   - pret: Preprocessor-based (`#ifdef`)
   - RHH: Runtime-based (`if (B_FEATURE)`)
   - Impact: Must convert all conditionals

3. **Data Structures:**
   - pret: Direct struct access
   - RHH: Enhanced structs, sometimes need getters/setters
   - Impact: Verify access patterns

4. **Naming Conventions:**
   - pret: Various styles
   - RHH: Strict PascalCase/camelCase/g/s prefixes
   - Impact: Systematic renaming required

### Save Compatibility

**Constraint:** Must preserve existing saves by default.

**Approach:**
- Use existing game stats for high scores (Option A)
- Only add SaveBlock data if explicitly configured (Option B)
- Document any save-breaking changes clearly

### Performance Targets

**Requirements:**
- 60 FPS during gameplay
- No memory leaks (100+ rounds tested)
- Responsive input (<1 frame latency)
- Smooth animations

## Blockers & Risks

### Current Blockers

None yet - project just starting.

### Potential Risks

1. **heyopc code quality unknown:**
   - Risk: Code may not be well-structured
   - Mitigation: Start with simplest game (Snake) to assess

2. **Graphics compatibility:**
   - Risk: heyopc graphics may use different formats
   - Mitigation: Establish conversion pipeline early

3. **Audio compatibility:**
   - Risk: Music/SFX may use incompatible formats
   - Mitigation: Test with existing RHH audio first

4. **Scope creep:**
   - Risk: Project may expand beyond 9 games
   - Mitigation: Stick to defined scope, defer Derby if needed

5. **Time estimates inaccurate:**
   - Risk: Games may take longer than estimated
   - Mitigation: Track actual hours, adjust future estimates

## Success Criteria

### Minimum Viable Product (MVP)

**Tier 1 Complete:**
- Snake, Flappy Bird, Blackjack working
- Full documentation established
- Workflow proven repeatable

### Full Success

**All 9 Games:**
- All minigames functional
- Save compatibility maintained
- 60 FPS performance
- Full test coverage
- Complete documentation
- Code meets RHH standards

### Stretch Goals

- Contribute back to RHH expansion (if desired)
- Add additional features (difficulty modes, tournaments)
- Create custom prizes for new games

## Timeline

**Phase 1 (Setup):** 1-2 weeks
- Documentation infrastructure
- Config system
- Common infrastructure

**Phase 2 (Snake):** 1-2 weeks
- First complete game implementation
- Workflow validation

**Phase 3-4 (Tier 1 Completion):** 3-4 weeks
- Flappy Bird
- Blackjack

**Phase 5-7 (Tier 2):** 4-6 weeks
- Voltorb Flip
- Gacha
- Pachinko

**Phase 8-10 (Tier 3):** 6-8 weeks
- Block Stacker
- Pinball
- Derby (if included)

**Total Estimated Timeline:** 3-5 months (part-time work)

## Resources

### Reference Materials

- **heyopc Repository:** https://github.com/heyopc/pokeemerald-gamecorner-expansion
- **RHH Documentation:** https://rh-hideout.github.io/pokeemerald-expansion/
- **RHH Discord:** https://discord.gg/6CzjAG6GZk
- **Existing CLAUDE.md:** `/home/chris/Documents/Github/pokeemerald/CLAUDE.md`
- **Styleguide:** `/home/chris/Documents/Github/pokeemerald/docs/STYLEGUIDE.md`

### Key Local Files

- **Current gamecorner:** `src/slot_machine.c`, `src/roulette.c`
- **Graphics reference:** `graphics/slot_machine/`, `graphics/roulette/`
- **Script reference:** `data/maps/MauvilleCity_GameCorner/scripts.inc`
- **Testing examples:** `test/battle/` (for test DSL patterns)

## Notes

### Design Decisions

1. **Mossdeep as hub:** Keeps Mauville untouched, new games in new location
2. **Config-driven:** All games can be disabled via configs
3. **Save-safe:** Default to no save breaks
4. **Test-first:** Write tests alongside code, not after
5. **Incremental:** One game at a time, fully complete before next

### Lessons Learned

This section will be updated as implementation progresses.

## Contact & Collaboration

**Project Lead:** User (chris)
**Assistant:** Claude Code
**Community:** RHH Discord (for questions/feedback)

---

**Last Updated:** 2025-12-16
**Current Phase:** Setup & Infrastructure
**Current Task:** Creating documentation foundation
