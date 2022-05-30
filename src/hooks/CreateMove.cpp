/*
 * CreateMove.cpp
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "hack.hpp"
#include "MiscTemporary.hpp"
#include <link.h>
#include <hacks/hacklist.hpp>
#include <settings/Bool.hpp>
#include <hacks/AntiAntiAim.hpp>
#include "NavBot.hpp"
#include "HookTools.hpp"
#include "teamroundtimer.hpp"
#include "HookedMethods.hpp"
#include "nospread.hpp"
#include "Warp.hpp"

static settings::Boolean minigun_jump{ "misc.minigun-jump-tf2c", "false" };
static settings::Boolean roll_speedhack{ "misc.roll-speedhack", "false" };
static settings::Boolean forward_speedhack{ "misc.roll-speedhack.forward", "false" };
settings::Boolean engine_pred{ "misc.engine-prediction", "true" };
static settings::Boolean debug_projectiles{ "debug.projectiles", "false" };
static settings::Int fullauto{ "misc.full-auto", "0" };

class CMoveData;
namespace engine_prediction
{
static Vector original_origin;

void RunEnginePrediction(IClientEntity *ent, CUserCmd *ucmd)
{
    if (!ent)
        return;

    typedef void (*SetupMoveFn)(IPrediction *, IClientEntity *, CUserCmd *, class IMoveHelper *, CMoveData *);
    typedef void (*FinishMoveFn)(IPrediction *, IClientEntity *, CUserCmd *, CMoveData *);

    void **predictionVtable  = *((void ***) g_IPrediction);
    SetupMoveFn oSetupMove   = (SetupMoveFn) (*(unsigned *) (predictionVtable + 19));
    FinishMoveFn oFinishMove = (FinishMoveFn) (*(unsigned *) (predictionVtable + 20));
    // CMoveData *pMoveData = (CMoveData*)(sharedobj::client->lmap->l_addr +
    // 0x1F69C0C);  CMoveData movedata {};
    auto object          = std::make_unique<char[]>(165);
    CMoveData *pMoveData = (CMoveData *) object.get();

    // Backup
    float frameTime = g_GlobalVars->frametime;
    float curTime   = g_GlobalVars->curtime;
    int tickcount   = g_GlobalVars->tickcount;
    original_origin = ent->GetAbsOrigin();

    CUserCmd defaultCmd{};
    if (ucmd == nullptr)
    {
        ucmd = &defaultCmd;
    }

    // Set Usercmd for prediction
    NET_VAR(ent, 4188, CUserCmd *) = ucmd;

    // Set correct CURTIME
    g_GlobalVars->curtime   = g_GlobalVars->interval_per_tick * NET_INT(ent, netvar.nTickBase);
    g_GlobalVars->frametime = g_GlobalVars->interval_per_tick;

    *g_PredictionRandomSeed = MD5_PseudoRandom(current_user_cmd->command_number) & 0x7FFFFFFF;

    // Run The Prediction
    g_IGameMovement->StartTrackPredictionErrors(reinterpret_cast<CBasePlayer *>(ent));
    oSetupMove(g_IPrediction, ent, ucmd, NULL, pMoveData);
    g_IGameMovement->ProcessMovement(reinterpret_cast<CBasePlayer *>(ent), pMoveData);
    oFinishMove(g_IPrediction, ent, ucmd, pMoveData);
    g_IGameMovement->FinishTrackPredictionErrors(reinterpret_cast<CBasePlayer *>(ent));

    // Reset User CMD
    NET_VAR(ent, 4188, CUserCmd *) = nullptr;

    g_GlobalVars->frametime = frameTime;
    g_GlobalVars->curtime   = curTime;
    g_GlobalVars->tickcount = tickcount;

    // Adjust tickbase
    NET_INT(ent, netvar.nTickBase)++;

    return;
}
// Restore Origin
void FinishEnginePrediction(IClientEntity *ent, CUserCmd *ucmd)
{
    const_cast<Vector &>(ent->GetAbsOrigin()) = original_origin;
    original_origin.Invalidate();
}
} // namespace engine_prediction

void PrecalculateCanShoot()
{
    auto weapon = g_pLocalPlayer->weapon();
    // Check if player and weapon are good
    if (CE_BAD(g_pLocalPlayer->entity) || CE_BAD(weapon))
    {
        calculated_can_shoot = false;
        return;
    }

    // flNextPrimaryAttack without reload
    static float next_attack = 0.0f;
    // Last shot fired using weapon
    static float last_attack = 0.0f;
    // Last weapon used
    static CachedEntity *last_weapon = nullptr;
    float server_time                = (float) (CE_INT(g_pLocalPlayer->entity, netvar.nTickBase)) * g_GlobalVars->interval_per_tick;
    float new_next_attack            = CE_FLOAT(weapon, netvar.flNextPrimaryAttack);
    float new_last_attack            = CE_FLOAT(weapon, netvar.flLastFireTime);

    // Reset everything if using a new weapon/shot fired
    if (new_last_attack != last_attack || last_weapon != weapon)
    {
        next_attack = new_next_attack;
        last_attack = new_last_attack;
        last_weapon = weapon;
    }
    // Check if can shoot
    calculated_can_shoot = next_attack <= server_time;
}
static int attackticks = 0;
static std::condition_variable cv;
static std::mutex cv_m;              
static CUserCmd* new_cmd;
static CUserCmd* new_current_cmd;
namespace hooked_methods
{
bool create_one_thread = false; 

bool is_done = false;

DEFINE_HOOKED_METHOD(CreateMove, bool, void *this_, float input_sample_time, CUserCmd *cmd)
{
    g_Settings.is_create_move = true;
  
    if(!create_one_thread){
        pthread_t new_thread;
        new_cmd=cmd;
        int test_int= pthread_create(&new_thread, NULL, run_rest, NULL);
        create_one_thread=true;
    }
    else{
          new_cmd = cmd;
    }
    EC::run(EC::CreateMoveEarly);
    bool ret;
    ret = original::CreateMove(this_, input_sample_time, cmd);
    current_user_cmd = cmd;
    new_current_cmd = current_user_cmd;
    cv.notify_all();
    is_done=false;
    return true;
}
void* run_rest(void* arg){
    while(true){
    std::unique_lock<std::mutex> lk(cv_m);
    cv.wait(lk); 
    PROF_SECTION(CreateMove);
    if (!new_cmd)
    {
        g_Settings.is_create_move = false;
        continue;
    }

#if ENABLE_VISUALS
    // Fix nolerp camera jitter
    if (nolerp)
    {
        QAngle viewangles = { new_cmd->viewangles.x, new_cmd->viewangles.y, new_cmd->viewangles.z };
        g_IEngine->SetViewAngles(viewangles);
    }
#endif

    // Disabled because this causes EXTREME aimbot inaccuracy
    // Actually dont disable it. It causes even more inaccuracy
    if (!new_cmd->command_number)
    {
        g_Settings.is_create_move = false;
        continue;
    }

    tickcount++;

    if (!isHackActive())
    {
        g_Settings.is_create_move = false;
        continue;
    }

    if (!g_IEngine->IsInGame())
    {
        g_Settings.bInvalid       = true;
        g_Settings.is_create_move = false;
        continue;
    }
    bool time_replaced;
    float curtime_old;
    float servertime;
#if ENABLE_VISUALS
    stored_buttons = new_current_cmd->buttons;
    if (freecam_is_toggled)
    {
        new_current_cmd->sidemove    = 0.0f;
        new_current_cmd->forwardmove = 0.0f;
    }
#endif
    if (new_current_cmd && current_user_cmd->command_number)
        last_cmd_number = current_user_cmd->command_number;

    /**bSendPackets = true;
    if (hacks::shared::lagexploit::ExploitActive()) {
        *bSendPackets = ((current_user_cmd->command_number % 4) == 0);
        //logging::Info("%d", *bSendPackets);
    }*/

    // logging::Info("canpacket: %i", ch->CanPacket());
    // if (!cmd) return ret;

    time_replaced = false;
    curtime_old   = g_GlobalVars->curtime;
    if (!g_Settings.bInvalid && CE_GOOD(g_pLocalPlayer->entity))
    {
        servertime            = (float) CE_INT(g_pLocalPlayer->entity, netvar.nTickBase) * g_GlobalVars->interval_per_tick;
        g_GlobalVars->curtime = servertime;
        time_replaced         = true;
    }
    if (g_Settings.bInvalid)
    {
        entity_cache::Invalidate();
    }

    //	PROF_BEGIN();
    // Do not update if in warp, since the entities will stay identical either way
    if (!hacks::tf2::warp::in_warp)
    {
        PROF_SECTION(EntityCache);
        entity_cache::Update();
    }
    //	PROF_END("Entity Cache updating");
    {
        PROF_SECTION(CM_PlayerResource);
        g_pPlayerResource->Update();
    }
    {
        PROF_SECTION(CM_LocalPlayer);
        g_pLocalPlayer->Update();
    }
    PrecalculateCanShoot();
    if (firstcm)
    {
        DelayTimer.update();
        if (identify)
        {
            sendIdentifyMessage(false);
        }
        EC::run(EC::FirstCM);
        firstcm = false;
    }
    g_Settings.bInvalid = false;
    if (CE_GOOD(g_pLocalPlayer->entity))
    {
        if (!g_pLocalPlayer->life_state && CE_GOOD(g_pLocalPlayer->weapon()))
        {
            // Walkbot can leave game.
            if (new_current_cmd->buttons & IN_ATTACK)
                ++attackticks;
            else
                attackticks = 0;
            if (fullauto)
                if (new_current_cmd->buttons & IN_ATTACK)
                    if (attackticks % *fullauto + 1 < *fullauto)
                        new_current_cmd->buttons &= ~IN_ATTACK;
            g_pLocalPlayer->isFakeAngleCM = false;
            static int fakelag_queue      = 0;
            if (CE_GOOD(LOCAL_E))
                if (!hacks::tf2::nospread::is_syncing && (fakelag_amount || (hacks::shared::antiaim::force_fakelag && hacks::shared::antiaim::isEnabled())))
                {
                    // Do not fakelag when trying to attack
                    bool do_fakelag = true;
                    switch (g_pLocalPlayer->weapon_mode)
                    {
                    case weapon_melee:
                    {
                        if (g_pLocalPlayer->weapon_melee_damage_tick)
                            do_fakelag = false;
                        break;
                    }
                    case weapon_hitscan:
                    {
                        if ((CanShoot() || isRapidFire(RAW_ENT(LOCAL_W))) && current_user_cmd->buttons & IN_ATTACK)
                            do_fakelag = false;
                        break;
                    }
                    default:
                        break;
                    }

                    if (fakelag_midair && CE_INT(LOCAL_E, netvar.iFlags) & FL_ONGROUND)
                        do_fakelag = false;

                    if (do_fakelag)
                    {
                        int fakelag_amnt = (*fakelag_amount > 1) ? *fakelag_amount : 1;
                        *bSendPackets    = fakelag_amnt == fakelag_queue;
                        if (*bSendPackets)
                            g_pLocalPlayer->isFakeAngleCM = true;
                        fakelag_queue++;
                        if (fakelag_queue > fakelag_amnt)
                            fakelag_queue = 0;
                    }
                }
            {
                PROF_SECTION(CM_antiaim);
                hacks::shared::antiaim::ProcessUserCmd(new_cmd);
            }
            if (debug_projectiles)
                projectile_logging::Update();
        }
    }
    {
        PROF_SECTION(CM_WRAPPER);
        EC::run(EC::CreateMove_NoEnginePred);

        if (engine_pred)
        {
            engine_prediction::RunEnginePrediction(RAW_ENT(LOCAL_E), new_current_cmd);
            g_pLocalPlayer->UpdateEye();
        }

        if (hacks::tf2::warp::in_warp)
            EC::run(EC::CreateMoveWarp);
        else
            EC::run(EC::CreateMove);
    }
    if (time_replaced)
        g_GlobalVars->curtime = curtime_old;
    g_Settings.bInvalid = false;
    {
        PROF_SECTION(CM_chat_stack);
        chat_stack::OnCreateMove();
    }

    // TODO Auto Steam Friend

