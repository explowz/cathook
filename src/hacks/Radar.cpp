/*
 * Radar.cpp
 *
 *  Created on: Mar 28, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <settings/Int.hpp>
#include <hacks/Aimbot.hpp>

#if ENABLE_VISUALS

namespace hacks::radar
{
static settings::Boolean radar_enabled{ "radar.enable", "false" };
static settings::Int shape{ "radar.shape", "0" };
static settings::Float size{ "radar.size", "300" };
static settings::Float zoom{ "radar.zoom", "10" };
static settings::Float opacity{ "radar.opacity", "0.4" };
static settings::Boolean show_cross{ "radar.show-cross", "true" };
static settings::Float healthbar{ "radar.healthbar", "2" };
static settings::Boolean enemies_over_teammates{ "radar.enemies-over-teammates", "true" };
static settings::Float icon_size{ "radar.icon-size", "20" };
static settings::Float radar_x{ "radar.x", "100" };
static settings::Float radar_y{ "radar.y", "100" };
static settings::Boolean use_icons{ "radar.use-icons", "true" };
static settings::Boolean show_teammates{ "radar.show.teammates", "true" };
static settings::Boolean show_teambuildings{ "radar.show.team.buildings", "true" };
static settings::Boolean show_healthpacks{ "radar.show.health", "true" };
static settings::Boolean show_ammopacks{ "radar.show.ammo", "true" };
static settings::Boolean hide_invis{ "radar.hide-invis", "false" };
static settings::Boolean aimbot_highlight{ "radar.aimbot-highlight", "false" };

static Timer invalid{};

std::pair<float, float> WorldToRadar(float x, float y)
{
    static float dx, dy, halfsize;
    static float ry, nx, ny;
    static QAngle angle;

    if (*zoom == 0.0f)
    {
        return { 0.0f, 0.0f };
    }

    dx = x - g_pLocalPlayer->v_Origin.x;
    dy = y - g_pLocalPlayer->v_Origin.y;

    dx /= *zoom;
    dy /= *zoom;

    g_IEngine->GetViewAngles(angle);
    ry = DEG2RAD(angle.y) + M_PI_F / 2.0f;

    dx = -dx;

    nx = dx * std::cos(ry) - dy * std::sin(ry);
    ny = dx * std::sin(ry) + dy * std::cos(ry);

    halfsize = *size / 2.0f;

    if (*shape == 0)
    {
        if (nx < -halfsize)
        {
            nx = -halfsize;
        }

        if (nx > halfsize)
        {
            nx = halfsize;
        }

        if (ny < -halfsize)
        {
            ny = -halfsize;
        }

        if (ny > halfsize)
        {
            ny = halfsize;
        }

        return { nx + halfsize - *icon_size / 2.0f, ny + halfsize - *icon_size / 2.0f };
    }
    else /* if (*shape == 1) */
    {
        float theta = atan2(ny, nx);

        if (nx > halfsize * std::cos(theta) && nx > 0.0f)
            nx = halfsize * std::cos(theta);
        if (nx < halfsize * std::cos(theta) && nx < 0.0f)
            nx = halfsize * std::cos(theta);
        if (ny > halfsize * std::sin(theta) && ny > 0.0f)
            ny = halfsize * std::sin(theta);
        if (ny < halfsize * std::sin(theta) && ny < 0.0f)
            ny = halfsize * std::sin(theta);

        return { nx + halfsize - *icon_size / 2.0f, ny + halfsize - *icon_size / 2.0f };
    }
}

static std::vector<std::vector<textures::sprite>> tx_class{};
static std::vector<textures::sprite> tx_teams{};
static std::vector<textures::sprite> tx_items{};
static std::vector<textures::sprite> tx_buildings{};
static std::vector<textures::sprite> tx_sentry{};

