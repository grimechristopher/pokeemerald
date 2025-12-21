#include "game_corner_pinball.h"
#include "game_corner_common.h"
#include "global.h"
#include "bg.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "overworld.h"
#include "palette.h"
#include "random.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/coins.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// ========================================
// Constants
// ========================================

#define TABLE_WIDTH 120
#define TABLE_HEIGHT 160
#define BALL_RADIUS 3
#define NUM_BUMPERS 5
#define MAX_BALLS 3

// Game states
enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_LAUNCH,
    STATE_PLAYING,
    STATE_BALL_LOST,
    STATE_GAME_OVER,
    STATE_SHOW_RESULT,
    STATE_WAIT_INPUT,
    STATE_FADE_OUT,
    STATE_EXIT,
};

// ========================================
// Structures
// ========================================

struct Ball
{
    s16 x;
    s16 y;
    s16 vx;
    s16 vy;
    bool8 active;
};

struct Flipper
{
    s16 x;
    s16 y;
    bool8 active;  // Currently flipping
    u8 angle;      // 0 = down, 45 = up
};

struct Bumper
{
    s16 x;
    s16 y;
    u8 radius;
    u16 points;
};

struct PinballGame
{
    struct Ball ball;
    struct Flipper leftFlipper;
    struct Flipper rightFlipper;
    struct Bumper bumpers[NUM_BUMPERS];
    u8 state;
    u16 timer;
    u16 score;
    u8 ballsRemaining;
    u16 launchPower;
    bool8 launching;
};

// ========================================
// EWRAM
// ========================================

static EWRAM_DATA struct PinballGame *sGame = NULL;
static EWRAM_DATA u8 sTextWindowId = 0;

// ========================================
// Bumper Positions
// ========================================

static const struct {
    s16 x;
    s16 y;
    u8 radius;
    u16 points;
} sBumperPositions[NUM_BUMPERS] = {
    {30, 40, 8, 100},
    {60, 30, 8, 100},
    {90, 40, 8, 100},
    {45, 60, 8, 150},
    {75, 60, 8, 150},
};

// ========================================
// Function Declarations
// ========================================

static void CB2_Pinball(void);
static void VBlankCB_Pinball(void);
static void Task_PinballMain(u8 taskId);
static void DrawUI(void);
static void HandleInput(u8 taskId);
static void UpdateBallPhysics(void);
static void CheckBumperCollisions(void);
static void CheckFlipperCollisions(void);
static void ActivateFlipper(struct Flipper *flipper);
static void DeactivateFlipper(struct Flipper *flipper);
static void LaunchBall(void);
static void LoseBall(void);
static void ShowResult(void);

// ========================================
// Initialization
// ========================================

void Pinball_Init(void)
{
    SetMainCallback2(CB2_Pinball);
}

static void CB2_Pinball(void)
{
    u32 i;

    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        sGame = AllocZeroed(sizeof(*sGame));
        sGame->state = STATE_INIT;
        sGame->timer = 0;
        sGame->score = 0;
        sGame->ballsRemaining = MAX_BALLS;
        sGame->launchPower = 0;
        sGame->launching = FALSE;

        // Initialize ball
        sGame->ball.active = FALSE;
        sGame->ball.x = 0;
        sGame->ball.y = 0;
        sGame->ball.vx = 0;
        sGame->ball.vy = 0;

        // Initialize flippers
        sGame->leftFlipper.x = 35;
        sGame->leftFlipper.y = TABLE_HEIGHT - 20;
        sGame->leftFlipper.active = FALSE;
        sGame->leftFlipper.angle = 0;

        sGame->rightFlipper.x = 85;
        sGame->rightFlipper.y = TABLE_HEIGHT - 20;
        sGame->rightFlipper.active = FALSE;
        sGame->rightFlipper.angle = 0;

        // Initialize bumpers
        for (i = 0; i < NUM_BUMPERS; i++)
        {
            sGame->bumpers[i].x = sBumperPositions[i].x;
            sGame->bumpers[i].y = sBumperPositions[i].y;
            sGame->bumpers[i].radius = sBumperPositions[i].radius;
            sGame->bumpers[i].points = sBumperPositions[i].points;
        }

        gMain.state++;
        break;
    case 1:
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetTasks();
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, (struct BgTemplate[]){
            {
                .bg = 0,
                .charBaseIndex = 0,
                .mapBaseIndex = 31,
                .screenSize = 0,
                .paletteMode = 0,
                .priority = 0,
                .baseTile = 0
            },
            {.bg = 3}
        }, 2);
        SetBgTilemapBuffer(0, Alloc(BG_SCREEN_SIZE));
        FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);
        CopyBgTilemapBufferToVram(0);
        gMain.state++;
        break;
    case 3:
        sTextWindowId = AddWindow(&(struct WindowTemplate){
            .bg = 0,
            .tilemapLeft = 2,
            .tilemapTop = 2,
            .width = 26,
            .height = 16,
            .paletteNum = 15,
            .baseBlock = 1
        });
        FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));
        PutWindowTilemap(sTextWindowId);
        CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        gMain.state++;
        break;
    case 4:
        LoadPalette((void*)gStandardMenuPalette, 0xF0, 0x20);
        gMain.state++;
        break;
    case 5:
        SetVBlankCallback(VBlankCB_Pinball);
        CreateTask(Task_PinballMain, 0);
        BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        sGame->state = STATE_FADE_IN;
        gMain.state++;
        break;
    default:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        gMain.state = 0;
        break;
    }
}

