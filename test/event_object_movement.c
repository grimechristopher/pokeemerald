#include "global.h"
#include "event_object_movement.h"
#include "field_control_avatar.h"
#include "field_player_avatar.h"
#include "fieldmap.h"
#include "follower_npc.h"
#include "sprite.h"
#include "test/test.h"

static const u32 sFrame16x32[256 / sizeof(u32)] = {0};
static const u32 sFrame32x32[512 / sizeof(u32)] = {0};

static const struct OamData sOam16x32 = {
    .shape = SPRITE_SHAPE(16x32),
    .size = SPRITE_SIZE(16x32),
};

static const struct OamData sOam32x32 = {
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
};

static const struct SpriteFrameImage sImages16x32[] = {
    {.data = sFrame16x32, .size = sizeof(sFrame16x32)},
};

static const struct SpriteFrameImage sImages32x32[] = {
    {.data = sFrame32x32, .size = sizeof(sFrame32x32)},
};

static const struct SpriteTemplate sSpriteTemplate16x32 = {
    .tileTag = TAG_NONE,
    .paletteTag = TAG_NONE,
    .oam = &sOam16x32,
    .images = sImages16x32,
    .callback = SpriteCallbackDummy,
};

static const struct ObjectEventGraphicsInfo sGraphicsInfo32x32 = {
    .tileTag = TAG_NONE,
    .size = sizeof(sFrame32x32),
    .oam = &sOam32x32,
    .images = sImages32x32,
};

static const struct ObjectEventGraphicsInfo sGraphicsInfo32x32WithDiagonals = {
    .tileTag = TAG_NONE,
    .size = sizeof(sFrame32x32),
    .oam = &sOam32x32,
    .images = sImages32x32,
    .hasDiagonalFrames = TRUE,
};

extern u16 LoadSheetGraphicsInfo(const struct ObjectEventGraphicsInfo *info, u16 uuid, struct Sprite *sprite);

TEST("GetFaceDirectionAnimNumForGraphicsInfo falls back to the East/West substitution when hasDiagonalFrames is unset")
{
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32, DIR_NORTHEAST), ANIM_STD_FACE_EAST);
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32, DIR_NORTHWEST), ANIM_STD_FACE_WEST);
}

TEST("GetFaceDirectionAnimNumForGraphicsInfo uses the real diagonal anim when hasDiagonalFrames is set")
{
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32WithDiagonals, DIR_NORTHEAST), ANIM_STD_FACE_NORTHEAST);
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32WithDiagonals, DIR_NORTHWEST), ANIM_STD_FACE_NORTHWEST);
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32WithDiagonals, DIR_SOUTHEAST), ANIM_STD_FACE_SOUTHEAST);
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32WithDiagonals, DIR_SOUTHWEST), ANIM_STD_FACE_SOUTHWEST);
}

TEST("GetFaceDirectionAnimNumForGraphicsInfo is unaffected for cardinal directions either way")
{
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32, DIR_NORTH), ANIM_STD_FACE_NORTH);
    EXPECT_EQ(GetFaceDirectionAnimNumForGraphicsInfo(&sGraphicsInfo32x32WithDiagonals, DIR_NORTH), ANIM_STD_FACE_NORTH);
}

TEST("LoadSheetGraphicsInfo reallocates non-sheet sprites when frame size changes")
{
    u16 i;
    u16 tileNum;
    u16 tileCount;
    u8 spriteId;
    struct Sprite *sprite;

    ASSUME(sOam16x32.size == sOam32x32.size);
    ASSUME(sOam16x32.shape != sOam32x32.shape);

    ResetSpriteData();
    spriteId = CreateSprite(&sSpriteTemplate16x32, 0, 0, 0);
    ASSUME(spriteId != MAX_SPRITES);

    sprite = &gSprites[spriteId];
    tileNum = sprite->oam.tileNum;
    tileCount = sGraphicsInfo32x32.images->size / TILE_SIZE_4BPP;

    EXPECT_EQ(sprite->images->size, sizeof(sFrame16x32));
    EXPECT_EQ(sGraphicsInfo32x32.images->size, sizeof(sFrame32x32));

    LoadSheetGraphicsInfo(&sGraphicsInfo32x32, 0, sprite);

    for (i = 0; i < tileCount; i++)
        EXPECT(SpriteTileAllocBitmapOp(tileNum + i, 2) != 0);

    DestroySprite(sprite);
}