void DrawEntity(float x, float y, CachedEntity *ent)
{
    rgba_t clr;
    float healthp;

    if (ent->m_Type() == ENTITY_PLAYER)
    {
        if (*hide_invis && IsPlayerInvisible(ent))
        {
            return;
        }

        const int clazz = CE_INT(ent, netvar.iClass);
        const int team  = ent->m_iTeam();
        int idx         = team - 2;
        if (idx < 0 || idx > 1)
        {
            return;
        }

        if (clazz <= 0 || clazz > 9)
        {
            return;
        }

        if (!ent->m_vecDormantOrigin())
        {
            return;
        }

        const auto &wtr = WorldToRadar(ent->m_vecDormantOrigin()->x, ent->m_vecDormantOrigin()->y);

        if (*use_icons)
        {
            tx_teams[idx].draw(x + wtr.first, y + wtr.second, *icon_size, *icon_size, colors::white);
            tx_class[0][clazz - 1].draw(x + wtr.first, y + wtr.second, *icon_size, *icon_size, colors::white);
        }
        else
        {
            tx_class[2 - idx][clazz - 1].draw(x + wtr.first, y + wtr.second, *icon_size, *icon_size, colors::white);
            draw::RectangleOutlined(x + wtr.first, y + wtr.second, *icon_size, *icon_size, *aimbot_highlight ? (ent == hacks::aimbot::CurrentTarget() ? colors::target : (idx ? colors::blu_v : colors::red_v)) : (idx ? colors::blu_v : colors::red_v), 1.0f);
        }

        if (ent->m_iMaxHealth() && *healthbar > 0)
        {
            healthp = static_cast<float>(ent->m_iHealth()) / static_cast<float>(ent->m_iMaxHealth());
            clr     = colors::Health(ent->m_iHealth(), ent->m_iMaxHealth());

            if (healthp > 1.0f)
            {
                healthp = 1.0f;
            }

            draw::RectangleOutlined(x + wtr.first, y + wtr.second + *icon_size, *icon_size, 4, colors::black, 0.5f);
            draw::Rectangle(x + wtr.first + 1, y + wtr.second + *icon_size + 1, (*icon_size - 2.0f) * healthp, *healthbar, clr);
        }
    }
    else if (ent->m_Type() == ENTITY_BUILDING)
    {
        if (ent->m_iClassID() == CL_CLASS(CObjectDispenser) || ent->m_iClassID() == CL_CLASS(CObjectSentrygun) || ent->m_iClassID() == CL_CLASS(CObjectTeleporter))
        {
            if (!ent->m_vecDormantOrigin())
            {
                return;
            }

            const auto &wtr = WorldToRadar(ent->m_vecDormantOrigin()->x, ent->m_vecDormantOrigin()->y);
            tx_teams[ent->m_iTeam() - 2].draw(x + wtr.first, y + wtr.second, *icon_size * 1.5f, *icon_size * 1.5f, colors::white);

            switch (ent->m_iClassID())
            {
            case CL_CLASS(CObjectDispenser):
            {
                tx_buildings[0].draw(x + wtr.first, y + wtr.second, *icon_size * 1.5f, *icon_size * 1.5f, colors::white);
                break;
            }
            case CL_CLASS(CObjectSentrygun):
            {
                int level   = CE_INT(ent, netvar.iUpgradeLevel);
                bool IsMini = CE_BYTE(ent, netvar.m_bMiniBuilding);
                if (IsMini)
                {
                    level = 4;
                }

                tx_sentry[level - 1].draw(x + wtr.first, y + wtr.second, *icon_size * 1.5f, *icon_size * 1.5f, colors::white);
                break;
            }
            case CL_CLASS(CObjectTeleporter):
            {
                tx_buildings[1].draw(x + wtr.first, y + wtr.second, *icon_size * 1.5f, *icon_size * 1.5f, colors::white);
                break;
            }
            }

            if (ent->m_iMaxHealth() > 0 && *healthbar > 0.0f)
            {
                healthp = static_cast<float>(ent->m_iHealth()) / static_cast<float>(ent->m_iMaxHealth());
                clr     = colors::Health(ent->m_iHealth(), ent->m_iMaxHealth());

                if (healthp > 1.0f)
                {
                    healthp = 1.0f;
                }

                draw::RectangleOutlined(x + wtr.first, y + wtr.second + *icon_size * 1.5f, *icon_size * 1.5f, 4, colors::black, 0.5f);
                draw::Rectangle(x + wtr.first + 1, y + wtr.second + (*icon_size + 1.0f) * 1.5f, (*icon_size * 1.5f - 2.0f) * healthp, *healthbar, clr);
            }
        }
    }
    else if (ent->m_Type() == ENTITY_GENERIC)
    {
        if (!ent->m_vecDormantOrigin())
        {
            return;
        }

        const model_t *model = RAW_ENT(ent)->GetModel();
        if (model)
        {
            const auto szName = g_IModelInfo->GetModelName(model);
            if (*show_healthpacks && Hash::IsHealth(szName))
            {
                const auto &wtr = WorldToRadar(ent->m_vecDormantOrigin()->x, ent->m_vecDormantOrigin()->y);
                float sz        = *icon_size * 0.15f * 0.5f;
                float sz2       = *icon_size * 0.85f;
                tx_items[0].draw(x + wtr.first + sz, y + wtr.second + sz, sz2, sz2, colors::white);
            }
            else if (*show_ammopacks && Hash::IsAmmo(szName))
            {
                const auto &wtr = WorldToRadar(ent->m_vecDormantOrigin()->x, ent->m_vecDormantOrigin()->y);
                float sz        = *icon_size * 0.15f * 0.5f;
                float sz2       = *icon_size * 0.85f;
                tx_items[1].draw(x + wtr.first + sz, y + wtr.second + sz, sz2, sz2, colors::white);
            }
        }
    }
}

