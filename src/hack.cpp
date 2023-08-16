/*
 * hack.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include <csignal>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <boost/stacktrace.hpp>
#include <visual/SDLHooks.hpp>

#ifdef __RDSEED__ // Used for InitRandom()
#include <x86intrin.h>
#else /* __RDSEED__ */
#include <random>
#include <fstream>
#include <chrono>
#include <sys/types.h>
#include <unistd.h>
#endif /* __RDSEED__ */

#include "hack.hpp"
#include "common.hpp"
#if ENABLE_GUI
#include "menu/GuiInterface.hpp"
#endif
#include <pwd.h>
#include <iostream>

#include "teamroundtimer.hpp"
#if EXTERNAL_DRAWING
#include "xoverlay.h"
#endif

#include "copypasted/CDumper.hpp"
#include "version.h"

/*
 *  Credits to josh33901 aka F1ssi0N for butifel F1Public and Darkstorm 2015
 * Linux
 */

// game_shutdown = Is full game shutdown or just detach
bool hack::game_shutdown = true;
bool hack::shutdown      = false;
bool hack::initialized   = false;

const std::string &hack::GetVersion()
{
    static std::string version("Unknown Version");
    if (version != "Unknown Version")
        return version;
#if defined(GIT_COMMIT_HASH) && defined(GIT_COMMITTER_DATE)
    version = "Version: #" GIT_COMMIT_HASH " " GIT_COMMITTER_DATE;
#endif
    return version;
}

const std::string &hack::GetType()
{
    static std::string version("Unknown Type");
    if (version != "Unknown Type")
        return version;
    version = "";
#if !ENABLE_IPC
    version += " NOIPC";
#endif
#if !ENABLE_GUI
    version += " NOGUI";
#else
    version += " GUI";
#endif

#if !ENABLE_VISUALS
    version += " NOVISUALS";
#endif

    version = version.substr(1);
    return version;
}

std::mutex hack::command_stack_mutex;
std::stack<std::string> &hack::command_stack()
{
    static std::stack<std::string> stack;
    return stack;
}

void hack::ExecuteCommand(const std::string &command)
{
    std::lock_guard<std::mutex> guard(hack::command_stack_mutex);
    hack::command_stack().push(command);
}

#if ENABLE_LOGGING
void critical_error_handler(int signum)
{
    std::signal(signum, SIG_DFL);

    passwd *pwd = getpwuid(getuid());
    if (!pwd)
    {
        std::cerr << "Critical error: getpwuid failed\n";
        std::abort();
    }

    std::ofstream out(strfmt("/tmp/cathook-%s-%d-segfault.log", pwd->pw_name, getpid()).get());
    if (!out)
    {
        std::cerr << "Critical error: cannot open log file\n";
        std::abort();
    }

    Dl_info info;
    if (!dladdr(reinterpret_cast<void *>(hack::ExecuteCommand), &info))
    {
        std::cerr << "Critical error: dladdr failed\n";
        std::abort();
    }

    for (auto i : boost::stacktrace::stacktrace())
    {
        Dl_info info2;
        if (dladdr(i.address(), &info2))
        {
            uintptr_t offset = reinterpret_cast<uintptr_t>(i.address()) - reinterpret_cast<uintptr_t>(info2.dli_fbase);
            out << (!strcmp(info2.dli_fname, info.dli_fname) ? "cathook" : info2.dli_fname) << '\t' << reinterpret_cast<void *>(offset) << std::endl;
        }
    }

    out.close();
    std::abort();
}
#endif

static void InitRandom()
{
#ifdef __RDSEED__
    unsigned int seed;
    do
    {
    } while (!_rdseed32_step(&seed));
    srand(seed);
#else
    std::random_device rd;
    unsigned int rand_seed = rd();

    if (rd.entropy() == 0) // Check if the random device is not truly random
    {
        logging::Info("Warning! Failed to read from random_device. Randomness is going to be weak.");

        auto now         = std::chrono::high_resolution_clock::now().time_since_epoch();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        rand_seed        = static_cast<unsigned int>(nanoseconds ^ nanoseconds & getpid());
    }

    // Seed random number generator
    srand(rand_seed);
#endif
}

