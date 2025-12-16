# Flappy Bird - Complete Code Analysis

## Document Purpose

This document provides a comprehensive breakdown of heyopc's Flappy Bird implementation (from `flappybird.c`, 1,914 lines). This serves as:
1. Reference implementation for understanding heyopc coding patterns
2. Template for analyzing other minigames
3. Guide for RHH-style port planning

**Source:** https://raw.githubusercontent.com/heyopc/pokeemerald-gamecorner-expansion/master/src/flappybird.c

## Executive Summary

**Game Type:** Physics-based endless runner (Flappy Bird clone)
**Code Size:** 1,914 lines of C code
**Complexity:** Medium (physics engine, parallax scrolling, collision detection)
**Character:** Butterfree (replaces bird)
**Mechanics:** Tap to flap, avoid pipes, score points

**Key Systems:**
- State machine (8 states)
- Physics engine (gravity, jump velocity, terminal velocity)
- Parallax scrolling (3 background layers at different speeds)
- Procedural obstacle generation (5 patterns)
- Sprite animation system (wing flapping, particle trails)
- Collision detection (hitboxes against obstacles and boundaries)
- High score persistence
- Coin rewards system

## 1. Core Data Structures

### Primary Game State (struct FlappyBird)

**Location in code:** Likely defined near top of flappybird.c

```c
struct FlappyBird {
    // State Management
    u8 state;                              // Current game state (FLAPPY_INIT, FLAPPY_INPUT, etc.)

    // UI Sprites
    u8 CreditSpriteIds[MAX_SPRITES_CREDIT]; // Score display (4 digits)
    u8 ButterfreeSpriteId;                  // Main character
    u8 ButterfreeHitboxSpriteId;            // Collision detection sprite
    u8 BorderSpriteId;                      // UI border decoration
    u8 BorderSprite2Id;
    u8 TrailSpriteId;                       // Particle trail effect
    u8 DamageSpriteId;                      // Death animation sprite
    u8 FlapSpriteId;                        // "FLAP" instruction text
    u8 ScoreSpriteId, Score2SpriteId;       // Score label sprites
    u8 ScoreSpriteIds[MAX_SPRITES_HISCORE]; // High score digits (4)
    u8 StartSpriteId;                       // "GO" sprite
    u8 OneSpriteId, TwoSpriteId, ThreeSpriteId; // Countdown: 3, 2, 1
    u8 GameOverSpriteId;                    // "GAME OVER" text
    u8 HiScoreSpriteId;                     // "HIGH SCORE" text

    // Scrolling State
    u16 scroll_fg_x_int;                    // Foreground X scroll (integer part)
    u16 scroll_fg_x_frac;                   // Foreground X scroll (fractional part)
    u16 scroll_bg_x_int;                    // Background X scroll (integer)
    u16 scroll_bg_x_frac;                   // Background X scroll (fractional)
    u16 bg2ScrollX;                         // BG2 current X position (0-256, wraps)

    // Physics
    s16 pos_y;                              // Bird's Y position (pixel-perfect)
    s16 speed_y;                            // Bird's Y velocity (signed for up/down)
    u8 flap_strength;                       // Flap input strength
    s16 velocity;                           // General velocity for death animation

    // Collision Boundaries (Dynamic)
    u16 MAX_Y, MIN_Y;                       // Safe zone Y boundaries (changes per obstacle)

    // Obstacles
    u8 Obstacle1Id;                         // Current obstacle pattern (1-5)
    u8 Obstacle2Id;                         // Next obstacle pattern

    // Particle Trail
    s16 TrailoffsetX, TrailoffsetY;         // Trail sprite offset from Butterfree

    // Timers & Flags
    u32 delay;                              // General frame delay counter
    u16 timerDelay;                         // Secondary timer
    u8 timer;                               // Main timer
    u8 yUpdate;                             // Y position update counter
    u8 isFlapping;                          // Flap state flag
    u8 RotateToggle;                        // Rotation animation toggle
    u8 jumping;                             // Vertical movement state
    u8 SFX;                                 // Sound effect sequence counter

    // Death Animation
    s16 DamageSpriteRotation;               // Rotation angle for death sprite
    s16 DamageSpriteY;                      // Death sprite Y position
    s16 jumpHeight, jumpSpeed, fallSpeed;   // Death animation physics

    // Scoring
    u32 Points;                             // Current score

    // (Total size estimation: ~100-120 bytes)
};

// Static instance allocated in EWRAM (fast RAM)
static EWRAM_DATA struct FlappyBird *sFlappy = NULL;
```

**Key Constants:**
```c
#define MAX_Y_POSITION 10           // Top boundary (ceiling)
#define MIN_Y_POSITION 140          // Bottom boundary (ground)
#define GRAVITY 1                   // Downward acceleration per frame
#define FLAP_STRENGTH -6            // Upward velocity on flap (negative = up)
#define SMOOTHING_FACTOR 4          // Trail particle smoothing
#define INITIAL_SHOVE 10            // Trail offset on flap

#define MAX_SPRITES_CREDIT 4        // Score display digits
#define MAX_SPRITES_HISCORE 4       // High score digits
```

## 2. State Machine

### States Overview

