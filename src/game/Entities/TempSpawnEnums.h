#ifndef TEMPSPAWNENUMS_H
#define TEMPSPAWNENUMS_H

enum class TempSpawnType
{
    MANUAL_DESPAWN              = 0,             // despawns when UnSummon() is called
    DEAD_DESPAWN                = 1,             // despawns when the creature disappears
    CORPSE_DESPAWN              = 2,             // despawns instantly after death
    CORPSE_TIMED_DESPAWN        = 3,             // despawns after a specified time after death (or when the creature disappears)
    TIMED_DESPAWN               = 4,             // despawns after a specified time
    TIMED_OOC_DESPAWN           = 5,             // despawns after a specified time after the creature is out of combat
    TIMED_OR_DEAD_DESPAWN       = 6,             // despawns after a specified time OR when the creature disappears
    TIMED_OR_CORPSE_DESPAWN     = 7,             // despawns after a specified time OR when the creature dies
    TIMED_OOC_OR_DEAD_DESPAWN   = 8,             // despawns after a specified time (OOC) OR when the creature disappears
    TIMED_OOC_OR_CORPSE_DESPAWN = 9,             // despawns after a specified time (OOC) OR when the creature dies
};

enum class TempSpawnLinkedAura
{
    LINKED_AURA_OWNER_CHECK = 0x00000001,
    LINKED_AURA_REMOVE_OWNER = 0x00000002
};

#endif
