/*
 * Misc.cpp
 *
 *  Created on: Nov 5, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <regex>
#include <hacks/AntiAim.hpp>
#include <settings/Bool.hpp>

#include "core/sharedobj.hpp"
#include "filesystem.h"
#include "DetourHook.hpp"
#include "hack.hpp"
#include <thread>

namespace hacks::misc
{
#if !ENFORCE_STREAM_SAFETY && ENABLE_VISUALS
static settings::Boolean render_zoomed{ "visual.render-local-zoomed", "true" };
#endif
static settings::Boolean anti_afk{ "misc.anti-afk", "true" };
static settings::Boolean auto_jump{ "misc.auto-jump", "false" };
static settings::Int auto_jump_chance{ "misc.auto-jump.chance", "100" };
#if !ENABLE_TEXTMODE
static settings::Int auto_strafe{ "misc.autostrafe", "2" };
#else
static settings::Int auto_strafe{ "misc.autostrafe", "0" };
#endif
static settings::Boolean nopush_enabled{ "misc.no-push", "true" };
static settings::Boolean force_wait{ "misc.force-enable-wait", "true" };
#if ENABLE_VISUALS
static settings::Boolean debug_info{ "misc.debug-info", "false" };
// Useful for debugging with showlagcompensation
static settings::Boolean show_spectators{ "misc.show-spectators", "false" };

// Need our own Text drawing
static std::array<std::string, 32> spectator_strings;
static size_t spectator_strings_count{ 0 };
static std::array<rgba_t, 32> spectator_strings_colors{ colors::empty };

void AddSpectatorString(const std::string &string, const rgba_t &color)
{
    spectator_strings[spectator_strings_count]        = string;
    spectator_strings_colors[spectator_strings_count] = color;
    ++spectator_strings_count;
}

void DrawSpectatorStrings()
{
    float y = draw::height / 4;
    for (size_t i = 0; i < spectator_strings_count; ++i)
    {
        float sx, sy;
        fonts::menu->stringSize(spectator_strings[i], &sx, &sy);
        draw::String((draw::width - sx) / 2, y, spectator_strings_colors[i], spectator_strings[i].c_str(), *fonts::center_screen);
        y += fonts::center_screen->size + 1;
    }
    spectator_strings_count = 0;
}
#endif

static settings::Boolean fix_cyoaanim{ "remove.contracker", "true" };

#if !ENFORCE_STREAM_SAFETY && ENABLE_VISUALS
static void tryPatchLocalPlayerShouldDraw(bool after)
{
    static BytePatch patch_shoulddraw{ CSignature::GetClientSignature, "80 BB ? ? ? ? ? 75 DE", 0xD, { 0xE0 } };
    static BytePatch patch_shoulddraw_wearable{ CSignature::GetClientSignature("0F 85 ? ? ? ? E9 ? ? ? ? 85 D2") + 1, { 0x80 } };

    if (after)
    {
        patch_shoulddraw.Patch();
        patch_shoulddraw_wearable.Patch();
    }
    else
    {
        patch_shoulddraw.Shutdown();
        patch_shoulddraw_wearable.Shutdown();
    }
}
#endif

static Timer anti_afk_timer{};
static int last_buttons{ 0 };

static void updateAntiAfk()
{
    if (current_user_cmd->buttons != last_buttons || g_pLocalPlayer->life_state)
    {
        anti_afk_timer.update();
        last_buttons = current_user_cmd->buttons;
    }
    else
    {
        static auto afk_timer = g_ICvar->FindVar("mp_idlemaxtime");
        if (!afk_timer)
            afk_timer = g_ICvar->FindVar("mp_idlemaxtime");
        // Trigger 10 seconds before kick
        else if (afk_timer->GetInt() != 0 && anti_afk_timer.check(afk_timer->m_nValue * 60 * 1000 - 10000))
        {
            // Game also checks if you move in spawn, so spam movement keys alternatively
            bool flip = false;
            current_user_cmd->buttons |= flip ? IN_FORWARD : IN_BACK;
            // Flip flip
            flip = !flip;
            if (anti_afk_timer.check(afk_timer->GetInt() * 60 * 1000 + 1000))
                anti_afk_timer.update();
        }
        last_buttons = current_user_cmd->buttons;
    }
}

CatCommand fix_cursor("fix_cursor", "Fix the GUI cursor being visible",
                      []()
                      {
                          g_ISurface->LockCursor();
                          g_ISurface->SetCursorAlwaysVisible(false);
                      });

int last_number{ 0 };

int getCarriedBuilding()
{
    if (CE_INT(LOCAL_E, netvar.m_bCarryingObject))
        return HandleToIDX(CE_INT(LOCAL_E, netvar.m_hCarriedObject));
    for (const auto &ent : entity_cache::valid_ents)
    {
        if (ent->m_Type() != ENTITY_BUILDING)
            continue;
        if (HandleToIDX(CE_INT(ent, netvar.m_hBuilder)) != LOCAL_E->m_IDX)
            continue;
        if (!CE_INT(ent, netvar.m_bPlacing))
            continue;
        return ent->m_IDX;
    }
    return -1;
}

#if ENABLE_VISUALS
struct wireframe_data
{
    Vector raw_min;
    Vector raw_max;
    Vector rotation;
    Vector origin;
};

std::vector<wireframe_data> wireframe_queue;
void QueueWireframeHitboxes(hitbox_cache::EntityHitboxCache &hb_cache)
{
    for (unsigned i = 0; i < hb_cache.GetNumHitboxes(); ++i)
    {
        auto hb        = hb_cache.GetHitbox(i);
        Vector raw_min = hb->bbox->bbmin;
        Vector raw_max = hb->bbox->bbmax;
        auto transform = hb_cache.GetBones()[hb->bbox->bone];
        Vector rotation;
        Vector origin;

        MatrixAngles(transform, *(QAngle *) &rotation, origin);
        wireframe_queue.push_back(wireframe_data{ raw_min, raw_max, rotation, origin });
    }
}

void DrawWireframeHitbox(wireframe_data data)
{
    g_IVDebugOverlay->AddBoxOverlay2(data.origin, data.raw_min, data.raw_max, VectorToQAngle(data.rotation), Color(0, 0, 0, 0), Color(255, 0, 0, 255), g_GlobalVars->interval_per_tick * 2);
}
#endif

static float normalizeRad(float a) noexcept
{
    return std::isfinite(a) ? std::remainder(a, M_PI_F * 2) : 0.0f;
}

static float angleDiffRad(float a1, float a2) noexcept
{
    float delta;

    delta = normalizeRad(a1 - a2);
    if (a1 > a2)
    {
        if (delta >= M_PI_F)
            delta -= M_PI_F * 2;
    }
    else if (delta <= -M_PI_F)
        delta += M_PI_F * 2;

    return delta;
}

static void CreateMove()
{
    if (current_user_cmd->command_number)
        last_number = current_user_cmd->command_number;
    // AntiAfk That after a certain time without movement keys depressed, causes
    // random keys to be spammed for 1 second
    if (*anti_afk)
        updateAntiAfk();

    if (*auto_jump && CE_GOOD(LOCAL_E) && !g_pLocalPlayer->life_state)
    {
        static int ticks_last_jump = 0;

        if (UniformRandomInt(0, 99) > *auto_jump_chance)
            return;

        bool ground = g_pLocalPlayer->flags & FL_ONGROUND;
        bool jump   = current_user_cmd->buttons & IN_JUMP;

        if (!ground && jump && ticks_last_jump++ >= 9)
            current_user_cmd->buttons = current_user_cmd->buttons & ~IN_JUMP;

        if (!jump)
            ticks_last_jump = 0;
    }

    // Automatically strafes in the air
    if (*auto_strafe && CE_GOOD(LOCAL_E) && !g_pLocalPlayer->life_state)
    {
        auto movetype = (unsigned) CE_VAR(LOCAL_E, 0x194, unsigned char);

        if (movetype == MoveType_t::MOVETYPE_NOCLIP || movetype == MoveType_t::MOVETYPE_LADDER)
            return;

        auto flags              = g_pLocalPlayer->flags;
        static bool was_jumping = false;
        bool is_jumping         = current_user_cmd->buttons & IN_JUMP;

        switch (*auto_strafe)
        {
        case 0: // Off
        default:
            break;
        case 1: // Regular strafe
        {
            if (!(flags & (FL_ONGROUND | FL_INWATER)) && (!is_jumping || was_jumping) && current_user_cmd->mousedx)
                current_user_cmd->sidemove = current_user_cmd->mousedx > 1 ? 450.f : -450.f;

            was_jumping = is_jumping;

            break;
        }
        case 2: // Multidirectional Airstrafe, Huge Credits to https://github.com/degeneratehyperbola/NEPS
        {
            const float speed = CE_VECTOR(LOCAL_E, netvar.vVelocity).Length2D();
            auto vel          = CE_VECTOR(LOCAL_E, netvar.vVelocity);

            if (flags & (FL_ONGROUND | FL_INWATER))
                break;
            if (speed < 2.0f)
                break;

            constexpr auto perfectDelta = [](float speed) noexcept
            {
                static auto speedVar  = 6062.0f; // Engine limit on 3 axis, see: reddit.com/r/tf2/comments/57hhl4/question_is_there_a_maximum_rocket_jumping/d8t9x82
                static auto airVar    = g_ICvar->FindVar("sv_airaccelerate");
                static auto wishSpeed = 30.0f; // This is hardcoded for tf2, unless you run sourcemod

                const auto term = wishSpeed / airVar->GetFloat() / speedVar * 100.0f / speed;

                if (term < 1.0f && term > -1.0f)
                    return acosf(term);

                return 0.0f;
            };

            const float pDelta = perfectDelta(speed);
            if (pDelta && (!is_jumping || was_jumping))
            {
                const float yaw     = DEG2RAD(g_pLocalPlayer->v_OrigViewangles.y);
                const float velDir  = atan2f(vel.y, vel.x) - yaw;
                const float wishAng = atan2f(-current_user_cmd->sidemove, current_user_cmd->forwardmove);
                const float delta   = angleDiffRad(velDir, wishAng);

                float moveDir = delta < 0.0f ? velDir + pDelta : velDir - pDelta;

                current_user_cmd->forwardmove = cosf(moveDir) * 450.0f;
                current_user_cmd->sidemove    = -sinf(moveDir) * 450.0f;
            }

            was_jumping = is_jumping;
            break;
        }
        }
    }

    // Simple No-Push through cvars
    g_ICvar->FindVar("tf_avoidteammates_pushaway")->SetValue(!*nopush_enabled);
}

#if ENABLE_VISUALS
void Draw()
{
    if (*show_spectators)
    {
        for (const auto &ent : entity_cache::valid_ents)
        {
            player_info_s info{};
            if (ent->m_Type() == ENTITY_PLAYER && HandleToIDX(CE_INT(ent, netvar.hObserverTarget)) == LOCAL_E->m_IDX && GetPlayerInfo(ent->m_IDX, &info) && ent != LOCAL_E)
            {
                auto observermode = "N/A";
                rgba_t color      = ent->m_iTeam() == TEAM_BLU ? colors::blu : (ent->m_iTeam() == TEAM_RED ? colors::red : colors::white);

                switch (CE_INT(ent, netvar.iObserverMode))
                {
                case OBS_MODE_DEATHCAM:
                {
                    observermode = "DeathCam";
                    break;
                }
                case OBS_MODE_FREEZECAM:
                {
                    observermode = "FreezeCam";
                    break;
                }
                case OBS_MODE_FIXED:
                {
                    observermode = "Fixed";
                    break;
                }
                case OBS_MODE_IN_EYE:
                {
                    observermode = "1st";
                    break;
                }
                case OBS_MODE_CHASE:
                {
                    observermode = "3rd";
                    break;
                }
                case OBS_MODE_POI:
                {
                    observermode = "POI";
                    break;
                }
                case OBS_MODE_ROAMING:
                {
                    observermode = "Roaming";
                    break;
                }
                default:
                    continue;
                }
                AddSpectatorString(format("[", observermode, "]", " ", info.name), color);
            }
        }
        DrawSpectatorStrings();
    }
    if (!*debug_info)
        return;
    if (CE_GOOD(LOCAL_W))
    {
        AddSideString(format("Slot: ", re::C_BaseCombatWeapon::GetSlot(RAW_ENT(LOCAL_W))));
        AddSideString(format("Taunt Concept: ", CE_INT(LOCAL_E, netvar.m_iTauntConcept)));
        AddSideString(format("Taunt Index: ", CE_INT(LOCAL_E, netvar.m_iTauntIndex)));
        AddSideString(format("Sequence: ", CE_INT(LOCAL_E, netvar.m_nSequence)));
        AddSideString(format("Velocity: ", LOCAL_E->m_vecVelocity.x, ' ', LOCAL_E->m_vecVelocity.y, ' ', LOCAL_E->m_vecVelocity.z));
        AddSideString(format("Velocity3: ", LOCAL_E->m_vecVelocity.Length()));
        AddSideString(format("Velocity2: ", LOCAL_E->m_vecVelocity.Length2D()));
        AddSideString("NetVar Velocity");
        Vector vel = CE_VECTOR(LOCAL_E, netvar.vVelocity);
        AddSideString(format("Velocity: ", vel.x, ' ', vel.y, ' ', vel.z));
        AddSideString(format("Velocity3: ", vel.Length()));
        AddSideString(format("Velocity2: ", vel.Length2D()));
        AddSideString(format("flSimTime: ", CE_FLOAT(LOCAL_E, netvar.m_flSimulationTime)));
        if (current_user_cmd)
            AddSideString(format("command_number: ", last_cmd_number));
        AddSideString(format("clip: ", CE_INT(LOCAL_W, netvar.m_iClip1)));
        AddSideString(format("Weapon state: ", CE_INT(LOCAL_W, netvar.iWeaponState)));
        AddSideString(format("ItemDefinitionIndex: ", CE_INT(LOCAL_W, netvar.iItemDefinitionIndex)));
        AddSideString(format("Maxspeed: ", CE_FLOAT(LOCAL_E, netvar.m_flMaxspeed)));
    }
}
#endif

void generate_schema()
{
    std::ifstream in("tf/scripts/items/items_game.txt");
    std::string outS((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::ofstream out(paths::getDataPath("/items_game.txt"));
    std::regex a(R"("equip_regions?".*?".*?")");
    std::regex b(R"("equip_regions?"\s*?\n\s*?\{[\s\S\n]*?\})");
    outS = std::regex_replace(outS, a, "");
    out << std::regex_replace(outS, b, "");
    out.close();
    logging::Info("Generating complete!");
}

static CatCommand generateschema("schema_generate", "Generate custom schema", generate_schema);

bool InitSchema(const char *fileName, const char *pathID, CUtlVector<CUtlString> *pVecErrors /* = NULL */)
{
    static auto GetItemSchema = reinterpret_cast<void *(*) ()>(CSignature::GetClientSignature("55 89 E5 57 56 53 83 EC ? 8B 1D ? ? ? ? 85 DB 89 D8"));

    static auto BInitTextBuffer = reinterpret_cast<bool (*)(void *, CUtlBuffer &, CUtlVector<CUtlString> *)>(CSignature::GetClientSignature("55 89 E5 57 56 53 8D 9D ? ? ? ? 81 EC ? ? ? ? 8B 7D ? 89 1C 24 "));
    void *schema                = (void *) ((uintptr_t) GetItemSchema() + 0x4);

    // Read the raw data
    CUtlBuffer bufRawData;
    bool bReadFileOK = g_IFileSystem->ReadFile(fileName, pathID, bufRawData);
    // Log something here if bReadFileOK is false

    if (!bReadFileOK)
    {
        logging::Info(("Failed reading item schema from " + std::string(fileName)).c_str());
        return false;
    }
    else
        logging::Info(("Read item schema from " + std::string(fileName)).c_str());

    // Wrap it with a text buffer reader
    CUtlBuffer bufText(bufRawData.Base(), bufRawData.TellPut(), CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER);

    // Use the standard init path
    return BInitTextBuffer(schema, bufText, pVecErrors);
}