```c
enum FlappyBirdState {
    FLAPPY_INIT,         // Initialization & countdown (3-2-1-Go)
    FLAPPY_INPUT,        // Main gameplay
    FLAPPY_STOP,         // Collision wobble effect
    FLAPPY_ROTATION,     // Death animation (rotate & fall)
    FLAPPY_GAMEOVER,     // Game over screen
    FLAPPY_HISCORE,      // High score achieved screen
    FLAPPY_EXIT,         // Fade out & return to field
    // Possibly more internal states
};
```

### State Transition Diagram

```
StartFlappyBird()
    ↓
FLAPPY_INIT (75 frames)
    ├─ Display countdown: 3, 2, 1, Go
    ├─ Play Butterfree cry
    └─ Initialize physics (pos_y, speed_y)
    ↓
FLAPPY_INPUT (Main Loop)
    ├─ Scroll backgrounds (parallax)
    ├─ Generate obstacles at X=126 and X=252
    ├─ Apply gravity to Butterfree
    ├─ Handle A button flap
    ├─ Update trail particle
    ├─ Check collisions (ceiling, floor, pipes)
    ├─ Score points when passing obstacles
    ├─ [Collision detected] → FLAPPY_STOP
    └─ [Continue] → Loop
    ↓
FLAPPY_STOP (15 frames)
    ├─ Wobble effect (sprite jitters up/down)
    ├─ Play collision sound
    └─ Transition to FLAPPY_ROTATION
    ↓
FLAPPY_ROTATION (Until off-screen)
    ├─ Bird rises then falls (parabolic arc)
    ├─ Damage sprite rotates continuously
    ├─ [Off-screen bottom] → FLAPPY_GAMEOVER
    └─ Loop
    ↓
FLAPPY_GAMEOVER (140 frames)
    ├─ Display "GAME OVER"
    ├─ Check if score > high score
    ├─ [New high score] → FLAPPY_HISCORE
    └─ [No new record] → FLAPPY_EXIT
    ↓
FLAPPY_HISCORE (100 frames)
    ├─ Display "HIGH SCORE"
    ├─ Update VAR_FLAPPY_HISCORE
    └─ Transition to FLAPPY_EXIT
    ↓
FLAPPY_EXIT
    ├─ Fade to black
    ├─ Free resources
    └─ CB2_ReturnToFieldContinueScriptPlayMapMusic()
```

### State Implementation Pattern

**Main game loop (FlappyBirdMain task function):**
```c
static void FlappyBirdMain(u8 taskId)
{
    switch (sFlappy->state)
    {
    case FLAPPY_INIT:
        HandleInitState();
        break;
    case FLAPPY_INPUT:
        HandleGameplayState();
        break;
    case FLAPPY_STOP:
        HandleCollisionWobble();
        break;
    case FLAPPY_ROTATION:
        HandleDeathAnimation();
        break;
    case FLAPPY_GAMEOVER:
        HandleGameOverScreen();
        break;
    case FLAPPY_HISCORE:
        HandleHighScoreScreen();
        break;
    case FLAPPY_EXIT:
        HandleExit();
        break;
    }
}
```

## 3. Physics System

### Gravity Application

**Every frame during FLAPPY_INPUT:**
```c
static void ApplyGravity(void)
{
    sFlappy->speed_y += GRAVITY;  // Accelerate downward (+1 per frame)
    sFlappy->pos_y += sFlappy->speed_y; // Update position

    // Terminal velocity cap
    if (sFlappy->speed_y > 4)
        sFlappy->speed_y = 4;

    // Ground collision
    if (sFlappy->pos_y > sFlappy->MIN_Y)
    {
        PlayBGM(MUS_NONE);
        PlaySE(SE_SUPER_EFFECTIVE);
        sFlappy->state = FLAPPY_STOP;
    }

    // Ceiling collision
    if (sFlappy->pos_y < sFlappy->MAX_Y)
    {
        sFlappy->state = FLAPPY_STOP;
    }
}
```

**Physics Parameters:**
- **Gravity:** +1 px/frame² (constant downward acceleration)
- **Flap Strength:** -6 px/frame (instant upward velocity)
- **Terminal Velocity:** 4 px/frame (maximum fall speed)

**Feel:**
- Fast falling (4 px/frame max ≈ 240 px/second at 60 FPS)
- Strong flap impulse creates "snappy" feel
- Low terminal velocity prevents uncontrollable falls

### Flap Mechanics

**Triggered by A button:**
```c
static void Flap(void)
{
    PlaySE(SE_M_WING_ATTACK);  // Wing sound effect

    // Start wing flapping animation
    if (gSprites[sFlappy->ButterfreeSpriteId].animNum == 0)
        gSprites[sFlappy->ButterfreeSpriteId].animNum = 1;  // Play flap animation
    else
    {
        gSprites[sFlappy->ButterfreeSpriteId].animCmdIndex = 0;  // Restart animation
        gSprites[sFlappy->ButterfreeSpriteId].animPaused = FALSE;
    }

    // Apply upward velocity
    sFlappy->speed_y = FLAP_STRENGTH;  // Set to -6 (upward)
}
```

**Input Handling:**
```c
// In main gameplay state
if (gMain.newKeys & A_BUTTON)
{
    Flap();
}
```

### Trail Particle Physics

