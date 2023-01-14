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

#ifndef MANGOS_WAYPOINTMANAGER_H
#define MANGOS_WAYPOINTMANAGER_H

#include "Common.h"

enum class WaypointPathOrigin
{
    NO_PATH                = 0,
    FROM_GUID              = 1,
    FROM_ENTRY             = 2,
    FROM_EXTERNAL          = 3,
    FROM_WAYPOINT_PATH     = 4,
};

struct WaypointNode
{
    Position pos{};
    uint32 delay{};
    uint32 script_id{};                                       // Added may 2010. WaypointBehavior w/DB data should in time be removed.
    WaypointNode(Position const& p_pos, uint32 p_delay, uint32 p_script_id)
        : pos{p_pos}, delay(p_delay), script_id(p_script_id) {}
};

typedef std::map<uint32 /*pointId*/, WaypointNode> WaypointPath;

class WaypointManager
{
    public:
        WaypointManager() : m_externalTable("external.waypointTable") {}

        void Load();

        // We may get a path for several occasions:

        // 1: When creature.MovementType=2
        //    1a) Path is selected by creature.guid == creature_movement.id
        //    1b) Path for 1a) does not exist and then use path from creature.GetEntry() == creature_movement_template.entry

        // 2: When creature_template.MovementType=2
        //    2a) Creature is summoned and has creature_template.MovementType=2
        //        Creators need to be sure that creature_movement_template is always valid for summons.
        //        Mob that can be summoned anywhere should not have creature_movement_template for example.

        WaypointPath* GetDefaultPath(uint32 entry, uint32 lowGuid, WaypointPathOrigin* wpOrigin = nullptr)
        {
            using WPO = WaypointPathOrigin;

            WaypointPath* path = GetPath(lowGuid);
            if (path && wpOrigin)
                *wpOrigin = WPO::FROM_GUID;

            // No movement found for guid
            if (!path)
            {
                path = GetPathTemplate(entry);
                if (path && wpOrigin)
                    *wpOrigin = WPO::FROM_ENTRY;

                if (!path)
                {
                    // check movement_template
                    path = GetPathMovementTemplate(entry);
                    if (path)
                        *wpOrigin = WPO::FROM_WAYPOINT_PATH;
                }
            }

            return path;
        }

        // Helper function to get a path provided the required information
        WaypointPath* GetPathFromOrigin(uint32 entry, uint32 lowGuid, int32 pathId, WaypointPathOrigin wpOrigin)
        {
            using WPO = WaypointPathOrigin;

            WaypointPathMap* wpMap;
            uint32 key;

            switch (wpOrigin)
            {
                case WPO::FROM_GUID:
                    key = lowGuid;
                    wpMap = &m_pathMap;
                    break;
                case WPO::FROM_ENTRY:
                    if (pathId >= 0xFF || pathId < 0)
                        return nullptr;
                    key = (entry << 8) + pathId;
                    wpMap = &m_pathTemplateMap;
                    break;
                case WPO::FROM_EXTERNAL:
                    if (pathId >= 0xFF || pathId < 0)
                        return nullptr;
                    key = (entry << 8) + pathId;
                    wpMap = &m_externalPathTemplateMap;
                    break;
                case WPO::FROM_WAYPOINT_PATH:
                    key = pathId;
                    wpMap = &m_pathMovementTemplateMap;
                    break;
                case WPO::NO_PATH:
                default:
                    return nullptr;
            }
            WaypointPathMap::iterator find = wpMap->find(key);
            return find != wpMap->end() ? &find->second : nullptr;
        }

        void DeletePath(uint32 id);

        /// Set external source table
        void SetExternalWPTable(char const* tableName) { m_externalTable = std::string(tableName); }
        std::string GetExternalWPTable() const { return m_externalTable; }
        /// Add Nodes from external sources
        bool AddExternalNode(uint32 entry, int32 pathId, uint32 pointId, Position const& pos, uint32 waittime, uint32 scriptId);

        // Toolbox for .wp add command
        /// Add a node as position pointId. If pointId == 0 then as last point
        WaypointNode const* AddNode(uint32 entry, uint32 dbGuid, uint32 pathId, uint32& pointId, WaypointPathOrigin wpDest, Position const& pos);

        // Toolbox for .wp modify command
        void DeleteNode(uint32 entry, uint32 dbGuid, uint32 point, uint32 pathId, WaypointPathOrigin wpOrigin);
        void SetNodePosition(uint32 entry, uint32 dbGuid, uint32 point, uint32 pathId, WaypointPathOrigin wpOrigin, Vec3 const& pos);
        void SetNodeWaittime(uint32 entry, uint32 dbGuid, uint32 point, uint32 pathId, WaypointPathOrigin wpOrigin, uint32 waittime);
        void SetNodeOrientation(uint32 entry, uint32 dbGuid, uint32 point, uint32 pathId, WaypointPathOrigin wpOrigin, float orientation);
        bool SetNodeScriptId(uint32 entry, uint32 dbGuid, uint32 point, uint32 pathId, WaypointPathOrigin wpOrigin, uint32 scriptId);

        // Small Helper for nice output
        static std::string GetOriginString(WaypointPathOrigin origin)
        {
            using WPO = WaypointPathOrigin;
            switch (origin)
            {
                case WPO::NO_PATH:          return "<no path>";
                case WPO::FROM_GUID:        return "guid";
                case WPO::FROM_ENTRY:       return "entry";
                case WPO::FROM_EXTERNAL:    return "external";
                default:                    return "invalid origin";
            }
        }

    private:
        WaypointPath* GetPath(uint32 id)
        {
            WaypointPathMap::iterator itr = m_pathMap.find(id);
            return itr != m_pathMap.end() ? &itr->second : nullptr;
        }

        WaypointPath* GetPathTemplate(uint32 entry)
        {
            WaypointPathMap::iterator itr = m_pathTemplateMap.find((entry << 8) /*+ pathId*/);
            return itr != m_pathTemplateMap.end() ? &itr->second : nullptr;
        }

        WaypointPath* GetPathMovementTemplate(uint32 entry)
        {
            WaypointPathMap::iterator itr = m_pathMovementTemplateMap.find((entry << 8) /*+ pathId*/);
            return itr != m_pathMovementTemplateMap.end() ? &itr->second : nullptr;
        }

        typedef std::unordered_map<uint32 /*guidOrEntry*/, WaypointPath> WaypointPathMap;

        WaypointPathMap* getMapForPathType(WaypointPathOrigin origin)
        {
            using WPO = WaypointPathOrigin;
            switch (origin)
            {
                case WPO::NO_PATH                   : return nullptr;
                case WPO::FROM_GUID                 : return &m_pathMap;
                case WPO::FROM_ENTRY                : return &m_pathTemplateMap;
                case WPO::FROM_EXTERNAL             : return &m_externalPathTemplateMap;
                case WPO::FROM_WAYPOINT_PATH    : return &m_pathMovementTemplateMap;
                default:                    return nullptr;
            }
        }

        WaypointPathMap m_pathMap;
        WaypointPathMap m_pathTemplateMap;
        WaypointPathMap m_externalPathTemplateMap;
        WaypointPathMap m_pathMovementTemplateMap;

        std::string m_externalTable;
};

#define sWaypointMgr MaNGOS::Singleton<WaypointManager>::Instance()

/// Accessor for Scripting library
bool AddWaypointFromExternal(uint32 entry, uint32 pathId, uint32 pointId, Position const& pos, uint32 waittime);

#endif