void Schema_Reload()
{
    logging::Info("Loading item schema...");
    bool ret = InitSchema(paths::getDataPath("/items_game.txt").c_str(), nullptr, nullptr);
    logging::Info("Loading %s", ret ? "Successful" : "Unsuccessful");
}

CatCommand schema("schema", "Load custom schema", Schema_Reload);

CatCommand update_gui_color("gui_color_update", "Update the GUI Color",
                            []()
                            {
                                hack::command_stack().emplace("cat set zk.style.tab-button.color.selected.background 1f1f1fff;cat set zk.style.tab-button.color.separator 000000ff;cat set zk.style.tab-button.color.hover.underline ffa200ff;cat set zk.style.tab-button.color.selected.underline ffa200ff;cat set zk.style.tooltip.border 446498ff;cat set zk.style.box.color.border 000000ff;cat set zk.style.color-preview.color.border 446498ff;cat set zk.style.modal-container.color.border 000000ff;cat set zk.style.tab-selection.color.border 000000ff;cat set zk.style.table.color.border 000000ff;cat set zk.style.checkbox.color.border 000000ff;cat set zk.style.checkbox.color.checked ffa200ff;cat set zk.style.checkbox.color.hover 1f1f1fff;cat set zk.style.input.color.border 000000ff;cat set zk.style.input.key.color.border 000000ff;cat set zk.style.input.select.border 000000ff;cat set zk.style.input.slider.color.handle_border 000000ff;cat set zk.style.input.slider.color.bar 1f1f1fff;cat set zk.style.input.text.color.border.active ffa200ff");
                                hack::command_stack().emplace("cat set zk.style.input.text.color.border.inactive 000000ff;cat set zk.style.tree-list-entry.color.lines 42BC99ff;cat set zk.style.window.color.border 000000ff;cat set zk.style.window-close-button.color.border 000000ff;cat set zk.style.window-header.color.background.active 2a2a2aff;cat set zk.style.window-header.color.border.inactive 000000ff;cat set zk.style.window-header.color.border.active 000000ff");
                            });

