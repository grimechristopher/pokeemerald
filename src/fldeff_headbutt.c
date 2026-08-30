#include "global.h"
#include "event_data.h"
#include "event_scripts.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "fldeff.h"
#include "party_menu.h"
#include "script.h"
#include "sound.h"
#include "task.h"
#include "constants/field_effects.h"
#include "constants/songs.h"

static void FieldCallback_Headbutt(void);
static void FieldMove_Headbutt(void);

// Called when Headbutt is used from the party menu, facing an ambient
// tree tagged MB_HEADBUTT_TREE (see GetInteractedMetatileScript in
// field_control_avatar.c for the other entry point: walking up to a
// headbuttable tree and pressing A).
bool32 SetUpFieldMove_Headbutt(void)
{
    if (IsPlayerFacingHeadbuttTree() == TRUE)
    {
        gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
        gPostMenuFieldCallback = FieldCallback_Headbutt;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void FieldCallback_Headbutt(void)
{
    gFieldEffectArguments[0] = GetCursorSelectionMonId();
    ScriptContext_SetupScript(EventScript_Headbutt);
}

bool8 FldEff_UseHeadbutt(void)
{
    u8 taskId = CreateFieldMoveTask();

    gTasks[taskId].data[8] = (u32)FieldMove_Headbutt >> 16;
    gTasks[taskId].data[9] = (u32)FieldMove_Headbutt;
    return FALSE;
}

// Unlike Rock Smash there's no object to remove or break - the tree stays
// put and can be headbutted again. HeadbuttWildEncounter (wild_encounter.c)
// does the actual encounter roll once this pose finishes.
static void FieldMove_Headbutt(void)
{
    PlaySE(SE_M_HEADBUTT);
    FieldEffectActiveListRemove(FLDEFF_USE_HEADBUTT);
    ScriptContext_Enable();
}