void Draw()
{
    if (!*radar_enabled)
    {
        return;
    }

    if (!g_IEngine->IsInGame())
    {
        return;
    }

    if (CE_BAD(LOCAL_E))
    {
        return;
    }

    float x, y;
    std::vector<CachedEntity *> enemies{};

    x                = *radar_x;
    y                = *radar_y;
    float radar_size = *size;
    float half_size  = radar_size / 2.0f;

    rgba_t outlineclr = *aimbot_highlight ? (hacks::aimbot::CurrentTarget() != nullptr ? colors::target : colors::gui) : colors::gui;

    if (*shape == 0)
    {
        draw::Rectangle(x, y, radar_size, radar_size, colors::Transparent(colors::black, *opacity));
        draw::RectangleOutlined(x, y, radar_size, radar_size, outlineclr, 0.5f);
    }
    else if (*shape == 1)
    {
        float center_x = x + half_size;
        float center_y = y + half_size;
        draw::Circle(center_x, center_y, half_size, outlineclr, 1, 100);
        draw::Circle(center_x, center_y, half_size / 2, colors::Transparent(colors::black, *opacity), half_size, 100);
    }

    if (*enemies_over_teammates)
    {
        enemies.clear();
    }

    std::vector<CachedEntity *> sentries;
    for (const auto &ent : entity_cache::valid_ents)
    {
        // For functions registered as Draw you have to check twice
        if (CE_INVALID(ent))
        {
            continue;
        }

        if (ent->m_iTeam() == tf_team::TEAM_UNK)
        {
            continue;
        }

        if (!ent->m_bAlivePlayer())
        {
            continue;
        }

        bool bEnemy      = ent->m_bEnemy();
        EntityType eType = ent->m_Type();

        if (!*show_teammates && eType == ENTITY_PLAYER && !bEnemy)
        {
            continue;
        }

        if (!*show_teambuildings && eType == ENTITY_BUILDING && !bEnemy)
        {
            continue;
        }

        if (bEnemy)
        {
            enemies.push_back(ent);
        }
        else if (ent->m_iClassID() == CL_CLASS(CObjectSentrygun))
        {
            sentries.push_back(ent);
        }

        DrawEntity(x, y, ent);
    }

    if (*enemies_over_teammates && *show_teammates)
    {
        for (auto enemy : enemies)
        {
            DrawEntity(x, y, enemy);
        }
    }

    for (auto Sentry : sentries)
    {
        DrawEntity(x, y, Sentry);
    }

    if (*show_cross)
    {
        draw::Line(x + half_size, y + half_size / 2, 0, half_size, colors::Transparent(colors::gui, 0.4f), 0.5f);
        draw::Line(x + half_size / 2, y + half_size, half_size, 0, colors::Transparent(colors::gui, 0.4f), 0.5f);
    }

    if (CE_GOOD(LOCAL_E))
    {
        DrawEntity(x, y, LOCAL_E);
        const auto &wtr = WorldToRadar(g_pLocalPlayer->v_Origin.x, g_pLocalPlayer->v_Origin.y);

        if (!*use_icons)
        {
            draw::RectangleOutlined(x + wtr.first, y + wtr.second, *icon_size, *icon_size, colors::gui, 0.5f);
        }
    }
}

static InitRoutine init(
    []()
    {
        // Background circles
        for (int i = 0; i < 2; ++i)
        {
            tx_teams.push_back(textures::atlas().create_sprite(704.0f, 384.0f + static_cast<float>(i) * 64.0f, 64.0f, 64.0f));
        }

        // Items
        for (int i = 0; i < 2; ++i)
        {
            tx_items.push_back(textures::atlas().create_sprite(640.0f, 384.0f + static_cast<float>(i) * 64.0f, 64.0f, 64.0f));
        }

        // Classes
        for (int i = 0; i < 3; ++i)
        {
            tx_class.emplace_back();
            for (int j = 0; j < 9; ++j)
            {
                tx_class[i].push_back(textures::atlas().create_sprite(static_cast<float>(j) * 64.0f, 320.0f + static_cast<float>(i) * 64.0f, 64.0f, 64.0f));
            }
        }

        for (int i = 0; i < 2; ++i)
        {
            tx_buildings.push_back(textures::atlas().create_sprite(576.0f + static_cast<float>(i) * 64.0f, 320.0f, 64.0f, 64.0f));
        }

        for (int i = 0; i < 4; ++i)
        {
            tx_sentry.push_back(textures::atlas().create_sprite(640.0f + static_cast<float>(i) * 64.0f, 256.0f, 64.0f, 64.0f));
        }

        logging::Info("Radar sprites loaded");
        EC::Register(EC::Draw, Draw, "DRAW_Radar");
    });
} // namespace hacks::radar
#endif
