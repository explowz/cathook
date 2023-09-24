/*
 * AntiDisguise.cpp
 *
 *  Created on: Nov 16, 2016
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"

namespace hacks::antidisguise
{
#if ENABLE_TEXTMODE
static settings::Boolean enable{ "remove.disguise", "true" };
#else
static settings::Boolean enable{ "remove.disguise", "false" };
#endif
static settings::Boolean no_invisibility{ "remove.cloak", "false" };

static void CreateMove()
{
    if (!*enable && !*no_invisibility)
    {
        return;
    }

    for (const auto &ent : entity_cache::player_cache)
    {
        if (RAW_ENT(ent)->IsDormant() || CE_INT(ent, netvar.iClass) != tf_class::tf_spy || ent == LOCAL_E)
        {
            continue;
        }

        if (HasCondition<TFCond_Disguised>(ent))
        {
            RemoveCondition<TFCond_Disguised>(ent);
        }

        if (*no_invisibility)
        {
            if (HasCondition<TFCond_Cloaked>(ent))
            {
                RemoveCondition<TFCond_Cloaked>(ent);
            }

            if (HasCondition<TFCond_CloakFlicker>(ent))
            {
                RemoveCondition<TFCond_CloakFlicker>(ent);
            }
        }
    }
}

static InitRoutine EC(
    []()
    {
        EC::Register(EC::CreateMove, CreateMove, "CM_AntiDisguise", EC::average);
        EC::Register(EC::CreateMoveWarp, CreateMove, "CMW_AntiDisguise", EC::average);
    });
} // namespace hacks::antidisguise
