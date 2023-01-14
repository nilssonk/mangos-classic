#ifndef TEMPSPAWNSETTINGS_H
#define TEMPSPAWNSETTINGS_H

#include "Common.h"
#include "Entities/ObjectGuid.h"
#include "Entities/TempSpawnEnums.h"

#include <cstdint>

class WorldObject;

struct TempSpawnSettings
{
    WorldObject* spawner = nullptr;
    uint32_t entry;
    Position pos;
    TempSpawnType spawnType;
    uint32_t despawnTime = 0;
    uint32_t corpseDespawnTime = 0;
    bool activeObject = false;
    bool setRun = false;
    uint32_t pathId = 0;
    uint32_t faction = 0;
    uint32_t modelId = 0;
    bool spawnCounting = false;
    bool forcedOnTop = false;
    uint32_t spellId = 0;
    ObjectGuid ownerGuid;
    uint32_t spawnDataEntry = 0;
    int32 movegen = -1;
    WorldObject* dbscriptTarget = nullptr;
    uint32_t level = 0;

    // TemporarySpawnWaypoint subsystem
    bool tempSpawnMovegen = false;
    uint32_t waypointId = 0;
    int32 spawnPathId = 0;
    uint32_t pathOrigin = 0;

    TempSpawnSettings() {}
    TempSpawnSettings(WorldObject* spawner, uint32_t entry, Position const& pos, TempSpawnType spawnType, uint32_t despawnTime, bool activeObject = false, bool setRun = false, uint32_t pathId = 0, uint32_t faction = 0,
        uint32_t modelId = 0, bool spawnCounting = false, bool forcedOnTop = false, uint32_t spellId = 0, int32 movegen = -1, uint32_t level = 0) :
        spawner(spawner), entry(entry), pos(pos), spawnType(spawnType), despawnTime(despawnTime), activeObject(activeObject), setRun(setRun), pathId(pathId), faction(faction), modelId(modelId), spawnCounting(spawnCounting),
        forcedOnTop(forcedOnTop), spellId(spellId), movegen(movegen), level(level)
    {}
};


#endif
