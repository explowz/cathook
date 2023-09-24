/*
 * localplayer.cpp
 *
 *  Created on: Oct 15, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "AntiAim.hpp"

CatCommand printfov("fov_print", "Dump achievements to file (development)",
                    []()
                    {
                        if (CE_GOOD(LOCAL_E))
                            logging::Info("%d", CE_INT(LOCAL_E, netvar.iFOV));
                    });

weaponmode GetWeaponModeloc()
{
    int weapon_handle, weapon_idx, slot;
    CachedEntity *weapon;

    if (CE_BAD(LOCAL_E) | CE_BAD(LOCAL_W))
        return weapon_invalid;
    weapon_handle = CE_INT(LOCAL_E, netvar.hActiveWeapon);
    weapon_idx    = HandleToIDX(weapon_handle);
    if (IDX_BAD(weapon_idx))
    {
        // logging::Info("IDX_BAD: %i", weapon_idx);
        return weaponmode::weapon_invalid;
    }
    weapon = ENTITY(weapon_idx);
    if (CE_BAD(weapon))
        return weaponmode::weapon_invalid;
    int classid = weapon->m_iClassID();
    slot        = re::C_BaseCombatWeapon::GetSlot(RAW_ENT(weapon));
    if (slot == 2)
        return weaponmode::weapon_melee;
    if (slot > 2)
        return weaponmode::weapon_pda;
    switch (classid)
    {
    case CL_CLASS(CTFLunchBox):
    case CL_CLASS(CTFLunchBox_Drink):
    case CL_CLASS(CTFBuffItem):
        return weaponmode::weapon_consumable;
    case CL_CLASS(CTFRocketLauncher_DirectHit):
    case CL_CLASS(CTFRocketLauncher):
    case CL_CLASS(CTFParticleCannon):
    case CL_CLASS(CTFGrenadeLauncher):
    case CL_CLASS(CTFPipebombLauncher):
    case CL_CLASS(CTFCompoundBow):
    case CL_CLASS(CTFBat_Wood):
    case CL_CLASS(CTFBat_Giftwrap):
    case CL_CLASS(CTFFlareGun):
    case CL_CLASS(CTFFlareGun_Revenge):
    case CL_CLASS(CTFSyringeGun):
    case CL_CLASS(CTFCrossbow):
    case CL_CLASS(CTFShotgunBuildingRescue):
    case CL_CLASS(CTFDRGPomson):
    case CL_CLASS(CTFWeaponFlameBall):
    case CL_CLASS(CTFRaygun):
    case CL_CLASS(CTFGrapplingHook):
    case CL_CLASS(CTFRocketLauncher_AirStrike):
    case CL_CLASS(CTFCannon):
    case CL_CLASS(CTFMechanicalArm):
        return weaponmode::weapon_projectile;
    case CL_CLASS(CTFJar):
    case CL_CLASS(CTFJarMilk):
    case CL_CLASS(CTFJarGas):
    case CL_CLASS(CTFCleaver):
        return weaponmode::weapon_throwable;
    case CL_CLASS(CWeaponMedigun):
        return weaponmode::weapon_medigun;
    case CL_CLASS(CTFWeaponSapper):
    case CL_CLASS(CTFWeaponPDA):
    case CL_CLASS(CTFWeaponPDA_Spy):
    case CL_CLASS(CTFWeaponPDA_Engineer_Build):
    case CL_CLASS(CTFWeaponPDA_Engineer_Destroy):
    case CL_CLASS(CTFWeaponBuilder):
    case CL_CLASS(CTFLaserPointer):
        return weaponmode::weapon_pda;
    default:
        return weaponmode::weapon_hitscan;
    }
}
void LocalPlayer::Update()
{
    CachedEntity *wep;

    entity_idx = g_IEngine->GetLocalPlayer();
    entity     = ENTITY(entity_idx);
    if (!entity || CE_BAD(entity))
    {
        team = 0;
        return;
    }
    holding_sniper_rifle     = false;
    holding_sapper           = false;
    weapon_melee_damage_tick = false;
    bRevving                 = false;
    bRevved                  = false;
    wep                      = weapon();
    if (CE_GOOD(wep))
    {
        weapon_mode = GetWeaponModeloc();
        switch (wep->m_iClassID())
        {
        case CL_CLASS(CTFSniperRifle):
        case CL_CLASS(CTFSniperRifleDecap):
        {
            holding_sniper_rifle = true;
            break;
        }
        case CL_CLASS(CTFWeaponBuilder):
        case CL_CLASS(CTFWeaponSapper):
        {
            holding_sapper = true;
            break;
        }
        case CL_CLASS(CTFMinigun):
        {
            switch (CE_INT(LOCAL_W, netvar.iWeaponState))
            {
            case 1:
            case 2:
            {
                bRevving = true;
                break;
            }
            case 3:
            {
                bRevved = true;
                break;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
        }
        // Detect when a melee hit will result in damage, useful for aimbot and antiaim
        if (weapon_mode == weapon_melee && CE_FLOAT(wep, netvar.flNextPrimaryAttack) > SERVER_TIME)
        {
            if (melee_damagetick == tickcount)
            {
                weapon_melee_damage_tick = true;
                melee_damagetick         = ULONG_MAX;
            }
            else if (bAttackLastTick && static_cast<long long>(melee_damagetick) - static_cast<long long>(tickcount) < 0)
                // Thirteen ticks after attack detection = damage tick
                melee_damagetick = tickcount + TIME_TO_TICKS(0.195f);
        }
        else
            melee_damagetick = 0;
    }
    team                   = entity->m_iTeam();
    flags                  = CE_INT(LOCAL_E, netvar.iFlags);
    alive                  = entity->m_bAlivePlayer();
    v_ViewOffset           = CE_VECTOR(entity, netvar.vViewOffset);
    v_Origin               = entity->m_vecOrigin();
    v_OrigViewangles       = current_user_cmd->viewangles;
    v_Eye                  = v_Origin + v_ViewOffset;
    clazz                  = CE_INT(entity, netvar.iClass);
    health                 = entity->m_iHealth();
    this->bUseSilentAngles = false;
    bZoomed                = HasCondition<TFCond_Zoomed>(entity);
    if (bZoomed)
    {
        if (flZoomBegin == 0.0f)
            flZoomBegin = g_GlobalVars->curtime;
    }
    else
        flZoomBegin = 0.0f;

    // Alive
    if (!alive)
    {
        spectator_state = NONE;
        for (const auto &ent: entity_cache::player_cache)
        {
            player_info_s info{};
            if (!RAW_ENT(ent)->IsDormant() && ent != LOCAL_E && HandleToIDX(CE_INT(ent, netvar.hObserverTarget)) == LOCAL_E->m_IDX && GetPlayerInfo(ent->m_IDX, &info))
            {
                switch (CE_INT(ent, netvar.iObserverMode))
                {
                default:
                    spectator_state = ANY;
                    break;
                case OBS_MODE_IN_EYE:
                    spectator_state = FIRSTPERSON;
                    break;
                }
            }
        }
    }
}

void LocalPlayer::UpdateEye()
{
    v_Eye = entity->m_vecOrigin() + CE_VECTOR(entity, netvar.vViewOffset);
}

void LocalPlayer::UpdateEnd()
{
    if (!isFakeAngleCM)
        realAngles = current_user_cmd->viewangles;
    bAttackLastTick = current_user_cmd->buttons & IN_ATTACK;
}

CachedEntity *LocalPlayer::weapon()
{
    int handle, eid;

    if (CE_BAD(entity))
        return nullptr;
    handle = CE_INT(entity, netvar.hActiveWeapon);
    eid    = HandleToIDX(handle);
    if (IDX_BAD(eid))
        return nullptr;
    return ENTITY(eid);
}

LocalPlayer *g_pLocalPlayer = nullptr;