TEST("IsMetatileDirectionallyImpassable returns FALSE for diagonal directions instead of reading out of bounds")
{
    struct ObjectEvent objectEvent = {0};
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_NORTHEAST), FALSE);
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_NORTHWEST), FALSE);
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_SOUTHEAST), FALSE);
    EXPECT_EQ(IsMetatileDirectionallyImpassable(&objectEvent, 0, 0, DIR_SOUTHWEST), FALSE);
}

static void PlaceTestObjectEvent(struct ObjectEvent *objectEvent, s16 x, s16 y)
{
    memset(objectEvent, 0, sizeof(*objectEvent));
    objectEvent->currentCoords.x = x;
    objectEvent->currentCoords.y = y;
    objectEvent->previousCoords.x = x;
    objectEvent->previousCoords.y = y;
    objectEvent->currentElevation = ELEVATION_DEFAULT;
}

// GetMapBorderIdAt (src/fieldmap.c) treats any coordinate within MAP_OFFSET (7) tiles of the
// grid edge as a border/connection check, returning CONNECTION_INVALID (blocking every move)
// unless sMapConnectionFlags says otherwise - which is unset in a bare test context. The grid
// has to be big enough that the whole test area sits strictly inside that margin: valid
// interior coordinates are 7 <= x < (width - 8) and 7 <= y < (height - 7). A 21x21 grid with
// the object event at (10, 10) gives a comfortable interior neighborhood (valid x: 7-12,
// valid y: 7-13) for every direction this task's tests move in.
#define TEST_MAP_SIZE 21
#define TEST_MAP_ORIGIN 10
static u16 sTestMapGrid[TEST_MAP_SIZE * TEST_MAP_SIZE];

static void SetUpTestMap(void)
{
    s32 i;
    for (i = 0; i < ARRAY_COUNT(sTestMapGrid); i++)
        sTestMapGrid[i] = PACK_ELEVATION(ELEVATION_DEFAULT);
    gBackupMapLayout.width = TEST_MAP_SIZE;
    gBackupMapLayout.height = TEST_MAP_SIZE;
    gBackupMapLayout.map = sTestMapGrid;
    // DoesObjectCollideWithObjectAt scans the global gObjectEvents[] array - clear it so a
    // stale .active entry left over from an unrelated earlier test can't cause a spurious
    // collision at one of this test's coordinates.
    memset(gObjectEvents, 0, sizeof(gObjectEvents));
}

static void BlockTestMapTile(s16 x, s16 y)
{
    sTestMapGrid[x + gBackupMapLayout.width * y] = PACK_ELEVATION(ELEVATION_DEFAULT) | MAPGRID_IMPASSABLE;
}

TEST("CanObjectEventMoveInDirection allows a plain cardinal move onto an open tile")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTH), TRUE);
}

TEST("CanObjectEventMoveInDirection blocks a plain cardinal move onto a blocked tile")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // directly north
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTH), FALSE);
}

TEST("CanObjectEventMoveInDirection allows a diagonal move when both flanks are open")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), TRUE);
}

TEST("CanObjectEventMoveInDirection allows a diagonal move when exactly one flank is open")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // north flank blocked
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), TRUE); // east flank still open
}

TEST("CanObjectEventMoveInDirection blocks corner-cutting when both flanks are blocked")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // north flank blocked
    BlockTestMapTile(TEST_MAP_ORIGIN + 1, TEST_MAP_ORIGIN); // east flank blocked
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), FALSE);
}

