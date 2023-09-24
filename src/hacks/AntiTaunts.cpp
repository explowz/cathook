//
// Created by Bintr
//

#include <settings/Bool.hpp>
#include "common.hpp"

namespace hacks::antitaunts
{
#if ENABLE_TEXTMODE
static settings::Boolean enable{ "remove.taunts", "true" };
#else
static settings::Boolean enable{ "remove.taunts", "false" };
#endif

static void CreateMove()
{
    if (!*enable)
    {
        return;
    }

    for (const auto &ent : entity_cache::player_cache)
    {
        if (RAW_ENT(ent)->IsDormant() || !HasCondition<TFCond_Taunting>(ent) || ent == LOCAL_E)
        {
            continue;
        }

        RemoveCondition<TFCond_Taunting>(ent);
    }
}

static InitRoutine EC(
    []()
    {
        EC::Register(EC::CreateMove, CreateMove, "CM_AntiTaunt");
        EC::Register(EC::CreateMoveWarp, CreateMove, "CMW_AntiTaunt");
    });
} // namespace hacks::antitaunts
