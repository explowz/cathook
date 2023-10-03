/*
 * playerresource.hpp
 *
 *  Created on: Nov 13, 2016
 *      Author: nullifiedcat
 */

#pragma once

class CachedEntity;

class TFPlayerResource
{
public:
    void Update();
    int GetMaxHealth(CachedEntity *player) const;
    int GetHealth(CachedEntity *player) const;
    int GetMaxBuffedHealth(CachedEntity *player) const;
    int GetClass(CachedEntity *player) const;

    int GetTeam(int idx) const;
    int GetScore(int idx) const;
    int GetKills(int idx) const;
    int GetDeaths(int idx) const;
    int GetLevel(int idx) const;
    int GetDamage(int idx) const;
    unsigned int GetAccountID(int idx) const;
    int GetPing(int idx) const;
    int GetClass(int idx) const;
    bool IsAlive(int idx) const;
    bool IsValid(int idx) const;

    int entity;
};

extern TFPlayerResource *g_pPlayerResource;
