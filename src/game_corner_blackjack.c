#include "game_corner_blackjack.h"
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

#define MAX_CARDS_PER_HAND 11
#define DECK_SIZE 52
#define DEALER_STAND_VALUE 17
#define BLACKJACK_VALUE 21

// Card ranks
enum {
    RANK_ACE = 1,
    RANK_JACK = 11,
    RANK_QUEEN = 12,
    RANK_KING = 13,
};

// Game states
enum {
    STATE_INIT,
    STATE_FADE_IN,
    STATE_DEAL_INITIAL,
    STATE_PLAYER_TURN,
    STATE_DEALER_REVEAL,
    STATE_DEALER_TURN,
    STATE_CHECK_WINNER,
    STATE_SHOW_RESULT,
    STATE_WAIT_INPUT,
    STATE_FADE_OUT,
    STATE_EXIT,
};

// Menu options
enum {
    MENU_HIT,
    MENU_STAND,
    MENU_QUIT,
    MENU_COUNT
};

// ========================================
// Structures
// ========================================

struct Card
{
    u8 rank;
    u8 suit;
};

struct Hand
{
    struct Card cards[MAX_CARDS_PER_HAND];
    u8 numCards;
    u16 value;
    bool8 hasAce;
};

struct BlackjackGame
{
    struct Card deck[DECK_SIZE];
    u8 deckPosition;
    struct Hand playerHand;
    struct Hand dealerHand;
    u8 state;
    u8 menuChoice;
    u16 bet;
    u16 timer;
    bool8 dealerHoleRevealed;
    s16 result;
};

// ========================================
// EWRAM
// ========================================

static EWRAM_DATA struct BlackjackGame *sGame = NULL;
static EWRAM_DATA u8 sTextWindowId = 0;

// ========================================
// Function Declarations
// ========================================

static void CB2_Blackjack(void);
static void VBlankCB_Blackjack(void);
static void Task_BlackjackMain(u8 taskId);
static void InitDeck(void);
static void ShuffleDeck(void);
static struct Card DeckDrawCard(void);
static void AddCardToHand(struct Hand *hand, struct Card card);
static void CalculateHandValue(struct Hand *hand);
static u8 GetCardValue(u8 rank);
static bool8 IsBlackjack(struct Hand *hand);
static bool8 IsBust(struct Hand *hand);
static void DealInitialCards(void);
static void PlayerHit(void);
static void DealerPlay(void);
static void CheckWinner(void);
static void DrawUI(void);
static void DrawHand(struct Hand *hand, bool8 isDealer, bool8 hideHole);
static void UpdateDisplay(void);
static void HandleInput(u8 taskId);
static void ShowResult(void);

// ========================================
// Initialization
// ========================================

void Blackjack_Init(void)
{
    SetMainCallback2(CB2_Blackjack);
}

static void CB2_Blackjack(void)
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
            Blackjack_Exit();
            return;
        }
        InitDeck();
        ShuffleDeck();
        sGame->bet = B_BLACKJACK_ENTRY_COST;
        sGame->state = STATE_INIT;
        sGame->menuChoice = MENU_HIT;
        sGame->dealerHoleRevealed = FALSE;
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
            {.bg = 0, .tilemapLeft = 2, .tilemapTop = 2, .width = 26, .height = 18, .paletteNum = 15, .baseBlock = 1},
            {}
        });
        sTextWindowId = 0;
        DrawStdWindowFrame(sTextWindowId, FALSE);
        PutWindowTilemap(sTextWindowId);
        CopyWindowToVram(sTextWindowId, COPYWIN_FULL);
        gMain.state++;
        break;
    case 4:
        CreateTask(Task_BlackjackMain, 0);
        SetVBlankCallback(VBlankCB_Blackjack);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        RunTasks();
        UpdatePaletteFade();
        break;
    }
}