**Smooth following behavior:**
```c
static void UpdateTrail(void)
{
    // Target position (behind Butterfree)
    s16 targetX = gSprites[sFlappy->ButterfreeSpriteId].x - sFlappy->TrailoffsetX;
    s16 targetY = gSprites[sFlappy->ButterfreeSpriteId].y - sFlappy->TrailoffsetY;

    // Current position
    s16 currentX = gSprites[sFlappy->TrailSpriteId].x;
    s16 currentY = gSprites[sFlappy->TrailSpriteId].y;

    // Smooth interpolation (move 1/4 of distance per frame)
    gSprites[sFlappy->TrailSpriteId].x = currentX + (targetX - currentX) / SMOOTHING_FACTOR;
    gSprites[sFlappy->TrailSpriteId].y = currentY + (targetY - currentY) / SMOOTHING_FACTOR;
}
```

**Effect:** Trail "lags" behind Butterfree, creating motion trail.

## 4. Parallax Scrolling System

### Background Layer Configuration

**Three scrolling layers:**

```c
struct BgTemplate sFlappyBGtemplates[] = {
    {
        .bg = 3,              // BG3 - Distant clouds (slowest)
        .charBaseIndex = 3,
        .mapBaseIndex = 29,
        .screenSize = 0,      // 32x32 tiles
        .paletteMode = 0,     // 4bpp (16 colors)
        .priority = 3,        // Lowest priority (back-most)
        .baseTile = 0
    },
    {
        .bg = 2,              // BG2 - Foreground pipes/obstacles
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 1,              // BG1 - Arcade machine frame
        .charBaseIndex = 1,
        .mapBaseIndex = 27,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 0,              // BG0 - UI text layer
        .charBaseIndex = 0,
        .mapBaseIndex = 0x17,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,        // Highest priority (front-most)
        .baseTile = 0
    }
};
```

### Parallax Scroll Implementation

**Different scroll speeds create depth:**
```c
static void ScrollX(void)
{
    // Foreground (BG2): 1 pixel per frame
    sFlappy->scroll_fg_x_frac += 256 * (3 / 2);  // Actually 1.5 px/frame with fractional

    // Background (BG3): Very slow (0.0625 px/frame = 1/16)
    sFlappy->scroll_bg_x_frac += 16;

    // Convert fractional to integer when >= 256
    if (sFlappy->scroll_fg_x_frac >= 256)
    {
        sFlappy->scroll_fg_x_frac -= 256;
        sFlappy->scroll_fg_x_int++;
    }

    if (sFlappy->scroll_bg_x_frac >= 256)
    {
        sFlappy->scroll_bg_x_frac -= 256;
        sFlappy->scroll_bg_x_int++;
    }

    // Apply to GPU registers
    SetGpuReg(REG_OFFSET_BG2HOFS, sFlappy->scroll_fg_x_int);  // Obstacles
    SetGpuReg(REG_OFFSET_BG3HOFS, sFlappy->scroll_bg_x_int);  // Clouds

    // Wrap at 256 pixels
    if (sFlappy->scroll_fg_x_int >= 256)
        sFlappy->scroll_fg_x_int = 0;
    if (sFlappy->scroll_bg_x_int >= 256)
        sFlappy->scroll_bg_x_int = 0;
}
```

**Speed ratios:**
- BG3 (clouds): 0.0625 px/frame (1:16 ratio)
- BG2 (obstacles): 1.5 px/frame (24:1 ratio compared to clouds)
- Creates strong depth perception

## 5. Obstacle Generation System

### Obstacle Patterns

**5 different gap heights:**
```c
enum ObstaclePattern {
    OBSTACLE_PATTERN_1 = 1,  // Gap near top
    OBSTACLE_PATTERN_2 = 2,  // Gap slightly lower
    OBSTACLE_PATTERN_3 = 3,  // Gap in middle
    OBSTACLE_PATTERN_4 = 4,  // Gap slightly higher
    OBSTACLE_PATTERN_5 = 5   // Gap near bottom
};
```

**Collision boundaries per pattern:**
```c
// Set dynamically when obstacle enters detection zone
switch (sFlappy->Obstacle1Id)
{
case 1: sFlappy->MAX_Y = 26;  sFlappy->MIN_Y = 68;  break;  // Top gap
case 2: sFlappy->MAX_Y = 34;  sFlappy->MIN_Y = 76;  break;
case 3: sFlappy->MAX_Y = 42;  sFlappy->MIN_Y = 84;  break;  // Middle gap
case 4: sFlappy->MAX_Y = 50;  sFlappy->MIN_Y = 92;  break;
case 5: sFlappy->MAX_Y = 58;  sFlappy->MIN_Y = 100; break;  // Bottom gap
}
```

**Gap size:** ~42 pixels consistently (MIN_Y - MAX_Y difference)

### Obstacle Spawning

**Triggers at specific scroll positions:**
```c
// In main gameplay loop
if (sFlappy->bg2ScrollX == 126 || sFlappy->bg2ScrollX == 252)
{
    // Generate new obstacle
    sFlappy->Obstacle1Id = (Random() % 5) + 1;  // Random 1-5
    ChangeObstacle1();  // Write tilemap to VRAM
}
```

