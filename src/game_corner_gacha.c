#include "game_corner_gacha.h"
#include "game_corner_common.h"
#include "global.h"
#include "bg.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "item.h"
#include "item_menu.h"
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
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// ========================================
// Constants
// ========================================

#define MAX_PRIZE_TIERS 3
#define MAX_PRIZES_PER_TIER 10

// Prize rarity tiers
enum {
    TIER_COMMON,
    TIER_RARE,
    TIER_SUPER_RARE,
};

// Game states
enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_MENU,
    STATE_PAYING,
    STATE_DISPENSING,
    STATE_SHOW_PRIZE,
    STATE_WAIT_INPUT,
    STATE_FADE_OUT,
    STATE_EXIT,
};

// ========================================
// Structures
// ========================================

struct Prize
{
    u16 itemId;
    u8 tier;
    u8 weight;  // Relative probability
};

struct GachaGame
{
    u8 state;
    u16 timer;
    u16 wonItemId;
    u8 wonTier;
    s16 result;
};

// ========================================
// EWRAM
// ========================================

static EWRAM_DATA struct GachaGame *sGame = NULL;
static EWRAM_DATA u8 sTextWindowId = 0;

// ========================================
// Prize Pool
// ========================================

// Prize pool configuration
// Common prizes (70% chance)
static const struct Prize sCommonPrizes[] = {
    {ITEM_POTION, TIER_COMMON, 10},
    {ITEM_SUPER_POTION, TIER_COMMON, 8},
    {ITEM_ANTIDOTE, TIER_COMMON, 10},
    {ITEM_PARALYZE_HEAL, TIER_COMMON, 10},
    {ITEM_AWAKENING, TIER_COMMON, 10},
    {ITEM_BURN_HEAL, TIER_COMMON, 10},
    {ITEM_ICE_HEAL, TIER_COMMON, 10},
    {ITEM_FULL_HEAL, TIER_COMMON, 5},
    {ITEM_REVIVE, TIER_COMMON, 3},
    {ITEM_RARE_CANDY, TIER_COMMON, 2},
};

// Rare prizes (25% chance)
static const struct Prize sRarePrizes[] = {
    {ITEM_HYPER_POTION, TIER_RARE, 10},
    {ITEM_MAX_POTION, TIER_RARE, 5},
    {ITEM_FULL_RESTORE, TIER_RARE, 3},
    {ITEM_MAX_REVIVE, TIER_RARE, 5},
    {ITEM_PP_UP, TIER_RARE, 8},
    {ITEM_PROTEIN, TIER_RARE, 8},
    {ITEM_IRON, TIER_RARE, 8},
    {ITEM_CALCIUM, TIER_RARE, 8},
    {ITEM_ZINC, TIER_RARE, 8},
    {ITEM_CARBOS, TIER_RARE, 8},
};

// Super rare prizes (5% chance)
static const struct Prize sSuperRarePrizes[] = {
    {ITEM_MAX_ELIXIR, TIER_SUPER_RARE, 10},
    {ITEM_PP_MAX, TIER_SUPER_RARE, 10},
    {ITEM_HP_UP, TIER_SUPER_RARE, 10},
    {ITEM_NUGGET, TIER_SUPER_RARE, 15},
    {ITEM_STAR_PIECE, TIER_SUPER_RARE, 8},
    {ITEM_HEART_SCALE, TIER_SUPER_RARE, 12},
    {ITEM_SUN_STONE, TIER_SUPER_RARE, 5},
    {ITEM_MOON_STONE, TIER_SUPER_RARE, 5},
    {ITEM_FIRE_STONE, TIER_SUPER_RARE, 5},
    {ITEM_THUNDER_STONE, TIER_SUPER_RARE, 5},
};

#define NUM_COMMON_PRIZES ARRAY_COUNT(sCommonPrizes)
#define NUM_RARE_PRIZES ARRAY_COUNT(sRarePrizes)
#define NUM_SUPER_RARE_PRIZES ARRAY_COUNT(sSuperRarePrizes)

// Tier weights (out of 100)
#define COMMON_TIER_WEIGHT 70
#define RARE_TIER_WEIGHT 25
#define SUPER_RARE_TIER_WEIGHT 5

// ========================================
// Function Declarations
// ========================================

static void CB2_Gacha(void);
static void VBlankCB_Gacha(void);
static void Task_GachaMain(u8 taskId);
static void DrawUI(void);
static void HandleInput(u8 taskId);
static void SelectRandomPrize(void);
static void ShowPrizeResult(void);
static void GivePrizeToPlayer(void);

// ========================================
// Initialization
// ========================================

void Gacha_Init(void)
{
    SetMainCallback2(CB2_Gacha);
}

