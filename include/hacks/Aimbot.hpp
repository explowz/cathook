/*
 * HAimbot.h
 *
 *  Created on: Oct 8, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

class ConVar;
class IClientEntity;

namespace hacks::shared::aimbot
{
extern settings::Boolean ignore_cloak;
extern unsigned last_target_ignore_timer;
// Used to store aimbot data to prevent calculating it again
struct AimbotCalculatedData_s
{
    unsigned long predict_tick{ 0 };
    bool predict_type{ 0 };
    Vector aim_position{ 0 };
    unsigned long vcheck_tick{ 0 };
    bool visible{ false };
    float fov{ 0 };
    int hitbox{ 0 };
};
static settings::Boolean normal_enable{ "aimbot.enable", "false" };
static settings::Button aimkey{ "aimbot.aimkey.button", "<null>" };
static settings::Int aimkey_mode{ "aimbot.aimkey.mode", "1" };
static settings::Boolean autoshoot{ "aimbot.autoshoot", "1" };
static settings::Boolean autoreload{ "aimbot.autoshoot.activate-heatmaker", "false" };
static settings::Boolean autoshoot_disguised{ "aimbot.autoshoot-disguised", "1" };
static settings::Int multipoint{ "aimbot.multipoint", "0" };
static settings::Int hitbox_mode{ "aimbot.hitbox-mode", "0" };
static settings::Float normal_fov{ "aimbot.fov", "0" };
static settings::Int priority_mode{ "aimbot.priority-mode", "0" };
static settings::Boolean wait_for_charge{ "aimbot.wait-for-charge", "0" };

static settings::Boolean silent{ "aimbot.silent", "1" };
static settings::Boolean target_lock{ "aimbot.lock-target", "0" };
#if ENABLE_VISUALS
static settings::Boolean assistance_only{ "aimbot.assistance.only", "0" };
#endif
static settings::Int hitbox{ "aimbot.hitbox", "0" };
static settings::Boolean zoomed_only{ "aimbot.zoomed-only", "1" };
static settings::Boolean only_can_shoot{ "aimbot.can-shoot-only", "1" };

static settings::Boolean extrapolate{ "aimbot.extrapolate", "0" };
static settings::Int normal_slow_aim{ "aimbot.slow", "0" };
static settings::Int miss_chance{ "aimbot.miss-chance", "0" };

static settings::Boolean projectile_aimbot{ "aimbot.projectile.enable", "true" };
static settings::Float proj_gravity{ "aimbot.projectile.gravity", "0" };
static settings::Float proj_speed{ "aimbot.projectile.speed", "0" };
static settings::Float proj_start_vel{ "aimbot.projectile.initial-velocity", "0" };

static settings::Float sticky_autoshoot{ "aimbot.projectile.sticky-autoshoot", "0.5" };

static settings::Boolean aimbot_debug{ "aimbot.debug", "0" };
settings::Boolean engine_projpred{ "aimbot.debug.engine-pp", "1" };

static settings::Boolean auto_spin_up{ "aimbot.auto.spin-up", "0" };
static settings::Boolean minigun_tapfire{ "aimbot.auto.tapfire", "false" };
static settings::Boolean auto_zoom{ "aimbot.auto.zoom", "0" };
static settings::Boolean auto_unzoom{ "aimbot.auto.unzoom", "0" };

static settings::Boolean backtrackAimbot{ "aimbot.backtrack", "0" };
static settings::Boolean backtrackLastTickOnly("aimbot.backtrack.only-last-tick", "true");
static bool force_backtrack_aimbot = false;
static settings::Boolean backtrackVischeckAll{ "aimbot.backtrack.vischeck-all", "0" };

// TODO maybe these should be moved into "Targeting"
static settings::Float max_range{ "aimbot.target.max-range", "4096" };
static settings::Boolean ignore_vaccinator{ "aimbot.target.ignore-vaccinator", "1" };
static settings::Boolean ignore_deadringer{ "aimbot.target.ignore-deadringer", "1" };
static settings::Boolean buildings_sentry{ "aimbot.target.sentry", "1" };
static settings::Boolean buildings_other{ "aimbot.target.other-buildings", "1" };
static settings::Boolean npcs{ "aimbot.target.npcs", "1" };
static settings::Boolean stickybot{ "aimbot.target.stickybomb", "0" };
static settings::Boolean rageonly{ "aimbot.target.ignore-non-rage", "0" };
static settings::Int teammates{ "aimbot.target.teammates", "0" };

/*
 * 0 Always on
 * 1 Disable if being spectated in first person
 * 2 Disable if being spectated
 */
static settings::Int specmode("aimbot.spectator-mode", "0");
static settings::Boolean specenable("aimbot.spectator.enable", "0");
static settings::Float specfov("aimbot.spectator.fov", "0");
static settings::Int specslow("aimbot.spectator.slow", "0");

// Functions used to calculate aimbot data, and if already calculated use it
Vector PredictEntity(CachedEntity *entity, bool vischeck);
bool VischeckPredictedEntity(CachedEntity *entity);
bool BacktrackVisCheck(CachedEntity *entity);

// Functions called by other functions for when certian game calls are run
void Reset();

// Stuff to make storing functions easy
bool isAiming();
CachedEntity *CurrentTarget();
bool ShouldAim();
CachedEntity *RetrieveBestTarget(bool aimkey_state);
bool IsTargetStateGood(CachedEntity *entity);
void Aim(CachedEntity *entity);
void DoAutoshoot(CachedEntity *target = nullptr);
bool small_box_checker(CachedEntity* target_entity);
int BestHitbox(CachedEntity *target);
int ClosestHitbox(CachedEntity *target);
void DoSlowAim(Vector &inputAngle);
bool UpdateAimkey();
float EffectiveTargetingRange();
} // namespace hacks::shared::aimbot