**Two obstacles on screen simultaneously:**
- First obstacle at X=126
- Second obstacle at X=252
- Ensures continuous challenge

### Tilemap Generation

**Direct VRAM write:**
```c
void ChangeObstacle1(void)
{
    u16* bg2Tilemap = (u16*)0x0600E000;  // BG2 tilemap base address

    // 5 tile patterns (one per obstacle type)
    u16 tileGroups[5][20][4] = {
        // Pattern 1: Gap at top
        {{0x1, 0x2, 0x3, 0x4}, {0x1, 0x5, 0x6, 0x7}, ...},
        // Pattern 2-5: Different gap positions
        ...
    };

    int groupIndex = sFlappy->Obstacle1Id - 1;
    int startX = 12;  // Tile column (4 tiles wide)
    int startY = 0;

    // Write 20 rows × 4 columns of tiles
    for (u32 y = 0; y < 20; y++)
    {
        for (u32 x = 0; x < 4; x++)
        {
            u32 tilemapIndex = (y + startY) * 32 + (x + startX);
            bg2Tilemap[tilemapIndex] = tileGroups[groupIndex][y][x];
        }
    }
}
```

**Tilemap organization:**
- 32x32 tile grid (256x256 pixels)
- Obstacles: 4 tiles wide, 20 tiles tall
- Each pattern has predefined tile indices

## 6. Collision Detection

### Hitbox System

**Two sprites for collision:**
1. **Butterfree Sprite:** Visual representation (64x64)
2. **Hitbox Sprite:** Collision detection (16x16, invisible)

**Hitbox positioning:**
```c
// Hitbox follows Butterfree center
gSprites[sFlappy->ButterfreeHitboxSpriteId].x = gSprites[sFlappy->ButterfreeSpriteId].x;
gSprites[sFlappy->ButterfreeHitboxSpriteId].y = gSprites[sFlappy->ButterfreeSpriteId].y;
```

### Collision Zones

**Dynamic boundaries based on scroll position:**
```c
// Obstacle 1 detection zone
if (sFlappy->bg2ScrollX > 48 && sFlappy->bg2ScrollX < 97)
{
    // Update MAX_Y and MIN_Y based on Obstacle1Id
    SetCollisionBoundaries(sFlappy->Obstacle1Id);
}

// Obstacle 2 detection zone
if (sFlappy->bg2ScrollX > 176 && sFlappy->bg2ScrollX < 225)
{
    // Update boundaries for Obstacle2Id
    SetCollisionBoundaries(sFlappy->Obstacle2Id);
}
```

**Collision check (every frame):**
```c
// Ground/ceiling collision in ApplyGravity()
if (sFlappy->pos_y > sFlappy->MIN_Y)  // Hit floor
    TriggerCollision();
if (sFlappy->pos_y < sFlappy->MAX_Y)  // Hit ceiling or pipe
    TriggerCollision();
```

**Note:** Collision is based on Y position only. X-axis collision handled by setting boundaries at specific scroll positions.

## 7. Scoring System

### Point Awarding

**Trigger: Passing an obstacle:**
```c
// Score when clearing obstacles
if ((sFlappy->bg2ScrollX == 225) ||  // Passed obstacle 2
    ((sFlappy->bg2ScrollX == 98) && (sFlappy->Obstacle1Id != 0)))  // Passed obstacle 1
{
    SoundEffect(sFlappy->SFX);  // Musical note sequence

    // Award coins (VAR_FLIP_WINNINGS)
    if (VarGet(VAR_FLIP_WINNINGS) < 9998)
        VarSet(VAR_FLIP_WINNINGS, VarGet(VAR_FLIP_WINNINGS) + 2);  // +2 coins
    else
        VarSet(VAR_FLIP_WINNINGS, 9999);  // Cap at 9999

    // Increment score
    if (sFlappy->Points < 9999)
        sFlappy->Points++;

    SetCreditDigits(sFlappy->Points);  // Update display
}
```

**Coin reward:** 2 coins per obstacle passed
**Score:** 1 point per obstacle passed

### High Score Persistence

**Check at game over:**
```c
if (sFlappy->Points > VarGet(VAR_FLAPPY_HISCORE))
{
    VarSet(VAR_FLAPPY_HISCORE, sFlappy->Points);
    sFlappy->state = FLAPPY_HISCORE;  // Show high score screen
}
else
{
    sFlappy->state = FLAPPY_EXIT;  // Regular game over
}
```

**Storage:** `VAR_FLAPPY_HISCORE` game variable (persistent across saves)

### Score Display

**4-digit display:**
```c
static void SetCreditDigits(u32 score)
{
    u8 digits[4];
    digits[0] = (score / 1000) % 10;  // Thousands
    digits[1] = (score / 100) % 10;   // Hundreds
    digits[2] = (score / 10) % 10;    // Tens
    digits[3] = score % 10;           // Ones

    for (u32 i = 0; i < 4; i++)
    {
        // Update sprite tile index to show digit
        gSprites[sFlappy->CreditSpriteIds[i]].tileTag = DIGIT_TAG_BASE + digits[i];
    }
}
```

## 8. Sprite System

### Sprite List (18+ sprites total)

**Character:**
1. Butterfree (64x64) - Main character with animations
2. Butterfree Hitbox (16x16) - Collision detection
3. Trail particle (32x32) - Motion trail effect