void hack::Hook()
{
    uintptr_t *clientMode;
    // Bad way to get clientmode.
    // FIXME [MP]?
    while (!(clientMode = **(uintptr_t ***) ((uintptr_t) (*(void ***) g_IBaseClient)[10] + 1)))
        usleep(10000);
    hooks::clientmode.Set((void *) clientMode);
    hooks::clientmode.HookMethod(HOOK_ARGS(CreateMove));
#if ENABLE_VISUALS
    hooks::clientmode.HookMethod(HOOK_ARGS(OverrideView));
#endif
    hooks::clientmode.HookMethod(HOOK_ARGS(LevelInit));
    hooks::clientmode.HookMethod(HOOK_ARGS(LevelShutdown));
    hooks::clientmode.Apply();

    hooks::clientmode4.Set((void *) clientMode, 4);
    hooks::clientmode4.HookMethod(HOOK_ARGS(FireGameEvent));
    hooks::clientmode4.Apply();

    hooks::client.Set(g_IBaseClient);
    hooks::client.HookMethod(HOOK_ARGS(DispatchUserMessage));
#if ENABLE_VISUALS
    hooks::client.HookMethod(HOOK_ARGS(FrameStageNotify));
    hooks::client.HookMethod(HOOK_ARGS(IN_KeyEvent));
#endif
    hooks::client.Apply();

#if ENABLE_VISUALS || ENABLE_TEXTMODE
    hooks::panel.Set(g_IPanel);
    hooks::panel.HookMethod(hooked_methods::methods::PaintTraverse, offsets::PaintTraverse(), &hooked_methods::original::PaintTraverse);
    hooks::panel.Apply();
#endif

    hooks::vstd.Set((void *) g_pUniformStream);
    hooks::vstd.HookMethod(HOOK_ARGS(RandomInt));
    hooks::vstd.Apply();
#if ENABLE_VISUALS
    CHudElement *chat_hud;
    while (!(chat_hud = g_CHUD->FindElement("CHudChat")))
        usleep(1000);
    hooks::chathud.Set(chat_hud);
    hooks::chathud.HookMethod(HOOK_ARGS(StartMessageMode));
    hooks::chathud.HookMethod(HOOK_ARGS(StopMessageMode));
    hooks::chathud.HookMethod(HOOK_ARGS(ChatPrintf));
    hooks::chathud.Apply();
#endif

    hooks::input.Set(g_IInput);
    hooks::input.HookMethod(HOOK_ARGS(GetUserCmd));
    hooks::input.HookMethod(HOOK_ARGS(CreateMoveInput));
    hooks::input.Apply();

#if ENABLE_VISUALS || ENABLE_TEXTMODE
    hooks::modelrender.Set(g_IVModelRender);
    hooks::modelrender.HookMethod(HOOK_ARGS(DrawModelExecute));
    hooks::modelrender.Apply();
#endif
    hooks::enginevgui.Set(g_IEngineVGui);
    hooks::enginevgui.HookMethod(HOOK_ARGS(Paint));
    hooks::enginevgui.Apply();

    hooks::engine.Set(g_IEngine);
    hooks::engine.HookMethod(HOOK_ARGS(ServerCmdKeyValues));
    hooks::engine.Apply();

    hooks::cmdclientunrestricted.Set(g_IEngine);
    hooks::cmdclientunrestricted.HookMethod(HOOK_ARGS(ClientCmd_Unrestricted));
    hooks::cmdclientunrestricted.Apply();

    hooks::demoplayer.Set(demoplayer);
    hooks::demoplayer.HookMethod(HOOK_ARGS(IsPlayingTimeDemo));
    hooks::demoplayer.Apply();

    hooks::eventmanager2.Set(g_IEventManager2);
    hooks::eventmanager2.HookMethod(HOOK_ARGS(FireEvent));
    hooks::eventmanager2.HookMethod(HOOK_ARGS(FireEventClientSide));
    hooks::eventmanager2.Apply();

    hooks::steamfriends.Set(g_ISteamFriends);
    hooks::steamfriends.HookMethod(HOOK_ARGS(GetFriendPersonaName));
    hooks::steamfriends.Apply();

    hooks::soundclient.Set(g_ISoundEngine);
    hooks::soundclient.HookMethod(HOOK_ARGS(EmitSound1));
    hooks::soundclient.HookMethod(HOOK_ARGS(EmitSound2));
    hooks::soundclient.HookMethod(HOOK_ARGS(EmitSound3));
    hooks::soundclient.Apply();

    hooks::prediction.Set(g_IPrediction);
    hooks::prediction.HookMethod(HOOK_ARGS(RunCommand));
    hooks::prediction.Apply();

    hooks::toolbox.Set(g_IToolFramework);
    hooks::toolbox.HookMethod(HOOK_ARGS(Think));
    hooks::toolbox.Apply();

#if ENABLE_VISUALS
    sdl_hooks::applySdlHooks();
#endif

    // FIXME [MP]
    logging::Info("Hooked!");
}