static void VBlankCB_Blackjack(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void Blackjack_Exit(void)
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

static void Task_BlackjackMain(u8 taskId)
{
    switch (sGame->state)
    {
    case STATE_INIT:
        DrawUI();
        sGame->state = STATE_FADE_IN;
        break;
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            DealInitialCards();
            sGame->state = STATE_DEAL_INITIAL;
            sGame->timer = 30;
        }
        break;
    case STATE_DEAL_INITIAL:
        if (--sGame->timer == 0)
        {
            UpdateDisplay();
            if (IsBlackjack(&sGame->playerHand))
            {
                sGame->dealerHoleRevealed = TRUE;
                UpdateDisplay();
                if (IsBlackjack(&sGame->dealerHand))
                    sGame->result = 0;
                else
                    sGame->result = (sGame->bet * 3) / 2;
                sGame->state = STATE_SHOW_RESULT;
            }
            else
            {
                sGame->state = STATE_PLAYER_TURN;
            }
        }
        break;
    case STATE_PLAYER_TURN:
        HandleInput(taskId);
        break;
    case STATE_DEALER_REVEAL:
        sGame->dealerHoleRevealed = TRUE;
        UpdateDisplay();
        sGame->timer = 60;
        sGame->state = STATE_DEALER_TURN;
        break;
    case STATE_DEALER_TURN:
        if (--sGame->timer == 0)
        {
            DealerPlay();
        }
        break;
    case STATE_CHECK_WINNER:
        CheckWinner();
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
        Blackjack_Exit();
        break;
    }
}

// ========================================
// Deck Management
// ========================================

static void InitDeck(void)
{
    u8 index = 0;
    for (u8 suit = 0; suit < 4; suit++)
    {
        for (u8 rank = 1; rank <= 13; rank++)
        {
            sGame->deck[index].suit = suit;
            sGame->deck[index].rank = rank;
            index++;
        }
    }
    sGame->deckPosition = 0;
}

static void ShuffleDeck(void)
{
    for (u8 i = DECK_SIZE - 1; i > 0; i--)
    {
        u8 j = Random() % (i + 1);
        struct Card temp = sGame->deck[i];
        sGame->deck[i] = sGame->deck[j];
        sGame->deck[j] = temp;
    }
}

static struct Card DeckDrawCard(void)
{
    if (sGame->deckPosition >= DECK_SIZE)
    {
        InitDeck();
        ShuffleDeck();
    }
    return sGame->deck[sGame->deckPosition++];
}

static void AddCardToHand(struct Hand *hand, struct Card card)
{
    if (hand->numCards < MAX_CARDS_PER_HAND)
    {
        hand->cards[hand->numCards++] = card;
        CalculateHandValue(hand);
    }
}

static void CalculateHandValue(struct Hand *hand)
{
    u16 value = 0;
    u8 aces = 0;

    hand->hasAce = FALSE;

    for (u8 i = 0; i < hand->numCards; i++)
    {
        u8 cardValue = GetCardValue(hand->cards[i].rank);
        value += cardValue;
        if (hand->cards[i].rank == RANK_ACE)
        {
            aces++;
            hand->hasAce = TRUE;
        }
    }

    if (aces > 0 && value + 10 <= BLACKJACK_VALUE)
        value += 10;

    hand->value = value;
}

static u8 GetCardValue(u8 rank)
{
    if (rank == RANK_ACE)
        return 1;
    else if (rank >= 10)
        return 10;
    else
        return rank;
}

static bool8 IsBlackjack(struct Hand *hand)
{
    return (hand->numCards == 2 && hand->value == BLACKJACK_VALUE);
}

static bool8 IsBust(struct Hand *hand)
{
    return (hand->value > BLACKJACK_VALUE);
}

// ========================================
// Game Logic
// ========================================

static void DealInitialCards(void)
{
    sGame->playerHand.numCards = 0;
    sGame->dealerHand.numCards = 0;
    AddCardToHand(&sGame->playerHand, DeckDrawCard());
    AddCardToHand(&sGame->dealerHand, DeckDrawCard());
    AddCardToHand(&sGame->playerHand, DeckDrawCard());
    AddCardToHand(&sGame->dealerHand, DeckDrawCard());
}

static void PlayerHit(void)
{
    AddCardToHand(&sGame->playerHand, DeckDrawCard());
    UpdateDisplay();
    if (IsBust(&sGame->playerHand))
    {
        sGame->result = -sGame->bet;
        sGame->state = STATE_SHOW_RESULT;
    }
}

static void DealerPlay(void)
{
    if (sGame->dealerHand.value < DEALER_STAND_VALUE)
    {
        AddCardToHand(&sGame->dealerHand, DeckDrawCard());
        UpdateDisplay();
        sGame->timer = 60;
    }
    else
    {
        sGame->state = STATE_CHECK_WINNER;
    }
}

