#include "game_corner_block_stacker.h"
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

#define FIELD_WIDTH 10
#define FIELD_HEIGHT 20
#define NUM_PIECE_TYPES 7
#define PIECE_SIZE 4

// Piece types
enum {
    PIECE_I,  // Line
    PIECE_O,  // Square
    PIECE_T,  // T-shape
    PIECE_S,  // S-shape
    PIECE_Z,  // Z-shape
    PIECE_J,  // J-shape
    PIECE_L,  // L-shape
};

// Game states
enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_PLAYING,
    STATE_SPAWN_PIECE,
    STATE_CLEAR_LINES,
    STATE_GAME_OVER,
    STATE_SHOW_RESULT,
    STATE_WAIT_INPUT,
    STATE_FADE_OUT,
    STATE_EXIT,
};

// ========================================
// Structures
// ========================================

struct Piece
{
    s8 x;
    s8 y;
    u8 type;
    u8 rotation;  // 0-3
};

struct BlockStackerGame
{
    u8 field[FIELD_HEIGHT][FIELD_WIDTH];
    struct Piece currentPiece;
    struct Piece nextPiece;
    u8 state;
    u16 timer;
    u16 dropTimer;
    u16 dropSpeed;
    u16 score;
    u16 linesCleared;
    u8 level;
};

// ========================================
// EWRAM
// ========================================

static EWRAM_DATA struct BlockStackerGame *sGame = NULL;
static EWRAM_DATA u8 sTextWindowId = 0;

// ========================================
// Piece Definitions
// ========================================

// Piece shapes (4x4 grid, 4 rotations each)
// 1 = filled, 0 = empty
static const u8 sPieceShapes[NUM_PIECE_TYPES][4][4][4] = {
    // I piece
    {
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}},
        {{0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0}},
    },
    // O piece
    {
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
    },
    // T piece
    {
        {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}},
    },
    // S piece
    {
        {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,1,0}, {0,0,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
        {{1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0}},
    },
    // Z piece
    {
        {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,1,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0}},
    },
    // J piece
    {
        {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,1,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0}},
    },
    // L piece
    {
        {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,1,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,0,0}},
        {{0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0}},
        {{1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0}},
    },
};

// ========================================
// Function Declarations
// ========================================

static void CB2_BlockStacker(void);
static void VBlankCB_BlockStacker(void);
static void Task_BlockStackerMain(u8 taskId);
static void DrawUI(void);
static void HandleInput(u8 taskId);
static void SpawnNewPiece(void);
static void GenerateRandomPiece(struct Piece *piece);
static bool8 CanPlacePiece(struct Piece *piece);
static void PlacePiece(struct Piece *piece);
static void LockPiece(void);
static void DropPiece(void);
static bool8 MovePiece(s8 dx, s8 dy);
static bool8 RotatePiece(void);
static void CheckLines(void);
static void ClearLine(u8 line);
static void ShowResult(void);

// ========================================
// Initialization
// ========================================

void BlockStacker_Init(void)
{
    SetMainCallback2(CB2_BlockStacker);
}