**UI Elements:**
4-7. Score digits (×4) - Current score display
8-11. High score digits (×4) - Best score display
12. Score label 1 - "SCORE" text
13. Score label 2 - Additional label
14. "FLAP" instruction - Tutorial text

**Countdown:**
15. "3" sprite
16. "2" sprite
17. "1" sprite
18. "GO" sprite

**Game Over:**
19. "GAME OVER" sprite
20. "HIGH SCORE" sprite

**Death Animation:**
21. Damage sprite (64x64) - Spinning on collision

**Borders:**
22-23. Border decorations - Arcade frame elements

### Animation System

**Butterfree Wing Flapping:**
```c
// Animation commands
static const union AnimCmd sButterfreeAnimCmd_0[] = {
    ANIMCMD_FRAME(0, 1),    // Frame 0: Wings down (idle)
    ANIMCMD_END
};

static const union AnimCmd sButterfreeAnimCmd_1[] = {
    ANIMCMD_FRAME(64, 10),  // Frame 1: Wings up
    ANIMCMD_FRAME(128, 10), // Frame 2: Wings peak
    ANIMCMD_FRAME(64, 10),  // Frame 1: Wings down
    ANIMCMD_FRAME(0, 10),   // Frame 0: Back to idle
    ANIMCMD_END
};

// Animation table
static const union AnimCmd *const sButterfreeAnimCmds[] = {
    sButterfreeAnimCmd_0,   // animNum 0: Idle
    sButterfreeAnimCmd_1,   // animNum 1: Flap cycle
};
```

**Frame timing:** 10 ticks per frame = 10/60 second ≈ 166ms per frame

**Trail Particle Fade:**
```c
static const union AnimCmd sTrailAnimCmd_0[] = {
    ANIMCMD_FRAME(0, 5),    // 8 frames total (fade sequence)
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_FRAME(48, 5),
    ANIMCMD_FRAME(64, 5),
    ANIMCMD_FRAME(80, 5),
    ANIMCMD_FRAME(96, 5),
    ANIMCMD_FRAME(112, 5),
    ANIMCMD_JUMP(0)         // Loop
};
```

**Effect:** Smooth fade-out as trail ages

### Death Animation (Affine Rotation)

**Damage sprite rotates:**
```c
static void SpriteCB_FlappyDamage(struct Sprite *sprite)
{
    sFlappy->DamageSpriteRotation += 8;  // 8 degrees per frame
    sFlappy->DamageSpriteRotation %= 360; // Wrap at 360

    // Setup affine transform
    struct ObjAffineSrcData affine = {
        .xScale = 256,    // 1:1 scale (256 = 1.0)
        .yScale = 256,
        .rotation = sFlappy->DamageSpriteRotation * 256  // Rotation angle
    };

    struct OamMatrix matrix;
    ObjAffineSet(&affine, &matrix, 1, 2);  // Calculate transformation matrix
    SetOamMatrix(0, matrix.a, matrix.b, matrix.c, matrix.d);  // Apply to hardware

    sprite->oam.x = sprite->x;
    sprite->oam.y = sprite->y;
}
```

**Rotation speed:** 8°/frame × 60 FPS = 480°/second ≈ 1.33 rotations/second

**Parabolic motion during rotation:**
```c
// Death animation physics
sFlappy->DamageSpriteY += sFlappy->velocity;
sFlappy->velocity += 1;  // Gravity

if (sFlappy->DamageSpriteY > 200)  // Off-screen bottom
    sFlappy->state = FLAPPY_GAMEOVER;
```

## 9. Audio Integration

### Music

**Game BGM:**
```c
PlayBGM(MUS_RG_CELADON);  // Main gameplay music
```

**Game Over:**
```c
PlayBGM(MUS_TOO_BAD);      // Sad fanfare
```

**High Score:**
```c
PlayBGM(MUS_LEVEL_UP);     // Victory fanfare
```

**Stop Music (on collision):**
```c
PlayBGM(MUS_NONE);
```

### Sound Effects

**Flap:**
```c
PlaySE(SE_M_WING_ATTACK);  // Wing flapping sound
```

**Collision:**
```c
PlaySE(SE_SUPER_EFFECTIVE); // Hit sound
```

**Death Animation:**
```c
PlaySE(SE_FAINT);           // Faint sound
```

**Countdown:**
```c
PlaySE(SE_CONTEST_PLACE);   // Beep for 3-2-1
```

**UI Activation:**
```c
PlaySE(SE_POKENAV_ON);      // Menu activation
```

**Score Point (Musical Sequence):**
```c
static void SoundEffect(u8 counter)
{
    switch (counter)
    {
    case 0: PlaySE(SE_NOTE_C); break;       // C note
    case 1: PlaySE(SE_NOTE_D); break;       // D note
    case 2: PlaySE(SE_NOTE_E); break;       // E note
    case 3: PlaySE(SE_NOTE_F); break;       // F note
    case 4: PlaySE(SE_NOTE_G); break;       // G note
    case 5: PlaySE(SE_NOTE_A); break;       // A note
    case 6: PlaySE(SE_NOTE_B); break;       // B note
    case 7: PlaySE(SE_NOTE_C_HIGH); break;  // High C
    }
    sFlappy->SFX = (counter + 1) % 8;  // Cycle through notes
}
```