static void VBlankCB_Pinball(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// ========================================
// Main Game Loop
// ========================================

static void Task_PinballMain(u8 taskId)
{
    switch (sGame->state)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            DrawUI();
            sGame->state = STATE_LAUNCH;
        }
        break;

    case STATE_LAUNCH:
        HandleInput(taskId);

        // Hold A to charge, release to launch
        if (JOY_HELD(A_BUTTON))
        {
            sGame->launching = TRUE;
            sGame->launchPower++;
            if (sGame->launchPower > 60)
                sGame->launchPower = 60;
        }
        else if (sGame->launching && JOY_NEW(~A_BUTTON))
        {
            LaunchBall();
            sGame->state = STATE_PLAYING;
        }
        break;

    case STATE_PLAYING:
        HandleInput(taskId);
        UpdateBallPhysics();
        CheckBumperCollisions();
        CheckFlipperCollisions();

        // Check if ball is lost (fell off bottom)
        if (sGame->ball.y > TABLE_HEIGHT + BALL_RADIUS)
        {
            LoseBall();
        }
        break;

    case STATE_BALL_LOST:
        sGame->ballsRemaining--;
        if (sGame->ballsRemaining > 0)
        {
            sGame->state = STATE_LAUNCH;
            DrawUI();
        }
        else
        {
            sGame->state = STATE_GAME_OVER;
        }
        break;

    case STATE_GAME_OVER:
        ShowResult();
        GameCorner_IncrementPlayCount(MINIGAME_PINBALL);
        if (GameCorner_IsNewHighScore(MINIGAME_PINBALL, sGame->score))
            GameCorner_UpdateHighScore(MINIGAME_PINBALL, sGame->score);
        sGame->state = STATE_WAIT_INPUT;
        break;

    case STATE_WAIT_INPUT:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            PlaySE(SE_SELECT);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sGame->state = STATE_FADE_OUT;
        }
        break;

    case STATE_FADE_OUT:
        if (!gPaletteFade.active)
        {
            sGame->state = STATE_EXIT;
        }
        break;

    case STATE_EXIT:
        DestroyTask(taskId);
        Pinball_Exit();
        break;
    }
}

// ========================================
// Ball Physics
// ========================================

static void LaunchBall(void)
{
    sGame->ball.active = TRUE;
    sGame->ball.x = 110;  // Right side launch
    sGame->ball.y = TABLE_HEIGHT - 30;
    sGame->ball.vx = -2;
    sGame->ball.vy = -(sGame->launchPower / 4);  // Convert power to velocity
    sGame->launchPower = 0;
    sGame->launching = FALSE;

    PlaySE(SE_BALL);
}

static void LoseBall(void)
{
    sGame->ball.active = FALSE;
    sGame->state = STATE_BALL_LOST;
    PlaySE(SE_FAILURE);
}

static void UpdateBallPhysics(void)
{
    if (!sGame->ball.active)
        return;

    // Apply gravity
    sGame->ball.vy += 1;
    if (sGame->ball.vy > 10)
        sGame->ball.vy = 10;

    // Update position
    sGame->ball.x += sGame->ball.vx;
    sGame->ball.y += sGame->ball.vy;

    // Bounce off walls
    if (sGame->ball.x <= BALL_RADIUS || sGame->ball.x >= TABLE_WIDTH - BALL_RADIUS)
    {
        sGame->ball.vx = -sGame->ball.vx;
        if (sGame->ball.x <= BALL_RADIUS)
            sGame->ball.x = BALL_RADIUS;
        else
            sGame->ball.x = TABLE_WIDTH - BALL_RADIUS;
        PlaySE(SE_PIN);
    }

    // Bounce off top
    if (sGame->ball.y <= BALL_RADIUS)
    {
        sGame->ball.vy = -sGame->ball.vy;
        sGame->ball.y = BALL_RADIUS;
        PlaySE(SE_PIN);
    }

    // Apply friction
    if (sGame->ball.vx > 0)
        sGame->ball.vx -= (sGame->ball.vx > 0) ? 0 : 0;
    else if (sGame->ball.vx < 0)
        sGame->ball.vx += (sGame->ball.vx < 0) ? 0 : 0;
}

