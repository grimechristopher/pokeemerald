#include "game_corner_voltorb_flip.h"
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

#define GRID_SIZE 5
#define TILE_VOLTORB 0
#define TILE_X1 1
#define TILE_X2 2
#define TILE_X3 3

// Game states
enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_WIN,
    STATE_SHOW_RESULT,
    STATE_WAIT_INPUT,
    STATE_FADE_OUT,
    STATE_EXIT,
};

// ========================================
// Structures
// ========================================

struct Tile
{
    u8 value;        // 0=Voltorb, 1=x1, 2=x2, 3=x3
    bool8 revealed;
    bool8 marked;    // Player can mark suspected Voltorbs
};

struct VoltorbFlipGame
{
    struct Tile grid[GRID_SIZE][GRID_SIZE];
    u8 cursorX;
    u8 cursorY;
    u8 state;
    u16 score;
    u16 bet;
    u16 timer;
    u8 level;
    u8 tilesRevealed;
    u8 totalNonVoltorbTiles;
    s16 result;
};

// ========================================
// EWRAM
// ========================================

static EWRAM_DATA struct VoltorbFlipGame *sGame = NULL;
static EWRAM_DATA u8 sTextWindowId = 0;

// ========================================
// Function Declarations
// ========================================

static void CB2_VoltorbFlip(void);
static void VBlankCB_VoltorbFlip(void);
static void Task_VoltorbFlipMain(u8 taskId);
static void InitializeBoard(void);
static void PlaceVoltorbsAndMultipliers(void);
static void CalculateHints(u8 *rowSums, u8 *colSums, u8 *rowVoltorbs, u8 *colVoltorbs);
static void DrawUI(void);
static void DrawGrid(void);
static void DrawHints(void);
static void HandleInput(u8 taskId);
static void FlipTile(u8 x, u8 y);
static void CheckWinCondition(void);
static void ShowResult(void);

// ========================================
// Initialization
// ========================================

void VoltorbFlip_Init(void)
{
    SetMainCallback2(CB2_VoltorbFlip);
}

static void CB2_VoltorbFlip(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetTasks();
        gMain.state++;
        break;
    case 1:
        sGame = AllocZeroed(sizeof(*sGame));
        if (sGame == NULL)
        {
            VoltorbFlip_Exit();
            return;
        }
        sGame->bet = B_VOLTORB_FLIP_ENTRY_COST;
        sGame->state = STATE_INIT;
        sGame->level = 1;
        sGame->score = 1;
        sGame->cursorX = 0;
        sGame->cursorY = 0;
        InitializeBoard();
        gMain.state++;
        break;
    case 2:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, (struct BgTemplate[]){
            {.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0},
            {}
        }, 1);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        gMain.state++;
        break;
    case 3:
        InitWindows((struct WindowTemplate[]){
            {.bg = 0, .tilemapLeft = 1, .tilemapTop = 1, .width = 28, .height = 18, .paletteNum = 15, .baseBlock = 1},
            {}
        });
        sTextWindowId = 0;
        DrawStdWindowFrame(sTextWindowId, FALSE);
        PutWindowTilemap(sTextWindowId);
        CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        gMain.state++;
        break;
    case 4:
        CreateTask(Task_VoltorbFlipMain, 0);
        SetVBlankCallback(VBlankCB_VoltorbFlip);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        RunTasks();
        UpdatePaletteFade();
        break;
    }
}