void hack::Initialize()
{
#if ENABLE_LOGGING
    ::signal(SIGSEGV, &critical_error_handler);
    ::signal(SIGABRT, &critical_error_handler);
#endif
    time_injected = time(nullptr);
/*passwd *pwd   = getpwuid(getuid());
char *logname = strfmt("/tmp/cathook-game-stdout-%s-%u.log", pwd->pw_name,
time_injected);
freopen(logname, "w", stdout);
free(logname);
logname = strfmt("/tmp/cathook-game-stderr-%s-%u.log", pwd->pw_name,
time_injected);
freopen(logname, "w", stderr);
free(logname);*/
// Essential files must always exist, except when the game is running in text
// mode.
#if ENABLE_VISUALS
    {
        std::vector<std::string> essential = { "fonts/tf2build.ttf" };
        for (const auto &s : essential)
        {
            std::ifstream exists(paths::getDataPath("/" + s), std::ios::in);
            if (!exists)
            {
                Error(("Missing essential file: " + s + "/%s\nYou MUST run install-data script to finish installation").c_str(), s.c_str());
            }
        }
    }
#endif
    logging::Info("Initializing...");
    InitRandom();
    sharedobj::LoadLauncher();

#if !ENABLE_TEXTMODE
    // remove epic source lock (needed for non-preload tf2)
    std::remove("/tmp/source_engine_2925226592.lock");
#endif

    sharedobj::LoadEarlyObjects();

#if ENABLE_TEXTMODE
    // Fix locale issues caused by steam update
    static BytePatch patch(CSignature::GetEngineSignature, "74 ? 89 5C 24 ? 8D 9D ? ? ? ? 89 74 24", 0, { 0x71 });
    patch.Patch();

    // Remove intro video which also causes some crashes
    static BytePatch patch_intro_video(CSignature::GetEngineSignature, "55 89 E5 57 56 53 83 EC 5C 8B 5D ? 8B 55", 0x9, { 0x83, 0xc4, 0x5c, 0x5b, 0x5e, 0x5f, 0x5d, 0xc3 });
    patch_intro_video.Patch();
#endif

    CreateEarlyInterfaces();

    // Applying the defaults needs to be delayed, because preloaded Cathook can not properly convert SDL codes to names before TF2 init
    settings::Manager::instance().applyDefaults();

    logging::Info("Clearing Early initializer stack");
    while (!init_stack_early().empty())
    {
        init_stack_early().top()();
        init_stack_early().pop();
    }
    logging::Info("Early Initializer stack done");
    sharedobj::LoadAllSharedObjects();
    CreateInterfaces();
#if ENABLE_LOGGING
    CDumper dumper;
    dumper.SaveDump();
#endif
    InitClassTable();

    BeginConVars();
    g_Settings.Init();
    EndConVars();

#if ENABLE_VISUALS
    draw::Initialize();
#if ENABLE_GUI
// FIXME put gui here
#endif
#endif

    gNetvars.init();
    InitNetVars();
    g_pLocalPlayer    = new LocalPlayer();
    g_pPlayerResource = new TFPlayerResource();
    g_pTeamRoundTimer = new CTeamRoundTimer();

    velocity::Init();
    playerlist::Load();

#if ENABLE_VISUALS
    InitStrings();
#endif /* TEXTMODE */
    logging::Info("Clearing initializer stack");
    while (!init_stack().empty())
    {
        init_stack().top()();
        init_stack().pop();
    }
    logging::Info("Initializer stack done");
#if ENABLE_TEXTMODE
    hack::command_stack().emplace("exec cat_autoexec_textmode");
    hack::command_stack().emplace("exec betrayals");
#else
    hack::command_stack().emplace("exec cat_autoexec");
#endif
    auto extra_exec = std::getenv("CH_EXEC");
    if (extra_exec)
        hack::command_stack().emplace(extra_exec);

    hack::initialized = true;
    for (int i = CGameRules::k_nMatchGroup_First; i < CGameRules::k_nMatchGroup_Count; ++i)
    {
        re::ITFMatchGroupDescription *desc = re::GetMatchGroupDescription(static_cast<CGameRules::EMatchGroup>(i));
        if (!desc || desc->m_iID > 9) // ID's over 9 are invalid
            continue;
        if (desc->m_bForceCompetitiveSettings)
        {
            desc->m_bForceCompetitiveSettings = false;
            logging::Info("Bypassed force competitive cvars!");
        }
    }
    hack::Hook();
}

void hack::Think()
{
    usleep(250000);
}

void hack::Shutdown()
{
    if (hack::shutdown)
        return;
    hack::shutdown = true;
    // Stop cathook stuff
    settings::cathook_disabled.store(true);
    playerlist::Save();
#if ENABLE_VISUALS
    sdl_hooks::cleanSdlHooks();
#endif
    logging::Info("Unregistering convars..");
    ConVar_Unregister();
    logging::Info("Unloading sharedobjects..");
    sharedobj::UnloadAllSharedObjects();
    logging::Info("Deleting global interfaces...");
    delete g_pLocalPlayer;
    delete g_pTeamRoundTimer;
    delete g_pPlayerResource;
#if ENABLE_GUI
    logging::Info("Shutting down GUI");
    gui::shutdown();
#endif
    if (!hack::game_shutdown)
    {
        logging::Info("Running shutdown handlers");
        EC::run(EC::Shutdown);
#if ENABLE_VISUALS
        g_pScreenSpaceEffects->DisableScreenSpaceEffect("_cathook_glow");
#if EXTERNAL_DRAWING
        xoverlay_destroy();
#endif
#endif
    }
    logging::Info("Releasing VMT hooks..");
    hooks::ReleaseAllHooks();
    logging::Info("Success..");
}
