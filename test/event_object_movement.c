#include "global.h"
#include "event_object_movement.h"
#include "fieldmap.h"
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

extern u16 LoadSheetGraphicsInfo(const struct ObjectEventGraphicsInfo *info, u16 uuid, struct Sprite *sprite);

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