CatCommand name("name_set", "Immediate name change",
                [](const CCommand &args)
                {
                    if (args.ArgC() < 2)
                    {
                        logging::Info("Set a name, silly");
                        return;
                    }
                    if (g_Settings.bInvalid)
                    {
                        logging::Info("Only works ingame!");
                        return;
                    }
                    std::string new_name(args.ArgS());
                    ReplaceSpecials(new_name);
                    NET_SetConVar setname("name", new_name.c_str());
                    auto *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
                    if (ch)
                    {
                        setname.SetNetChannel(ch);
                        setname.SetReliable(false);
                        ch->SendNetMsg(setname, false);
                    }
                });

CatCommand set_value("set", "Set value",
                     [](const CCommand &args)
                     {
                         if (args.ArgC() < 2)
                             return;
                         ConVar *var = g_ICvar->FindVar(args.Arg(1));
                         if (!var)
                             return;
                         std::string value(args.Arg(2));
                         ReplaceSpecials(value);
                         var->m_fMaxVal = 999999999.9f;
                         var->m_fMinVal = -999999999.9f;
                         var->SetValue(value.c_str());
                         logging::Info("Set '%s' to '%s'", args.Arg(1), value.c_str());
                     });

CatCommand get_value("get", "Set value",
                     [](const CCommand &args)
                     {
                         if (args.ArgC() < 2)
                             return;
                         ConVar *var = g_ICvar->FindVar(args.Arg(1));
                         if (!var)
                             return;
                         logging::Info("'%s': '%s'", args.Arg(1), var->GetString());
                     });