**Effect:** Ascending musical notes as you score, creating satisfying audio feedback

### Pokemon Cry

**Butterfree cry at start:**
```c
PlayCry_Normal(SPECIES_BUTTERFREE, 0);
```

## 10. Graphics Assets

### Background Layers

**BG3 - Clouds (Parallax far):**
- File: `flappy-bg.4bpp.lz` (graphics/game_corner_flappybird/)
- Palette: `flappy-bg.gbapal`
- Tilemap: `flappy-bg.bin.lz`

**BG2 - Obstacles (Parallax near):**
- File: `flappy-fg.4bpp.lz`
- Palette: `flappy-fg.gbapal`
- Tilemap: `flappy-fg.bin.lz` (procedurally modified)

**BG1 - Arcade Frame:**
- File: `arcade-screen.4bpp.lz`
- Palette: `arcade-screen.gbapal`

### Sprites

**Butterfree:**
- File: `butterfree.4bpp.lz` (64x64, 2 animation frames)
- Palette: `butterfree.gbapal`
- Frames: Idle (0), Flap cycle (1-3)

**Butterfree Hitbox:**
- File: `butterfree-hitbox.4bpp.lz` (16x16)
- Palette: `butterfree-hitbox.gbapal`
- Invisible during gameplay

**Trail:**
- File: `trail.4bpp.lz` (32x32, 8 fade frames)
- Palette: `trail.gbapal`

**UI Text:**
- `start.4bpp.lz` - "GO" (64x32)
- `gameover.4bpp.lz` - "GAME OVER" (64x32)
- `hiscore.4bpp.lz` - "HIGH SCORE" (64x32)
- `one.4bpp.lz`, `two.4bpp.lz`, `three.4bpp.lz` - Countdown (64x32 each)
- `flap.4bpp.lz` - "FLAP" instruction (32x8)
- `score.4bpp.lz`, `score2.4bpp.lz` - Score labels (64x32)

**Digits:**
- `digits.4bpp.lz` - Number font (4 digits × 8x16 tiles)

**Effects:**
- `damage.4bpp.lz` - Death sprite (64x64, 2 frames with rotation)
- `topleft.4bpp.lz`, `bottomleft.4bpp.lz` - Borders (64x32)

**Total Assets:** 20+ graphics files, 10+ palette files

### Graphics Organization

**Recommended structure:**
```
graphics/game_corner_flappybird/
├── backgrounds/
│   ├── clouds.png               → flappy-bg.4bpp.lz
│   ├── clouds.gbapal
│   ├── obstacles.png            → flappy-fg.4bpp.lz
│   ├── obstacles.gbapal
│   └── arcade_frame.png         → arcade-screen.4bpp.lz
├── sprites/
│   ├── butterfree_idle.png      → butterfree.4bpp.lz (frame 0)
│   ├── butterfree_flap.png      → (frames 1-3)
│   ├── butterfree.gbapal
│   ├── trail.png                → trail.4bpp.lz
│   ├── trail.gbapal
│   └── damage.png               → damage.4bpp.lz
└── ui/
    ├── start.png                → start.4bpp.lz
    ├── gameover.png
    ├── hiscore.png
    ├── countdown_3.png
    ├── countdown_2.png
    ├── countdown_1.png
    ├── flap_text.png
    ├── score_label.png
    ├── digits.png
    └── borders.png
```

## 11. Memory Management

### Allocation

**Main struct:**
```c
void StartFlappyBird(void)
{
    sFlappy = AllocZeroed(sizeof(struct FlappyBird));  // EWRAM allocation
    if (sFlappy == NULL)
    {
        // Handle allocation failure
        return;
    }

    // Create main task
    u8 taskId = CreateTask(FadeToFlappyBirdScreen, 0);
}
```

**Decompression buffer usage:**
```c
// LZ77 decompression pattern
struct SpriteSheet spriteSheet;
LZ77UnCompWram(sSpriteSheet_Butterfree.data, gDecompressionBuffer);  // Decompress to temp buffer
spriteSheet.data = gDecompressionBuffer;
spriteSheet.size = sSpriteSheet_Butterfree.size;
spriteSheet.tag = BUTTERFREE_GFXTAG;
LoadSpriteSheet(&spriteSheet);  // Copy to VRAM
```

### Cleanup

**Exit sequence:**
```c
static void ExitFlappyBird(void)
{
    if (!gPaletteFade.active)  // Wait for fade complete
    {
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        FREE_AND_SET_NULL(sFlappy);  // Free memory and null pointer
    }
}
```

**Resource destruction:**
- All sprites automatically freed by ResetSpriteData() in initialization
- Background layers reset by ResetBgsAndClearDma3BusyFlags()
- Task removed by SetMainCallback2()

## 12. Integration with Game Corner

### Entry Point

**Script trigger (from map event):**
```assembly
MossdeepCity_GameCorner_EventScript_FlappyBird::
    lockall
    checkitem ITEM_COIN_CASE
    goto_if_eq VAR_RESULT, FALSE, EventScript_NoCoinCase
    setvar VAR_0x8004, MINIGAME_ID_FLAPPYBIRD
    specialvar VAR_RESULT, GetMinigameId
    playflappybird VAR_RESULT  # Custom script command
    releaseall
    end
```