TEST("CanObjectEventMoveInDirection blocks a diagonal move onto a blocked destination even with both flanks open")
{
    struct ObjectEvent objectEvent;
    SetUpTestMap();
    PlaceTestObjectEvent(&objectEvent, TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    BlockTestMapTile(TEST_MAP_ORIGIN + 1, TEST_MAP_ORIGIN - 1); // the actual NE destination tile
    EXPECT_EQ(CanObjectEventMoveInDirection(&objectEvent, DIR_NORTHEAST), FALSE);
}

TEST("IsDiagonalMoveBlockedByCorner returns FALSE for a cardinal direction")
{
    struct ObjectEvent objectEvent = {0};
    objectEvent.currentCoords.x = 10;
    objectEvent.currentCoords.y = 10;
    EXPECT_EQ(IsDiagonalMoveBlockedByCorner(&objectEvent, DIR_NORTH), FALSE);
}

// CheckForPlayerAvatarCollision is the player's own collision path - a separate function
// from CanObjectEventMoveInDirection (used by NPC wander). These integration-level tests
// drive it directly, against the player's own gObjectEvents entry (gPlayerAvatar.objectEventId),
// to confirm the no-corner-cutting rule is actually wired up there and not just unit-tested
// in isolation via IsDiagonalMoveBlockedByCorner/CanObjectEventMoveInDirection above.
TEST("CheckForPlayerAvatarCollision blocks corner-cutting on the player's own diagonal move when both flanks are blocked")
{
    u8 savedObjectEventId = gPlayerAvatar.objectEventId;

    SetUpTestMap();
    PlaceTestObjectEvent(&gObjectEvents[0], TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    gPlayerAvatar.objectEventId = 0;
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // north flank blocked
    BlockTestMapTile(TEST_MAP_ORIGIN + 1, TEST_MAP_ORIGIN); // east flank blocked

    EXPECT_EQ(CheckForPlayerAvatarCollision(DIR_NORTHEAST), COLLISION_IMPASSABLE);

    gPlayerAvatar.objectEventId = savedObjectEventId;
}

TEST("CheckForPlayerAvatarCollision allows the player's diagonal move when at least one flank is open")
{
    u8 savedObjectEventId = gPlayerAvatar.objectEventId;

    SetUpTestMap();
    PlaceTestObjectEvent(&gObjectEvents[0], TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    gPlayerAvatar.objectEventId = 0;
    BlockTestMapTile(TEST_MAP_ORIGIN, TEST_MAP_ORIGIN - 1); // north flank blocked, east flank still open

    EXPECT_EQ(CheckForPlayerAvatarCollision(DIR_NORTHEAST), COLLISION_NONE);

    gPlayerAvatar.objectEventId = savedObjectEventId;
}

TEST("GetDiagonalMoveDirection combines a held vertical and horizontal direction into a diagonal")
{
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NORTH, DIR_EAST), DIR_NORTHEAST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NORTH, DIR_WEST), DIR_NORTHWEST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_SOUTH, DIR_EAST), DIR_SOUTHEAST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_SOUTH, DIR_WEST), DIR_SOUTHWEST);
}

TEST("GetDiagonalMoveDirection falls back to whichever single axis is held")
{
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NORTH, DIR_NONE), DIR_NORTH);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NONE, DIR_EAST), DIR_EAST);
    EXPECT_EQ(GetDiagonalMoveDirection(DIR_NONE, DIR_NONE), DIR_NONE);
}

TEST("ResolveStairsMoveDirection prefers the horizontal component of a diagonal input")
{
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NORTHEAST), DIR_EAST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NORTHWEST), DIR_WEST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_SOUTHEAST), DIR_EAST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_SOUTHWEST), DIR_WEST);
}

TEST("ResolveStairsMoveDirection passes cardinal input through unchanged")
{
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NORTH), DIR_NORTH);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_SOUTH), DIR_SOUTH);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_EAST), DIR_EAST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_WEST), DIR_WEST);
    EXPECT_EQ(ResolveStairsMoveDirection(DIR_NONE), DIR_NONE);
}