CatCommand say_lines("say_lines", "Say with newlines (\\n)",
                     [](const CCommand &args)
                     {
                         std::string message(args.ArgS());
                         ReplaceSpecials(message);
                         std::string cmd = format("say ", message);
                         g_IEngine->ServerCmd(cmd.c_str());
                     });

CatCommand disconnect("disconnect", "Disconnect with custom reason",
                      [](const CCommand &args)
                      {
                          auto ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
                          if (!ch)
                              return;
                          std::string string = args.ArgS();
                          ReplaceSpecials(string);
                          ch->Shutdown(string.c_str());
                      });

CatCommand disconnect_vac("disconnect_vac", "Disconnect (fake VAC)",
                          []()
                          {
                              auto ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
                              if (!ch)
                                  return;
                              ch->Shutdown("VAC banned from secure server\n");
                          });

// Netvars stuff
void DumpRecvTable(CachedEntity *ent, RecvTable *table, int depth, const char *ft, unsigned acc_offset)
{
    bool forcetable = ft && strlen(ft);
    if (!forcetable || !strcmp(ft, table->GetName()))
        logging::Info("==== TABLE: %s", table->GetName());
    for (int i = 0; i < table->GetNumProps(); ++i)
    {
        RecvProp *prop = table->GetProp(i);
        if (!prop)
            continue;
        if (prop->GetDataTable())
            DumpRecvTable(ent, prop->GetDataTable(), depth + 1, ft, acc_offset + prop->GetOffset());
        if (forcetable && strcmp(ft, table->GetName()) != 0)
            continue;
        switch (prop->GetType())
        {
        case SendPropType::DPT_Float:
            logging::Info("TABLE %s IN DEPTH %d: %s [0x%04x] = %f", table ? table->GetName() : "none", depth, prop->GetName(), prop->GetOffset(), CE_FLOAT(ent, acc_offset + prop->GetOffset()));
            break;
        case SendPropType::DPT_Int:
            logging::Info("TABLE %s IN DEPTH %d: %s [0x%04x] = %i | %u | %hd | %hu", table ? table->GetName() : "none", depth, prop->GetName(), prop->GetOffset(), CE_INT(ent, acc_offset + prop->GetOffset()), CE_VAR(ent, acc_offset + prop->GetOffset(), unsigned int), CE_VAR(ent, acc_offset + prop->GetOffset(), short), CE_VAR(ent, acc_offset + prop->GetOffset(), unsigned short));
            break;
        case SendPropType::DPT_String:
            logging::Info("TABLE %s IN DEPTH %d: %s [0x%04x] = 'not yet supported'", table ? table->GetName() : "none", depth, prop->GetName(), prop->GetOffset());
            break;
        case SendPropType::DPT_Vector:
            logging::Info("TABLE %s IN DEPTH %d: %s [0x%04x] = (%f, %f, %f)", table ? table->GetName() : "none", depth, prop->GetName(), prop->GetOffset(), CE_FLOAT(ent, acc_offset + prop->GetOffset()), CE_FLOAT(ent, acc_offset + prop->GetOffset() + 4), CE_FLOAT(ent, acc_offset + prop->GetOffset() + 8));
            break;
        case SendPropType::DPT_VectorXY:
            logging::Info("TABLE %s IN DEPTH %d: %s [0x%04x] = (%f, %f)", table ? table->GetName() : "none", depth, prop->GetName(), prop->GetOffset(), CE_FLOAT(ent, acc_offset + prop->GetOffset()), CE_FLOAT(ent, acc_offset + prop->GetOffset() + 4));
            break;
            // There are some missing things here
        default:
            break;
        }
    }
    if (!ft || !strcmp(ft, table->GetName()))
        logging::Info("==== END OF TABLE: %s IN DEAPTH %d", table->GetName(), depth);
}

