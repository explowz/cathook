/*
 * playerresource.cpp
 *
 *  Created on: Nov 13, 2016
 *      Author: nullifiedcat
 */

#include <playerresource.hpp>

#include "common.hpp"

void TFPlayerResource::Update()
{
    entity = 0;
    for (const auto &ent_not_raw : entity_cache::valid_ents)
    {
        auto ent = RAW_ENT(ent_not_raw);
        if (ent->GetClientClass()->m_ClassID == RCC_PLAYERRESOURCE)
        {
            entity = ent_not_raw->m_IDX;
            return;
        }
    }
}

int TFPlayerResource::GetHealth(CachedEntity *player) const
{
    IClientEntity *ent;
    int idx;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE || !player)
        return 0;
    idx = player->m_IDX;
    if (idx >= 64 || idx < 0)
        return 0;
    return *(int *) ((unsigned) ent + netvar.m_iHealth_Resource + 4 * idx);
}

int TFPlayerResource::GetMaxHealth(CachedEntity *player) const
{
    IClientEntity *ent;
    int idx;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE || !player)
        return 0;
    idx = player->m_IDX;
    if (idx >= 64 || idx < 0)
        return 0;
    return *(int *) ((unsigned) ent + netvar.res_iMaxHealth + 4 * idx);
}

int TFPlayerResource::GetMaxBuffedHealth(CachedEntity *player) const
{
    IClientEntity *ent;
    int idx;

    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE || !player)
        return 0;
    idx = player->m_IDX;
    if (idx >= 64 || idx < 0)
        return 0;
    return *(int *) ((unsigned) ent + netvar.res_iMaxBuffedHealth + 4 * idx);
}

int TFPlayerResource::GetTeam(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 0)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(int *) ((unsigned) ent + netvar.res_iTeam + 4 * idx);
}

int TFPlayerResource::GetScore(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 1)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(int *) ((unsigned) ent + netvar.m_iTotalScore_Resource + 4 * idx);
}

int TFPlayerResource::GetKills(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 1)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(int *) ((unsigned) ent + netvar.m_iKills_Resource + 4 * idx);
}

int TFPlayerResource::GetDeaths(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 1)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(int *) ((unsigned) ent + netvar.m_iDeaths_Resource + 4 * idx);
}

int TFPlayerResource::GetLevel(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 1)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(int *) ((unsigned) ent + netvar.m_iPlayerLevel_Resource + 4 * idx);
}

int TFPlayerResource::GetDamage(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 1)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(int *) ((unsigned) ent + netvar.m_iDamage_Resource + 4 * idx);
}

unsigned int TFPlayerResource::GetAccountID(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 1)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(unsigned *) ((unsigned) ent + netvar.m_iAccountID_Resource + 4 * idx);
}

int TFPlayerResource::GetPing(int idx) const
{
    IClientEntity *ent;

    if (idx >= g_GlobalVars->maxClients || idx < 1)
        return 0;
    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    return *(int *) ((unsigned) ent + netvar.m_iPing_Resource + 4 * idx);
}

int TFPlayerResource::GetClass(CachedEntity *player) const
{
    IClientEntity *ent;
    int idx;

    ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    idx = player->m_IDX;
    if (idx >= g_GlobalVars->maxClients || idx < 0)
        return 0;
    return *(int *) ((unsigned) ent + netvar.res_iPlayerClass + 4 * idx);
}

bool TFPlayerResource::IsAlive(int idx) const
{
    IClientEntity *ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return false;
    if (idx >= g_GlobalVars->maxClients || idx < 0)
        return false;
    return *(bool *) ((unsigned) ent + netvar.res_bAlive + idx);
}

bool TFPlayerResource::IsValid(int idx) const
{
    IClientEntity *ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return false;
    if (idx >= g_GlobalVars->maxClients || idx < 0)
        return false;
    return *(bool *) ((unsigned) ent + netvar.res_bValid + idx);
}

int TFPlayerResource::GetClass(int idx) const
{
    IClientEntity *ent = g_IEntityList->GetClientEntity(entity);
    if (!ent || ent->GetClientClass()->m_ClassID != RCC_PLAYERRESOURCE)
        return 0;
    if (idx >= g_GlobalVars->maxClients || idx < 0)
        return 0;
    return *(int *) ((unsigned) ent + netvar.res_iPlayerClass + 4 * idx);
}

TFPlayerResource *g_pPlayerResource{ nullptr };