**C entry:**
```c
void StartFlappyBird(void)
{
    // Called from script command
    // Allocate resources
    // Fade to game screen
    // Begin gameplay loop
}
```

### Coin System Integration

**Coin rewards:**
```c
// Using VAR_FLIP_WINNINGS for coin tracking
u16 currentCoins = VarGet(VAR_FLIP_WINNINGS);
VarSet(VAR_FLIP_WINNINGS, currentCoins + 2);  // Award 2 coins per obstacle
```

**Note:** `VAR_FLIP_WINNINGS` is temporary storage. Final coin award happens after exiting game.

### Exit Callback

**Return to overworld:**
```c
CB2_ReturnToFieldContinueScriptPlayMapMusic();
```

**Effect:**
- Restores field callback
- Resumes background music
- Returns control to player
- Script continues execution

## 13. Dependencies

### Required Headers

```c
#include "global.h"           // Core GBA types, EWRAM_DATA macro
#include "malloc.h"           // AllocZeroed, FREE_AND_SET_NULL
#include "battle.h"           // Pokemon species constants (SPECIES_BUTTERFREE)
#include "bg.h"               // Background layer management (InitBgsFromTemplates, etc.)
#include "coins.h"            // Coin system (GetCoins, SetCoins) - may not be directly used
#include "decompress.h"       // LZ77UnCompWram, gDecompressionBuffer
#include "gpu_regs.h"         // SetGpuReg, REG_OFFSET_* constants
#include "graphics.h"         // Graphics management
#include "m4a.h"              // Audio system (PlaySE, PlayBGM, PlayCry_Normal)
#include "main.h"             // Main game loop (SetMainCallback2, SetVBlankCallback, etc.)
#include "menu.h"             // Text windows/menus
#include "palette.h"          // Palette loading (LoadPalette, etc.)
#include "random.h"           // Random() function
#include "script.h"           // Script callbacks (CB2_ReturnToFieldContinueScriptPlayMapMusic)
#include "sound.h"            // Sound playback (PlaySE, PlayBGM, PlayCry_Normal)
#include "sprite.h"           // Sprite management (CreateSprite, gSprites, etc.)
#include "task.h"             // Task system (CreateTask, gTasks)
#include "text.h"             // Text rendering
#include "window.h"           // Window templates
#include "constants/coins.h"  // MAX_COINS
#include "constants/songs.h"  // MUS_*, SE_* constants
#include "constants/vars.h"   // VAR_* constants
```

### Required Game Variables

```c
VAR_FLIP_WINNINGS       // Temporary coin storage during game
VAR_FLAPPY_HISCORE      // High score persistence
```

**Allocation needed:**
- Reserve unused VAR slots in `include/constants/vars.h`
- Or use existing game stat slots

### Required Pokemon Species

```c
SPECIES_BUTTERFREE      // For cry sound: PlayCry_Normal(SPECIES_BUTTERFREE, 0)
```

## 14. Performance Characteristics

### Frame Rate Target

**60 FPS (16.67ms per frame):**
- Main loop runs once per frame via task system
- Physics updates every frame
- Collision checks every frame
- Sprite updates every frame

### CPU Budget

**Estimated CPU usage:**
- Physics calculation: <1%
- Collision detection: <1%
- Sprite updates: ~5%
- Background scrolling: ~2%
- Audio: ~5%
- **Total: ~15% CPU**

**Remaining budget:** 85% for GBA overhead and other systems

### Memory Usage

**RAM (EWRAM):**
- struct FlappyBird: ~120 bytes
- Decompression buffer: 32KB (shared with other systems)
- Sprite OAM: 20 sprites × 8 bytes = 160 bytes
- **Total: <1KB dedicated + shared buffers**

**VRAM:**
- BG tiles: ~32KB (3 backgrounds)
- Sprite tiles: ~16KB (18+ sprites)
- Tilemaps: ~4KB
- **Total: ~52KB (well within 96KB VRAM limit)**

### Optimization Opportunities

**Current implementation likely has:**
1. Tight physics loop (minimal branching)
2. Fixed-point math for scroll (fractional tracking)
3. Direct VRAM writes for obstacles (fast)
4. Sprite pooling (reuse sprite IDs)

**Potential improvements:**
1. Could use sprite sheet compression (.smol format)
2. Could reduce sprite count (combine UI elements)
3. Could optimize collision (early-exit checks)

## 15. Port Planning for RHH Expansion

### Code Adaptation Required

**Naming Conventions:**
- Functions: `init_game()` → `InitFlappyBird()`
- Variables: `current_score` → `currentScore`
- Globals: `score` → `gFlappyScore`
- Statics: `static int level` → `static u32 sLevel`

**Type Changes:**
- `int` → `s32/u32` as appropriate
- `bool` → `bool8`
- `true/false` → `TRUE/FALSE`

**Loop Iterators:**
```c
// Before (pret)
int i;
for (i = 0; i < count; i++)

// After (RHH)
for (u32 i = 0; i < count; i++)
```

