# Git Branching Strategy for ROM Hack Development

## Branch Structure

```
upstream/master (RHH pokeemerald-expansion - read-only)
    ↓
master (your fork - periodically synced with upstream)
    ↓
dev (your ROM hack - all features combined)
    ↓
    ├── feature/gamecorner-minigames
    ├── feature/new-battle-mechanics
    ├── feature/custom-region
    └── feature/qol-improvements
```

## Branch Purposes

### `master` - Upstream Mirror
- **Purpose:** Stay close to RHH's pokeemerald-expansion
- **Updates:** Sync with upstream regularly
- **Direct Commits:** Minimal (only urgent fixes or upstream contributions)
- **Merges Into:** Feature branches and `dev`

### `dev` - Your ROM Hack
- **Purpose:** Combined version with ALL features
- **Updates:** Merge completed feature branches here
- **Direct Commits:** Small tweaks, bug fixes, balancing
- **Testing:** Main playtesting branch
- **Releases:** Tag releases from this branch

### `feature/*` - Individual Features
- **Purpose:** Develop ONE feature in isolation
- **Updates:** Merge from `master` when upstream changes
- **Direct Commits:** All development for that feature
- **Merges Into:** `dev` when complete
- **Examples:**
  - `feature/gamecorner-minigames` - Port 9 minigames
  - `feature/new-battle-items` - Add custom held items
  - `feature/difficulty-modes` - Implement difficulty system

## Daily Workflow

### 1. Starting a New Feature

```bash
# Ensure master is up-to-date
git checkout master
git fetch upstream
git merge upstream/master
git push origin master

# Create feature branch from master
git checkout -b feature/your-feature-name master
git push -u origin feature/your-feature-name

# Work on feature
# ... make commits ...
```

### 2. Working on Existing Feature

```bash
# Switch to feature branch
git checkout feature/gamecorner-minigames

# Make changes
# ... edit files ...

# Commit
git add .
git commit -m "Implement Snake minigame core logic"
git push
```

### 3. Syncing Feature with Upstream Changes

When RHH releases updates you want:

```bash
# Update master
git checkout master
git fetch upstream
git merge upstream/master
git push origin master

# Update feature branch
git checkout feature/gamecorner-minigames
git merge master

# Resolve conflicts if any
# ... fix conflicts ...
git add .
git commit -m "Merge upstream changes from master"
git push
```

### 4. Merging Completed Feature into Dev

```bash
# Ensure feature is ready
git checkout feature/gamecorner-minigames
# ... final testing ...

# Switch to dev
git checkout dev

# Merge feature
git merge feature/gamecorner-minigames --no-ff
# The --no-ff creates a merge commit for clean history

# Test combined features
# ... playtesting ...

# Push to remote
git push origin dev

# Optional: Delete feature branch if completely done
git branch -d feature/gamecorner-minigames
git push origin --delete feature/gamecorner-minigames
```

### 5. Keeping Dev Updated with Master

Periodically merge master into dev to get upstream fixes:

```bash
git checkout dev
git merge master
# Resolve conflicts (favor your features when conflicts)
git push origin dev
```

## Conflict Resolution Strategy

