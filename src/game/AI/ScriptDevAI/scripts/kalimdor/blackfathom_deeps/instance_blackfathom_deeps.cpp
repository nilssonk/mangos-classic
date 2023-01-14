/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Instance_Blackfathom_Deeps
SD%Complete: 50
SDComment: Quest support: 6921
SDCategory: Blackfathom Deeps
EndScriptData

*/

#include "AI/ScriptDevAI/include/sc_common.h"
#include "blackfathom_deeps.h"

namespace {

/* Encounter 0 = Twilight Lord Kelris
   Encounter 1 = Shrine event
   Must kill twilight lord for shrine event to be possible
   Encounter 2 = Baron Aquanis (spawned by GO use but should only spawn once per instance)
 */


/* This is the spawn pattern for the event mobs
*     D
* 0        3
* 1   S    4
* 2        5
*     E

* This event spawns 4 sets of mobs
* The order in whitch the fires are lit doesn't matter

* First:    3 Snapjaws:     Positions 0, 1, 5
* Second:   2 Servants:     Positions 1, 4
* Third:    4 Crabs:        Positions 0, 2, 3, 4
* Fourth:  10 Murkshallows: Positions 2*0, 1, 2*2; 3, 2*4, 2*5

* On wipe the mobs don't despawn; they stay there until player returns
*/

const Position aSpawnLocations[6] =                  // Should be near the correct positions
{
    { -768.949f, -174.413f, -25.87f, 3.09f},                // Left side
    { -768.888f, -164.238f, -25.87f, 3.09f},
    { -768.951f, -153.911f, -25.88f, 3.09f},
    { -867.782f, -174.352f, -25.87f, 6.27f},                // Right side
    { -867.875f, -164.089f, -25.87f, 6.27f},
    { -867.859f, -153.927f, -25.88f, 6.27f}
};

struct PosCount
{
    uint8 m_uiCount, m_uiSummonPosition;
};

struct SummonInformation
{
    uint8 m_uiWaveIndex;
    uint32 m_uiNpcEntry;
    PosCount m_aCountAndPos[MAX_COUNT_POS];
};

// ASSERT m_uiSummonPosition < 6 (see aSpawnLocations)
const SummonInformation aWaveSummonInformation[] =
{
    {0, NPC_AKUMAI_SNAPJAW,         {{1, 0}, {1, 1}, {1, 5}}},
    {1, NPC_AKUMAI_SERVANT,         {{1, 1}, {1, 4}, {0, 0}}},
    {2, NPC_BARBED_CRUSTACEAN,      {{1, 0}, {1, 2}, {0, 0}}},
    {2, NPC_BARBED_CRUSTACEAN,      {{1, 3}, {1, 4}, {0, 0}}},
    {3, NPC_MURKSHALLOW_SOFTSHELL,  {{2, 0}, {1, 1}, {2, 2}}},
    {3, NPC_MURKSHALLOW_SOFTSHELL,  {{1, 3}, {2, 4}, {2, 5}}}
};

const Position afAquanisPos{-782.21f, -63.26f, -42.43f, 2.36f};

} // anonymous namespace

instance_blackfathom_deeps::instance_blackfathom_deeps(Map* pMap) : ScriptedInstance(pMap),
    m_uiWaveCounter(0)
{
    Initialize();
}

void instance_blackfathom_deeps::Initialize()
{
    memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
    memset(&m_uiSpawnMobsTimer, 0, sizeof(m_uiSpawnMobsTimer));
}

void instance_blackfathom_deeps::OnCreatureCreate(Creature* pCreature)
{
    if (pCreature->GetEntry() == NPC_KELRIS)
        m_npcEntryGuidStore[NPC_KELRIS] = pCreature->GetObjectGuid();
}

