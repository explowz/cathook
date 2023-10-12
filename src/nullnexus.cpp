#include "config.h"
#if ENABLE_NULLNEXUS
#include "libnullnexus/nullnexus.hpp"
#include "DetourHook.hpp"
#include "nullnexus.hpp"
#if ENABLE_VISUALS
#include "colors.hpp"
#include "MiscTemporary.hpp"
#endif

namespace nullnexus
{
static settings::Boolean enabled("nullnexus.enabled", "true");
static settings::Boolean anon("nullnexus.user.anon", "false");
static settings::String address("nullnexus.host", "nullnexus.cathook.club");
static settings::String port("nullnexus.port", "3000");
static settings::String endpoint("nullnexus.endpoint", "/api/v1/client");
#if ENABLE_TEXTMODE
static settings::Boolean proxyenabled("nullnexus.proxy.enabled", "true");
#else
static settings::Boolean proxyenabled("nullnexus.proxy.enabled", "false");
#endif
static settings::String proxysocket("nullnexus.relay.socket", "/tmp/nullnexus.sock");
static settings::Boolean authenticate("nullnexus.auth", "true");
#if ENABLE_VISUALS
static settings::Rgba colour("nullnexus.user.colour");
#endif

static NullNexus nexus;

void printmsg(std::string &usr, std::string &msg, int colour = 0xe1ad01)
{
#if !ENFORCE_STREAM_SAFETY && ENABLE_VISUALS
    if (msg.size() > 128 || usr.size() > 32)
    {
        logging::Info("Nullnexus: Message too large.");
        return;
    }
    if (g_Settings.bInvalid)
        g_ICvar->ConsoleColorPrintf(MENU_COLOR, "[Nullnexus] %s: %s\n", usr.c_str(), msg.c_str());
    else
        PrintChat("\x07%06X[Nullnexus] \x07%06X%s\x01: %s", 0x1434a4, colour, usr.c_str(), msg.c_str());
#endif
}

void printmsgcopy(std::string usr, std::string msg)
{
    printmsg(usr, msg);
}

namespace handlers
{
void message(std::string usr, std::string msg, int colour)
{
    printmsg(usr, msg, colour);
}

void authedplayers(std::vector<std::string> steamids)
{
    // Check if we are in a game
    if (g_Settings.bInvalid)
        return;
    for (unsigned i = 0; i <= g_GlobalVars->maxClients; i++)
    {
        player_info_s pinfo{};
        if (GetPlayerInfo(i, &pinfo))
        {
            if (pinfo.friendsID == 0)
                continue;
            MD5Value_t result{};
            std::string steamidhash = std::to_string(pinfo.friendsID) + pinfo.name;
            MD5_ProcessSingleBuffer(steamidhash.c_str(), strlen(steamidhash.c_str()), result);
            std::stringstream ss;
            ss << std::hex;
            for (auto i : result.bits)
                ss << std::setw(2) << std::setfill('0') << (int) i;
            steamidhash = ss.str();
            std::remove_if(steamids.begin(), steamids.end(),
                           [&steamidhash, &pinfo](std::string &steamid)
                           {
                               if (steamid == steamidhash)
                               {
                                   // Use actual steamid to set cat status
                                   if (playerlist::ChangeState(pinfo.friendsID, playerlist::k_EState::CAT))
                                       PrintChat("Detected \x07%06X%s\x01 as a Nullnexus user.", 0xe1ad01, pinfo.name);
                                   return true;
                               }
                               return false;
                           });
        }
    }
}
} // namespace handlers

static std::string server_steamid;

// Update info about the current server we are on.
void updateServer(NullNexus::UserSettings &settings)
{
    auto *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    // Additional currently inactive security measure, may be activated at any time
    static int *gHostSpawnCount = *reinterpret_cast<int **>(CSignature::GetEngineSignature("A3 ? ? ? ? A1 ? ? ? ? 8B 10 89 04 24 FF 52 ? 83 C4 2C") + sizeof(char));
    if (ch && *authenticate && !server_steamid.empty())
    {
        // SDR Makes this unusable :(
        // auto addr = ch->GetRemoteAddress();

        player_info_s pinfo{};
        if (GetPlayerInfo(g_pLocalPlayer->entity_idx, &pinfo))
        {
            MD5Value_t result{};
            std::string steamidhash = std::to_string(pinfo.friendsID) + pinfo.name;
            MD5_ProcessSingleBuffer(steamidhash.c_str(), strlen(steamidhash.c_str()), result);
            std::stringstream ss;
            ss << std::hex;
            for (auto i : result.bits)
                ss << std::setw(2) << std::setfill('0') << (int) i;
            steamidhash        = ss.str();
            settings.tf2server = NullNexus::TF2Server(true, server_steamid, steamidhash, *gHostSpawnCount);
            return;
        }
    }
    // Not connected
    settings.tf2server = NullNexus::TF2Server(false);
}

static bool waiting_status_data = false;

static DetourHook ProcessPrint_detour_hook;

typedef bool *(*ProcessPrint_t)(void *baseclient, SVC_Print *msg);

// Need to do this so the function below resolves
void updateServer();

bool ProcessPrint_detour_fn(void *baseclient, SVC_Print *msg)
{
    if (waiting_status_data && msg->m_szText)
    {
        auto msg_str = std::string(msg->m_szText);
        std::vector<std::string> lines;

        std::istringstream tokenStream(msg_str);
        std::string line;

        while (std::getline(tokenStream, line))
        {
            if (!line.empty())
            {
                lines.push_back(line);
            }
        }

        auto str = lines[0];

        if (str.rfind("steamid : ", 0) == 0)
        {
            waiting_status_data = false;

            if (str.length() < 12)
                server_steamid = "";
            else
            {
                str = str.substr(10);
                if (str == "not logged in" || str.rfind(']') == __gnu_cxx::__alloc_traits<std::allocator<std::basic_string<char>>, std::basic_string<char>>::value_type::npos)
                    str = "";
                else
                    str = str.substr(0, str.rfind(']') + 1);
                server_steamid = str;
            }
            updateServer();
        }
    }

    auto original = (ProcessPrint_t) ProcessPrint_detour_hook.GetOriginalFunc();
    auto ret_val  = original(baseclient, msg);
    ProcessPrint_detour_hook.RestorePatch();
    return ret_val;
}

// Update server steamid
void updateSteamID()
{
    waiting_status_data = true;
    g_IEngine->ServerCmd("status");
}

// Update info about the current server we are on.
void updateServer()
{
    if (!g_IEngine->IsInGame() || !server_steamid.empty())
    {
        NullNexus::UserSettings settings;
        updateServer(settings);
        nexus.changeData(settings);
    }
    else
        updateSteamID();
}

void updateData()
{
    std::string username = *anon ? "anon" : g_ISteamFriends->GetPersonaName();
    std::optional<int> newcolour        = std::nullopt;
#if ENABLE_VISUALS
    if ((*colour).r || (*colour).g || (*colour).b)
    {
        int r     = (*colour).r * 255;
        int g     = (*colour).g * 255;
        int b     = (*colour).b * 255;
        newcolour = (r << 16) + (g << 8) + b;
    }
#endif
    NullNexus::UserSettings settings;
    settings.username = username;
    settings.colour   = newcolour;
    // Tell nullnexus about the current server we are connected to.
    updateServer(settings);

    nexus.changeData(settings);
}

bool sendmsg(std::string &msg)
{
    if (!enabled)
    {
        printmsgcopy("Cathook", "Error! Nullnexus is disabled!");
        return false;
    }
    if (nexus.sendChat(msg))
        return true;
    printmsgcopy("Cathook", "Error! Couldn't send message.");
    return false;
}

template <typename T> void rvarCallback(settings::VariableBase<T> &, T)
{
    std::thread reload(
        []()
        {
            std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
            updateData();
            if (*enabled)
            {
                if (*proxyenabled)
                    nexus.connectunix(*proxysocket, *endpoint, true);
                else
                    nexus.connect(*address, *port, *endpoint, true);
            }
            else
                nexus.disconnect();
        });
    reload.detach();
}

template <typename T> void rvarDataCallback(settings::VariableBase<T> &, T)
{
    std::thread reload(
        []()
        {
            std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
            updateData();
        });
    reload.detach();
}

static InitRoutine init(
    []()
    {
        updateData();
        enabled.installChangeCallback(rvarCallback<bool>);
        address.installChangeCallback(rvarCallback<std::string>);
        port.installChangeCallback(rvarCallback<std::string>);
        endpoint.installChangeCallback(rvarCallback<std::string>);

        proxyenabled.installChangeCallback(rvarCallback<bool>);
        proxysocket.installChangeCallback(rvarCallback<std::string>);

#if ENABLE_VISUALS
        colour.installChangeCallback(rvarDataCallback<rgba_t>);
#endif
        anon.installChangeCallback(rvarDataCallback<bool>);
        authenticate.installChangeCallback(rvarDataCallback<bool>);

        nexus.setHandlerChat(handlers::message);
        nexus.setHandlerAuthedplayers(handlers::authedplayers);
        if (*enabled)
        {
            if (*proxyenabled)
                nexus.connectunix(*proxysocket, *endpoint, true);
            else
                nexus.connect(*address, *port, *endpoint, true);
        }

    // Search for the following string: /home/buildbot/buildslave/rel_hl2_client_linux/build/src/engine/baseclientstate.cpp
    // How the function looked at the time that this signature was created (29.07.2023):
#pragma region Disassembled Function Using Ghidra
    /*
    undefined4 FUN_002bbf40(int param_1,int param_2)
    {
        int iVar1;
        char cVar2;
        int iVar3;
        undefined4 local_3c;
        undefined4 local_38;
        undefined4 local_2c;
        undefined4 local_28;
        int local_24;

        local_3c = 0;
        local_38 = 0;
        if (_exit == 0) {
            local_24 = 0;
            local_2c = 0;
            local_28 = 0;
            if (_DAT_00ae7030 != 0) goto LAB_002bc030;
        }
        else {
            (**(code **)(_exit + 0x50))
                (_exit,&local_3c,0,0,0,0,
                 "/home/buildbot/buildslave/rel_hl2_client_linux/build/src/engine/baseclientstate.cpp"
                 ,0x52f,&DAT_008b4050,"(%s)%s","Unaccounted","ProcessSetPause");
            local_24 = _exit;
            local_2c = local_3c;
            local_28 = local_38;
            if (_DAT_00ae7030 != 0) {
            LAB_002bc030:
                iVar1 = _DAT_00ae79dc;
                local_3c = local_2c;
                local_38 = local_28;
                */
    /* try { // try from 002bc036 to 002bc03a has its CatchHandler @ 002bc0f6 */ /*
   iVar3 = ThreadGetCurrentId();
   if (iVar1 == iVar3) {
       if (*_DAT_00ae7038 != "ProcessSetPause") {
           */
    /* try { // try from 002bc0d8 to 002bc0e9 has its CatchHandler @ 002bc0f6 */ /*
   _DAT_00ae7038 =
       (char **)CVProfNode::GetSubNode
       ((char *)_DAT_00ae7038,(int)"ProcessSetPause",&DAT_00000001,
        (int)"Unaccounted");
}
CVProfNode::EnterScope();
DAT_00ae7034 = '\0';
}
*(undefined *)(param_1 + 0x19c) = *(undefined *)(param_2 + 0x14);
iVar1 = _DAT_00ae79dc;
if (((DAT_00ae7034 == '\0') || (_DAT_00ae7030 != 0)) &&
(iVar3 = ThreadGetCurrentId(), iVar1 == iVar3)) {
cVar2 = CVProfNode::ExitScope();
if (cVar2 != '\0') {
   _DAT_00ae7038 = (char **)_DAT_00ae7038[0x19];
}
DAT_00ae7034 = _DAT_00ae7038 == (char **)0xae703c;
}
goto LAB_002bbff0;
}
}
*(undefined *)(param_1 + 0x19c) = *(undefined *)(param_2 + 0x14);
local_3c = local_2c;
local_38 = local_28;
LAB_002bbff0:
if (local_24 != 0) {
(**(code **)(local_24 + 0x54))(local_24,local_2c,local_28,0,0,0);
}
return 1;
}
*/
#pragma endregion
        uintptr_t processprint_addr = CSignature::GetEngineSignature("55 89 E5 57 56 53 83 EC 5C C7 45 ? 00 00 00 00 A1 ? ? ? ? C7 45 ? 00 00 00 00 8B 5D ? 8B 75 ? 85 C0 0F 84 ? ? ? ? 8D 55 ? 89 04 24 89 54 24 ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? ? ? ? ? C7 44 24 ? 2F 05 00 00");

        ProcessPrint_detour_hook.Init(processprint_addr, (void *) ProcessPrint_detour_fn);

        EC::Register(
            EC::Shutdown,
            []()
            {
                nexus.disconnect();
                ProcessPrint_detour_hook.Shutdown();
            },
            "SHUTDOWN_Nullnexus");

        EC::Register(
            EC::FirstCM, []() { updateServer(); }, "FIRSTCM_nullnexus");

        EC::Register(
            EC::LevelShutdown,
            []()
            {
                server_steamid = "";
                updateServer();
            },
            "RESET_nullnexus");
    });

static CatCommand nullnexus_send("nullnexus_send", "Send message to IRC",
                                 [](const CCommand &args)
                                 {
                                     std::string msg(args.ArgS());
                                     sendmsg(msg);
                                 });
} // namespace nullnexus
#endif