// CatCommand to dumb netvar info
static CatCommand dump_vars("debug_dump_netvars", "Dump netvars of entity",
                            [](const CCommand &args)
                            {
                                if (args.ArgC() < 2)
                                    return;
                                if (!atoi(args[1]))
                                    return;
                                int idx           = atoi(args[1]);
                                CachedEntity *ent = ENTITY(idx);
                                if (CE_BAD(ent))
                                    return;
                                ClientClass *clz = RAW_ENT(ent)->GetClientClass();
                                logging::Info("Entity %i: %s", ent->m_IDX, clz->GetName());
                                const char *ft = (args.ArgC() > 1 ? args[2] : nullptr);
                                DumpRecvTable(ent, clz->m_pRecvTable, 0, ft, 0);
                            });

static CatCommand dump_vars_by_name("debug_dump_netvars_name", "Dump netvars of entity with target name",
                                    [](const CCommand &args)
                                    {
                                        if (args.ArgC() < 2)
                                            return;
                                        std::string name(args.Arg(1));
                                        for (const auto &ent : entity_cache::valid_ents)
                                        {
                                            ClientClass *clz = RAW_ENT(ent)->GetClientClass();
                                            if (!clz)
                                                continue;
                                            std::string clazz_name(clz->GetName());
                                            if (clazz_name.find(name) == std::string::npos)
                                                continue;
                                            logging::Info("Entity %i: %s", ent->m_IDX, clz->GetName());
                                            const char *ft = (args.ArgC() > 1 ? args[2] : nullptr);
                                            DumpRecvTable(ent, clz->m_pRecvTable, 0, ft, 0);
                                        }
                                    });

