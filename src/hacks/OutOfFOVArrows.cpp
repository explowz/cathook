/*
 * OutOfFOVArrows.cpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Bintr
 */

#include <settings/Bool.hpp>
#include "common.hpp"
#include "PlayerTools.hpp"
#include "OverrideView.hpp"

namespace hacks::outoffovarrows
{
static settings::Boolean enable{ "oof-arrows.enable", "false" };
static settings::Float angle{ "oof-arrows.angle", "5" };
static settings::Float length{ "oof-arrows.length", "5" };
static settings::Float min_dist{ "oof-arrows.min-dist", "0" };
static settings::Float max_dist{ "oof-arrows.max-dist", "0" };
static settings::Boolean ignore_friends{ "oof-arrows.ignore-friends", "true" };
static settings::Boolean enemy_team_colors{ "oof-arrows.enemy-team-colors", "true" };

bool ShouldRun()
{
    if (!*enable || g_IEngineVGui->IsGameUIVisible())
        return false;

    if (g_pLocalPlayer->life_state)
        return false;

    return true;
}

void DrawArrowTo(const Vector &vecFromPos, const Vector &vecToPos, rgba_t color)
{
    color.a                = 150;
    auto GetClockwiseAngle = [&](const Vector &vecViewAngle, const Vector &vecAimAngle) -> float
    {
        Vector vecAngle = Vector();
        AngleVectors2(VectorToQAngle(vecViewAngle), &vecAngle);

        Vector vecAim = Vector();
        AngleVectors2(VectorToQAngle(vecAimAngle), &vecAim);

        return -atan2(vecAngle.x * vecAim.y - vecAngle.y * vecAim.x, vecAngle.x * vecAim.x + vecAngle.y * vecAim.y);
    };

    auto MapFloat = [&](float x, float in_min, float in_max, float out_min, float out_max) -> float { return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; };

    Vector vecAngleTo = CalcAngle(vecFromPos, vecToPos);

    QAngle qAngle;
    Vector vecViewAngle;

    g_IEngine->GetViewAngles(qAngle);
    vecViewAngle = QAngleToVector(qAngle);

    const float deg  = GetClockwiseAngle(vecViewAngle, vecAngleTo);
    const float xrot = cos(deg - PI / 2);
    const float yrot = sin(deg - PI / 2);

    int ScreenWidth, ScreenHeight;
    g_IEngine->GetScreenSize(ScreenWidth, ScreenHeight);

    const float x1 = ((ScreenWidth * 0.15) + 5.0f) * xrot;
    const float y1 = ((ScreenWidth * 0.15) + 5.0f) * yrot;
    const float x2 = ((ScreenWidth * 0.15) + 15.0f) * xrot;
    const float y2 = ((ScreenWidth * 0.15) + 15.0f) * yrot;

    float arrow_angle  = DEG2RAD(*angle);
    float arrow_length = *length;

    const Vector line{ x2 - x1, y2 - y1, 0.0f };
    const float length = line.Length();

    const float fpoint_on_line = arrow_length / (atanf(arrow_angle) * length);
    const Vector point_on_line = Vector(x2, y2, 0) + (line * fpoint_on_line * -1.0f);
    const Vector normal_vector{ -line.y, line.x, 0.0f };
    const Vector normal = Vector(arrow_length, arrow_length, 0.0f) / (length * 2);

    const Vector rotation = normal * normal_vector;
    const Vector left     = point_on_line + rotation;
    const Vector right    = point_on_line - rotation;

    auto cx = static_cast<float>(ScreenWidth / 2);
    auto cy = static_cast<float>(ScreenHeight / 2);

    float fMap       = clamp(MapFloat(vecFromPos.DistTo(vecToPos), *max_dist, *min_dist, 0.0f, 1.0f), 0.0f, 1.0f);
    rgba_t HeatColor = color;
    HeatColor.a      = static_cast<byte>(fMap * 255.0f);

    draw::Line(cx + x2, cy + y2, cx + left.x, cy + left.y, HeatColor, 0.5);
    draw::Line(cx + x2, cy + y2, cx + right.x, cy + right.y, HeatColor, 0.5);
    draw::Line(cx + left.x, cy + left.y, cx + right.x, cy + right.y, HeatColor, 0.5);
}

static std::vector<Vector> m_vecPlayers{};
void Run()
{
    if (!ShouldRun())
        return;

    Vector vLocalPos = LOCAL_E->m_vecOrigin();

    m_vecPlayers.clear();

    for (const auto &pEnemy : entity_cache::valid_ents)
    {
        if (!pEnemy || !pEnemy->m_bAlivePlayer() || IsPlayerInvisible(pEnemy) || HasCondition<TFCond_HalloweenGhostMode>(pEnemy))
            continue;

        if (*ignore_friends && playerlist::IsFriend(pEnemy))
            continue;

        Vector vEnemyPos  = pEnemy->m_vecOrigin();
        float fFovToEnemy = GetFov(g_pLocalPlayer->v_OrigViewangles, g_pLocalPlayer->v_Eye, vEnemyPos);

        if (fFovToEnemy < *override_fov)
            continue;

        m_vecPlayers.push_back(vEnemyPos);
    }
    if (m_vecPlayers.empty())
        return;

    for (const auto &Player : m_vecPlayers)
    {
        rgba_t teamColor;
        if (!*enemy_team_colors)
        {
            if (LOCAL_E->m_iTeam() == 2)
            {
                teamColor = colors::blu;
            }
            else
            {
                teamColor = colors::red;
            }
        }
        else
        {
            teamColor = colors::gui;
        }
        DrawArrowTo(vLocalPos, Player, teamColor);
    }
}

#if ENABLE_VISUALS
static InitRoutine EC([]() { EC::Register(EC::Draw, Run, "outoffovarrows", EC::average); });
#endif
} // namespace hacks::outoffovarrows