static void CheckBumperCollisions(void)
{
    u32 i;

    for (i = 0; i < NUM_BUMPERS; i++)
    {
        s16 dx = sGame->ball.x - sGame->bumpers[i].x;
        s16 dy = sGame->ball.y - sGame->bumpers[i].y;
        u16 distSq = dx * dx + dy * dy;
        u16 minDist = BALL_RADIUS + sGame->bumpers[i].radius;

        if (distSq < minDist * minDist)
        {
            // Collision! Bounce ball away
            sGame->ball.vx = dx / 2;
            sGame->ball.vy = dy / 2;

            // Add some minimum velocity
            if (sGame->ball.vx == 0)
                sGame->ball.vx = (Random() % 2) ? 2 : -2;
            if (sGame->ball.vy == 0)
                sGame->ball.vy = -3;

            // Award points
            sGame->score += sGame->bumpers[i].points;

            PlaySE(SE_DING_DONG);
            DrawUI();
        }
    }
}

static void CheckFlipperCollisions(void)
{
    // Simple collision: if ball is near flipper and flipper is active, launch ball upward
    if (sGame->leftFlipper.active)
    {
        s16 dx = sGame->ball.x - sGame->leftFlipper.x;
        s16 dy = sGame->ball.y - sGame->leftFlipper.y;

        if (dx * dx + dy * dy < 400)  // Within ~20 units
        {
            sGame->ball.vy = -8;
            sGame->ball.vx = 3;
            PlaySE(SE_PIN);
        }
    }

    if (sGame->rightFlipper.active)
    {
        s16 dx = sGame->ball.x - sGame->rightFlipper.x;
        s16 dy = sGame->ball.y - sGame->rightFlipper.y;

        if (dx * dx + dy * dy < 400)
        {
            sGame->ball.vy = -8;
            sGame->ball.vx = -3;
            PlaySE(SE_PIN);
        }
    }
}

// ========================================
// Flipper Control
// ========================================

static void ActivateFlipper(struct Flipper *flipper)
{
    flipper->active = TRUE;
    flipper->angle = 45;
}

static void DeactivateFlipper(struct Flipper *flipper)
{
    flipper->active = FALSE;
    flipper->angle = 0;
}

// ========================================
// UI Rendering
// ========================================

static void DrawUI(void)
{
    u8 str[64];

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    // Title
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_Pinball, 70, 2, TEXT_SKIP_DRAW, NULL);

    // Score
    StringCopy(str, gText_Score);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->score, STR_CONV_MODE_LEFT_ALIGN, 6);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 30, TEXT_SKIP_DRAW, NULL);

    // Balls
    StringCopy(str, gText_Balls);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->ballsRemaining, STR_CONV_MODE_LEFT_ALIGN, 1);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 45, TEXT_SKIP_DRAW, NULL);

    // Controls
    if (sGame->state == STATE_LAUNCH)
    {
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_HoldALaunch, 4, 70, TEXT_SKIP_DRAW, NULL);
    }
    else
    {
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_LeftFlipper, 4, 70, TEXT_SKIP_DRAW, NULL);
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_RightFlipper, 4, 85, TEXT_SKIP_DRAW, NULL);
    }

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void ShowResult(void)
{
    u8 str[128];

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_GameOver, 70, 2, TEXT_SKIP_DRAW, NULL);

    // Final score
    StringCopy(str, gText_FinalScore);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->score, STR_CONV_MODE_LEFT_ALIGN, 6);
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, str, 4, 50, TEXT_SKIP_DRAW, NULL);

    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_PressA, 60, 100, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);

    if (GameCorner_IsNewHighScore(MINIGAME_PINBALL, sGame->score))
        PlayFanfare(MUS_OBTAIN_ITEM);
}

// ========================================
// Input Handling
// ========================================

static void HandleInput(u8 taskId)
{
    if (sGame->state == STATE_PLAYING)
    {
        // Left flipper
        if (JOY_HELD(DPAD_LEFT) || JOY_HELD(L_BUTTON))
        {
            ActivateFlipper(&sGame->leftFlipper);
        }
        else
        {
            DeactivateFlipper(&sGame->leftFlipper);
        }

        // Right flipper
        if (JOY_HELD(DPAD_RIGHT) || JOY_HELD(R_BUTTON))
        {
            ActivateFlipper(&sGame->rightFlipper);
        }
        else
        {
            DeactivateFlipper(&sGame->rightFlipper);
        }
    }

    // Quit
    if (JOY_NEW(B_BUTTON | START_BUTTON))
    {
        if (sGame->state == STATE_PLAYING || sGame->state == STATE_LAUNCH)
        {
            PlaySE(SE_SELECT);
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sGame->state = STATE_FADE_OUT;
        }
    }
}

// ========================================
// Exit
// ========================================

void Pinball_Main(void)
{
    // Main callback - not used
}

void Pinball_Exit(void)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(CB2_ReturnToField);
        ScriptContext_Enable();
        if (sTextWindowId != 0)
        {
            RemoveWindow(sTextWindowId);
            sTextWindowId = 0;
        }
        FREE_AND_SET_NULL(sGame);
    }
}