void instance_blackfathom_deeps::OnObjectCreate(GameObject* pGo)
{
    switch (pGo->GetEntry())
    {
        case GO_PORTAL_DOOR:
            if (m_auiEncounter[1] == DONE)
                pGo->SetGoState(GO_STATE_ACTIVE);

            m_goEntryGuidStore[GO_PORTAL_DOOR] = pGo->GetObjectGuid();
            break;
        case GO_SHRINE_1:
        case GO_SHRINE_2:
        case GO_SHRINE_3:
        case GO_SHRINE_4:
            if (m_auiEncounter[1] == DONE)
                pGo->SetGoState(GO_STATE_ACTIVE);
            break;
    }
}

void instance_blackfathom_deeps::DoSpawnMobs(uint8 uiWaveIndex)
{
    Creature* pKelris = GetSingleCreatureFromStorage(NPC_KELRIS);
    if (!pKelris)
        return;

    auto const& respawn_pos = pKelris->GetRespawnPosition();

    for (auto i : aWaveSummonInformation)
    {
        if (i.m_uiWaveIndex != uiWaveIndex)
            continue;

        // Summon mobs at positions
        for (uint8 j = 0; j < MAX_COUNT_POS; ++j)
        {
            for (uint8 k = 0; k < i.m_aCountAndPos[j].m_uiCount; ++k)
            {
                uint8 uiPos = i.m_aCountAndPos[j].m_uiSummonPosition;
                auto tmp_pos = aSpawnLocations[uiPos];

                // Adapt fPosY slightly in case of higher summon-counts
                if (i.m_aCountAndPos[j].m_uiCount > 1)
                    tmp_pos.y = tmp_pos.y - INTERACTION_DISTANCE / 2 + k * INTERACTION_DISTANCE / i.m_aCountAndPos[j].m_uiCount;

                if (Creature* pSummoned = pKelris->SummonCreature(i.m_uiNpcEntry, tmp_pos, TempSpawnType::DEAD_DESPAWN, 0))
                {
                    pSummoned->GetMotionMaster()->MovePoint(0, respawn_pos);
                    m_lWaveMobsGuids[uiWaveIndex].push_back(pSummoned->GetGUIDLow());
                }
            }
        }
    }
}

void instance_blackfathom_deeps::SetData(uint32 uiType, uint32 uiData)
{
    switch (uiType)
    {
        case TYPE_KELRIS:                                   // EventAI must set instance data (1,3) at his death
            if (m_auiEncounter[0] != DONE && uiData == DONE)
                m_auiEncounter[0] = uiData;
            break;
        case TYPE_SHRINE:
            m_auiEncounter[1] = uiData;
            if (uiData == IN_PROGRESS)
            {
                m_uiSpawnMobsTimer[m_uiWaveCounter] = 3000;
                ++m_uiWaveCounter;
            }
            else if (uiData == DONE)
                DoUseDoorOrButton(GO_PORTAL_DOOR);
            break;
        case TYPE_AQUANIS:
            m_auiEncounter[2] = uiData;
            break;
    }

    if (uiData == DONE)
    {
        OUT_SAVE_INST_DATA;

        std::ostringstream saveStream;

        saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2];

        m_strInstData = saveStream.str();
        SaveToDB();
        OUT_SAVE_INST_DATA_COMPLETE;
    }
}

uint32 instance_blackfathom_deeps::GetData(uint32 uiType) const
{
    switch (uiType)
    {
        case TYPE_KELRIS: return m_auiEncounter[0];
        case TYPE_SHRINE: return m_auiEncounter[1];
        case TYPE_AQUANIS: return m_auiEncounter[2];
        default:
            return 0;
    }
}

void instance_blackfathom_deeps::Load(const char* chrIn)
{
    if (!chrIn)
    {
        OUT_LOAD_INST_DATA_FAIL;
        return;
    }

    OUT_LOAD_INST_DATA(chrIn);

    std::istringstream loadStream(chrIn);
    loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2];

    for (uint32& i : m_auiEncounter)
    {
        if (i == IN_PROGRESS)
            i = NOT_STARTED;
    }

    OUT_LOAD_INST_DATA_COMPLETE;
}