static void VBlankCB_VoltorbFlip(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void VoltorbFlip_Exit(void)
{
    if (sGame != NULL)
    {
        Free(sGame);
        sGame = NULL;
    }
    SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
}

// ========================================
// Main Loop
// ========================================

static void Task_VoltorbFlipMain(u8 taskId)
{
    switch (sGame->state)
    {
    case STATE_INIT:
        DrawUI();
        DrawGrid();
        DrawHints();
        sGame->state = STATE_FADE_IN;
        break;
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            sGame->state = STATE_PLAYING;
        }
        break;
    case STATE_PLAYING:
        HandleInput(taskId);
        break;
    case STATE_GAME_OVER:
        sGame->result = -(s16)sGame->bet;
        sGame->state = STATE_SHOW_RESULT;
        break;
    case STATE_WIN:
        sGame->result = (sGame->score - 1) * sGame->bet;
        sGame->state = STATE_SHOW_RESULT;
        break;
    case STATE_SHOW_RESULT:
        ShowResult();
        sGame->state = STATE_WAIT_INPUT;
        break;
    case STATE_WAIT_INPUT:
        if (JOY_NEW(A_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sGame->state = STATE_FADE_OUT;
        }
        break;
    case STATE_FADE_OUT:
        if (!gPaletteFade.active)
            sGame->state = STATE_EXIT;
        break;
    case STATE_EXIT:
        DestroyTask(taskId);
        VoltorbFlip_Exit();
        break;
    }
}

// ========================================
// Board Generation
// ========================================

static void InitializeBoard(void)
{
    // Clear the board
    for (u8 y = 0; y < GRID_SIZE; y++)
    {
        for (u8 x = 0; x < GRID_SIZE; x++)
        {
            sGame->grid[y][x].value = TILE_X1;
            sGame->grid[y][x].revealed = FALSE;
            sGame->grid[y][x].marked = FALSE;
        }
    }

    PlaceVoltorbsAndMultipliers();
}

static void PlaceVoltorbsAndMultipliers(void)
{
    u8 voltorbsToPlace = 3 + sGame->level;  // Level 1: 4 Voltorbs, Level 2: 5, etc.
    u8 multipliersToPlace = 3 + (sGame->level / 2);  // Some x2 and x3 tiles

    if (voltorbsToPlace > 10)
        voltorbsToPlace = 10;
    if (multipliersToPlace > 8)
        multipliersToPlace = 8;

    // Place Voltorbs randomly
    for (u8 i = 0; i < voltorbsToPlace; i++)
    {
        u8 x, y;
        do {
            x = Random() % GRID_SIZE;
            y = Random() % GRID_SIZE;
        } while (sGame->grid[y][x].value == TILE_VOLTORB);

        sGame->grid[y][x].value = TILE_VOLTORB;
    }

    // Place multipliers randomly
    for (u8 i = 0; i < multipliersToPlace; i++)
    {
        u8 x, y;
        do {
            x = Random() % GRID_SIZE;
            y = Random() % GRID_SIZE;
        } while (sGame->grid[y][x].value != TILE_X1);

        u8 multiplier = (Random() % 2) + 2;  // x2 or x3
        sGame->grid[y][x].value = multiplier;
    }

    // Count non-Voltorb tiles
    sGame->totalNonVoltorbTiles = 0;
    for (u8 y = 0; y < GRID_SIZE; y++)
    {
        for (u8 x = 0; x < GRID_SIZE; x++)
        {
            if (sGame->grid[y][x].value != TILE_VOLTORB)
                sGame->totalNonVoltorbTiles++;
        }
    }
    sGame->tilesRevealed = 0;
}

static void CalculateHints(u8 *rowSums, u8 *colSums, u8 *rowVoltorbs, u8 *colVoltorbs)
{
    for (u8 i = 0; i < GRID_SIZE; i++)
    {
        rowSums[i] = 0;
        colSums[i] = 0;
        rowVoltorbs[i] = 0;
        colVoltorbs[i] = 0;
    }

    for (u8 y = 0; y < GRID_SIZE; y++)
    {
        for (u8 x = 0; x < GRID_SIZE; x++)
        {
            u8 value = sGame->grid[y][x].value;

            if (value == TILE_VOLTORB)
            {
                rowVoltorbs[y]++;
                colVoltorbs[x]++;
            }
            else
            {
                rowSums[y] += value;
                colSums[x] += value;
            }
        }
    }
}

// ========================================
// UI Drawing
// ========================================

static void DrawUI(void)
{
    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));

    u8 str[32];
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_VoltorbFlip, 60, 2, TEXT_SKIP_DRAW, NULL);

    StringCopy(str, gText_Score);
    ConvertIntToDecimalStringN(gStringVar1, sGame->score, STR_CONV_MODE_LEFT_ALIGN, 5);
    StringAppend(str, gStringVar1);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 160, 2, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void DrawGrid(void)
{
    u8 str[4];

    for (u8 y = 0; y < GRID_SIZE; y++)
    {
        for (u8 x = 0; x < GRID_SIZE; x++)
        {
            u8 screenX = 20 + (x * 20);
            u8 screenY = 20 + (y * 16);

            if (sGame->grid[y][x].revealed)
            {
                // Show the tile value
                u8 value = sGame->grid[y][x].value;
                if (value == TILE_VOLTORB)
                    StringCopy(str, gText_Voltorb);
                else if (value == TILE_X1)
                    StringCopy(str, gText_X1);
                else if (value == TILE_X2)
                    StringCopy(str, gText_X2);
                else
                    StringCopy(str, gText_X3);
            }
            else if (sGame->grid[y][x].marked)
            {
                StringCopy(str, gText_Marked);
            }
            else
            {
                StringCopy(str, gText_Hidden);
            }

            // Highlight cursor
            u8 color = (x == sGame->cursorX && y == sGame->cursorY) ? TEXT_COLOR_RED : TEXT_COLOR_DARK_GRAY;
            AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, screenX, screenY, color, NULL);
        }
    }

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void DrawHints(void)
{
    u8 rowSums[GRID_SIZE];
    u8 colSums[GRID_SIZE];
    u8 rowVoltorbs[GRID_SIZE];
    u8 colVoltorbs[GRID_SIZE];
    u8 str[16];

    CalculateHints(rowSums, colSums, rowVoltorbs, colVoltorbs);

    // Draw row hints (right side)
    for (u8 y = 0; y < GRID_SIZE; y++)
    {
        u8 screenY = 20 + (y * 16);
        ConvertIntToDecimalStringN(str, rowSums[y], STR_CONV_MODE_LEFT_ALIGN, 2);
        StringAppendN(str, gText_Slash, 1);
        ConvertIntToDecimalStringN(gStringVar1, rowVoltorbs[y], STR_CONV_MODE_LEFT_ALIGN, 1);
        StringAppend(str, gStringVar1);
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 130, screenY, TEXT_SKIP_DRAW, NULL);
    }

    // Draw column hints (bottom)
    for (u8 x = 0; x < GRID_SIZE; x++)
    {
        u8 screenX = 20 + (x * 20);
        ConvertIntToDecimalStringN(str, colSums[x], STR_CONV_MODE_LEFT_ALIGN, 2);
        StringAppendN(str, gText_Slash, 1);
        ConvertIntToDecimalStringN(gStringVar1, colVoltorbs[x], STR_CONV_MODE_LEFT_ALIGN, 1);
        StringAppend(str, gStringVar1);
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, screenX, 105, TEXT_SKIP_DRAW, NULL);
    }

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

