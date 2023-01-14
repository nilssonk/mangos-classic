/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
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

#ifndef MANGOS_GRIDNOTIFIERS_H
#define MANGOS_GRIDNOTIFIERS_H

#include "Entities/UpdateData.h"

#include "Entities/Corpse.h"
#include "Entities/Object.h"
#include "Entities/DynamicObject.h"
#include "Entities/GameObject.h"
#include "Entities/Player.h"
#include "Entities/Unit.h"

#include <memory>

namespace MaNGOS
{
    struct VisibleNotifier
    {
        Camera& camera_;
        UpdateData data_;
        GuidSet clientGUIDs_;
        WorldObjectSet visibleNow_;

        explicit VisibleNotifier(Camera& c) : camera_(c), clientGUIDs_(c.GetOwner()->GetClientGuids()) {}
        template<class T> void Visit(GridRefManager<T>& m);
        void Visit(CameraMapType& /*m*/) {}
        void Notify(void);
    };

    struct VisibleChangesNotifier
    {
        WorldObject& object_;

        explicit VisibleChangesNotifier(WorldObject& object) : object_(object), m_unvisitedGuids(object_.GetClientGuidsIAmAt()) {}
        template<class T> void Visit(GridRefManager<T>&) {}
        void Visit(CameraMapType&);

        GuidSet& GetUnvisitedGuids() { return m_unvisitedGuids; }

        GuidSet m_unvisitedGuids;
    };

    struct MessageDeliverer
    {
        Player const& player_;
        WorldPacket const& message_;
        bool toSelf_;
        MessageDeliverer(Player const& pl, WorldPacket const& msg, bool to_self) : player_(pl), message_(msg), toSelf_(to_self) {}
        void Visit(CameraMapType& m);
        template<class SKIP> void Visit(GridRefManager<SKIP>&) {}
    };

    struct MessageDelivererExcept
    {
        WorldPacket const&  message_;
        Player const* skipped_receiver_;

        MessageDelivererExcept(WorldPacket const& msg, Player const* skipped)
            : message_(msg), skipped_receiver_(skipped) {}

        void Visit(CameraMapType& m);
        template<class SKIP> void Visit(GridRefManager<SKIP>&) {}
    };

    struct ObjectMessageDeliverer
    {
        WorldPacket const& message_;
        explicit ObjectMessageDeliverer(WorldPacket const& msg) : message_(msg) {}
        void Visit(CameraMapType& m);
        template<class SKIP> void Visit(GridRefManager<SKIP>&) {}
    };

    struct MessageDistDeliverer
    {
        Player const& player_;
        WorldPacket const& message_;
        bool toSelf_;
        bool ownTeamOnly_;
        float dist_;

        MessageDistDeliverer(Player const& pl, WorldPacket const& msg, float dist, bool to_self, bool ownTeamOnly)
            : player_(pl), message_(msg), toSelf_(to_self), ownTeamOnly_(ownTeamOnly), dist_(dist) {}
        void Visit(CameraMapType& m);
        template<class SKIP> void Visit(GridRefManager<SKIP>&) {}
    };

    struct ObjectMessageDistDeliverer
    {
        WorldObject const& object_;
        WorldPacket const& message_;
        float dist_;
        ObjectMessageDistDeliverer(WorldObject const& obj, WorldPacket const& msg, float dist) : object_(obj), message_(msg), dist_(dist) {}
        void Visit(CameraMapType& m);
        template<class SKIP> void Visit(GridRefManager<SKIP>&) {}
    };

    struct ObjectUpdater
    {
        ObjectUpdater(WorldObjectUnSet& otus, const uint32& diff) : m_objectToUpdateSet(otus), m_timeDiff(diff) {}
        template<class T> void Visit(GridRefManager<T>& m);
        void Visit(PlayerMapType&) {}
        void Visit(CorpseMapType&) {}
        void Visit(CameraMapType&) {}
        void Visit(CreatureMapType&);

        private:
            WorldObjectUnSet& m_objectToUpdateSet;
            uint32 m_timeDiff;
    };

    struct PlayerVisitObjectsNotifier
    {
        Player& player_;
        PlayerVisitObjectsNotifier(Player& pl) : player_(pl) {}
        template<class T> void Visit(GridRefManager<T>&) {}
#ifdef _MSC_VER
        template<> void Visit(PlayerMapType&);
        template<> void Visit(CreatureMapType&);
#endif
    };

    struct CreatureVisitObjectsNotifier
    {
        Creature& creature_;
        CreatureVisitObjectsNotifier(Creature& c) : creature_(c) {}
        template<class T> void Visit(GridRefManager<T>&) {}
#ifdef _MSC_VER
        template<> void Visit(PlayerMapType&);
        template<> void Visit(CreatureMapType&);
#endif
    };

    struct DynamicObjectUpdater
    {
        DynamicObject& dynobject_;
        Unit* check_;
        bool positive_;
        DynamicObjectUpdater(DynamicObject& dynobject, Unit* caster, bool positive) : dynobject_(dynobject), positive_(positive)
        {
            check_ = caster;
            Unit* owner = check_->GetOwner();
            if (owner)
                check_ = owner;
        }

        template<class T> inline void Visit(GridRefManager<T>&) {}
#ifdef _MSC_VER
        template<> inline void Visit<Player>(PlayerMapType&);
        template<> inline void Visit<Creature>(CreatureMapType&);
#endif

        void VisitHelper(Unit* target);
    };