TEST("A diagonal MoveCoords step moves both axes by exactly one tile")
{
    s16 x = 5, y = 5;
    MoveCoords(DIR_NORTHEAST, &x, &y);
    EXPECT_EQ(x, 6);
    EXPECT_EQ(y, 4);

    x = 5; y = 5;
    MoveCoords(DIR_SOUTHWEST, &x, &y);
    EXPECT_EQ(x, 4);
    EXPECT_EQ(y, 6);
}

TEST("GetFaceDirectionMovementAction resolves every diagonal direction to a valid movement action")
{
    EXPECT(GetFaceDirectionMovementAction(DIR_NORTHEAST) != MOVEMENT_ACTION_NONE);
    EXPECT(GetFaceDirectionMovementAction(DIR_NORTHWEST) != MOVEMENT_ACTION_NONE);
    EXPECT(GetFaceDirectionMovementAction(DIR_SOUTHEAST) != MOVEMENT_ACTION_NONE);
    EXPECT(GetFaceDirectionMovementAction(DIR_SOUTHWEST) != MOVEMENT_ACTION_NONE);
}

TEST("gStandardDirectionsWithDiagonals has exactly the 4 cardinal and 4 diagonal directions")
{
    s32 i;
    bool8 seen[CARDINAL_DIRECTION_COUNT + 4] = {0};

    EXPECT_EQ(ARRAY_COUNT(gStandardDirectionsWithDiagonals), 8);
    for (i = 0; i < ARRAY_COUNT(gStandardDirectionsWithDiagonals); i++)
    {
        enum Direction direction = gStandardDirectionsWithDiagonals[i];
        EXPECT(direction >= DIR_SOUTH && direction <= DIR_NORTHEAST);
        EXPECT(!seen[direction]);
        seen[direction] = TRUE;
    }
}

TEST("GetFollowerStepDirection returns a diagonal direction when both axes differ by one tile")
{
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 6, 4), DIR_NORTHEAST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 4, 4), DIR_NORTHWEST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 6, 6), DIR_SOUTHEAST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 4, 6), DIR_SOUTHWEST);
}

TEST("GetFollowerStepDirection falls back to a cardinal direction when only one axis differs")
{
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 5, 4), DIR_NORTH);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 5, 6), DIR_SOUTH);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 4, 5), DIR_WEST);
    EXPECT_EQ(GetFollowerStepDirection(5, 5, 6, 5), DIR_EAST);
}

TEST("GetWalkNormalMovementAction resolves every diagonal direction to a distinct, valid action")
{
    EXPECT_EQ(GetWalkNormalMovementAction(DIR_NORTHEAST), MOVEMENT_ACTION_WALK_NORMAL_DIAGONAL_UP_RIGHT);
    EXPECT_EQ(GetWalkNormalMovementAction(DIR_NORTHWEST), MOVEMENT_ACTION_WALK_NORMAL_DIAGONAL_UP_LEFT);
    EXPECT_EQ(GetWalkNormalMovementAction(DIR_SOUTHEAST), MOVEMENT_ACTION_WALK_NORMAL_DIAGONAL_DOWN_RIGHT);
    EXPECT_EQ(GetWalkNormalMovementAction(DIR_SOUTHWEST), MOVEMENT_ACTION_WALK_NORMAL_DIAGONAL_DOWN_LEFT);
}

TEST("GetWalkFastMovementAction resolves every diagonal direction to a distinct, valid action")
{
    EXPECT_EQ(GetWalkFastMovementAction(DIR_NORTHEAST), MOVEMENT_ACTION_WALK_FAST_DIAGONAL_UP_RIGHT);
    EXPECT_EQ(GetWalkFastMovementAction(DIR_NORTHWEST), MOVEMENT_ACTION_WALK_FAST_DIAGONAL_UP_LEFT);
    EXPECT_EQ(GetWalkFastMovementAction(DIR_SOUTHEAST), MOVEMENT_ACTION_WALK_FAST_DIAGONAL_DOWN_RIGHT);
    EXPECT_EQ(GetWalkFastMovementAction(DIR_SOUTHWEST), MOVEMENT_ACTION_WALK_FAST_DIAGONAL_DOWN_LEFT);
}