#if ENABLE_IPC
    {
        PROF_SECTION(CM_playerlist);
        static Timer ipc_update_timer{};
        //	playerlist::DoNotKillMe();
        if (ipc_update_timer.test_and_set(1000 * 10))
        {
            ipc::UpdatePlayerlist();
        }
    }
#endif
    g_pLocalPlayer->UpdateEnd();
    g_Settings.is_create_move = false;
    if (nolerp)
    {
        static const ConVar *pUpdateRate = g_pCVar->FindVar("cl_updaterate");
        if (!pUpdateRate)
            pUpdateRate = g_pCVar->FindVar("cl_updaterate");
        else
        {
            float interp = MAX(cl_interp->GetFloat(), cl_interp_ratio->GetFloat() / pUpdateRate->GetFloat());
            current_user_cmd->tick_count += TIME_TO_TICKS(interp);
        }
    }
    logging::Info("LOGGED FROM NEW THREAD");
    is_done=true;
    }
}
void WriteCmd(IInput *input, CUserCmd *cmd, int sequence_nr)
{
    // Write the usercmd
    GetVerifiedCmds(input)[sequence_nr % VERIFIED_CMD_SIZE].m_cmd = *cmd;
    GetVerifiedCmds(input)[sequence_nr % VERIFIED_CMD_SIZE].m_crc = GetChecksum(cmd);
    GetCmds(input)[sequence_nr % VERIFIED_CMD_SIZE]               = *cmd;
}

