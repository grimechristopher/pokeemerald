#include "game_corner_derby.h"
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

#define NUM_RACERS 4
#define TRACK_LENGTH 100
#define MIN_BET 10
#define MAX_BET 100

// Game states
enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_BETTING,
    STATE_CONFIRM_BET,
    STATE_RACE_START,
    STATE_RACING,
    STATE_RACE_FINISH,
    STATE_SHOW_RESULT,
    STATE_WAIT_INPUT,
    STATE_FADE_OUT,
    STATE_EXIT,
};

// ========================================
// Structures
// ========================================

struct Racer
{
    u8 id;
    const u8 *name;
    u16 position;  // 0 to TRACK_LENGTH
    u8 speed;      // Base speed
    u16 odds;      // Payout odds (e.g., 200 = 2x payout)
};

struct DerbyGame
{
    struct Racer racers[NUM_RACERS];
    u8 state;
    u16 timer;
    u8 selectedRacer;
    u16 betAmount;
    u8 winner;
    bool8 playerWon;
    u16 payout;
};

// ========================================
// EWRAM
// ========================================

static EWRAM_DATA struct DerbyGame *sGame = NULL;
static EWRAM_DATA u8 sTextWindowId = 0;

// ========================================
// Racer Names
// ========================================

static const u8 sRacerName1[] = _("RAPIDASH");
static const u8 sRacerName2[] = _("DODRIO");
static const u8 sRacerName3[] = _("ARCANINE");
static const u8 sRacerName4[] = _("ZEBSTRIKA");

// ========================================
// Function Declarations
// ========================================

static void CB2_Derby(void);
static void VBlankCB_Derby(void);
static void Task_DerbyMain(u8 taskId);
static void DrawUI(void);
static void DrawBettingScreen(void);
static void DrawRaceScreen(void);
static void HandleInput(u8 taskId);
static void InitializeRacers(void);
static void UpdateRace(void);
static void DetermineWinner(void);
static void CalculatePayout(void);
static void ShowResult(void);

// ========================================
// Initialization
// ========================================

void Derby_Init(void)
{
    SetMainCallback2(CB2_Derby);
}

static void CB2_Derby(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        sGame = AllocZeroed(sizeof(*sGame));
        sGame->state = STATE_INIT;
        sGame->timer = 0;
        sGame->selectedRacer = 0;
        sGame->betAmount = MIN_BET;
        sGame->winner = 0;
        sGame->playerWon = FALSE;
        sGame->payout = 0;

        InitializeRacers();
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
        SetVBlankCallback(VBlankCB_Derby);
        CreateTask(Task_DerbyMain, 0);
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

static void VBlankCB_Derby(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// ========================================
// Main Game Loop
// ========================================

static void Task_DerbyMain(u8 taskId)
{
    switch (sGame->state)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            DrawBettingScreen();
            sGame->state = STATE_BETTING;
        }
        break;

    case STATE_BETTING:
        HandleInput(taskId);
        break;

    case STATE_CONFIRM_BET:
        // Deduct coins
        if (!GameCorner_TryTakeCoins(sGame->betAmount))
        {
            GameCorner_ShowInsufficientCoinsMessage(sGame->betAmount);
            sGame->state = STATE_BETTING;
        }
        else
        {
            PlaySE(SE_SHOP);
            sGame->state = STATE_RACE_START;
            sGame->timer = 60;  // 1 second countdown
        }
        break;

    case STATE_RACE_START:
        sGame->timer--;
        if (sGame->timer == 0)
        {
            DrawRaceScreen();
            sGame->state = STATE_RACING;
            PlaySE(SE_SUCCESS);
        }
        break;

    case STATE_RACING:
        UpdateRace();
        DrawRaceScreen();
        break;

    case STATE_RACE_FINISH:
        DetermineWinner();
        CalculatePayout();
        sGame->state = STATE_SHOW_RESULT;
        break;

    case STATE_SHOW_RESULT:
        ShowResult();
        GameCorner_IncrementPlayCount(MINIGAME_DERBY);
        if (sGame->playerWon && sGame->payout > 0)
            GameCorner_AddCoins(sGame->payout);
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
        Derby_Exit();
        break;
    }
}

// ========================================
// Racer Management
// ========================================

static void InitializeRacers(void)
{
    u32 i;

    // Racer 1 - Favorite (lower odds)
    sGame->racers[0].id = 0;
    sGame->racers[0].name = sRacerName1;
    sGame->racers[0].position = 0;
    sGame->racers[0].speed = 3;
    sGame->racers[0].odds = 150;  // 1.5x payout

    // Racer 2 - Mid odds
    sGame->racers[1].id = 1;
    sGame->racers[1].name = sRacerName2;
    sGame->racers[1].position = 0;
    sGame->racers[1].speed = 2;
    sGame->racers[1].odds = 200;  // 2x payout

    // Racer 3 - Mid odds
    sGame->racers[2].id = 2;
    sGame->racers[2].name = sRacerName3;
    sGame->racers[2].position = 0;
    sGame->racers[2].speed = 2;
    sGame->racers[2].odds = 250;  // 2.5x payout

    // Racer 4 - Underdog (higher odds)
    sGame->racers[3].id = 3;
    sGame->racers[3].name = sRacerName4;
    sGame->racers[3].position = 0;
    sGame->racers[3].speed = 1;
    sGame->racers[3].odds = 400;  // 4x payout

    // Randomize starting positions slightly
    for (i = 0; i < NUM_RACERS; i++)
    {
        sGame->racers[i].position = Random() % 3;
    }
}