TEST("GetWalkSlowMovementAction resolves every diagonal direction to a distinct, valid action")
{
    EXPECT_EQ(GetWalkSlowMovementAction(DIR_NORTHEAST), MOVEMENT_ACTION_WALK_SLOW_DIAGONAL_UP_RIGHT);
    EXPECT_EQ(GetWalkSlowMovementAction(DIR_NORTHWEST), MOVEMENT_ACTION_WALK_SLOW_DIAGONAL_UP_LEFT);
    EXPECT_EQ(GetWalkSlowMovementAction(DIR_SOUTHEAST), MOVEMENT_ACTION_WALK_SLOW_DIAGONAL_DOWN_RIGHT);
    EXPECT_EQ(GetWalkSlowMovementAction(DIR_SOUTHWEST), MOVEMENT_ACTION_WALK_SLOW_DIAGONAL_DOWN_LEFT);
}

TEST("GetWalkFasterMovementAction falls back to the WALK_FAST diagonal action")
{
    EXPECT_EQ(GetWalkFasterMovementAction(DIR_NORTHEAST), MOVEMENT_ACTION_WALK_FAST_DIAGONAL_UP_RIGHT);
}

TEST("GetPlayerRunMovementAction falls back to the WALK_FAST diagonal action")
{
    EXPECT_EQ(GetPlayerRunMovementAction(DIR_NORTHEAST), MOVEMENT_ACTION_WALK_FAST_DIAGONAL_UP_RIGHT);
}

TEST("A dirn_to_anim table with no diagonal entries falls back safely instead of reading out of bounds")
{
    // gJump2MovementActions is deliberately NOT extended with diagonal entries (ledges are
    // out of scope for diagonal movement) - confirm the macro's bounds check makes this safe
    // rather than reading past the array, for every diagonal direction including the
    // DIR_SOUTHWEST edge case that used to pass the old off-by-one `>` check.
    EXPECT_EQ(GetJump2MovementAction(DIR_SOUTHWEST), MOVEMENT_ACTION_JUMP_2_DOWN);
    EXPECT_EQ(GetJump2MovementAction(DIR_SOUTHEAST), MOVEMENT_ACTION_JUMP_2_DOWN);
    EXPECT_EQ(GetJump2MovementAction(DIR_NORTHWEST), MOVEMENT_ACTION_JUMP_2_DOWN);
    EXPECT_EQ(GetJump2MovementAction(DIR_NORTHEAST), MOVEMENT_ACTION_JUMP_2_DOWN);
}

TEST("GetLedgeJumpDirection does not trigger a ledge jump for diagonal directions")
{
    EXPECT_EQ(GetLedgeJumpDirection(0, 0, DIR_NORTHEAST), DIR_NONE);
    EXPECT_EQ(GetLedgeJumpDirection(0, 0, DIR_NORTHWEST), DIR_NONE);
    EXPECT_EQ(GetLedgeJumpDirection(0, 0, DIR_SOUTHEAST), DIR_NONE);
    EXPECT_EQ(GetLedgeJumpDirection(0, 0, DIR_SOUTHWEST), DIR_NONE);
}

TEST("FieldGetPlayerInput does not combine diagonal input while surfing")
{
    struct FieldInput input = {0};
    u8 savedFlags = gPlayerAvatar.flags;

    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_SURFING;
    FieldGetPlayerInput(&input, 0, DPAD_UP | DPAD_RIGHT);
    EXPECT_EQ(input.dpadDirection, DIR_NORTH);

    gPlayerAvatar.flags = savedFlags;
}