static CatCommand debug_print_weaponid("debug_weaponid", "Print the weapon IDs of all currently equiped weapons",
                                       [](const CCommand &)
                                       {
                                           // Invalid player
                                           if (CE_BAD(LOCAL_E))
                                               return;
                                           int *hWeapons = &CE_INT(LOCAL_E, netvar.hMyWeapons);
                                           // Go through the handle array and search for the item
                                           for (int i = 0; hWeapons[i]; ++i)
                                           {
                                               if (IDX_BAD(HandleToIDX(hWeapons[i])))
                                                   continue;
                                               // Get the weapon
                                               CachedEntity *weapon = ENTITY(HandleToIDX(hWeapons[i]));
                                               // Print weaponid
                                               logging::Info("weapon %i: %i", i, re::C_TFWeaponBase::GetWeaponID(RAW_ENT(weapon)));
                                           }
                                       });

#if ENABLE_VISUALS && !ENFORCE_STREAM_SAFETY

typedef void (*UpdateLocalPlayerVisionFlags_t)();
UpdateLocalPlayerVisionFlags_t UpdateLocalPlayerVisionFlags_fn;

int *g_nLocalPlayerVisionFlags;
int *g_nLocalPlayerVisionFlagsWeaponsCheck;

#define access_ptr(p, i) ((uint8_t *) &(p))[i]