// ========================================
// Input Handling
// ========================================

static void HandleInput(u8 taskId)
{
    if (JOY_NEW(DPAD_UP) && sGame->cursorY > 0)
    {
        sGame->cursorY--;
        DrawGrid();
    }
    else if (JOY_NEW(DPAD_DOWN) && sGame->cursorY < GRID_SIZE - 1)
    {
        sGame->cursorY++;
        DrawGrid();
    }
    else if (JOY_NEW(DPAD_LEFT) && sGame->cursorX > 0)
    {
        sGame->cursorX--;
        DrawGrid();
    }
    else if (JOY_NEW(DPAD_RIGHT) && sGame->cursorX < GRID_SIZE - 1)
    {
        sGame->cursorX++;
        DrawGrid();
    }
    else if (JOY_NEW(A_BUTTON))
    {
        FlipTile(sGame->cursorX, sGame->cursorY);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        // Mark/unmark tile as suspected Voltorb
        if (!sGame->grid[sGame->cursorY][sGame->cursorX].revealed)
        {
            sGame->grid[sGame->cursorY][sGame->cursorX].marked =
                !sGame->grid[sGame->cursorY][sGame->cursorX].marked;
            DrawGrid();
        }
    }
    else if (JOY_NEW(START_BUTTON))
    {
        // Quit game
        sGame->result = 0;
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        sGame->state = STATE_FADE_OUT;
    }
}

static void FlipTile(u8 x, u8 y)
{
    struct Tile *tile = &sGame->grid[y][x];

    if (tile->revealed || tile->marked)
        return;

    tile->revealed = TRUE;
    sGame->tilesRevealed++;

    if (tile->value == TILE_VOLTORB)
    {
        // Hit a Voltorb - game over
        DrawGrid();
        sGame->state = STATE_GAME_OVER;
    }
    else
    {
        // Multiply score
        if (tile->value > TILE_X1)
            sGame->score *= tile->value;

        DrawGrid();
        DrawUI();

        // Check if won
        CheckWinCondition();
    }
}

static void CheckWinCondition(void)
{
    if (sGame->tilesRevealed >= sGame->totalNonVoltorbTiles)
    {
        // Won - revealed all non-Voltorb tiles
        sGame->state = STATE_WIN;
    }
}

static void ShowResult(void)
{
    const u8 *message;

    if (sGame->state == STATE_SHOW_RESULT)
    {
        if (sGame->result > 0)
            message = gText_YouWin;
        else if (sGame->result < 0)
            message = gText_GameOver;
        else
            message = gText_Quit;

        FillWindowPixelRect(sTextWindowId, PIXEL_FILL(1), 60, 70, 100, 30);
        AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, message, 90, 75, TEXT_SKIP_DRAW, NULL);

        u8 str[32];
        StringCopy(str, gText_FinalScore);
        ConvertIntToDecimalStringN(gStringVar1, sGame->score, STR_CONV_MODE_LEFT_ALIGN, 5);
        StringAppend(str, gStringVar1);
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 75, 88, TEXT_SKIP_DRAW, NULL);

        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_PressA, 85, 100, TEXT_SKIP_DRAW, NULL);
        CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
    }
}

// ========================================
// Entry Points
// ========================================

void VoltorbFlip_Main(void)
{
    // Called by task system - actual logic in Task_VoltorbFlipMain
}