static void CB2_Gacha(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        sGame = AllocZeroed(sizeof(*sGame));
        sGame->state = STATE_INIT;
        sGame->timer = 0;
        sGame->wonItemId = ITEM_NONE;
        sGame->wonTier = TIER_COMMON;
        sGame->result = 0;
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
        SetVBlankCallback(VBlankCB_Gacha);
        CreateTask(Task_GachaMain, 0);
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

static void VBlankCB_Gacha(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// ========================================
// Main Game Loop
// ========================================

static void Task_GachaMain(u8 taskId)
{
    switch (sGame->state)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            DrawUI();
            sGame->state = STATE_MENU;
        }
        break;

    case STATE_MENU:
        HandleInput(taskId);
        break;

    case STATE_PAYING:
        // Deduct coins
        if (!GameCorner_TryTakeCoins(B_GACHA_ENTRY_COST))
        {
            GameCorner_ShowInsufficientCoinsMessage(B_GACHA_ENTRY_COST);
            sGame->state = STATE_MENU;
        }
        else
        {
            PlaySE(SE_SHOP);
            sGame->state = STATE_DISPENSING;
            sGame->timer = 60;  // 1 second animation
        }
        break;

    case STATE_DISPENSING:
        // Animate capsule dispensing
        sGame->timer--;
        if (sGame->timer == 0)
        {
            SelectRandomPrize();
            sGame->state = STATE_SHOW_PRIZE;
        }
        break;

    case STATE_SHOW_PRIZE:
        ShowPrizeResult();
        GivePrizeToPlayer();
        GameCorner_IncrementPlayCount(MINIGAME_GACHA);
        sGame->state = STATE_WAIT_INPUT;
        break;

    case STATE_WAIT_INPUT:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            sGame->state = STATE_MENU;
            DrawUI();
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
        Gacha_Exit();
        break;
    }
}

// ========================================
// Prize Selection
// ========================================

static void SelectRandomPrize(void)
{
    u32 tierRoll = Random() % 100;
    u8 selectedTier;
    const struct Prize *prizePool;
    u32 prizeCount;

    // Determine tier based on weighted probability
    if (tierRoll < SUPER_RARE_TIER_WEIGHT)
    {
        selectedTier = TIER_SUPER_RARE;
        prizePool = sSuperRarePrizes;
        prizeCount = NUM_SUPER_RARE_PRIZES;
    }
    else if (tierRoll < SUPER_RARE_TIER_WEIGHT + RARE_TIER_WEIGHT)
    {
        selectedTier = TIER_RARE;
        prizePool = sRarePrizes;
        prizeCount = NUM_RARE_PRIZES;
    }
    else
    {
        selectedTier = TIER_COMMON;
        prizePool = sCommonPrizes;
        prizeCount = NUM_COMMON_PRIZES;
    }

    // Calculate total weight for selected tier
    u32 totalWeight = 0;
    u32 i;
    for (i = 0; i < prizeCount; i++)
    {
        totalWeight += prizePool[i].weight;
    }

    // Select prize within tier based on weight
    u32 prizeRoll = Random() % totalWeight;
    u32 currentWeight = 0;

    for (i = 0; i < prizeCount; i++)
    {
        currentWeight += prizePool[i].weight;
        if (prizeRoll < currentWeight)
        {
            sGame->wonItemId = prizePool[i].itemId;
            sGame->wonTier = selectedTier;
            return;
        }
    }

    // Fallback (should never happen)
    sGame->wonItemId = prizePool[0].itemId;
    sGame->wonTier = selectedTier;
}

static void GivePrizeToPlayer(void)
{
    if (sGame->wonItemId != ITEM_NONE)
    {
        AddBagItem(sGame->wonItemId, 1);
    }
}

// ========================================
// UI Rendering
// ========================================

static void DrawUI(void)
{
    u8 str[64];

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    // Title
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_Gacha, 80, 2, TEXT_SKIP_DRAW, NULL);

    // Instructions
    StringCopy(str, gText_GachaInstructions);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 30, TEXT_SKIP_DRAW, NULL);

    // Cost
    ConvertIntToDecimalStringN(str, B_GACHA_ENTRY_COST, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringAppend(str, gText_CoinsPerPlay);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 50, TEXT_SKIP_DRAW, NULL);

    // Controls
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_AToPlay, 4, 70, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_BToQuit, 4, 85, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void ShowPrizeResult(void)
{
    u8 str[128];
    const u8 *tierName;

    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    // Title
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_YouWin, 80, 2, TEXT_SKIP_DRAW, NULL);

    // Tier name
    switch (sGame->wonTier)
    {
    case TIER_COMMON:
        tierName = gText_Common;
        break;
    case TIER_RARE:
        tierName = gText_Rare;
        break;
    case TIER_SUPER_RARE:
        tierName = gText_SuperRare;
        break;
    default:
        tierName = gText_Common;
        break;
    }

    StringCopy(str, tierName);
    StringAppend(str, gText_Prize);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 30, TEXT_SKIP_DRAW, NULL);

    // Item name
    CopyItemName(sGame->wonItemId, str);
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, str, 4, 50, TEXT_SKIP_DRAW, NULL);

    // Continue prompt
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_PressA, 60, 100, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);

    // Play sound effect based on rarity
    if (sGame->wonTier == TIER_SUPER_RARE)
        PlayFanfare(MUS_OBTAIN_ITEM);
    else if (sGame->wonTier == TIER_RARE)
        PlaySE(SE_WIN_OPEN);
    else
        PlaySE(SE_BALL);
}

// ========================================
// Input Handling
// ========================================

static void HandleInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        sGame->state = STATE_PAYING;
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

void Gacha_Main(void)
{
    // Main callback - not used for this implementation
}

void Gacha_Exit(void)
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
