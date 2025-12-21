#include "game_corner_pachinko.h"
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

#define MAX_BALLS 10
#define PLAYFIELD_WIDTH 160
#define PLAYFIELD_HEIGHT 120
#define BALL_SIZE 4
#define NUM_PRIZE_SLOTS 5

// Prize slot types
enum {
    SLOT_EMPTY,
    SLOT_SMALL,
    SLOT_MEDIUM,
    SLOT_LARGE,
    SLOT_JACKPOT,
};

// Game states
enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_PLAYING,
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
    s16 vx;  // Velocity X
    s16 vy;  // Velocity Y
    bool8 active;
    u8 slot;  // Prize slot the ball landed in
};

struct PachinkoGame
{
    struct Ball balls[MAX_BALLS];
    u8 state;
    u16 timer;
    u16 ballsRemaining;
    u16 ballsLaunched;
    u16 coinsWon;
    u16 launchPower;
    bool8 launching;
};

// ========================================
// EWRAM
// ========================================

static EWRAM_DATA struct PachinkoGame *sGame = NULL;
static EWRAM_DATA u8 sTextWindowId = 0;

// ========================================
// Prize Slot Configuration
// ========================================

// Prize slot positions (bottom of playfield)
static const struct {
    u8 x;
    u8 type;
    u16 payout;
} sPrizeSlots[NUM_PRIZE_SLOTS] = {
    {20, SLOT_SMALL, 1},
    {50, SLOT_MEDIUM, 3},
    {80, SLOT_JACKPOT, 10},
    {110, SLOT_MEDIUM, 3},
    {140, SLOT_SMALL, 1},
};

// ========================================
// Function Declarations
// ========================================

static void CB2_Pachinko(void);
static void VBlankCB_Pachinko(void);
static void Task_PachinkoMain(u8 taskId);
static void DrawUI(void);
static void HandleInput(u8 taskId);
static void LaunchBall(void);
static void UpdateBalls(void);
static void UpdateBallPhysics(struct Ball *ball);
static u8 CheckBallInSlot(struct Ball *ball);
static void ShowResult(void);

// ========================================
// Initialization
// ========================================

void Pachinko_Init(void)
{
    SetMainCallback2(CB2_Pachinko);
}

