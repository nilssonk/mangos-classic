/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_MAR_H
#define DEF_MAR_H

enum
{
    TYPE_NOXXION                = 0,
    MAX_ENCOUNTER               = 1,

    NPC_NOXXION                 = 13282,
    NPC_SPEWED_LARVA            = 13533,

    GO_LARVA_SPEWER             = 178559,
    GO_CORRUPTION_SPEWER        = 178570,
};

class instance_maraudon : public ScriptedInstance
{
    public:
        instance_maraudon(Map* map);
        ~instance_maraudon() {}

        void Initialize() override;

        void OnObjectCreate(GameObject* go) override;
        void OnCreatureDeath(Creature* creature) override;
        void OnCreatureEvade(Creature* creature) override;

        void SetData(uint32 type, uint32 data) override;
        uint32 GetData(uint32 type) const override;

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override;

        void Update(const uint32 diff) override;

    protected:
        uint32 m_encounter[MAX_ENCOUNTER];
        uint32 m_spewLarvaTimer;
        std::string m_strInstData;
};


#endif