// This gets called before the other CreateMove, but since we run original first in here all the stuff gets called after normal CreateMove is done
bool run_many = false;
DEFINE_HOOKED_METHOD(CreateMoveInput, void, IInput *this_, int sequence_nr, float input_sample_time, bool arg3)
{
    
    bSendPackets = reinterpret_cast<bool *>((uintptr_t) __builtin_frame_address(1) - 8);
    // Call original function, includes Normal CreateMove
    original::CreateMoveInput(this_, sequence_nr, input_sample_time, arg3);

    CUserCmd *cmd = nullptr;
    if (this_ && GetCmds(this_) && sequence_nr > 0)
        cmd = this_->GetUserCmd(sequence_nr);

    if (!cmd)
        return;

    current_late_user_cmd = cmd;

    if (!isHackActive())
    {
        WriteCmd(this_, current_late_user_cmd, sequence_nr);
        return;
    }

    if (!g_IEngine->IsInGame())
    {
        WriteCmd(this_, current_late_user_cmd, sequence_nr);
        return;
    }

    PROF_SECTION(CreateMoveInput);

    // Run EC
    EC::run(EC::CreateMoveLate);

    if (CE_GOOD(LOCAL_E))
    {
        // Restore prediction
        if (engine_prediction::original_origin.IsValid())
            engine_prediction::FinishEnginePrediction(RAW_ENT(LOCAL_E), current_late_user_cmd);
    }
    // Write the usercmd
    WriteCmd(this_, current_late_user_cmd, sequence_nr);
}
} // namespace hooked_methods