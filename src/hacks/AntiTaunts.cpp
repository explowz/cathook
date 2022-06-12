/*
 * AntiTaunts.cpp
 *
 *  Created on: Jun 12, 2022
 *      Author: MrRealTime
 * 
 *  (Mostly Modified AntiDisguise.cpp)
 */

#include <settings/Bool.hpp>
#include "common.hpp"

namespace hacks::antitaunts
{
#if ENABLE_TEXTMODE
static settings::Boolean enable{ "remove.taunts", "true" };
#else
static settings::Boolean enable{ "remove.taunts", "false" };
#endif
static settings::Boolean no_invisibility{ "remove.cloak", "false" };

void cm()
{
	CachedEntity *ent;
	
    if (*no_invisibility)
    {
        return;
    }
	
    for (int i = 1; i <= g_IEngine->GetMaxClients(); i++)
    {
        ent = ENTITY(i);
        if (CE_BAD(ent) || ent == LOCAL_E || ent->m_Type() != ENTITY_PLAYER)
        {
            continue;
        }
		
        if (*enable)
        {
            RemoveCondition<TFCond_Taunting>(ent);
        }
    }
}
static InitRoutine EC(
    []()
    {
        EC::Register(EC::CreateMove, cm, "antitaunts", EC::average);
        EC::Register(EC::CreateMoveWarp, cm, "antitaunts_w", EC::average);
    });
} // namespace hacks::antidisguise
