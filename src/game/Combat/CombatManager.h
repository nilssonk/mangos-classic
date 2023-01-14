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

#ifndef COMBATMANAGER_H
#define COMBATMANAGER_H

#include "Common.h"
#include "Entities/Object.h"

#include <functional>

enum EvadeState
{
    EVADE_NONE,
    EVADE_COMBAT, // In a dungeon in combat evades all hits until target becomes reachable
    EVADE_HOME, // When running home evades all hits and disables some AI actions
};

class Unit;

class CombatManager
{
    public:
        using LeashingCheck = std::function<bool(Unit const&)>;
        using EvadeEffect = std::function<void(Unit&)>;
    
        CombatManager(Unit* owner);

        void Update(const uint32 diff);

        // evade handling
        bool IsInEvadeMode() const { return m_evadeTimer > 0 || m_evadeState; }
        bool IsEvadingHome() const { return m_evadeState == EVADE_HOME; }
        bool IsEvadeRegen() const { return (m_evadeTimer > 0 && m_evadeTimer <= 5000) || m_evadeState; } // Only regen after 5 seconds, or when in permanent evade
        void StartEvadeTimer() { m_evadeTimer = 10000; } // 10 seconds after which action is taken
        void StopEvade(); // Stops either timer or evade state
        EvadeState GetEvadeState() const { return m_evadeState; }
        void SetEvadeState(EvadeState state); // Propagated to pets
        void OnCombatStart();

        // combat timer handling
        uint32 GetCombatTimer() const { return m_combatTimer; }
        void TriggerCombatTimer(Unit* target);
        void TriggerCombatTimer(bool pvp);
        void TriggerCombatTimer(uint32 timer);
        void StopCombatTimer() { m_combatTimer = 0; }
        bool IsLeashingDisabled() { return m_leashingDisabled; }
        void SetLeashingDisable(bool apply) { m_leashingDisabled = apply; }
        void SetLeashingCheck(LeashingCheck&& leashingCheck, EvadeEffect&& evadeEffect = {}) {
            m_leashingCheck = std::move(leashingCheck);
            m_evadeEffect = std::move(evadeEffect);
        } // if check evals as true - evade and trigger effect
        void SetForcedCombat(bool state) { m_forcedCombat = state; }
    private:
        Unit* m_owner;

        // evade handling
        uint32 m_evadeTimer; // Used for evade during combat when mob is not running home and target isnt reachable
        EvadeState m_evadeState; // Used for evade during running home
        uint32 m_combatTick;

        // combat timer handling
        uint32 m_combatTimer;
        Position m_lastRefreshPos;
        bool m_leashingDisabled;                            // disables leashing timer for script mobs
        LeashingCheck m_leashingCheck;
        EvadeEffect m_evadeEffect;

        // vanilla bloodrage
        bool m_forcedCombat;
};

#endif