static void CB2_Pachinko(void)
{
    u32 i;

    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        sGame = AllocZeroed(sizeof(*sGame));
        sGame->state = STATE_INIT;
        sGame->timer = 0;
        sGame->ballsRemaining = 10;
        sGame->ballsLaunched = 0;
        sGame->coinsWon = 0;
        sGame->launchPower = 0;
        sGame->launching = FALSE;

        // Initialize balls
        for (i = 0; i < MAX_BALLS; i++)
        {
            sGame->balls[i].active = FALSE;
            sGame->balls[i].x = 0;
            sGame->balls[i].y = 0;
            sGame->balls[i].vx = 0;
            sGame->balls[i].vy = 0;
            sGame->balls[i].slot = SLOT_EMPTY;
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
        // Create text window
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
        SetVBlankCallback(VBlankCB_Pachinko);
        CreateTask(Task_PachinkoMain, 0);
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

static void VBlankCB_Pachinko(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// ========================================
// Main Game Loop
// ========================================

static void Task_PachinkoMain(u8 taskId)
{
    switch (sGame->state)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            DrawUI();
            sGame->state = STATE_PLAYING;
        }
        break;

    case STATE_PLAYING:
        HandleInput(taskId);
        UpdateBalls();

        // Check if game over (no balls remaining and no active balls)
        if (sGame->ballsRemaining == 0)
        {
            u32 i;
            bool8 anyActive = FALSE;
            for (i = 0; i < MAX_BALLS; i++)
            {
                if (sGame->balls[i].active)
                {
                    anyActive = TRUE;
                    break;
                }
            }

            if (!anyActive)
            {
                sGame->state = STATE_GAME_OVER;
            }
        }
        break;

    case STATE_GAME_OVER:
        ShowResult();
        GameCorner_IncrementPlayCount(MINIGAME_PACHINKO);
        if (sGame->coinsWon > 0)
            GameCorner_AddCoins(sGame->coinsWon);
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
        Pachinko_Exit();
        break;
    }
}

// ========================================
// Ball Physics
// ========================================

static void LaunchBall(void)
{
    u32 i;

    // Find inactive ball
    for (i = 0; i < MAX_BALLS; i++)
    {
        if (!sGame->balls[i].active)
        {
            sGame->balls[i].active = TRUE;
            sGame->balls[i].x = 80;  // Center of playfield
            sGame->balls[i].y = 10;  // Top of playfield

            // Add some random horizontal velocity
            sGame->balls[i].vx = (Random() % 3) - 1;  // -1, 0, or 1
            sGame->balls[i].vy = 2;  // Initial downward velocity
            sGame->balls[i].slot = SLOT_EMPTY;

            sGame->ballsLaunched++;
            sGame->ballsRemaining--;

            PlaySE(SE_BALL);
            break;
        }
    }
}

static void UpdateBalls(void)
{
    u32 i;

    for (i = 0; i < MAX_BALLS; i++)
    {
        if (sGame->balls[i].active)
        {
            UpdateBallPhysics(&sGame->balls[i]);
        }
    }
}

static void UpdateBallPhysics(struct Ball *ball)
{
    // Apply gravity
    ball->vy += 1;
    if (ball->vy > 8)
        ball->vy = 8;  // Terminal velocity

    // Update position
    ball->x += ball->vx;
    ball->y += ball->vy;

    // Bounce off walls
    if (ball->x < BALL_SIZE || ball->x > PLAYFIELD_WIDTH - BALL_SIZE)
    {
        ball->vx = -ball->vx;
        ball->x += ball->vx;  // Move back inside
    }

    // Random horizontal movement (simulate pin bounces)
    if ((Random() % 10) < 3)
    {
        ball->vx += (Random() % 3) - 1;
        if (ball->vx > 2)
            ball->vx = 2;
        if (ball->vx < -2)
            ball->vx = -2;
    }

    // Check if ball reached bottom
    if (ball->y >= PLAYFIELD_HEIGHT)
    {
        u8 slotType = CheckBallInSlot(ball);
        if (slotType != SLOT_EMPTY)
        {
            ball->slot = slotType;

            // Award coins based on slot
            u32 i;
            for (i = 0; i < NUM_PRIZE_SLOTS; i++)
            {
                if (ball->x >= sPrizeSlots[i].x - 10 && ball->x <= sPrizeSlots[i].x + 10)
                {
                    sGame->coinsWon += sPrizeSlots[i].payout;

                    if (sPrizeSlots[i].type == SLOT_JACKPOT)
                        PlaySE(SE_WIN_OPEN);
                    else
                        PlaySE(SE_DING_DONG);
                    break;
                }
            }
        }

        ball->active = FALSE;
    }
}

static u8 CheckBallInSlot(struct Ball *ball)
{
    u32 i;

    for (i = 0; i < NUM_PRIZE_SLOTS; i++)
    {
        if (ball->x >= sPrizeSlots[i].x - 10 && ball->x <= sPrizeSlots[i].x + 10)
        {
            return sPrizeSlots[i].type;
        }
    }

    return SLOT_EMPTY;
}

// ========================================
// UI Rendering
// ========================================

static void DrawUI(void)
{
    u8 str[64];

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    // Title
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_Pachinko, 70, 2, TEXT_SKIP_DRAW, NULL);

    // Balls remaining
    StringCopy(str, gText_Balls);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->ballsRemaining, STR_CONV_MODE_LEFT_ALIGN, 2);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 30, TEXT_SKIP_DRAW, NULL);

    // Coins won
    StringCopy(str, gText_CoinsWon);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->coinsWon, STR_CONV_MODE_LEFT_ALIGN, 4);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 45, TEXT_SKIP_DRAW, NULL);

    // Controls
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_AToLaunch, 4, 70, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_BToQuit, 4, 85, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void ShowResult(void)
{
    u8 str[128];

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    // Title
    if (sGame->coinsWon > 0)
    {
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_YouWin, 80, 2, TEXT_SKIP_DRAW, NULL);
    }
    else
    {
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_GameOver, 70, 2, TEXT_SKIP_DRAW, NULL);
    }

    // Total coins won
    StringCopy(str, gText_TotalCoins);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->coinsWon, STR_CONV_MODE_LEFT_ALIGN, 4);
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, str, 4, 50, TEXT_SKIP_DRAW, NULL);

    // Continue prompt
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_PressA, 60, 100, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);

    if (sGame->coinsWon >= 20)
        PlayFanfare(MUS_OBTAIN_ITEM);
    else if (sGame->coinsWon > 0)
        PlaySE(SE_WIN_OPEN);
}

// ========================================
// Input Handling
// ========================================

static void HandleInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (sGame->ballsRemaining > 0)
        {
            LaunchBall();
            DrawUI();  // Update balls remaining display
        }
    }
    else if (JOY_NEW(B_BUTTON | START_BUTTON))
    {
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sGame->state = STATE_FADE_OUT;
    }
}

// ========================================
// Exit
// ========================================

void Pachinko_Main(void)
{
    // Main callback - not used for this implementation
}

void Pachinko_Exit(void)
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