    // SEARCHERS & LIST SEARCHERS & WORKERS

    /* Model Searcher class:
    template<class Check>
    struct SomeSearcher
    {
        ResultType& result_;
        Check & check_;

        SomeSearcher(ResultType& result, Check & check)
            : phaseMask_(check.GetFocusObject().GetPhaseMask()), result_(result), check_(check) {}

        void Visit(CreatureMapType &m);
        {
            ..some code fast return if result found

            for(CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                if (!itr->getSource()->InSamePhase(phaseMask_))
                    continue;

                if (!check_(itr->getSource()))
                    continue;

                ..some code for update result and possible stop search
            }
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED> &) {}
    };
    */

    // WorldObject searchers & workers

    template<class Check>
    struct WorldObjectSearcher
    {
        WorldObject*& object_;
        Check& check_;

        WorldObjectSearcher(WorldObject*& result, Check& check) : object_(result), check_(check) {}

        void Visit(GameObjectMapType& m);
        void Visit(PlayerMapType& m);
        void Visit(CreatureMapType& m);
        void Visit(CorpseMapType& m);
        void Visit(DynamicObjectMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Check>
    struct WorldObjectListSearcher
    {
        WorldObjectList& objects_;
        Check& check_;

        WorldObjectListSearcher(WorldObjectList& objects, Check& check) : objects_(objects), check_(check) {}

        void Visit(PlayerMapType& m);
        void Visit(CreatureMapType& m);
        void Visit(CorpseMapType& m);
        void Visit(GameObjectMapType& m);
        void Visit(DynamicObjectMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Do>
    struct WorldObjectWorker
    {
        Do const& do_;

        explicit WorldObjectWorker(Do const& _do) : do_(_do) {}

        void Visit(GameObjectMapType& m)
        {
            for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                do_(itr->getSource());
        }

        void Visit(PlayerMapType& m)
        {
            for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                do_(itr->getSource());
        }
        void Visit(CreatureMapType& m)
        {
            for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                do_(itr->getSource());
        }

        void Visit(CorpseMapType& m)
        {
            for (CorpseMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                do_(itr->getSource());
        }

        void Visit(DynamicObjectMapType& m)
        {
            for (DynamicObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                do_(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // Gameobject searchers

    template<class Check>
    struct GameObjectSearcher
    {
        GameObject*& object_;
        Check& check_;

        GameObjectSearcher(GameObject*& result, Check& check) : object_(result), check_(check) {}

        void Visit(GameObjectMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // Last accepted by Check GO if any (Check can change requirements at each call)
    template<class Check>
    struct GameObjectLastSearcher
    {
        GameObject*& object_;
        Check& check_;

        GameObjectLastSearcher(GameObject*& result, Check& check) : object_(result), check_(check) {}

        void Visit(GameObjectMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Check>
    struct GameObjectListSearcher
    {
        GameObjectList& objects_;
        Check& check_;

        GameObjectListSearcher(GameObjectList& objects, Check& check) : objects_(objects), check_(check) {}

        void Visit(GameObjectMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // Unit searchers

    // First accepted by Check Unit if any
    template<class Check>
    struct UnitSearcher
    {
        Unit*& object_;
        Check& check_;

        UnitSearcher(Unit*& result, Check& check) : object_(result), check_(check) {}

        void Visit(CreatureMapType& m);
        void Visit(PlayerMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // Last accepted by Check Unit if any (Check can change requirements at each call)
    template<class Check>
    struct UnitLastSearcher
    {
        Unit*& object_;
        Check& check_;

        UnitLastSearcher(Unit*& result, Check& check) : object_(result), check_(check) {}

        void Visit(CreatureMapType& m);
        void Visit(PlayerMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // All accepted by Check units if any
    template<class Check>
    struct UnitListSearcher
    {
        UnitList& objects_;
        Check& check_;

        UnitListSearcher(UnitList& objects, Check& check) : objects_(objects), check_(check) {}

        void Visit(PlayerMapType& m);
        void Visit(CreatureMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // Creature searchers

    template<class Check>
    struct CreatureSearcher
    {
        Creature*& object_;
        Check& check_;

        CreatureSearcher(Creature*& result, Check& check) : object_(result), check_(check) {}

        void Visit(CreatureMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // Last accepted by Check Creature if any (Check can change requirements at each call)
    template<class Check>
    struct CreatureLastSearcher
    {
        Creature*& object_;
        Check& check_;

        CreatureLastSearcher(Creature*& result, Check& check) : object_(result), check_(check) {}

        void Visit(CreatureMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Check>
    struct CreatureListSearcher
    {
        CreatureList& objects_;
        Check& check_;

        CreatureListSearcher(CreatureList& objects, Check& check) : objects_(objects), check_(check) {}

        void Visit(CreatureMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Do>
    struct CreatureWorker
    {
        Do& do_;

        CreatureWorker(WorldObject const* /*searcher*/, Do& _do) : do_(_do) {}

        void Visit(CreatureMapType& m)
        {
            for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                do_(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // Player searchers

    template<class Check>
    struct PlayerSearcher
    {
        Player*& object_;
        Check& check_;

        PlayerSearcher(Player*& result, Check& check) : object_(result), check_(check) {}

        void Visit(PlayerMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Check>
    struct PlayerListSearcher
    {
        PlayerList& objects_;
        Check& check_;

        PlayerListSearcher(PlayerList& objects, Check& check)
            : objects_(objects), check_(check) {}

        void Visit(PlayerMapType& m);

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Do>
    struct PlayerWorker
    {
        Do& do_;

        explicit PlayerWorker(Do& _do) : do_(_do) {}

        void Visit(PlayerMapType& m)
        {
            for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
                do_(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    template<class Do>
    struct CameraDistWorker
    {
        WorldObject const* searcher_;
        float dist_;
        Do& do_;

        CameraDistWorker(WorldObject const* searcher, float _dist, Do& _do)
            : searcher_(searcher), dist_(_dist), do_(_do) {}

        void Visit(CameraMapType& m)
        {
            for (CameraMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                Camera* camera = itr->getSource();
                if (camera->GetBody()->IsWithinDist(searcher_, dist_))
                    do_(camera->GetOwner());
            }
        }
        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    struct CameraDistLambdaWorker
    {
        WorldObject const* searcher_;
        float dist_;
        std::function<void(Player*)> const& do_;

        CameraDistLambdaWorker(WorldObject const* searcher, float _dist, std::function<void(Player*)> const& _do)
            : searcher_(searcher), dist_(_dist), do_(_do) {}

        void Visit(CameraMapType& m)
        {
            for (auto& itr : m)
            {
                Camera* camera = itr.getSource();
                if (camera->GetBody()->IsWithinDist(searcher_, dist_))
                    do_(camera->GetOwner());
            }
        }
        template<class NOT_INTERESTED> void Visit(GridRefManager<NOT_INTERESTED>&) {}
    };

    // CHECKS && DO classes

    /* Model Check class:
    class SomeCheck
    {
        public:
            SomeCheck(SomeObjecType const* fobj, ..some other args) : fobj_(fobj), ...other inits {}
            WorldObject const& GetFocusObject() const { return *fobj_; }
            bool operator()(Creature* u)                    and for other intresting typs (Player/GameObject/Camera
            {
                return ..(code return true if Object fit to requirenment);
            }
            template<class NOT_INTERESTED> bool operator()(NOT_INTERESTED*) { return false; }
        private:
            SomeObjecType const* fobj_;                    // Focus object used for check distance from, phase, so place in world
            ..other values need for check
    };
    */

    // Can only work for unit since WO -> Corpse reaction is undefined
    class CannibalizeObjectCheck
    {
        public:
            CannibalizeObjectCheck(Unit const* fobj, float range) : fobj_(fobj), range_(range) {}
            WorldObject const& GetFocusObject() const { return *fobj_; }
            bool operator()(Player* u)
            {
                if (fobj_->CanAssist(u) || u->IsAlive() || u->IsTaxiFlying())
                    return false;

                return fobj_->IsWithinDistInMap(u, range_);
            }
            bool operator()(Corpse* u);
            bool operator()(Creature* u)
            {
                if (fobj_->CanAssist(u) || u->IsAlive() || u->IsTaxiFlying() ||
                        (u->GetCreatureTypeMask() & CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD) == 0)
                    return false;

                return fobj_->IsWithinDistInMap(u, range_);
            }
            template<class NOT_INTERESTED> bool operator()(NOT_INTERESTED*) { return false; }
        private:
            Unit const* fobj_;
            float range_;
    };

    // WorldObject do classes

    class RespawnDo
    {
        public:
            RespawnDo() {}
            void operator()(Creature* u) const;
            void operator()(GameObject* u) const;
            void operator()(WorldObject*) const {}
            void operator()(Corpse*) const {}
    };

    // GameObject checks

    class GameObjectFocusCheck
    {
        public:
            GameObjectFocusCheck(WorldObject const* unit, uint32 focusId) : object_(unit), focusId_(focusId) {}
            WorldObject const& GetFocusObject() const { return *object_; }
            bool operator()(GameObject* go) const
            {
                GameObjectInfo const* goInfo = go->GetGOInfo();
                if (goInfo->type != GAMEOBJECT_TYPE_SPELL_FOCUS)
                    return false;

                if (!go->IsSpawned())
                    return false;

                if (goInfo->spellFocus.focusId != focusId_)
                    return false;

                float dist = (float)goInfo->spellFocus.dist;

                return go->IsWithinDistInMap(object_, dist);
            }
        private:
            WorldObject const* object_;
            uint32 focusId_;
    };

    // Find the nearest Fishing hole and return true only if source object is in range of hole
    class NearestGameObjectFishingHoleCheck
    {
        public:
            NearestGameObjectFishingHoleCheck(WorldObject const& obj, float range) : obj_(obj), range_(range) {}
            WorldObject const& GetFocusObject() const { return obj_; }
            bool operator()(GameObject* go)
            {
                if (go->GetGOInfo()->type == GAMEOBJECT_TYPE_FISHINGHOLE && go->IsSpawned() && obj_.IsWithinDistInMap(go, range_) && obj_.IsWithinDistInMap(go, (float)go->GetGOInfo()->fishinghole.radius))
                {
                    range_ = obj_.GetDistance(go, true, DIST_CALC_COMBAT_REACH);
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return range_; }
        private:
            WorldObject const& obj_;
            float  range_;

            // prevent clone
            NearestGameObjectFishingHoleCheck(NearestGameObjectFishingHoleCheck const&);
    };

    // Success at unit in range, range update for next check (this can be used with GameobjectLastSearcher to find nearest GO)
    class NearestGameObjectEntryInObjectRangeCheck
    {
        public:
            NearestGameObjectEntryInObjectRangeCheck(WorldObject const& obj, uint32 entry, float range) : obj_(obj), entry_(entry), range_(range) {}
            WorldObject const& GetFocusObject() const { return obj_; }
            bool operator()(GameObject* go)
            {
                if (go->GetEntry() == entry_ && obj_.IsWithinDistInMap(go, range_))
                {
                    range_ = obj_.GetDistance(go);        // use found GO range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return range_; }
        private:
            WorldObject const& obj_;
            uint32 entry_;
            float  range_;

            // prevent clone this object
            NearestGameObjectEntryInObjectRangeCheck(NearestGameObjectEntryInObjectRangeCheck const&);
    };

    // Success at go in range
    class AllGameObjectEntriesListInObjectRangeCheck
    {
        public:
            AllGameObjectEntriesListInObjectRangeCheck(WorldObject const& obj, std::set<uint32>& entries, float range, bool is3D = true) : obj_(obj), entries_(entries), range_(range), is_3D(is3D) {}
            WorldObject const& GetFocusObject() const { return obj_; }
            bool operator()(GameObject* go)
            {
                if (go->IsSpawned() && entries_.find(go->GetEntry()) != entries_.end() && obj_.IsWithinDistInMap(go, range_, is_3D))
                    return true;

                return false;
            }

            std::vector<uint32> ranges_;
        private:
            WorldObject const& obj_;
            std::set<uint32>& entries_;
            float  range_;
            bool   is_3D;

            // prevent clone this object
            AllGameObjectEntriesListInObjectRangeCheck(AllGameObjectEntriesListInObjectRangeCheck const&);
    };
    
    // combine with above somehow? fuck
    class AllGameObjectsMatchingOneEntryInRange
    {
        public:
            AllGameObjectsMatchingOneEntryInRange(WorldObject const* pObject, std::vector<uint32> const& entries, float fMaxRange)
                : m_pObject(pObject), entries(entries), m_fRange(fMaxRange) {}
            bool operator() (GameObject* pGo)
            {
                for (const auto entry : entries) {
                    if (pGo->GetEntry() == entry && m_pObject->IsWithinDist(pGo, m_fRange, false)) {
                        return true;
                    }
                }
                return false;
            }

        private:
            WorldObject const* m_pObject;
            std::vector<uint32> entries;
            float m_fRange;
    };

    // x y z version of above
    class AllGameObjectEntriesListInPosRangeCheck
    {
    public:
        AllGameObjectEntriesListInPosRangeCheck(Vec3 const& p_pos, std::set<uint32>& entries, float range, bool is3D = true) : pos(p_pos), entries_(entries), range_(range), is_3D(is3D) {}
        bool operator()(GameObject* go)
        {
            if (go->IsSpawned() && entries_.find(go->GetEntry()) != entries_.end() && go->GetDistance(pos, DIST_CALC_COMBAT_REACH) < range_)
                return true;

            return false;
        }

        std::vector<uint32> ranges_;
    private:
        Vec3 pos;
        std::set<uint32>& entries_;
        float  range_;
        bool   is_3D;

        // prevent clone this object
        AllGameObjectEntriesListInPosRangeCheck(AllGameObjectEntriesListInObjectRangeCheck const&);
    };

    // Success at gameobject in range of xyz, range update for next check (this can be use with GameobjectLastSearcher to find nearest GO)
    class NearestGameObjectEntryInPosRangeCheck
    {
        WorldObject const& obj_;
        uint32 entry_;
        Vec3 pos_;
        float range_;

    public:
        NearestGameObjectEntryInPosRangeCheck(WorldObject const& obj, uint32 entry, Vec3 const& pos, float range)
                : obj_(obj), entry_(entry), pos_(pos), range_(range) {}

        bool operator()(GameObject* go)
        {
            if (go->GetEntry() == entry_ && go->IsWithinDist(pos_, range_))
            {
                // use found GO range as new range limit for next check
                range_ = go->GetDistance(pos_);
                return true;
            }

            return false;
        }

        WorldObject const& GetFocusObject() const { return obj_; }
        float GetLastRange() const { return range_; }

        
        // prevent clone this object
        ~NearestGameObjectEntryInPosRangeCheck() = default;
        NearestGameObjectEntryInPosRangeCheck(NearestGameObjectEntryInPosRangeCheck const&) = delete;
        NearestGameObjectEntryInPosRangeCheck(NearestGameObjectEntryInPosRangeCheck &&) = delete;
        NearestGameObjectEntryInPosRangeCheck& operator=(NearestGameObjectEntryInPosRangeCheck const&) = delete;
        NearestGameObjectEntryInPosRangeCheck& operator=(NearestGameObjectEntryInPosRangeCheck&&) = delete;
    };

    // Success at gameobject with entry in range of provided xyz
    class GameObjectEntryInPosRangeCheck
    {
        WorldObject const& obj_;
        uint32 entry_;
        Vec3 pos_;
        float range_;

        public:
            GameObjectEntryInPosRangeCheck(WorldObject const& obj, uint32 entry, Vec3 const& pos, float range)
                : obj_(obj), entry_(entry), pos_(pos), range_(range) {}

            WorldObject const& GetFocusObject() const { return obj_; }

            bool operator()(GameObject* go)
            {
                return go->GetEntry() == entry_ && go->IsWithinDist(pos_, range_);
            }

            float GetLastRange() const { return range_; }

            // prevent clone this object
            ~GameObjectEntryInPosRangeCheck() = default;
            GameObjectEntryInPosRangeCheck(GameObjectEntryInPosRangeCheck const&) = delete;
            GameObjectEntryInPosRangeCheck(GameObjectEntryInPosRangeCheck &&) = delete;
            GameObjectEntryInPosRangeCheck& operator=(GameObjectEntryInPosRangeCheck const&) = delete;
            GameObjectEntryInPosRangeCheck& operator=(GameObjectEntryInPosRangeCheck&&) = delete;
    };

    class GameObjectInPosRangeCheck
    {
        WorldObject const& obj_;
        Vec3 pos_;
        float range_;

    public:
        GameObjectInPosRangeCheck(WorldObject const& obj, Vec3 const& pos, float range)
            : obj_(obj), pos_(pos), range_(range) {}

        WorldObject const& GetFocusObject() const { return obj_; }

        bool operator()(GameObject* go)
        {
            return go->IsWithinDist(pos_, range_);
        }

        float GetLastRange() const { return range_; }

        // prevent clone this object
        ~GameObjectInPosRangeCheck() = default;
        GameObjectInPosRangeCheck(GameObjectInPosRangeCheck const&) = delete;
        GameObjectInPosRangeCheck(GameObjectInPosRangeCheck &&) = delete;
        GameObjectInPosRangeCheck& operator=(GameObjectInPosRangeCheck const&) = delete;
        GameObjectInPosRangeCheck& operator=(GameObjectInPosRangeCheck&&) = delete;
    };

    // Unit checks

    class MostHPMissingInRangeCheck
    {
        public:
            MostHPMissingInRangeCheck(Unit const* obj, float range, float hp, bool onlyInCombat, bool targetSelf) : obj_(obj), range_(range), hp_(hp), onlyInCombat_(onlyInCombat), targetSelf_(targetSelf) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                if (!u->IsAlive() || (onlyInCombat_ && !u->IsInCombat()))
                    return false;

                if (!targetSelf_ && u == obj_)
                    return false;

                if (obj_->CanAssist(u) && obj_->IsWithinDistInMap(u, range_))
                {
                    if (u->GetMaxHealth() - u->GetHealth() > hp_)
                    {
                        hp_ = u->GetMaxHealth() - u->GetHealth();
                        return true;
                    }
                }
                return false;
            }
        private:
            Unit const* obj_;
            float range_;
            float hp_;
            bool onlyInCombat_;
            bool targetSelf_;
    };

    class MostHPPercentMissingInRangeCheck
    {
        public:
            MostHPPercentMissingInRangeCheck(Unit const* obj, float range, float hp, bool onlyInCombat, bool targetSelf) : obj_(obj), range_(range), hp_(hp), onlyInCombat_(onlyInCombat), targetSelf_(targetSelf) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                if (!u->IsAlive() || (onlyInCombat_ && !u->IsInCombat()))
                    return false;

                if (!targetSelf_ && u == obj_)
                    return false;

                if (obj_->CanAssist(u) && obj_->IsWithinDistInMap(u, range_))
                {
                    if (100.f - u->GetHealthPercent() > hp_)
                    {
                        hp_ = 100.f - u->GetHealthPercent();
                        return true;
                    }
                }
                return false;
            }
        private:
            Unit const* obj_;
            float range_;
            float hp_;
            bool onlyInCombat_;
            bool targetSelf_;
    };

    class FriendlyEligibleDispelInRangeCheck
    {
        public:
            FriendlyEligibleDispelInRangeCheck(Unit const* obj, float range, uint32 dispelMask, uint32 mechanicMask, bool self) :
                obj_(obj), range_(range), m_dispelMask(dispelMask), m_mechanicMask(mechanicMask), m_self(self) {}
            Unit const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                if (!u->IsAlive() || !u->IsInCombat() || !obj_->CanAssist(u) || !obj_->IsWithinDistInMap(u, range_))
                    return false;

                if (!m_self && obj_ == u)
                    return false;

                if (!u->IsImmobilizedState() && !u->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED) && !u->isFrozen() && !u->IsCrowdControlled())
                    return false;

                if (!m_dispelMask && !m_mechanicMask)
                    return true;

                return u->HasMechanicMaskOrDispelMaskAura(m_dispelMask, m_mechanicMask, obj_);
            }
        private:
            Unit const* obj_;
            float range_;
            uint32 m_dispelMask;
            uint32 m_mechanicMask;
            bool m_self;
    };

    class FriendlyMissingBuffInRangeInCombatCheck
    {
        public:
            FriendlyMissingBuffInRangeInCombatCheck(Unit const* obj, float range, uint32 spellid) : obj_(obj), range_(range), spell_(spellid) {}
            Unit const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                return u->IsAlive() && u->IsInCombat() && obj_->CanAssist(u) && obj_->IsWithinDistInMap(u, range_) && !(u->HasAura(spell_, EFFECT_INDEX_0) || u->HasAura(spell_, EFFECT_INDEX_1) || u->HasAura(spell_, EFFECT_INDEX_2));
            }
        private:
            Unit const* obj_;
            float range_;
            uint32 spell_;
    };

    class FriendlyMissingBuffInRangeNotInCombatCheck
    {
    public:
        FriendlyMissingBuffInRangeNotInCombatCheck(Unit const* obj, float range, uint32 spellid) : obj_(obj), range_(range), spell_(spellid) {}
        Unit const& GetFocusObject() const { return *obj_; }
        bool operator()(Unit* u)
        {
            return u->IsAlive() && obj_->CanAssist(u) && obj_->IsWithinDistInMap(u, range_) && !(u->HasAura(spell_, EFFECT_INDEX_0) || u->HasAura(spell_, EFFECT_INDEX_1) || u->HasAura(spell_, EFFECT_INDEX_2));
        }
    private:
        Unit const* obj_;
        float range_;
        uint32 spell_;
    };

    class AnyUnfriendlyUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyUnitInObjectRangeCheck(WorldObject const* obj, float range) : obj_(obj), range_(range)
            {
                controlledByPlayer_ = obj->IsControlledByPlayer();
            }
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u) const
            {
                // ignore totems
                if (u->GetTypeId() == TYPEID_UNIT && ((Creature*)u)->IsTotem())
                    return false;
                
                return u->IsAlive() && obj_->CanAttackSpell(u) && obj_->IsWithinDistInMap(u, range_) && obj_->IsWithinLOSInMap(u);
            }
        private:
            WorldObject const* obj_;
            bool controlledByPlayer_;
            float range_;
    };

    class AnyFriendlyUnitInObjectRangeCheck
    {
        public:
            AnyFriendlyUnitInObjectRangeCheck(WorldObject const* obj, SpellEntry const* spellInfo, float range) : obj_(obj), spellInfo_(spellInfo), range_(range) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                return u->IsAlive() && obj_->IsWithinDistInMap(u, range_) && obj_->CanAssistSpell(u, spellInfo_);
            }
        private:
            WorldObject const* obj_;
            SpellEntry const* spellInfo_;
            float range_;
    };

    class AnyFriendlyOrGroupMemberUnitInUnitRangeCheck
    {
        public:
            AnyFriendlyOrGroupMemberUnitInUnitRangeCheck(Unit const* obj, Unit const* target, Group const* group, SpellEntry const* spellInfo, float range)
                : group_(group), obj_(obj), target_(target), spellInfo_(spellInfo), range_(range) {}
            Unit const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                if (!u->IsAlive() || !target_->IsWithinDistInMap(u, range_) || !obj_->CanAssistSpell(u, spellInfo_))
                    return false;

                //if group is defined then we apply group members only filtering
                if (group_)
                {
                    switch (u->GetTypeId())
                    {
                        case TYPEID_PLAYER:
                        {
                            if (group_ != static_cast<Player*>(u)->GetGroup())
                                return false;

                            break;
                        }
                        case TYPEID_UNIT:
                        {
                            Creature* creature = static_cast<Creature*>(u);

                            //the only other possible group members besides players are pets, we exclude anyone else
                            if (!creature->IsPet())
                                return false;

                            Group* group = nullptr;

                            if (Unit* owner = creature->GetOwner(nullptr, true))
                            {
                                if (owner->GetTypeId() == TYPEID_PLAYER)
                                {
                                    group = static_cast<Player*>(owner)->GetGroup();
                                }
                            }

                            if (group_ != group)
                                return false;

                            break;
                        }
                        default: return false;
                    }
                }
                else if (u->IsCreature() && !static_cast<Creature*>(u)->IsPet())
                    return false;

                return true;
            }
        private:
            Group const* group_;
            Unit const* obj_;
            Unit const* target_;
            SpellEntry const* spellInfo_;
            float range_;
    };

    class AnyUnitInObjectRangeCheck
    {
        public:
            AnyUnitInObjectRangeCheck(WorldObject const* obj, float range) : obj_(obj), range_(range) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                return u->IsAlive() && obj_->IsWithinDistInMap(u, range_);
            }
        private:
            WorldObject const* obj_;
            float range_;
    };

    class AnyUnitFulfillingConditionInRangeCheck
    {
        public:
            AnyUnitFulfillingConditionInRangeCheck(WorldObject const* obj, std::function<bool(Unit*)> functor, float radius, DistanceCalculation type = DIST_CALC_COMBAT_REACH)
                    : obj_(obj), functor_(functor), range_(radius), type_(type) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                return functor_(u) && obj_->GetDistance(u, true, type_) <= range_;
            }
        private:
            WorldObject const* obj_;
            std::function<bool(Unit*)> functor_;
            float range_;
            DistanceCalculation type_;
    };

    // Success at unit in range, range update for next check (this can be use with UnitLastSearcher to find nearest unit)
    class NearestAttackableUnitInObjectRangeCheck
    {
        public:
            NearestAttackableUnitInObjectRangeCheck(Unit* source, Unit* owner, float range) : m_source(source), m_owner(owner), m_range(range) {}
            NearestAttackableUnitInObjectRangeCheck(NearestAttackableUnitInObjectRangeCheck const&) =  delete;

            Unit const& GetFocusObject() const { return *m_source; }

            bool operator()(Unit* currUnit)
            {
                if (currUnit->IsAlive() && (m_source->IsAttackedBy(currUnit) || (m_owner && m_owner->IsAttackedBy(currUnit)) || m_source->IsEnemy(currUnit) || currUnit->IsEnemy(m_source))
                    && m_source->CanAttack(currUnit)
                    && currUnit->IsVisibleForOrDetect(m_source, m_source, false)
                    && m_source->IsWithinDistInMap(currUnit, m_range))
                {
                    m_range = m_source->GetDistance(currUnit);        // use found unit range as new range limit for next check
                    return true;
                }

                return false;
            }

        private:
            Unit*       m_source;
            Unit*       m_owner;
            float       m_range;
    };

    class AnyAoETargetUnitInObjectRangeCheck
    {
        public:
            AnyAoETargetUnitInObjectRangeCheck(WorldObject const* obj, SpellEntry const* spellInfo, float range)
                : obj_(obj), spellInfo_(spellInfo), range_(range)
            {
                targetForPlayer_ = obj_->IsControlledByPlayer();
            }
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Unit* u)
            {
                // Check contains checks for: live, non-selectable, non-attackable flags, flight check and GM check, ignore totems
                if (u->GetTypeId() == TYPEID_UNIT && ((Creature*)u)->IsTotem())
                    return false;

                return obj_->CanAttackSpell(u, spellInfo_) && obj_->IsWithinDistInMap(u, range_);
            }

        private:
            WorldObject const* obj_;
            SpellEntry const* spellInfo_;
            float range_;
            bool targetForPlayer_;
    };

    // do attack at call of help to friendly crearture
    class CallOfHelpCreatureInRangeDo
    {
        public:
            CallOfHelpCreatureInRangeDo(Unit* funit, Unit* enemy, float range)
                : funit_(funit), enemy_(enemy), range_(range)
            {}
            void operator()(Creature* u);

        private:
            Unit* const funit_;
            Unit* const enemy_;
            float range_;
    };

    class AnyDeadUnitCheck
    {
        public:
            explicit AnyDeadUnitCheck(WorldObject const* fobj) : fobj_(fobj) {}
            WorldObject const& GetFocusObject() const { return *fobj_; }
            bool operator()(Unit* u) { return !u->IsAlive(); }
        private:
            WorldObject const* fobj_;
    };

    class AnyStealthedCheck
    {
        public:
            explicit AnyStealthedCheck(WorldObject const* fobj) : fobj_(fobj) {}
            WorldObject const& GetFocusObject() const { return *fobj_; }
            bool operator()(Unit* u) { return u->GetVisibility() == VISIBILITY_GROUP_STEALTH; }
        private:
            WorldObject const* fobj_;
    };

    // Creature checks

    class AnyAssistCreatureInRangeCheck
    {
        public:
            AnyAssistCreatureInRangeCheck(Unit* funit, Unit* enemy, float range)
                : funit_(funit), enemy_(enemy), range_(range)
            {
            }
            WorldObject const& GetFocusObject() const { return *funit_; }
            bool operator()(Creature* u);

        private:
            Unit* const funit_;
            Unit* const enemy_;
            float range_;
    };

    class NearestAssistCreatureInCreatureRangeCheck
    {
        public:
            NearestAssistCreatureInCreatureRangeCheck(Creature* obj, Unit* enemy, float range)
                : obj_(obj), enemy_(enemy), range_(range) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Creature* u)
            {
                if (u == obj_ || u->IsDead() || u->IsInCombat())
                    return false;

                if (!u->CanAssist(obj_) || !u->CanAttack(enemy_))
                    return false;

                if (!obj_->IsWithinDistInMap(u, range_))
                    return false;

                if (!obj_->IsWithinLOSInMap(u))
                    return false;

                range_ = obj_->GetDistance(u);            // use found unit range as new range limit for next check
                return true;
            }
            float GetLastRange() const { return range_; }

            // prevent clone this object
            NearestAssistCreatureInCreatureRangeCheck(NearestAssistCreatureInCreatureRangeCheck const&);

        private:
            Creature* const obj_;
            Unit* const enemy_;
            float  range_;
    };

    // Success at unit in range, range update for next check (this can be use with CreatureLastSearcher to find nearest creature)
    class NearestCreatureEntryWithLiveStateInObjectRangeCheck
    {
        public:
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(WorldObject const& obj, uint32 entry, bool onlyAlive, bool onlyDead, float range, bool excludeSelf = false, bool is3D = true)
                : obj_(obj), entry_(entry), range_(range * range), onlyAlive_(onlyAlive), onlyDead_(onlyDead), excludeSelf_(excludeSelf), is_3D(is3D), foundOutOfRange_(false) {}
            WorldObject const& GetFocusObject() const { return obj_; }
            bool operator()(Creature* u)
            {
                if (u->GetEntry() == entry_ && ((onlyAlive_ && u->IsAlive()) || (onlyDead_ && u->IsCorpse()) || (!onlyAlive_ && !onlyDead_)) && (!excludeSelf_ || (&obj_ != u)))
                {
                    float dist = obj_.GetDistance(u, true, DIST_CALC_NONE);
                    if (dist < range_)
                    {
                        range_ = dist; // use found unit range as new range limit for next check
                        return true;
                    }
                    foundOutOfRange_ = true;
                }
                return false;
            }
            float GetLastRange() const { return sqrt(range_); }
        private:
            WorldObject const& obj_;
            uint32 entry_;
            float  range_;
            bool   onlyAlive_;
            bool   onlyDead_;
            bool   excludeSelf_;
            bool   is_3D;
            bool   foundOutOfRange_;

            // prevent clone this object
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(NearestCreatureEntryWithLiveStateInObjectRangeCheck const&);
    };

    // Success at unit in range, range update for next check (this can be used with CreatureListSearcher to find creatures with given entry)
    class AllCreatureEntriesWithLiveStateInObjectRangeCheck
    {
        public:
            AllCreatureEntriesWithLiveStateInObjectRangeCheck(WorldObject const& obj, std::set<uint32>& entries, bool alive, float range, bool guids = false, bool excludeSelf = false, bool is3D = true)
                : obj_(obj), entries_(entries), range_(range), guids_(guids), alive_(alive), excludeSelf_(excludeSelf), is_3D(is3D), foundOutOfRange_(false) {}
            WorldObject const& GetFocusObject() const { return obj_; }
            bool operator()(Creature* u)
            {
                if (entries_.find(guids_ ? u->GetDbGuid() : u->GetEntry()) != entries_.end() && ((alive_ && u->IsAlive()) || (!alive_ && u->IsCorpse())) && (!excludeSelf_ || (&obj_ != u)))
                {
                    if (obj_.IsWithinCombatDistInMap(u, range_, is_3D))
                        return true;
                    foundOutOfRange_ = true;
                }
                return false;
            }
            bool FoundOutOfRange() const { return foundOutOfRange_; }
        private:
            WorldObject const& obj_;
            std::set<uint32>& entries_;
            float  range_;
            bool   guids_;
            bool   alive_;
            bool   excludeSelf_;
            bool   is_3D;
            bool   foundOutOfRange_;

            // prevent clone this object
            AllCreatureEntriesWithLiveStateInObjectRangeCheck(AllCreatureEntriesWithLiveStateInObjectRangeCheck const&);
    };

    class AllCreaturesOfEntryInRangeCheck
    {
        public:
            AllCreaturesOfEntryInRangeCheck(const WorldObject* pObject, uint32 uiEntry, float fMaxRange) : m_pObject(pObject), m_uiEntry(uiEntry), m_fRange(fMaxRange) {}
            WorldObject const& GetFocusObject() const { return *m_pObject; }
            bool operator()(Unit* pUnit)
            {
                return pUnit->GetEntry() == m_uiEntry && m_pObject->IsWithinDist(pUnit, m_fRange, false);
            }

            // prevent clone this object
            AllCreaturesOfEntryInRangeCheck(AllCreaturesOfEntryInRangeCheck const&);

        private:
            const WorldObject* m_pObject;
            uint32 m_uiEntry;
            float m_fRange;
    };

    class AllCreaturesMatchingOneEntryInRange
    {
        public:
            AllCreaturesMatchingOneEntryInRange(WorldObject const* pObject, std::vector<uint32> const& entries, float fMaxRange)
                : m_pObject(pObject), entries(entries), m_fRange(fMaxRange) {}
            bool operator() (Unit* pUnit)
            {
                for (const auto entry : entries) {
                    if (pUnit->GetEntry() == entry && m_pObject->IsWithinDist(pUnit, m_fRange, false)) {
                        return true;
                    }
                }
                return false;
            }

        private:
            WorldObject const* m_pObject;
            std::vector<uint32> entries;
            float m_fRange;
    };

    // Player checks and do

    class AnyPlayerInObjectRangeCheck
    {
        public:
            AnyPlayerInObjectRangeCheck(WorldObject const* obj, float range) : obj_(obj), range_(range) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Player* u)
            {
                return u->IsAlive() && obj_->IsWithinDistInMap(u, range_);
            }
        private:
            WorldObject const* obj_;
            float range_;
    };

    class AnyPlayerInObjectRangeWithAuraCheck
    {
        public:
            AnyPlayerInObjectRangeWithAuraCheck(WorldObject const* obj, float range, uint32 spellId)
                : obj_(obj), range_(range), spellId_(spellId) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Player* u)
            {
                return u->IsAlive()
                       && obj_->IsWithinDistInMap(u, range_)
                       && u->HasAura(spellId_);
            }
        private:
            WorldObject const* obj_;
            float range_;
            uint32 spellId_;
    };

    class AnyPlayerInCapturePointRange
    {
        public:
            AnyPlayerInCapturePointRange(WorldObject const* obj, float range)
                : obj_(obj), range_(range) {}
            WorldObject const& GetFocusObject() const { return *obj_; }
            bool operator()(Player* u)
            {
                return u->CanUseCapturePoint() &&
                       obj_->IsWithinDistInMap(u, range_);
            }
        private:
            WorldObject const* obj_;
            float range_;
    };

    // Prepare using Builder localized packets with caching and send to player
    template<class Builder>
    class LocalizedPacketDo
    {
        public:
            explicit LocalizedPacketDo(Builder& builder) : builder_(builder) {}

            void operator()(Player* p);

        private:
            Builder& builder_;
            std::vector<std::unique_ptr<WorldPacket>> data_cache_;         // 0 = default, i => i-1 locale index
    };

    // Prepare using Builder localized packets with caching and send to player
    template<class Builder>
    class LocalizedPacketListDo
    {
        public:
            typedef std::vector<std::unique_ptr<WorldPacket>> WorldPacketList;
            explicit LocalizedPacketListDo(Builder& builder) : builder_(builder) {}

            void operator()(Player* p);

        private:
            Builder& builder_;
            std::vector<WorldPacketList> data_cache_;
            // 0 = default, i => i-1 locale index
    };

#ifndef _MSC_VER
    template<> void PlayerVisitObjectsNotifier::Visit<Player>(PlayerMapType&);
    template<> void PlayerVisitObjectsNotifier::Visit<Creature>(CreatureMapType&);
    template<> void CreatureVisitObjectsNotifier::Visit<Player>(PlayerMapType&);
    template<> void CreatureVisitObjectsNotifier::Visit<Creature>(CreatureMapType&);
    template<> inline void DynamicObjectUpdater::Visit<Creature>(CreatureMapType&);
    template<> inline void DynamicObjectUpdater::Visit<Player>(PlayerMapType&);
#endif
}
#endif
