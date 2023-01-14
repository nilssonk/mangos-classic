/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_BFD_H
#define DEF_BFD_H

enum
{
    MAX_ENCOUNTER               = 3,
    MAX_FIRES                   = 4,
    MAX_COUNT_POS               = 3,

    TYPE_KELRIS                 = 1,
    TYPE_SHRINE                 = 2,
    TYPE_AQUANIS                = 3,

    NPC_KELRIS                  = 4832,
    NPC_BARON_AQUANIS           = 12876,

    // Shrine event
    NPC_AKUMAI_SERVANT          = 4978,
    NPC_AKUMAI_SNAPJAW          = 4825,
    NPC_BARBED_CRUSTACEAN       = 4823,
    NPC_MURKSHALLOW_SOFTSHELL   = 4977,

    GO_PORTAL_DOOR              = 21117,
    GO_SHRINE_1                 = 21118,
    GO_SHRINE_2                 = 21119,
    GO_SHRINE_3                 = 21120,
    GO_SHRINE_4                 = 21121,
    GO_FATHOM_STONE             = 177964,
};

class instance_blackfathom_deeps : public ScriptedInstance
{
    public:
        instance_blackfathom_deeps(Map* pMap);
        ~instance_blackfathom_deeps() {}

        void Initialize() override;

        void OnCreatureCreate(Creature* pCreature) override;
        void OnObjectCreate(GameObject* pGo) override;
        void OnCreatureDeath(Creature* pCreature) override;

        void Update(const uint32 diff) override;

        void SetData(uint32 uiType, uint32 uiData) override;
        uint32 GetData(uint32 uiType) const override;

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override;

    protected:
        void DoSpawnMobs(uint8 uiWaveIndex);
        bool IsWaveEventFinished() const;

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiSpawnMobsTimer[MAX_FIRES];
        uint8 m_uiWaveCounter;

        std::list<uint32> m_lWaveMobsGuids[MAX_FIRES];
};

#endif
