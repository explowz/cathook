// file created by Spice on 6/12/22 9:52PM

#include "common.hpp"
#include <PlayerTools.hpp>

namespace player_danger
{
static settings::Boolean dangercheck{ "danger.enable", "false" };
static settings::Int draw_string_x{ "danger.draw-info.x", "1500" };
static settings::Int draw_string_y{ "danger.draw-info.y", "600" };
static settings::Int fov1{ "danger.fov-looking-at", "15" };
static settings::Int fov2{ "danger.fov-can-see", "90" };
static settings::Int fov3{ "danger.fov-visible", "180" };

static std::array<std::string, 32> danger_strings;
static size_t danger_strings_count{ 0 };
static std::array<rgba_t, 32> danger_strings_colors{ colors::empty };

void AddDangerString(const std::string &string, const rgba_t &color)
{
    danger_strings[danger_strings_count]        = string;
    danger_strings_colors[danger_strings_count] = color;
    ++danger_strings_count;
}

void DrawDangerStrings()
{
    float x = *draw_string_x;
    float y = *draw_string_y;
    for (size_t i = 0; i < danger_strings_count; ++i)
    {
        float sx, sy;
        fonts::menu->stringSize(danger_strings[i], &sx, &sy);
        draw::String(x, y, danger_strings_colors[i], danger_strings[i].c_str(), *fonts::center_screen);
        y += fonts::center_screen->size + 1;
    }
    danger_strings_count = 0;
}

static void FOVCheck()
{
    if (!*dangercheck)
        return;
    if (!g_IEngine->IsInGame())
        return;
    if (CE_BAD(LOCAL_E))
        return;
    if (!g_pLocalPlayer->alive)
        return;
    for (const auto &ent : entity_cache::player_cache)
    {
        if (RAW_ENT(ent)->IsDormant())
            continue;
        if (!ent->m_bEnemy())
            continue;
        if (!player_tools::shouldTarget(ent))
            continue;
        player_info_s info{};
        if (!GetPlayerInfo(ent->m_IDX, &info) /* || !info.friendsID*/)
            continue;
        else
        {
            std::string name(info.name);
            auto target       = ent->m_vecOrigin();
            auto local_origin = g_pLocalPlayer->v_Origin;
            GetHitbox(ent, head, target);
            GetHitbox(LOCAL_E, head, local_origin);
            if (IsVectorVisible(local_origin, target, true))
            {
                rgba_t color;
                float fov = GetFov(ent->m_vecAngle(), ent->hitboxes.GetHitbox(head)->center, LOCAL_E->hitboxes.GetHitbox(head)->center);
                if (fov < *fov1)
                {
                    color = colors::red;
                    AddDangerString(name + " is looking at you!", color);
                }
                else if (fov < *fov2)
                {
                    color = colors::yellow;
                    AddDangerString(name + " can see you", color);
                }
                else if (fov < *fov3)
                {
                    color = colors::green;
                    AddDangerString(name + " is visible", color);
                }
            }
        }
    }
    DrawDangerStrings();
}

static InitRoutine EC([]() { EC::Register(EC::Draw, FOVCheck, "DRAW_Danger", EC::average); });
} // namespace player_danger