static void CheckWinner(void)
{
    u16 pVal = sGame->playerHand.value;
    u16 dVal = sGame->dealerHand.value;

    if (IsBust(&sGame->playerHand))
        sGame->result = -sGame->bet;
    else if (IsBust(&sGame->dealerHand))
        sGame->result = sGame->bet;
    else if (pVal > dVal)
        sGame->result = sGame->bet;
    else if (dVal > pVal)
        sGame->result = -sGame->bet;
    else
        sGame->result = 0;
}

// ========================================
// UI Drawing
// ========================================

static void DrawUI(void)
{
    FillWindowPixelBuffer(sTextWindowId, PIXEL_FILL(1));
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, gText_Blackjack, 80, 4, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void DrawHand(struct Hand *hand, bool8 isDealer, bool8 hideHole)
{
    s16 y = isDealer ? 30 : 100;
    u8 str[64];

    FillWindowPixelRect(sTextWindowId, PIXEL_FILL(1), 0, y, 26 * 8, 30);

    StringCopy(str, isDealer ? gText_Dealer : gText_Player);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 10, y, TEXT_SKIP_DRAW, NULL);

    for (u8 i = 0; i < hand->numCards; i++)
    {
        if (isDealer && i == 1 && hideHole)
        {
            StringCopy(str, gText_HiddenCard);
        }
        else
        {
            u8 rank = hand->cards[i].rank;
            if (rank == 1)
                StringCopy(str, gText_Ace);
            else if (rank == 11)
                StringCopy(str, gText_Jack);
            else if (rank == 12)
                StringCopy(str, gText_Queen);
            else if (rank == 13)
                StringCopy(str, gText_King);
            else
                ConvertIntToDecimalStringN(str, rank, STR_CONV_MODE_LEFT_ALIGN, 2);
        }
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 20 + (i * 16), y + 12, TEXT_SKIP_DRAW, NULL);
    }

    ConvertIntToDecimalStringN(gStringVar1, hand->value, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringCopy(str, gText_Value);
    StringAppend(str, gStringVar1);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, str, 10, y + 24, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void UpdateDisplay(void)
{
    DrawHand(&sGame->playerHand, FALSE, FALSE);
    DrawHand(&sGame->dealerHand, TRUE, !sGame->dealerHoleRevealed);
}

static void HandleInput(u8 taskId)
{
    if (JOY_NEW(DPAD_UP))
        sGame->menuChoice = (sGame->menuChoice + MENU_COUNT - 1) % MENU_COUNT;
    else if (JOY_NEW(DPAD_DOWN))
        sGame->menuChoice = (sGame->menuChoice + 1) % MENU_COUNT;
    else if (JOY_NEW(A_BUTTON))
    {
        switch (sGame->menuChoice)
        {
        case MENU_HIT:
            PlayerHit();
            break;
        case MENU_STAND:
            sGame->state = STATE_DEALER_REVEAL;
            break;
        case MENU_QUIT:
            sGame->result = 0;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sGame->state = STATE_FADE_OUT;
            break;
        }
    }

    // Draw menu
    FillWindowPixelRect(sTextWindowId, PIXEL_FILL(1), 160, 90, 48, 40);
    const u8 *menuTexts[] = {gText_Hit, gText_Stand, gText_Quit};
    for (u8 i = 0; i < MENU_COUNT; i++)
    {
        u8 color = (i == sGame->menuChoice) ? TEXT_COLOR_RED : TEXT_COLOR_DARK_GRAY;
        AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, menuTexts[i], 162, 92 + (i * 12), color, NULL);
    }
    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

static void ShowResult(void)
{
    const u8 *message;

    if (IsBust(&sGame->playerHand))
        message = gText_Bust;
    else if (sGame->result > 0)
        message = gText_YouWin;
    else if (sGame->result < 0)
        message = gText_YouLose;
    else
        message = gText_Push;

    FillWindowPixelRect(sTextWindowId, PIXEL_FILL(1), 60, 70, 100, 20);
    AddTextPrinterParameterized(sTextWindowId, FONT_NORMAL, message, 90, 75, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(sTextWindowId, FONT_SMALL, gText_PressA, 70, 90, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(sTextWindowId, COPYWIN_GFX);
}

// ========================================
// Entry Points (required by common infrastructure)
// ========================================

void Blackjack_Main(void)
{
    // Called by task system - actual logic in Task_BlackjackMain
}
