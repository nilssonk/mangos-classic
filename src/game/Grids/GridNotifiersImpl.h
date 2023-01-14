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

#ifndef MANGOS_GRIDNOTIFIERSIMPL_H
#define MANGOS_GRIDNOTIFIERSIMPL_H

#include "Grids/GridNotifiers.h"
#include "Server/WorldPacket.h"
#include "Entities/Player.h"
#include "AI/BaseAI/UnitAI.h"
#include "Spells/SpellAuras.h"
#include "Server/DBCEnums.h"
#include "Server/SQLStorages.h"
#include "Spells/SpellMgr.h"

#include <memory>

template<class T>
inline void MaNGOS::VisibleNotifier::Visit(GridRefManager<T>& m)
{
    for (typename GridRefManager<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        camera_.UpdateVisibilityOf(iter->getSource(), data_, visibleNow_);
        clientGUIDs_.erase(iter->getSource()->GetObjectGuid());
    }
}

inline void MaNGOS::ObjectUpdater::Visit(CreatureMapType& m)
{
    for (auto& iter : m)
        m_objectToUpdateSet.emplace(iter.getSource());
}

inline void UnitVisitObjectsNotifierWorker(Unit* unitA, Unit* unitB)
{
    if (unitA->hasUnitState(UNIT_STAT_LOST_CONTROL) ||
        unitA->GetCombatManager().IsInEvadeMode() ||
        !unitA->AI()->IsVisible(unitB))
        return;

    unitA->AI()->MoveInLineOfSight(unitB);
}

inline void PlayerVisitCreatureWorker(Player* pl, Creature* c)
{
    if (!c->hasUnitState(UNIT_STAT_LOST_CONTROL))
    {
        if (c->AI() && c->AI()->IsVisible(pl) && !c->GetCombatManager().IsInEvadeMode())
            c->AI()->MoveInLineOfSight(pl);
    }

    if (!pl->hasUnitState(UNIT_STAT_LOST_CONTROL))
    {
        if (pl->AI() && pl->AI()->IsVisible(c) && !pl->GetCombatManager().IsInEvadeMode())
            pl->AI()->MoveInLineOfSight(c);
    }
}

inline void PlayerVisitPlayerWorker(Player* p1, Player* p2)
{
    if (!p2->hasUnitState(UNIT_STAT_LOST_CONTROL))
    {
        if (p2->AI() && p2->AI()->IsVisible(p1) && !p2->GetCombatManager().IsInEvadeMode())
            p2->AI()->MoveInLineOfSight(p1);
    }

    if (!p1->hasUnitState(UNIT_STAT_LOST_CONTROL))
    {
        if (p1->AI() && p1->AI()->IsVisible(p2) && !p1->GetCombatManager().IsInEvadeMode())
            p1->AI()->MoveInLineOfSight(p2);
    }
}

inline void CreatureVisitCreatureWorker(Creature* c1, Creature* c2)
{
    if (!c1->hasUnitState(UNIT_STAT_LOST_CONTROL))
    {
        if (c1->AI() && c1->AI()->IsVisible(c2) && !c1->GetCombatManager().IsInEvadeMode())
            c1->AI()->MoveInLineOfSight(c2);
    }

    if (!c2->hasUnitState(UNIT_STAT_LOST_CONTROL))
    {
        if (c2->AI() && c2->AI()->IsVisible(c1) && !c2->GetCombatManager().IsInEvadeMode())
            c2->AI()->MoveInLineOfSight(c1);
    }
}

template<>
inline void MaNGOS::PlayerVisitObjectsNotifier::Visit(CreatureMapType& m)
{
    if (!player_.IsAlive() || player_.IsTaxiFlying())
        return;

    bool playerHasAI = player_.AI() != nullptr;

    for (auto& iter : m)
    {
        Creature* creature = iter.getSource();
        if (!creature->IsAlive())
            continue;

        UnitVisitObjectsNotifierWorker(creature, &player_);

        if (playerHasAI)
            UnitVisitObjectsNotifierWorker(&player_, creature);
    }
}

template<>
inline void MaNGOS::PlayerVisitObjectsNotifier::Visit(PlayerMapType& m)
{
    if (!player_.IsAlive() || player_.IsTaxiFlying())
        return;

    bool playerHasAI = player_.AI() != nullptr;

    for (auto& iter : m)
    {
        Player* player = iter.getSource();
        if (player->IsAlive() && !player->IsTaxiFlying())
            continue;

        if (player->AI())
            UnitVisitObjectsNotifierWorker(player, &player_);

        if (playerHasAI)
            UnitVisitObjectsNotifierWorker(&player_, player);
    }
}

