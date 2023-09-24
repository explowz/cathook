/*
 * entitycache.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <settings/Float.hpp>
#include "soundcache.hpp"

inline void CachedEntity::Update()
{
#ifndef PROXY_ENTITY
    m_pEntity = g_IEntityList->GetClientEntity(idx);
    if (!m_pEntity)
        return;
#endif
    hitboxes.InvalidateCache();
    m_bVisCheckComplete = false;
}

inline CachedEntity::CachedEntity(int idx) : m_IDX(idx), hitboxes(hitbox_cache::EntityHitboxCache{ idx })
{
#ifndef PROXY_ENTITY
    m_pEntity = nullptr;
#endif
}

inline CachedEntity::~CachedEntity()
{
    delete player_info;
    player_info = nullptr;
}

bool CachedEntity::IsVisible()
{
    PROF_SECTION(CE_IsVisible)
    if (m_bVisCheckComplete)
    {
        return m_bAnyHitboxVisible;
    }

    auto hitbox = hitboxes.GetHitbox(std::max(0, (hitboxes.GetNumHitboxes() >> 1) - 1));
    Vector result;
    if (!hitbox)
    {
        result = m_vecOrigin();
    }
    else
    {
        result = hitbox->center;
    }

    // Just check a centered hitbox. This is mostly used for ESP anyway
    if (IsEntityVectorVisible(this, result, true, MASK_SHOT_HULL, nullptr, true))
    {
        m_bAnyHitboxVisible = true;
        m_bVisCheckComplete = true;
        return true;
    }

    m_bAnyHitboxVisible = false;
    m_bVisCheckComplete = true;

    return false;
}

namespace entity_cache
{
std::unordered_map<int, CachedEntity> array;
std::vector<CachedEntity *> valid_ents;
std::vector<CachedEntity *> player_cache;
int previous_max = 0;
int previous_ent = 0;

void Update()
{
    max              = g_IEntityList->GetHighestEntityIndex();
    int current_ents = g_IEntityList->NumberOfEntities(false);
    valid_ents.clear();
    player_cache.clear();

    if (g_Settings.bInvalid)
    {
        return;
    }

    if (max >= MAX_ENTITIES)
    {
        max = MAX_ENTITIES - 1;
    }

    // pre-allocate memory
    valid_ents.reserve(max);
    player_cache.reserve(g_GlobalVars->maxClients);

    if (previous_max == max && previous_ent == current_ents)
    {
        for (auto &[key, val] : array)
        {
            val.Update();
            auto internal_entity = val.InternalEntity();
            if (internal_entity)
            {
                valid_ents.emplace_back(&val);
                auto val_type = val.m_Type();
                if (val_type == ENTITY_PLAYER || val_type == ENTITY_BUILDING || val_type == ENTITY_NPC)
                {
                    if (val.m_bAlivePlayer())
                    {
                        if (!internal_entity->IsDormant())
                        {
                            val.hitboxes.UpdateBones();
                        }

                        if (val_type == ENTITY_PLAYER)
                        {
                            player_cache.emplace_back(&val);
                        }
                    }
                }

                if (val_type == ENTITY_PLAYER)
                {
                    GetPlayerInfo(val.m_IDX, val.player_info);
                }
            }
        }
        previous_max = max;
        previous_ent = current_ents;
    }
    else
    {
        for (int i = 0; i <= max; ++i)
        {
            if (!g_IEntityList->GetClientEntity(i) || !g_IEntityList->GetClientEntity(i)->GetClientClass()->m_ClassID)
            {
                continue;
            }

            CachedEntity &ent = array.try_emplace(i, CachedEntity{ i }).first->second;
            ent.Update();
            auto internal_entity = ent.InternalEntity();
            if (internal_entity)
            {
                auto ent_type = ent.m_Type();
                valid_ents.emplace_back(&ent);
                if (ent_type == ENTITY_PLAYER || ent_type == ENTITY_BUILDING || ent_type == ENTITY_NPC)
                {
                    if (ent.m_bAlivePlayer())
                    {
                        if (!internal_entity->IsDormant())
                        {
                            ent.hitboxes.UpdateBones();
                        }

                        if (ent_type == ENTITY_PLAYER)
                        {
                            player_cache.emplace_back(&ent);
                        }
                    }
                }

                // Even dormant players have player info
                if (ent_type == ENTITY_PLAYER)
                {
                    if (!ent.player_info)
                    {
                        ent.player_info = new player_info_s;
                    }

                    GetPlayerInfo(ent.m_IDX, ent.player_info);
                }
            }
        }
        previous_max = max;
        previous_ent = current_ents;
    }
}

void Invalidate()
{
    array.clear();
}

void Shutdown()
{
    array.clear();
    previous_max = 0;
    max          = -1;
}

int max = 1;
} // namespace entity_cache