static void CB2_BlockStacker(void)
{
    u32 i, j;

    switch (gMain.state)
    {
    case 0:
        SetVBlankCallback(NULL);
        sGame = AllocZeroed(sizeof(*sGame));
        sGame->state = STATE_INIT;
        sGame->timer = 0;
        sGame->dropTimer = 0;
        sGame->dropSpeed = 30;  // Frames per drop
        sGame->score = 0;
        sGame->linesCleared = 0;
        sGame->level = 1;

        // Clear field
        for (i = 0; i < FIELD_HEIGHT; i++)
        {
            for (j = 0; j < FIELD_WIDTH; j++)
            {
                sGame->field[i][j] = 0;
            }
        }

        // Generate first piece
        GenerateRandomPiece(&sGame->nextPiece);
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
        SetVBlankCallback(VBlankCB_BlockStacker);
        CreateTask(Task_BlockStackerMain, 0);
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

static void VBlankCB_BlockStacker(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// ========================================
// Main Game Loop
// ========================================

static void Task_BlockStackerMain(u8 taskId)
{
    switch (sGame->state)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            DrawUI();
            sGame->state = STATE_SPAWN_PIECE;
        }
        break;

    case STATE_SPAWN_PIECE:
        SpawnNewPiece();
        if (!CanPlacePiece(&sGame->currentPiece))
        {
            sGame->state = STATE_GAME_OVER;
        }
        else
        {
            sGame->state = STATE_PLAYING;
            DrawUI();
        }
        break;

    case STATE_PLAYING:
        HandleInput(taskId);

        // Auto drop
        sGame->dropTimer++;
        if (sGame->dropTimer >= sGame->dropSpeed)
        {
            sGame->dropTimer = 0;
            DropPiece();
        }
        break;

    case STATE_CLEAR_LINES:
        CheckLines();
        sGame->state = STATE_SPAWN_PIECE;
        DrawUI();
        break;

    case STATE_GAME_OVER:
        ShowResult();
        GameCorner_IncrementPlayCount(MINIGAME_BLOCK_STACKER);
        if (GameCorner_IsNewHighScore(MINIGAME_BLOCK_STACKER, sGame->score))
            GameCorner_UpdateHighScore(MINIGAME_BLOCK_STACKER, sGame->score);
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
        BlockStacker_Exit();
        break;
    }
}

// ========================================
// Piece Management
// ========================================

static void GenerateRandomPiece(struct Piece *piece)
{
    piece->type = Random() % NUM_PIECE_TYPES;
    piece->rotation = 0;
    piece->x = FIELD_WIDTH / 2 - 2;
    piece->y = 0;
}

static void SpawnNewPiece(void)
{
    sGame->currentPiece = sGame->nextPiece;
    GenerateRandomPiece(&sGame->nextPiece);
}

static bool8 CanPlacePiece(struct Piece *piece)
{
    u32 i, j;

    for (i = 0; i < PIECE_SIZE; i++)
    {
        for (j = 0; j < PIECE_SIZE; j++)
        {
            if (sPieceShapes[piece->type][piece->rotation][i][j])
            {
                s8 fx = piece->x + j;
                s8 fy = piece->y + i;

                // Check bounds
                if (fx < 0 || fx >= FIELD_WIDTH || fy >= FIELD_HEIGHT)
                    return FALSE;

                // Check collision (but allow negative y for spawning)
                if (fy >= 0 && sGame->field[fy][fx])
                    return FALSE;
            }
        }
    }

    return TRUE;
}

static void LockPiece(void)
{
    u32 i, j;

    for (i = 0; i < PIECE_SIZE; i++)
    {
        for (j = 0; j < PIECE_SIZE; j++)
        {
            if (sPieceShapes[sGame->currentPiece.type][sGame->currentPiece.rotation][i][j])
            {
                s8 fx = sGame->currentPiece.x + j;
                s8 fy = sGame->currentPiece.y + i;

                if (fy >= 0 && fy < FIELD_HEIGHT && fx >= 0 && fx < FIELD_WIDTH)
                {
                    sGame->field[fy][fx] = sGame->currentPiece.type + 1;
                }
            }
        }
    }

    PlaySE(SE_PIN);
    sGame->state = STATE_CLEAR_LINES;
}

static void DropPiece(void)
{
    struct Piece testPiece = sGame->currentPiece;
    testPiece.y++;

    if (CanPlacePiece(&testPiece))
    {
        sGame->currentPiece.y++;
    }
    else
    {
        LockPiece();
    }
}

static bool8 MovePiece(s8 dx, s8 dy)
{
    struct Piece testPiece = sGame->currentPiece;
    testPiece.x += dx;
    testPiece.y += dy;

    if (CanPlacePiece(&testPiece))
    {
        sGame->currentPiece.x += dx;
        sGame->currentPiece.y += dy;
        return TRUE;
    }

    return FALSE;
}

static bool8 RotatePiece(void)
{
    struct Piece testPiece = sGame->currentPiece;
    testPiece.rotation = (testPiece.rotation + 1) % 4;

    if (CanPlacePiece(&testPiece))
    {
        sGame->currentPiece.rotation = testPiece.rotation;
        PlaySE(SE_SELECT);
        return TRUE;
    }

    return FALSE;
}

// ========================================
// Line Clearing
// ========================================

static void CheckLines(void)
{
    u32 i, j;
    u32 linesCleared = 0;

    for (i = 0; i < FIELD_HEIGHT; i++)
    {
        bool8 fullLine = TRUE;

        for (j = 0; j < FIELD_WIDTH; j++)
        {
            if (sGame->field[i][j] == 0)
            {
                fullLine = FALSE;
                break;
            }
        }

        if (fullLine)
        {
            ClearLine(i);
            linesCleared++;
        }
    }

    if (linesCleared > 0)
    {
        sGame->linesCleared += linesCleared;

        // Score: 1 line = 100, 2 = 300, 3 = 500, 4 = 800
        u16 points[] = {0, 100, 300, 500, 800};
        sGame->score += points[linesCleared > 4 ? 4 : linesCleared];

        // Increase level every 10 lines
        sGame->level = (sGame->linesCleared / 10) + 1;

        // Increase speed
        if (sGame->dropSpeed > 5)
            sGame->dropSpeed = 30 - (sGame->level * 2);

        PlaySE(SE_SUCCESS);
    }
}

static void ClearLine(u8 line)
{
    u32 i, j;

    // Move all lines above down
    for (i = line; i > 0; i--)
    {
        for (j = 0; j < FIELD_WIDTH; j++)
        {
            sGame->field[i][j] = sGame->field[i - 1][j];
        }
    }

    // Clear top line
    for (j = 0; j < FIELD_WIDTH; j++)
    {
        sGame->field[0][j] = 0;
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
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_BlockStacker, 50, 2, TEXT_SKIP_DRAW, NULL);

    // Score
    StringCopy(str, gText_Score);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->score, STR_CONV_MODE_LEFT_ALIGN, 6);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 30, TEXT_SKIP_DRAW, NULL);

    // Lines
    StringCopy(str, gText_Lines);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->linesCleared, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 45, TEXT_SKIP_DRAW, NULL);

    // Level
    StringCopy(str, gText_Level);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->level, STR_CONV_MODE_LEFT_ALIGN, 2);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 60, TEXT_SKIP_DRAW, NULL);

    // Controls
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_DPadMove, 4, 80, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_ARotate, 4, 92, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_DownDrop, 4, 104, TEXT_SKIP_DRAW, NULL);

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
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, str, 4, 40, TEXT_SKIP_DRAW, NULL);

    // Lines cleared
    StringCopy(str, gText_Lines);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->linesCleared, STR_CONV_MODE_LEFT_ALIGN, 3);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 60, TEXT_SKIP_DRAW, NULL);

    // Level reached
    StringCopy(str, gText_Level);
    ConvertIntToDecimalStringN(str + StringLength(str), sGame->level, STR_CONV_MODE_LEFT_ALIGN, 2);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 4, 75, TEXT_SKIP_DRAW, NULL);

    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_PressA, 60, 100, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);

    if (GameCorner_IsNewHighScore(MINIGAME_BLOCK_STACKER, sGame->score))
        PlayFanfare(MUS_OBTAIN_ITEM);
    else
        PlaySE(SE_FAILURE);
}

// ========================================
// Input Handling
// ========================================

static void HandleInput(u8 taskId)
{
    if (JOY_NEW(DPAD_LEFT))
    {
        if (MovePiece(-1, 0))
            PlaySE(SE_SELECT);
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (MovePiece(1, 0))
            PlaySE(SE_SELECT);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (MovePiece(0, 1))
        {
            sGame->score += 1;  // Bonus for manual drop
            PlaySE(SE_SELECT);
        }
    }
    else if (JOY_NEW(A_BUTTON))
    {
        RotatePiece();
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

void BlockStacker_Main(void)
{
    // Main callback - not used
}

void BlockStacker_Exit(void)
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