template<>
inline void MaNGOS::CreatureVisitObjectsNotifier::Visit(PlayerMapType& m)
{
    if (!creature_.IsAlive())
        return;

    for (auto& iter : m)
    {
        Player* player = iter.getSource();
        if (!player->IsAlive() || player->IsTaxiFlying())
            continue;

        if (player->AI())
            UnitVisitObjectsNotifierWorker(player, &creature_);

        UnitVisitObjectsNotifierWorker(&creature_, player);
    }
}

template<>
inline void MaNGOS::CreatureVisitObjectsNotifier::Visit(CreatureMapType& m)
{
    if (!creature_.IsAlive())
        return;

    for (auto& iter : m)
    {
        Creature* creature = iter.getSource();
        if (creature == &creature_ || !creature->IsAlive())
            continue;

        UnitVisitObjectsNotifierWorker(creature, &creature_);

        UnitVisitObjectsNotifierWorker(&creature_, creature);
    }
}

inline void MaNGOS::DynamicObjectUpdater::VisitHelper(Unit* target)
{
    if (!target->IsAlive() || target->IsTaxiFlying())
        return;

    if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsTotem())
        return;

    Unit* caster = dynobject_.GetCaster();
    float radius = dynobject_.GetRadius();
    if (caster->IsPlayerControlled() && !target->IsPlayerControlled())
        radius += target->GetCombatReach();
    if (dynobject_.GetDistance(target, true, DIST_CALC_NONE) > radius * radius)
        return;

    // Evade target
    if (target->GetCombatManager().IsInEvadeMode())
        return;

    // Check player targets and remove if in GM mode or GM invisibility (for not self casting case)
    if (target->GetTypeId() == TYPEID_PLAYER && target != check_ && (((Player*)target)->IsGameMaster() || ((Player*)target)->GetVisibility() == VISIBILITY_OFF))
        return;

    SpellEntry const* spellInfo = sSpellTemplate.LookupEntry<SpellEntry>(dynobject_.GetSpellId());
    SpellEffectIndex eff_index = dynobject_.GetEffIndex();

    SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(spellInfo->Id);
    if (bounds.first != bounds.second)
    {
        bool found = false;
        for (SQLMultiStorage::SQLMultiSIterator<SpellTargetEntry> spellST_ = bounds.first; spellST_ != bounds.second; ++spellST_)
        {
            if (spellST_->CanNotHitWithSpellEffect(eff_index))
                continue;

            // only creature entries supported for this target type
            if (spellST_->type == SPELL_TARGET_TYPE_GAMEOBJECT || spellST_->type == SPELL_TARGET_TYPE_GAMEOBJECT_GUID)
                continue;

            if (target->GetEntry() == spellST_->targetEntry)
            {
                if (spellST_->type == SPELL_TARGET_TYPE_DEAD && ((Creature*)target)->IsCorpse())
                    found = true;
                else if (spellST_->type == SPELL_TARGET_TYPE_CREATURE && target->IsAlive())
                    found = true;

                break;
            }
        }

        if (!found)
            return;
    }
    // This condition is only needed due to missing neutral spell type
    else if(dynobject_.GetTarget() != TARGET_ENUM_UNITS_SCRIPT_AOE_AT_DEST_LOC)
    {
        // for player casts use less strict negative and more stricted positive targeting
        if (positive_)
        {
            if (!caster->CanAssistSpell(target, spellInfo))
                return;
        }
        else
        {
            if (!caster->CanAttackSpell(target, spellInfo, true))
                return;
        }
    }

    if (spellInfo->HasAttribute(SPELL_ATTR_EX3_ONLY_ON_PLAYER) && target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Check target immune to spell or aura
    if (!spellInfo->HasAttribute(SPELL_ATTR_NO_IMMUNITIES)) // confirmed 40657 - Ancient Flames goes through immunity
        if (target->IsImmuneToSpell(spellInfo, false, (1 << eff_index), caster) || target->IsImmuneToSpellEffect(spellInfo, eff_index, false))
            return;

    if (!spellInfo->HasAttribute(SPELL_ATTR_EX2_IGNORE_LINE_OF_SIGHT) && !dynobject_.IsWithinLOSInMap(target))
        return;

    // Apply PersistentAreaAura on target
    // in case 2 dynobject overlap areas for same spell, same holder is selected, so dynobjects share holder
    SpellAuraHolder* holder = target->GetSpellAuraHolder(spellInfo->Id, dynobject_.GetCasterGuid());

    if (holder)
    {
        if (!holder->GetAuraByEffectIndex(eff_index))
        {
            PersistentAreaAura* Aur = new PersistentAreaAura(spellInfo, eff_index, &dynobject_.GetDamage(), &dynobject_.GetBasePoints(), holder, target, caster);
            holder->AddAura(Aur, eff_index);
            target->AddAuraToModList(Aur);
            Aur->ApplyModifier(true, true);
        }
        else if (holder->GetAuraDuration() >= 0 && uint32(holder->GetAuraDuration()) < dynobject_.GetDuration())
        {
            holder->SetAuraDuration(dynobject_.GetDuration());
            holder->UpdateAuraDuration();
        }
    }
    else
    {
        holder = CreateSpellAuraHolder(spellInfo, target, caster);
        PersistentAreaAura* Aur = new PersistentAreaAura(spellInfo, eff_index, &dynobject_.GetDamage(), &dynobject_.GetBasePoints(), holder, target, caster);
        holder->SetAuraDuration(dynobject_.GetDuration());
        holder->AddAura(Aur, eff_index);
        if (!target->AddSpellAuraHolder(holder))
            delete holder;
    }

    if (!dynobject_.IsAffecting(target))
    {
        dynobject_.AddAffected(target);
        caster->CasterHitTargetWithSpell(caster, target, spellInfo, false);
    }
}