void instance_blackfathom_deeps::OnCreatureDeath(Creature* pCreature)
{
    if (pCreature->GetEntry() == NPC_BARON_AQUANIS)
        SetData(TYPE_AQUANIS, DONE);

    // Only use this function if shrine event is in progress
    if (m_auiEncounter[1] != IN_PROGRESS)
        return;

    switch (pCreature->GetEntry())
    {
        case NPC_AKUMAI_SERVANT:
            m_lWaveMobsGuids[1].remove(pCreature->GetGUIDLow());
            break;
        case NPC_AKUMAI_SNAPJAW:
            m_lWaveMobsGuids[0].remove(pCreature->GetGUIDLow());
            break;
        case NPC_BARBED_CRUSTACEAN:
            m_lWaveMobsGuids[2].remove(pCreature->GetGUIDLow());
            break;
        case NPC_MURKSHALLOW_SOFTSHELL:
            m_lWaveMobsGuids[3].remove(pCreature->GetGUIDLow());
            break;
    }

    if (IsWaveEventFinished())
        SetData(TYPE_SHRINE, DONE);
}

// Check if all the summoned event mobs are dead
bool instance_blackfathom_deeps::IsWaveEventFinished() const
{
    // If not all fires are lighted return
    if (m_uiWaveCounter < MAX_FIRES)
        return false;

    // Check if all mobs are dead
    for (const auto& m_lWaveMobsGuid : m_lWaveMobsGuids)
    {
        if (!m_lWaveMobsGuid.empty())
            return false;
    }

    return true;
}

void instance_blackfathom_deeps::Update(uint32 uiDiff)
{
    // Only use this function if shrine event is in progress
    if (m_auiEncounter[1] != IN_PROGRESS)
        return;

    for (uint8 i = 0; i < MAX_FIRES; ++i)
    {
        if (m_uiSpawnMobsTimer[i])
        {
            if (m_uiSpawnMobsTimer[i] <= uiDiff)
            {
                DoSpawnMobs(i);
                m_uiSpawnMobsTimer[i] = 0;
            }
            else
                m_uiSpawnMobsTimer[i] -= uiDiff;
        }
    }
}

InstanceData* GetInstanceData_instance_blackfathom_deeps(Map* pMap)
{
    return new instance_blackfathom_deeps(pMap);
}

bool GOUse_go_fire_of_akumai(Player* /*pPlayer*/, GameObject* pGo)
{
    instance_blackfathom_deeps* pInstance = (instance_blackfathom_deeps*)pGo->GetInstanceData();

    if (!pInstance)
        return true;

    if (pInstance->GetData(TYPE_SHRINE) == DONE)
        return true;

    if (pInstance->GetData(TYPE_KELRIS) == DONE)
    {
        pInstance->SetData(TYPE_SHRINE, IN_PROGRESS);
        return false;
    }

    return true;
}

bool GOUse_go_fathom_stone(Player* pPlayer, GameObject* pGo)
{
    instance_blackfathom_deeps* pInstance = (instance_blackfathom_deeps*)pGo->GetInstanceData();
    if (!pInstance)
        return true;

    if (pInstance->GetData(TYPE_AQUANIS) == NOT_STARTED)
    {
        pPlayer->SummonCreature(NPC_BARON_AQUANIS, afAquanisPos, TempSpawnType::DEAD_DESPAWN, 0);
        pInstance->SetData(TYPE_AQUANIS, IN_PROGRESS);
    }

    return false;
}

void AddSC_instance_blackfathom_deeps()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "instance_blackfathom_deeps";
    pNewScript->GetInstanceData = &GetInstanceData_instance_blackfathom_deeps;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "go_fire_of_akumai";
    pNewScript->pGOUse = &GOUse_go_fire_of_akumai;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "go_fathom_stone";
    pNewScript->pGOUse = &GOUse_go_fathom_stone;
    pNewScript->RegisterSelf();
}