**Config Integration:**
```c
// Add to include/config/game_corner.h
#define B_GAMECORNER_FLAPPYBIRD TRUE
#define B_GAMECORNER_FLAPPY_DIFFICULTY GEN_LATEST

// Use runtime check
if (B_GAMECORNER_FLAPPYBIRD)
    StartFlappyBird();
```

### Graphics Port

**Asset conversion:**
1. Extract all graphics from heyopc repo
2. Convert to PNG if needed
3. Place in `graphics/game_corner_flappybird/`
4. Create .gbapal files for palettes
5. Build system auto-generates .4bpp files

**Example:**
```bash
# Existing file from heyopc
butterfree.4bpp.lz

# Convert to RHH format
# 1. Decompress if LZ77 compressed
# 2. Convert to PNG: butterfree.png
# 3. Create palette: butterfree.gbapal
# 4. Build system generates: butterfree.4bpp (from PNG)
```

### Audio Port

**Music files:**
- Extract .mid files from heyopc
- Place in `sound/` directory
- Add constants to `include/constants/songs.h`

**Example:**
```c
// Add to songs.h
#define MUS_FLAPPYBIRD_GAME  (MUS_GAME_CORNER + 1)
#define SE_FLAPPY_FLAP       (SE_LAST + 1)
```

### Script Integration

**Add to field_specials.c:**
```c
u16 GetFlappyBirdId(void)
{
    // Map machine coordinates to game ID
    return MINIGAME_ID_FLAPPYBIRD;
}
```

**Add to data/specials.inc:**
```c
def_special GetFlappyBirdId
```

**Map script (Mossdeep B1F):**
```assembly
MossdeepCity_GameCorner_B1F_EventScript_FlappyBird::
    lockall
    checkitem ITEM_COIN_CASE
    goto_if_eq VAR_RESULT, FALSE, MossdeepCity_GameCorner_EventScript_NoCoinCase
    setvar VAR_0x8004, MINIGAME_ID_FLAPPYBIRD
    specialvar VAR_RESULT, GetFlappyBirdId
    call StartFlappyBird
    releaseall
    end
```

### Testing Strategy

**Unit Tests (test/game_corner_flappybird.c):**
```c
SINGLE_BATTLE_TEST("Flappy Bird: Flap applies upward velocity")
{
    GIVEN {
        InitFlappyBird();
        sFlappy->speed_y = 0;  // Start with no velocity
    }
    WHEN {
        Flap();
    }
    THEN {
        EXPECT_EQ(sFlappy->speed_y, FLAP_STRENGTH);  // Should be -6
    }
}

SINGLE_BATTLE_TEST("Flappy Bird: Gravity accelerates downward")
{
    GIVEN {
        InitFlappyBird();
        sFlappy->speed_y = 0;
        sFlappy->pos_y = 50;
    }
    WHEN {
        ApplyGravity();
    }
    THEN {
        EXPECT_EQ(sFlappy->speed_y, GRAVITY);  // Should be +1
        EXPECT_EQ(sFlappy->pos_y, 51);  // Should move down 1 pixel
    }
}
```

**Integration Tests:**
- Complete playthrough (init → gameplay → game over → exit)
- Collision detection accuracy (hitbox vs obstacles)
- Score tracking (increment per obstacle)
- High score persistence (save/load)
- Coin rewards (verify VAR_FLIP_WINNINGS)

**Performance Tests:**
- Frame rate stability (60 FPS)
- Memory leak detection (100+ rounds)
- Input latency (<1 frame)

### Estimated Effort

**Code Port:** 6-8 hours
- Copy source file
- Apply naming conventions
- Update includes
- Convert types

**Graphics Port:** 3-4 hours
- Extract assets
- Convert to PNG
- Create palettes
- Test in-game

**Audio Port:** 1-2 hours
- Extract music/SFX
- Add to sound directory
- Register constants

**Integration:** 2-3 hours
- Script integration
- Special function registration
- Map event setup

**Testing:** 3-4 hours
- Write unit tests
- Integration testing
- Performance profiling

**Total:** 15-21 hours

## 16. Summary

### Key Takeaways

1. **Well-Structured Code:** Clean state machine, modular functions
2. **Solid Physics:** Simple but effective gravity + jump system
3. **Good Visuals:** Parallax scrolling creates depth, smooth animations
4. **Fair Gameplay:** Consistent obstacle gaps, clear collision
5. **Polished Feedback:** Musical scoring, rotation death animation
6. **Proper Integration:** Coin system, high scores, clean entry/exit

### Complexity Rating: Medium

**Easy aspects:**
- State machine is straightforward
- Physics is simple (linear gravity)
- Collision is Y-axis only

**Challenging aspects:**
- Parallax scrolling with fractional tracking
- Direct VRAM tilemap writes
- Affine rotation for death animation
- 18+ sprite management
- Tight frame-rate requirements

### Recommended for Tier 1 Implementation

**Rationale:**
- Already fully analyzed (this document)
- Tests multiple complex systems (physics, parallax, animation)
- Good reference for other physics-based games
- Popular game (players will recognize and enjoy)
- Medium complexity (not too easy, not overwhelming)

---

**Document Version:** 1.0
**Last Updated:** 2025-12-16
**Source Code:** heyopc/pokeemerald-gamecorner-expansion/src/flappybird.c (1,914 lines)
**Purpose:** Complete technical reference for RHH port