### Master vs Upstream
- **Strategy:** Always favor upstream (RHH's changes)
- **Reason:** Keep master as clean mirror

### Master vs Feature Branch
- **Strategy:** Favor feature (your work)
- **Reason:** Feature is your custom code

### Feature vs Dev
- **Strategy:** Depends on feature priority
- **Reason:** Some features may override others

### Dev vs Master
- **Strategy:** Favor dev (your features)
- **Reason:** Dev is your final product

## Example: Game Corner Feature

### Current Status

```bash
$ git branch -a
* feature/gamecorner-minigames
  dev
  master
```

### Development Flow

```bash
# 1. Implement config system
git checkout feature/gamecorner-minigames
# ... create include/config/game_corner.h ...
git add include/config/game_corner.h
git commit -m "Add Game Corner config system"

# 2. Implement common infrastructure
# ... create src/game_corner_common.c ...
git add src/game_corner_common.c include/game_corner_common.h
git commit -m "Implement Game Corner common infrastructure"

# 3. Port first minigame (Snake)
# ... create src/game_corner_snake.c ...
git add src/game_corner_snake.c include/game_corner_snake.h
git commit -m "Port Snake minigame from heyopc"

# 4. Test Snake in isolation
make && ./pokeemerald.elf
# ... test Snake thoroughly ...

# 5. Continue with more minigames
# ... repeat for Flappy Bird, Blackjack, etc. ...

# 6. When all 9 minigames complete, merge to dev
git checkout dev
git merge feature/gamecorner-minigames --no-ff -m "Merge Game Corner minigames feature

Complete implementation of 9 new minigames:
- Snake
- Flappy Bird
- Blackjack
- Voltorb Flip
- Gacha
- Pachinko
- Block Stacker
- Pinball
- Derby

Includes shared infrastructure, config system, and Mossdeep integration."
```

## Advanced: Rebasing Feature Branches

For cleaner history, use rebase instead of merge:

```bash
# Update master
git checkout master
git pull upstream master
git push origin master

# Rebase feature onto updated master
git checkout feature/gamecorner-minigames
git rebase master

# If conflicts occur
# ... resolve conflicts ...
git add .
git rebase --continue

# Force push (rebase rewrites history)
git push --force-with-lease
```

**Warning:** Only rebase if you haven't shared the branch or if you're the only developer.

## Upstream Contribution Workflow

If you want to contribute a feature back to RHH:

```bash
# 1. Create branch from clean master
git checkout master
git pull upstream master
git checkout -b feature/upstream-contribution

# 2. Implement feature following RHH standards
# ... clean implementation, no ROM-hack-specific code ...

# 3. Push to your fork
git push -u origin feature/upstream-contribution

# 4. Create Pull Request on GitHub
# Target: rh-hideout/pokeemerald-expansion:master
# Source: your-username/pokeemerald:feature/upstream-contribution
```

## Tagging Releases

Mark stable versions of your ROM hack:

```bash
git checkout dev
# ... ensure everything is tested and stable ...

git tag -a v1.0.0 -m "Release v1.0.0 - Game Corner Minigames

Features:
- 9 new Game Corner minigames in Mossdeep
- Complete prize system
- High score tracking
- Config-driven difficulty settings"

git push origin v1.0.0
```

## Branch Protection (GitHub Settings)

Recommended settings on GitHub:

### `master` Branch
- ✅ Require pull request reviews
- ✅ Require status checks to pass
- ✅ Restrict who can push (only you)

### `dev` Branch
- ✅ Require status checks to pass
- ⚠️ Allow direct pushes (for quick fixes)

### `feature/*` Branches
- ✅ Delete after merge to dev
- ✅ Allow force push (for rebasing)

## Workflow Cheat Sheet

```bash
# Create new feature
git checkout -b feature/name master

# Daily work
git checkout feature/name
# ... edit ...
git add .
git commit -m "Message"
git push

# Sync with upstream
git checkout master && git pull upstream master && git push
git checkout feature/name && git merge master

# Merge to dev
git checkout dev && git merge feature/name --no-ff

# Update dev from master
git checkout dev && git merge master
```

## Emergency: Fix Critical Bug

If a critical bug is found in dev:

```bash
# Option 1: Fix directly in dev (small fix)
git checkout dev
# ... fix bug ...
git commit -m "Fix critical crash in Game Corner"
git push

# Option 2: Create hotfix branch (larger fix)
git checkout -b hotfix/game-corner-crash dev
# ... fix bug ...
git commit -m "Fix Game Corner crash on exit"
git checkout dev
git merge hotfix/game-corner-crash
git branch -d hotfix/game-corner-crash
```

## Visualizing Your Branches

```bash
# Show branch graph
git log --oneline --graph --all --decorate

# Show current branches
git branch -vv

# Show remote branches
git branch -r
```

## Summary

**master:** Upstream mirror (periodic syncs with RHH)
**dev:** Your ROM hack (all features combined)
**feature/\*:** Individual features (isolated development)

**Key Benefits:**
- ✅ Features developed in isolation (easy testing)
- ✅ Easy to sync with upstream (master stays clean)
- ✅ Can selectively include/exclude features in dev
- ✅ Can share individual features with others
- ✅ Can contribute features back to RHH

**Current Setup:**
```
master ← syncs with upstream/master
dev ← your ROM hack (has Game Corner docs)
feature/gamecorner-minigames ← current work (ready for implementation)
```

---

**Last Updated:** 2025-12-16
**Current Feature:** Game Corner Minigames Port