static InitRoutine init_pyrovision(
    []()
    {
#if !ENFORCE_STREAM_SAFETY
        EC::Register(
            EC::Paint,
            []()
            {
                if (CE_GOOD(LOCAL_E) && HasCondition<TFCond_HalloweenKartNoTurn>(LOCAL_E))
                    RemoveCondition<TFCond_HalloweenKartNoTurn>(LOCAL_E);
            },
            "remove_cart_cond");
        static BytePatch cart_patch1(CSignature::GetClientSignature, "0F 84 ? ? ? ? F3 0F 10 A2", 0x0, { 0x90, 0xE9 });
        static BytePatch cart_patch2(CSignature::GetClientSignature, "0F 85 ? ? ? ? 89 F8 84 C0 75 72", 0x0, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
#endif
    });
#endif

static CatCommand print_eye_diff("debug_print_eye_diff", "debug", []() { logging::Info("%f", g_pLocalPlayer->v_Eye.z - LOCAL_E->m_vecOrigin().z); });

static CatCommand print_hash("debug_print_hash", "Log the models and hashes of all entities",
                             []()
                             {
                                 for (auto &ent : entity_cache::valid_ents)
                                 {
                                     const model_t *model = RAW_ENT(ent)->GetModel();
                                     const auto szName    = g_IModelInfo->GetModelName(model);
                                     logging::Info("Model name: %s   Hash: %#x", szName, Hash::String(szName));
                                 }
                             });

void Shutdown()
{
#if ENABLE_VISUALS && !ENFORCE_STREAM_SAFETY
    // unpatching local player
    render_zoomed = false;
#endif
}

static ProxyFnHook cyoa_anim_hook{};

void cyoaview_nethook(const CRecvProxyData *data, void *pPlayer, void *out)
{
    int value      = data->m_Value.m_Int;
    int *value_out = (int *) out;
    if (!fix_cyoaanim)
    {
        *value_out = value;
        return;
    }
    // Mark as not doing cyoa thing
    if (CE_BAD(LOCAL_E) || pPlayer != RAW_ENT(LOCAL_E))
        *value_out = false;
}

// This function does two things.
// 1. It patches the check that's supposed to update the "isWaitCommandEnabled" on the cvar buffer to always skip
// 2. It sets the wait command to enabled
// The Former is the main logic, the latter is, so you don't accidentally perma disable it either
inline void force_wait_func(bool after)
{
    static auto enable_wait_signature = CSignature::GetEngineSignature("74 ? A2 ? ? ? ? C7 44 24 ? 01 00 00 00");
    // Jump if not overflow, aka always jump in this case
    static BytePatch patch_wait(enable_wait_signature, { 0x71 });
    if (after)
    {
        // Enable the wait command
        int **enable_wait = (int **) (enable_wait_signature + 3);

        BytePatch::mprotectAddr((uintptr_t) enable_wait, 4, PROT_READ | PROT_WRITE | PROT_EXEC);
        BytePatch::mprotectAddr((uintptr_t) *enable_wait, 4, PROT_READ | PROT_WRITE | PROT_EXEC);

        **enable_wait = true;
        BytePatch::mprotectAddr((uintptr_t) enable_wait, 4, PROT_EXEC);

        patch_wait.Patch();
    }
    else
        patch_wait.Shutdown();
}

void callback_force_wait(settings::VariableBase<bool> &, bool after)
{
    force_wait_func(after);
}

static InitRoutine init(
    []()
    {
        HookNetvar({ "DT_TFPlayer", "m_bViewingCYOAPDA" }, cyoa_anim_hook, cyoaview_nethook);

        force_wait.installChangeCallback(callback_force_wait);
        force_wait_func(true);

        EC::Register(EC::Shutdown, Shutdown, "draw_local_player", EC::average);
        EC::Register(EC::CreateMove_NoEnginePred, CreateMove, "cm_misc_hacks", EC::early);
        EC::Register(EC::CreateMoveWarp, CreateMove, "cmw_misc_hacks", EC::average);
#if ENABLE_VISUALS
        EC::Register(EC::Draw, Draw, "draw_misc_hacks", EC::average);
#if !ENFORCE_STREAM_SAFETY
        if (render_zoomed)
            tryPatchLocalPlayerShouldDraw(true);
        render_zoomed.installChangeCallback([](settings::VariableBase<bool> &, bool after) { tryPatchLocalPlayerShouldDraw(after); });
        uintptr_t addr_scrbrd = CSignature::GetClientSignature("8B 10 89 74 24 04 89 04 24 FF 92 ? ? ? ? 83 F8 02 75 09");

        // Address to the function we need to jump to
        uintptr_t target_addr = e8call_direct(CSignature::GetClientSignature("E8 ? ? ? ? 83 FE 2D"));
        uintptr_t rel_addr    = ((uintptr_t) target_addr - ((uintptr_t) addr_scrbrd + 2)) - 5;

        static BytePatch cyoa_patch{ CSignature::GetClientSignature, "75 ? 80 BB ? ? ? ? 00 74 ? A1 ? ? ? ? 8B 10 C7 44 24", 0, { 0xEB } };
        cyoa_patch.Patch();
        EC::Register(
            EC::Shutdown,
            []()
            {
                cyoa_patch.Shutdown();
                tryPatchLocalPlayerShouldDraw(false);
                force_wait_func(false);
            },
            "shutdown_stealthkill");
#endif
#endif
    });
} // namespace hacks::misc
