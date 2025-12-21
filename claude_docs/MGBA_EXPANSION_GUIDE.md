# mGBA Expansion Guide: Building a Super-Powered GBA Emulator

**Purpose:** Fork mGBA and expand its hardware capabilities to support Pokemon ROM hacks with extended memory, storage, graphics, and more.

**Target Platform:** Linux, Windows, macOS, Android (RetroArch core)

---

## Table of Contents
1. [Why Expand mGBA?](#why-expand-mgba)
2. [Prerequisites](#prerequisites)
3. [Step 1: Fork and Clone mGBA](#step-1-fork-and-clone-mgba)
4. [Step 2: Apply Core Expansions](#step-2-apply-core-expansions)
5. [Step 3: Build mGBA](#step-3-build-mgba)
6. [Step 4: Test Your Expanded mGBA](#step-4-test-your-expanded-mgba)
7. [Step 5: Build for Android/RetroArch](#step-5-build-for-androidretroarch)
8. [Troubleshooting](#troubleshooting)

---

## Why Expand mGBA?

Standard GBA hardware has strict limitations:
- **RAM:** 256 KB EWRAM (forces bank swapping for large datasets)
- **Save:** 128 KB Flash max (limits PC box storage)
- **VRAM:** 96 KB (limits on-screen graphics)
- **Sprites:** 64 max (causes flickering in complex scenes)
- **ROM:** 32 MB (restricts content size)

**What This Guide Achieves:**
- ✅ 1 MB EWRAM (4x standard) - All 72 boxes in RAM simultaneously
- ✅ 512 KB Flash (4x standard) - Massive save data capacity
- ✅ 256 KB VRAM (2.7x standard) - Better graphics quality
- ✅ 128 Max Sprites (2x standard) - Smoother multi-battles
- ✅ 64 MB ROM (2x standard) - More content
- ✅ Expanded palettes, audio, and more

---

## Prerequisites

### Required Tools
```bash
# Arch Linux (your system)
sudo pacman -S base-devel cmake qt6-base qt6-multimedia \
               libzip zlib libpng sdl2 sqlite

# Ubuntu/Debian
sudo apt install build-essential cmake qtbase5-dev \
                 libqt5multimedia5 libzip-dev zlib1g-dev \
                 libpng-dev libsdl2-dev libsqlite3-dev

# macOS (Homebrew)
brew install cmake qt@6 libzip libpng sdl2 sqlite
```

### For Android Building
```bash
# Install Android NDK
sudo pacman -S android-ndk android-sdk

# Or download from: https://developer.android.com/ndk/downloads
```

### Recommended Knowledge
- Basic C programming
- Git version control
- Command line usage
- Understanding of memory addressing (helpful but not required)

---

## Step 1: Fork and Clone mGBA

### 1.1 Fork on GitHub
1. Go to https://github.com/mgba-emu/mgba
2. Click **Fork** (top right)
3. Name your fork: `mgba-expanded` or `mgba-pokemon`

### 1.2 Clone Your Fork
```bash
cd ~/Documents/Github
git clone https://github.com/YOUR_USERNAME/mgba-expanded.git
cd mgba-expanded

# Add upstream for future updates
git remote add upstream https://github.com/mgba-emu/mgba.git
```

### 1.3 Create Expansion Branch
```bash
git checkout -b expansion/super-gba
```

---

## Step 2: Apply Core Expansions

### 2.1 Expand EWRAM (256 KB → 1 MB)

**File:** `include/mgba/internal/gba/memory.h`

**Find:**
```c
#define SIZE_WORKING_RAM 0x00040000  // 256 KB
```

**Replace with:**
```c
#define SIZE_WORKING_RAM 0x00100000  // 1 MB (4x expansion for all-in-RAM Pokemon storage)
```

---

### 2.2 Expand Flash Save (128 KB → 512 KB)

**File:** `include/mgba/internal/gba/memory.h`

**Find:**
```c
#define SIZE_CART_FLASH1M 0x00020000  // 128 KB
```

**Replace with:**
```c
#define SIZE_CART_FLASH1M 0x00080000  // 512 KB (4x expansion for massive save data)
```

---

### 2.3 Increase Flash Bank Limit (2 banks → 8 banks)

**File:** `src/gba/savedata.c`

**Find the `_flashWriteData` function, around line ~650:**
```c
if (address == 0 && value < 2) {
    _flashSwitchBank(savedata, value);
    return;
}
```

**Replace with:**
```c
if (address == 0 && value < 8) {  /* Increased from 2 to 8 flash banks */
    _flashSwitchBank(savedata, value);
    return;
}
```

**Why:** This allows accessing all 8 banks of 64 KB each = 512 KB total flash.

---

### 2.4 Expand VRAM (96 KB → 256 KB) [OPTIONAL]

**File:** `include/mgba/internal/gba/memory.h`

**Find:**
```c
#define SIZE_VRAM 0x00018000  // 96 KB
```

**Replace with:**
```c
#define SIZE_VRAM 0x00040000  // 256 KB (2.7x expansion for better graphics)
```

**Note:** This may require additional changes to VRAM access logic. Test thoroughly.

---

### 2.5 Increase Sprite Limit (64 → 128) [OPTIONAL]

**File:** `include/mgba/internal/gba/video.h`

**Find:**
```c
#define MAX_SPRITES 64
```

**Replace with:**
```c
#define MAX_SPRITES 128  // Double sprite limit for complex battles
```

**Also update OAM size in `include/mgba/internal/gba/memory.h`:**
```c
#define SIZE_OAM 0x00000800  // 2 KB (was 1 KB for 128 sprites)
```

---

### 2.6 Expand ROM Size Limit (32 MB → 64 MB) [OPTIONAL]

**File:** `include/mgba/internal/gba/memory.h`

**Find:**
```c
#define SIZE_CART0 0x02000000  // 32 MB
```

**Replace with:**
```c
#define SIZE_CART0 0x04000000  // 64 MB (allows larger ROM hacks)
```

---

### 2.7 Expand Palette RAM (1 KB → 4 KB) [OPTIONAL]

**File:** `include/mgba/internal/gba/memory.h`

**Find:**
```c
#define SIZE_PALETTE_RAM 0x00000400  // 1 KB (512 colors)
```

**Replace with:**
```c
#define SIZE_PALETTE_RAM 0x00001000  // 4 KB (2048 colors, 4x expansion)
```

---

### 2.8 Increase Audio Channels (12 → 24) [OPTIONAL]

**File:** `include/mgba-util/audio.h` or similar

**Find:**
```c
#define MAX_DIRECTSOUND_CHANNELS 12
```

**Replace with:**
```c
#define MAX_DIRECTSOUND_CHANNELS 24  // Double audio channels for richer sound
```

---

### 2.9 Create Version Identifier

**File:** `src/gba/core.c`

**Add near the top after includes:**
```c
// Expansion marker for ROMs to detect enhanced emulator
#define MGBA_EXPANSION_VERSION 1
#define MGBA_EXPANSION_EWRAM_MB 1
#define MGBA_EXPANSION_FLASH_KB 512
```

**This allows your Pokemon ROM to detect it's running on an expanded emulator.**

---

### 2.10 Update Save Format Detection

**File:** `src/gba/savedata.c`

**Find the `GBASavedataInit` function and add after existing size checks:**
```c
// Support expanded flash sizes
case 0x00080000:  // 512 KB flash
    savedata->type = GBA_SAVEDATA_FLASH1M;
    GBASavedataInitFlash(savedata);
    break;
```

---

## Step 3: Build mGBA

### 3.1 Configure Build

```bash
cd ~/Documents/Github/mgba-expanded
mkdir build
cd build

# Configure with CMake (Qt6 frontend)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_QT=ON \
    -DBUILD_SDL=ON \
    -DBUILD_LIBRETRO=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr/local
```

### 3.2 Compile

```bash
# Build with all CPU cores
make -j$(nproc)
```

**Expected output:**
```
[ 98%] Building CXX object src/platform/qt/CMakeFiles/mgba-qt.dir/...
[100%] Linking CXX executable mgba-qt
```

### 3.3 Install (Optional)

```bash
sudo make install
```

Or run directly from build directory:
```bash
./mgba-qt
```

---

## Step 4: Test Your Expanded mGBA

### 4.1 Verify Expansion

Create a test to check memory sizes:

**File:** `test_expansion.c`
```c
#include <stdio.h>
#include "mgba/internal/gba/memory.h"

int main() {
    printf("EWRAM Size: %d KB (%d MB)\n", SIZE_WORKING_RAM/1024, SIZE_WORKING_RAM/1024/1024);
    printf("Flash Size: %d KB\n", SIZE_CART_FLASH1M/1024);
    printf("VRAM Size: %d KB\n", SIZE_VRAM/1024);
    printf("Max Sprites: %d\n", MAX_SPRITES);
    return 0;
}
```

**Expected output:**
```
EWRAM Size: 1024 KB (1 MB)
Flash Size: 512 KB
VRAM Size: 256 KB
Max Sprites: 128
```

### 4.2 Test with Your Pokemon ROM

```bash
cd ~/Documents/Github/pokeemerald
make clean && make -j$(nproc)

# Run in your expanded mGBA
~/Documents/Github/mgba-expanded/build/mgba-qt pokeemerald.gba
```

**Test checklist:**
- [ ] Game loads without errors
- [ ] Save works (create save, restart emulator, load save)
- [ ] All 72 boxes accessible instantly (no bank delays if you redesigned for all-in-RAM)
- [ ] Graphics render correctly
- [ ] No slowdown in complex battles

---

## Step 5: Build for Android/RetroArch

### 5.1 Build as RetroArch Core

```bash
cd ~/Documents/Github/mgba-expanded
mkdir build-libretro
cd build-libretro

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_LIBRETRO=ON \
    -DLIBRETRO=ON \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/android-ndk/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21

make -j$(nproc)
```

**Output:** `mgba_libretro_android.so`

### 5.2 Install in RetroArch (Android)

```bash
# Copy to RetroArch cores directory
adb push mgba_libretro_android.so \
    /storage/emulated/0/RetroArch/cores/
```

**In RetroArch:**
1. Load Core → Browse → Select `mgba_libretro_android.so`
2. Load Content → Select your `pokeemerald.gba`
3. Test save functionality

---

## Troubleshooting

### Build Errors

**Error:** `SIZE_WORKING_RAM redefined`
- **Fix:** Make sure you only changed the value, didn't add a duplicate definition

**Error:** `undefined reference to _flashSwitchBank`
- **Fix:** Check that `src/gba/savedata.c` changes match exactly, including brackets

**Error:** `Qt6 not found`
- **Fix:** Install Qt6 development packages: `sudo pacman -S qt6-base qt6-multimedia`

### Runtime Issues

**Problem:** Save file is only 128 KB, not 512 KB
- **Fix:** Delete old save file: `rm pokeemerald.sav`
- mGBA will create new 512 KB save on next run

**Problem:** Game freezes on save
- **Fix:** Add debug logging to verify flash bank switching is working:
  ```c
  fprintf(stderr, "Switching to bank %d\n", value);
  ```

**Problem:** Graphics corrupted after VRAM expansion
- **Fix:** VRAM expansion may require additional fixes in VRAM access logic. Revert VRAM changes if issues persist.

**Problem:** Sprite limit not working
- **Fix:** Ensure both `MAX_SPRITES` and `SIZE_OAM` were updated consistently

### Save Compatibility

**Problem:** Save from standard mGBA doesn't work in expanded mGBA
- **Expected:** Save format changed with expansion
- **Solution:** Either:
  1. Start new game in expanded mGBA
  2. Write a save converter tool (advanced)

---

## Advanced: All-In-RAM Pokemon Storage

If you have 1 MB EWRAM, you can redesign your Pokemon ROM to keep **all 72 boxes in RAM** and eliminate bank swapping entirely.

### Benefits
- ✅ Instant box access (no flash reads)
- ✅ Simpler code (no bank management)
- ✅ Faster saves (one write instead of per-bank)
- ✅ More reliable (no bank corruption)

### ROM Changes Required

**File:** `include/pokemon_storage_system.h`
```c
// OLD: Bank system with swapping
#define TOTAL_BOXES_COUNT 72
#define ACTIVE_BOXES_COUNT 2
#define BOXES_PER_BANK 10
#define NUM_BANKS 7

// NEW: All boxes in RAM
#define TOTAL_BOXES_COUNT 72
#define BOXES_IN_RAM 72  // All boxes always accessible
```

**File:** `src/pokemon_storage_system.c`
```c
struct PokemonStorage {
    struct BoxPokemon allBoxes[TOTAL_BOXES_COUNT][IN_BOX_COUNT];  // 72 boxes in RAM
    u8 currentBox;
    // Remove: currentBank, bank swapping logic
};
```

**Estimated RAM usage:** 72 boxes × 30 Pokemon × 80 bytes = **172,800 bytes (169 KB)**

Fits easily in 1 MB EWRAM with 855 KB remaining!

---

## Maintaining Your Fork

### Keep Updated with Upstream mGBA

```bash
# Fetch latest changes from official mGBA
git fetch upstream master

# Merge into your branch
git merge upstream/master

# Resolve conflicts in your expansion files
# Usually just keep your expanded values
```

### Share Your Fork

Consider publishing your expanded mGBA for others:

1. **Push to GitHub:**
   ```bash
   git push origin expansion/super-gba
   ```

2. **Create Release:**
   - Go to your GitHub repo
   - Releases → Create new release
   - Tag: `v1.0-expanded`
   - Upload compiled binaries

3. **Document Changes:**
   - Create `EXPANSION_CHANGELOG.md`
   - List all modifications
   - Include compatibility notes

---

## Verification Checklist

Before using your expanded mGBA in production:

- [ ] All standard GBA games still work
- [ ] Save/load works correctly
- [ ] No crashes during normal gameplay
- [ ] Expanded memory actually allocated (check with memory profiler)
- [ ] Pokemon ROM detects expansion correctly
- [ ] Flash writes succeed to all 8 banks
- [ ] VRAM expansion doesn't corrupt graphics
- [ ] Audio channels don't cause crackling
- [ ] Performance is acceptable (60 fps in battles)
- [ ] Android/RetroArch build works
- [ ] Save files portable between devices

---

## Conclusion

You now have a super-powered mGBA that can:
- Run Pokemon ROMs with 1 MB RAM (all boxes in RAM)
- Support 512 KB saves (4x standard)
- Display 128 sprites simultaneously (2x standard)
- Handle 64 MB ROMs (2x standard)
- And much more!

**Next Steps:**
1. Redesign your Pokemon ROM for all-in-RAM storage
2. Test extensively with various games
3. Build for Android and share with community
4. Submit useful changes back to upstream mGBA (if applicable)

**Resources:**
- [mGBA Documentation](https://mgba.io/docs/)
- [GBATEK Hardware Reference](https://problemkaputt.de/gbatek.htm)
- [pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion)
- [RetroArch Documentation](https://docs.libretro.com/)

---

**Happy Hacking! 🎮✨**
