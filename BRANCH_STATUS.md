# Branch Status Report

**Generated:** 2025-12-16

## Current Branch Relationship

```
upstream/master (6855a5a732)
    ↓ (2 commits ahead)
master (90ac6116e5)
    ↓ (4 commits with custom changes)
    ↓ + Game Corner docs
dev (e4991f5fa1)
    ↓ + Git workflow guide
feature/gamecorner-minigames (fd39fe9faf)
```

## Detailed Analysis

### 1. Master vs Upstream/Master

**Status:** ❌ NOT EQUAL

**Master has 4 commits not in upstream:**
1. `90ac6116e5` - Merge upstream master with extensive battle system improvements
2. `041190572b` - Refactor map data and fix JSON formatting
3. `b6bd2adfb4` - **Remove levels from stat calculations for battles. default to level 50.**
4. `e370bfa523` - Update README for clarity on pokeemerald options

**Upstream has 2 commits not in master:**
1. `6855a5a732` - Fix species gfx change in link battles (#8552)
2. `ec1a283b1b` - Add SUB_HIT check to tests (#8413)

**Code Differences:**
- ✅ **Custom battle system:** Level 50 default for all battles (7 files modified)
- ✅ **Map changes:** LittlerootTown map data and JSON formatting
- ✅ **Test cleanup:** Removed some test code (107 lines from substitute.c, 121 lines from test_runner)
- ❌ **Missing upstream fixes:** Species gfx link battle fix, SUB_HIT test improvements

**Conclusion:** Your `master` is NOT a clean mirror of upstream. It has custom battle system changes.

---

### 2. Feature/gamecorner-minigames vs Master

**Status:** ✅ SAME CODE, only docs added

**Differences:**
```
master:
  - Has custom battle system (level 50 default)
  - Has map refactoring
  - NO Game Corner docs
  - NO source code changes for Game Corner

feature/gamecorner-minigames:
  - Has SAME custom battle system
  - Has SAME map refactoring
  - Has Game Corner docs (12 files)
  - Has Git workflow guide (1 file)
  - NO source code changes for Game Corner yet
```

**Files in feature branch not in master:**
1. `CLAUDE.md` - Repository guide
2. `GIT_WORKFLOW.md` - Branching workflow guide
3. `claude_docs/gamecorner_project/` - 11 documentation files

**Source Code:** ✅ Identical (no actual Game Corner implementation yet)

**Conclusion:** Feature branch is master + documentation only. No code changes yet.

---

### 3. Dev vs Feature/gamecorner-minigames

**Status:** ✅ ALMOST IDENTICAL

**Only Difference:**
- `feature/gamecorner-minigames` has `GIT_WORKFLOW.md`
- `dev` does NOT have `GIT_WORKFLOW.md`

**Why:** GIT_WORKFLOW.md was committed to feature branch AFTER dev was created.

**Conclusion:** Branches are essentially the same. Feature branch is 1 commit ahead.

---

## Modified Files by Branch

### Master (vs upstream)
**Modified Core Files:**
- `src/battle_ai_util.c` - Level 50 default
- `src/battle_dome.c` - Level 50 default
- `src/battle_script_commands.c` - Level 50 default
- `src/battle_tv.c` - Level 50 default
- `src/battle_util.c` - Level 50 default
- `src/battle_util2.c` - Level 50 default
- `src/pokemon.c` - Level 50 default
- `data/layouts/LittlerootTown/map.bin` - Map data
- `data/maps/LittlerootTown/map.json` - Map JSON

**Removed Files:**
- `test/battle/move_effect/substitute.c` - Test cleanup
- Partial removal from `test/test_runner_battle.c`
- Partial removal from `docs/tutorials/how_to_testing_system.md`
- Partial removal from `include/test/battle.h`
- Partial removal from `include/test_runner.h`

### Dev (master + docs)
**All master changes PLUS:**
- `CLAUDE.md`
- `claude_docs/gamecorner_project/` (11 files)

### Feature/gamecorner-minigames (dev + workflow)
**All dev changes PLUS:**
- `GIT_WORKFLOW.md`

---

## Game Corner Implementation Status

### Documentation: ✅ Complete
- Architecture comparison
- Code style migration guide
- Common infrastructure design
- Save strategy
- Config design
- Testing approach
- Integration guides (Mauville, Mossdeep, Prize System)
- Flappy Bird analysis (reference)

### Source Code: ❌ Not Started
- `include/config/game_corner.h` - NOT created
- `src/game_corner_common.c` - NOT created
- Minigame implementations - NOT created
- Map files (Mossdeep) - NOT created
- Script files - NOT created
- Graphics - NOT ported
- Audio - NOT ported

---

## Recommendations

### Option 1: Keep Current Structure (Recommended)
**Accept that master has custom changes:**
- ✅ Master = Upstream + your battle system tweaks
- ✅ Feature branches = Master + specific feature
- ✅ Dev = All features combined

**Workflow:**
```bash
# Sync master with upstream periodically
git checkout master
git pull upstream master
# Resolve conflicts (favor your battle system changes)
git push

# Update feature branches
git checkout feature/gamecorner-minigames
git merge master
```

### Option 2: Create Clean Upstream Mirror
**If you want a truly clean mirror:**
```bash
# Create new branch that purely mirrors upstream
git checkout -b upstream-mirror upstream/master
git push -u origin upstream-mirror

# Keep master as your "base" with battle system changes
# Feature branches continue from master
```

### Option 3: Move Battle System to Feature Branch
**Isolate battle system changes:**
```bash
# Create battle system feature branch
git checkout -b feature/battle-system-level50 upstream/master
git cherry-pick b6bd2adfb4  # Level 50 commit
git cherry-pick 041190572b  # Map refactor
git cherry-pick e370bfa523  # README update

# Reset master to upstream
git checkout master
git reset --hard upstream/master
git push --force-with-lease

# Now master is clean mirror
# Both features can merge to dev independently
```

---

## Current Situation Summary

**Your branches are correctly set up for feature development, BUT:**

1. ❌ **Master ≠ Upstream** - Master has custom battle system (level 50 default)
2. ✅ **Feature = Master + Docs** - Correctly isolated (no code yet)
3. ✅ **Dev = Master + Docs** - Correctly combined

**This is fine IF:**
- You accept master will always have battle system changes
- You sync master with upstream and resolve conflicts manually
- You're okay with all features building on top of level 50 battle system

**This is NOT fine IF:**
- You want Game Corner feature to work with vanilla battle system
- You want to contribute Game Corner back to RHH (requires clean master)
- You want to selectively enable/disable battle system vs Game Corner

---

## Action Items

### Immediate (Choose One)

**A. Accept Current State (Easiest):**
- Continue with current structure
- Accept master has battle system changes
- All features build on this base

**B. Clean Up Master (Most Flexible):**
- Move battle system to `feature/battle-system-level50`
- Reset master to upstream
- Both features merge to dev independently

### Next Steps

**After deciding on structure:**
1. ✅ Sync master with upstream (get those 2 missing commits)
2. ✅ Merge updated master into feature branches
3. ✅ Start implementing Game Corner code in `feature/gamecorner-minigames`
4. ✅ Test features individually before merging to dev

---

**Current Working Branch:** `dev`
**Ready for Development:** `feature/gamecorner-minigames`
**Needs Sync:** `master` (2 commits behind upstream)