static void UpdateRace(void)
{
    u32 i;
    bool8 raceFinished = FALSE;

    for (i = 0; i < NUM_RACERS; i++)
    {
        // Random movement based on speed
        u8 movement = sGame->racers[i].speed + (Random() % 3);
        sGame->racers[i].position += movement;

        if (sGame->racers[i].position >= TRACK_LENGTH)
        {
            sGame->racers[i].position = TRACK_LENGTH;
            raceFinished = TRUE;
        }
    }

    if (raceFinished)
    {
        sGame->state = STATE_RACE_FINISH;
    }
}

static void DetermineWinner(void)
{
    u32 i;
    u16 maxPosition = 0;
    u8 winner = 0;

    for (i = 0; i < NUM_RACERS; i++)
    {
        if (sGame->racers[i].position > maxPosition)
        {
            maxPosition = sGame->racers[i].position;
            winner = i;
        }
    }

    sGame->winner = winner;
    sGame->playerWon = (sGame->winner == sGame->selectedRacer);
}

static void CalculatePayout(void)
{
    if (sGame->playerWon)
    {
        u32 winnings = (sGame->betAmount * sGame->racers[sGame->selectedRacer].odds) / 100;
        sGame->payout = (u16)winnings;
    }
    else
    {
        sGame->payout = 0;
    }
}

// ========================================
// UI Rendering
// ========================================

static void DrawBettingScreen(void)
{
    u8 str[64];
    u32 i;

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    // Title
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_Derby, 80, 2, TEXT_SKIP_DRAW, NULL);

    // Instructions
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_SelectRacer, 4, 20, TEXT_SKIP_DRAW, NULL);

    // List racers
    for (i = 0; i < NUM_RACERS; i++)
    {
        u8 y = 35 + (i * 12);

        // Highlight selected
        if (i == sGame->selectedRacer)
            StringCopy(str, gText_RightArrow);
        else
            StringCopy(str, gText_Space);

        StringAppend(str, sGame->racers[i].name);
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, y, TEXT_SKIP_DRAW, NULL);

        // Show odds
        StringCopy(str, gText_Odds);
        ConvertIntToDecimalStringN(str + StringLength(str), sGame->racers[i].odds / 100, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringAppendN(str, gText_Period, 1);
        ConvertIntToDecimalStringN(str + StringLength(str), (sGame->racers[i].odds % 100) / 10, STR_CONV_MODE_LEFT_ALIGN, 1);
        StringAppend(str, gText_X);
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 120, y, TEXT_SKIP_DRAW, NULL);
    }

    // Bet amount
    StringCopy(str, gText_Bet);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->betAmount, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 95, TEXT_SKIP_DRAW, NULL);

    // Controls
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_UpDownSelect, 4, 110, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void DrawRaceScreen(void)
{
    u8 str[64];
    u32 i;

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_Racing, 80, 2, TEXT_SKIP_DRAW, NULL);

    // Show racer positions
    for (i = 0; i < NUM_RACERS; i++)
    {
        u8 y = 30 + (i * 15);

        StringCopy(str, sGame->racers[i].name);
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, y, TEXT_SKIP_DRAW, NULL);

        // Progress bar (simple text representation)
        u8 progress = (sGame->racers[i].position * 10) / TRACK_LENGTH;
        u8 j;
        for (j = 0; j < progress; j++)
        {
            StringAppendN(str, gText_Block, 1);
        }
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 80, y, TEXT_SKIP_DRAW, NULL);
    }

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void ShowResult(void)
{
    u8 str[128];

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    // Title
    if (sGame->playerWon)
    {
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_YouWin, 80, 2, TEXT_SKIP_DRAW, NULL);
    }
    else
    {
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_YouLose, 70, 2, TEXT_SKIP_DRAW, NULL);
    }

    // Winner
    StringCopy(str, gText_Winner);
    StringAppend(str, sGame->racers[sGame->winner].name);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 40, TEXT_SKIP_DRAW, NULL);

    // Payout
    if (sGame->playerWon)
    {
        StringCopy(str, gText_Payout);
        ConvertIntToDecimalStringN(str + StringLength(str), sGame->payout, STR_CONV_MODE_LEFT_ALIGN, 4);
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, str, 4, 60, TEXT_SKIP_DRAW, NULL);
    }

    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_PressA, 60, 100, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);

    if (sGame->playerWon)
        PlayFanfare(MUS_OBTAIN_ITEM);
    else
        PlaySE(SE_FAILURE);
}

// ========================================
// Input Handling
// ========================================

static void HandleInput(u8 taskId)
{
    if (sGame->state == STATE_BETTING)
    {
        if (JOY_NEW(DPAD_UP))
        {
            if (sGame->selectedRacer > 0)
            {
                sGame->selectedRacer--;
                PlaySE(SE_SELECT);
                DrawBettingScreen();
            }
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            if (sGame->selectedRacer < NUM_RACERS - 1)
            {
                sGame->selectedRacer++;
                PlaySE(SE_SELECT);
                DrawBettingScreen();
            }
        }
        else if (JOY_NEW(DPAD_LEFT))
        {
            if (sGame->betAmount > MIN_BET)
            {
                sGame->betAmount -= 10;
                PlaySE(SE_SELECT);
                DrawBettingScreen();
            }
        }
        else if (JOY_NEW(DPAD_RIGHT))
        {
            if (sGame->betAmount < MAX_BET)
            {
                sGame->betAmount += 10;
                PlaySE(SE_SELECT);
                DrawBettingScreen();
            }
        }
        else if (JOY_NEW(A_BUTTON))
        {
            PlaySE(SE_SELECT);
            sGame->state = STATE_CONFIRM_BET;
        }
        else if (JOY_NEW(B_BUTTON | START_BUTTON))
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

void Derby_Main(void)
{
    // Main callback - not used
}

void Derby_Exit(void)
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