template<>
inline void MaNGOS::DynamicObjectUpdater::Visit(CreatureMapType&  m)
{
    for (auto& itr : m)
        VisitHelper(itr.getSource());
}

template<>
inline void MaNGOS::DynamicObjectUpdater::Visit(PlayerMapType&  m)
{
    for (auto& itr : m)
        VisitHelper(itr.getSource());
}

// SEARCHERS & LIST SEARCHERS & WORKERS

// WorldObject searchers & workers

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(GameObjectMapType& m)
{
    // already found
    if (object_)
        return;

    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (object_)
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (object_)
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(CorpseMapType& m)
{
    // already found
    if (object_)
        return;

    for (CorpseMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectSearcher<Check>::Visit(DynamicObjectMapType& m)
{
    // already found
    if (object_)
        return;

    for (DynamicObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(CorpseMapType& m)
{
    for (CorpseMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(GameObjectMapType& m)
{
    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

template<class Check>
void MaNGOS::WorldObjectListSearcher<Check>::Visit(DynamicObjectMapType& m)
{
    for (DynamicObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

// Gameobject searchers

template<class Check>
void MaNGOS::GameObjectSearcher<Check>::Visit(GameObjectMapType& m)
{
    // already found
    if (object_)
        return;

    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::GameObjectLastSearcher<Check>::Visit(GameObjectMapType& m)
{
    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
            object_ = itr->getSource();
    }
}

template<class Check>
void MaNGOS::GameObjectListSearcher<Check>::Visit(GameObjectMapType& m)
{
    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

// Unit searchers

template<class Check>
void MaNGOS::UnitSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (object_)
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::UnitSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (object_)
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::UnitLastSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
            object_ = itr->getSource();
    }
}

template<class Check>
void MaNGOS::UnitLastSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
            object_ = itr->getSource();
    }
}

template<class Check>
void MaNGOS::UnitListSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

template<class Check>
void MaNGOS::UnitListSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

// Creature searchers

template<class Check>
void MaNGOS::CreatureSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (object_)
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::CreatureLastSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
            object_ = itr->getSource();
    }
}

template<class Check>
void MaNGOS::CreatureListSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

template<class Check>
void MaNGOS::PlayerSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (object_)
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (check_(itr->getSource()))
        {
            object_ = itr->getSource();
            return;
        }
    }
}

template<class Check>
void MaNGOS::PlayerListSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (check_(itr->getSource()))
            objects_.push_back(itr->getSource());
}

template<class Builder>
void MaNGOS::LocalizedPacketDo<Builder>::operator()(Player* p)
{
    int32 loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx + 1;

    // create if not cached yet
    if (data_cache_.size() < cache_idx + 1 || !data_cache_[cache_idx])
    {
        if (data_cache_.size() < cache_idx + 1)
            data_cache_.resize(cache_idx + 1);

        auto data = std::unique_ptr<WorldPacket>(new WorldPacket());

        builder_(*data, loc_idx);

        data_cache_[cache_idx] = std::move(data);
    }

    p->SendDirectMessage(*data_cache_[cache_idx]);
}

template<class Builder>
void MaNGOS::LocalizedPacketListDo<Builder>::operator()(Player* p)
{
    int32 loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx + 1;
    WorldPacketList* data_list;

    // create if not cached yet
    if (data_cache_.size() < cache_idx + 1 || data_cache_[cache_idx].empty())
    {
        if (data_cache_.size() < cache_idx + 1)
            data_cache_.resize(cache_idx + 1);

        data_list = &data_cache_[cache_idx];

        builder_(*data_list, loc_idx);
    }
    else
        data_list = &data_cache_[cache_idx];

    for (size_t i = 0; i < data_list->size(); ++i)
        p->SendDirectMessage(*(*data_list)[i]);
}

#endif                                                      // MANGOS_GRIDNOTIFIERSIMPL_H