TEST("FieldGetPlayerInput combines diagonal input while on foot")
{
    struct FieldInput input = {0};
    u8 savedFlags = gPlayerAvatar.flags;

    gPlayerAvatar.flags = PLAYER_AVATAR_FLAG_ON_FOOT;
    FieldGetPlayerInput(&input, 0, DPAD_UP | DPAD_RIGHT);
    EXPECT_EQ(input.dpadDirection, DIR_NORTHEAST);

    gPlayerAvatar.flags = savedFlags;
}

TEST("DetermineFollowerNPCDirection returns a diagonal direction when both axes differ")
{
    struct ObjectEvent player = {0};
    struct ObjectEvent follower = {0};

    player.currentCoords.x = 12;
    player.currentCoords.y = 8;
    follower.currentCoords.x = 10;
    follower.currentCoords.y = 10;

    EXPECT_EQ(DetermineFollowerNPCDirection(&player, &follower), DIR_NORTHEAST);
}

TEST("DetermineFollowerNPCDirection returns DIR_NONE when the follower is on the player's tile")
{
    struct ObjectEvent player = {0};
    struct ObjectEvent follower = {0};

    player.currentCoords.x = 12;
    player.currentCoords.y = 8;
    follower.currentCoords.x = 12;
    follower.currentCoords.y = 8;

    EXPECT_EQ(DetermineFollowerNPCDirection(&player, &follower), DIR_NONE);
}

TEST("DetermineFollowerNPCDirection matches DetermineObjectEventDirectionFromObject for a pure cardinal delta")
{
    struct ObjectEvent player = {0};
    struct ObjectEvent follower = {0};

    player.currentCoords.x = 12;
    player.currentCoords.y = 8;
    follower.currentCoords.x = 10;
    follower.currentCoords.y = 8;

    EXPECT_EQ(DetermineFollowerNPCDirection(&player, &follower), DetermineObjectEventDirectionFromObject(&player, &follower));
    EXPECT_EQ(DetermineFollowerNPCDirection(&player, &follower), DIR_EAST);
}

TEST("ResolveFollowerNPCCardinalDirection decomposes each diagonal to its horizontal component and passes cardinal input through")
{
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_NORTHEAST), DIR_EAST);
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_SOUTHEAST), DIR_EAST);
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_NORTHWEST), DIR_WEST);
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_SOUTHWEST), DIR_WEST);
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_NORTH), DIR_NORTH);
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_SOUTH), DIR_SOUTH);
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_WEST), DIR_WEST);
    EXPECT_EQ(ResolveFollowerNPCCardinalDirection(DIR_EAST), DIR_EAST);
}

TEST("ResolveFollowerNPCCardinalDirection never returns a diagonal direction, keeping cardinal-only table lookups like FollowerNPCHideMovementsSpeedTable in bounds")
{
    enum Direction directions[] = {DIR_SOUTH, DIR_NORTH, DIR_WEST, DIR_EAST, DIR_SOUTHWEST, DIR_SOUTHEAST, DIR_NORTHWEST, DIR_NORTHEAST};
    s32 i;

    for (i = 0; i < ARRAY_COUNT(directions); i++)
    {
        enum Direction resolved = ResolveFollowerNPCCardinalDirection(directions[i]);
        EXPECT(resolved >= DIR_SOUTH && resolved <= DIR_EAST);
    }
}

TEST("GetFollowerNPCDirectionalAction resolves a diagonal direction to the matching diagonal walk action for bases with diagonal sprite data")
{
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_WALK_NORMAL_DOWN, DIR_NORTHEAST), GetWalkNormalMovementAction(DIR_NORTHEAST));
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_WALK_SLOW_DOWN, DIR_SOUTHWEST), GetWalkSlowMovementAction(DIR_SOUTHWEST));
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_WALK_FAST_DOWN, DIR_NORTHWEST), GetWalkFastMovementAction(DIR_NORTHWEST));

    // Cardinal input is untouched - same linear offset as before this fix.
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_WALK_NORMAL_DOWN, DIR_EAST), MOVEMENT_ACTION_WALK_NORMAL_RIGHT);
}

TEST("GetFollowerNPCDirectionalAction decomposes a diagonal direction to a cardinal action instead of producing a wrong ledge-jump action")
{
    // Before this fix, DetermineFollowerNPCState's RETURN_STATE macro computed
    // state + (dir - 1) unconditionally. For a normal walk step (the common per-tile
    // follow path), MOVEMENT_ACTION_WALK_NORMAL_DOWN + (DIR_NORTHEAST - 1) landed on
    // MOVEMENT_ACTION_JUMP_2_RIGHT - a ledge-jump action - instead of a diagonal walk.
    u32 brokenResult = MOVEMENT_ACTION_WALK_NORMAL_DOWN + (DIR_NORTHEAST - 1);
    u32 fixedResult = GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_WALK_NORMAL_DOWN, DIR_NORTHEAST);

    EXPECT_EQ(brokenResult, MOVEMENT_ACTION_JUMP_2_RIGHT);
    EXPECT_NE(fixedResult, MOVEMENT_ACTION_JUMP_2_RIGHT);
    EXPECT_EQ(fixedResult, MOVEMENT_ACTION_WALK_NORMAL_DIAGONAL_UP_RIGHT);

    // The ledge-jump base itself has no diagonal sprite data - a diagonal direction there
    // must decompose to a valid cardinal jump action (horizontal component preferred, same
    // as ResolveFollowerNPCCardinalDirection), not read past the DOWN/UP/LEFT/RIGHT block
    // into an unrelated MOVEMENT_ACTION_* constant.
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_JUMP_2_DOWN, DIR_NORTHEAST), MOVEMENT_ACTION_JUMP_2_RIGHT);
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_JUMP_2_DOWN, DIR_SOUTHEAST), MOVEMENT_ACTION_JUMP_2_RIGHT);
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_JUMP_2_DOWN, DIR_NORTHWEST), MOVEMENT_ACTION_JUMP_2_LEFT);
    EXPECT_EQ(GetFollowerNPCDirectionalAction(MOVEMENT_ACTION_JUMP_2_DOWN, DIR_SOUTHWEST), MOVEMENT_ACTION_JUMP_2_LEFT);
}

TEST("DetermineFollowerNPCState resolves a diagonal direction to a diagonal walk action instead of a wrong ledge-jump action")
{
    struct ObjectEvent follower = {0};
    u8 savedObjectEventId = gPlayerAvatar.objectEventId;
    u32 result;

    SetUpTestMap();
    PlaceTestObjectEvent(&gObjectEvents[0], TEST_MAP_ORIGIN, TEST_MAP_ORIGIN);
    gPlayerAvatar.objectEventId = 0;
    PlaceTestObjectEvent(&follower, TEST_MAP_ORIGIN - 1, TEST_MAP_ORIGIN + 1);

    // Before the RETURN_STATE fix, this exact call - a normal walk step (the common
    // per-tile follow path reached via NPCFollow with state == MOVEMENT_ACTION_WALK_
    // NORMAL_DOWN) taking a diagonal direction - computed
    // MOVEMENT_ACTION_WALK_NORMAL_DOWN + (DIR_NORTHEAST - 1) and returned
    // MOVEMENT_ACTION_JUMP_2_RIGHT: a ledge-jump action instead of a diagonal walk.
    result = DetermineFollowerNPCState(&follower, MOVEMENT_ACTION_WALK_NORMAL_DOWN, DIR_NORTHEAST);

    EXPECT_NE(result, MOVEMENT_ACTION_JUMP_2_RIGHT);
    EXPECT_EQ(result, MOVEMENT_ACTION_WALK_NORMAL_DIAGONAL_UP_RIGHT);

    gPlayerAvatar.objectEventId = savedObjectEventId;
}
