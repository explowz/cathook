/*
 * CGameRules.h
 *
 *  Created on: Nov 23, 2017
 *      Author: nullifiedcat
 */

/*
 * !! READ ME BEFORE ADDING NEW MEMBERS !!
 * Padding should only be done using arrays of chars in order to keep things as simple as possible.
 *
 * If we're trying to add a new boolean member, then keep in mind that Little-endian is used,
 * meaning that the byte the boolean uses is stored at the smallest address. The remaining 3 bytes are usually
 * useless, and must be properly padded using "bool_pad". Do NOT include the boolean padding into a general "pad" padding array.
 * */

#pragma once

bool TF_IsHolidayActive(/*EHoliday*/ int eHoliday);

class CGameRules
{
public:
    enum gamerules_roundstate_t
    {
        // initialize the game, create teams
        GR_STATE_INIT = 0,

        // Before players have joined the game. Periodically checks to see if enough players are ready
        // to start a game. Also reverts to this when there are no active players
        GR_STATE_PREGAME,

        // The game is about to start, wait a bit and spawn everyone
        GR_STATE_STARTGAME,

        // All players are respawned, frozen in place
        GR_STATE_PREROUND,

        // Round is on, playing normally
        GR_STATE_RND_RUNNING,

        // Someone has won the round
        GR_STATE_TEAM_WIN,

        // No one has won, manually restart the game, reset scores
        GR_STATE_RESTART,

        // No one has won, restart the game
        GR_STATE_STALEMATE,

        // Game is over, showing the scoreboard etc
        GR_STATE_GAME_OVER,

        // Game is in a bonus state, transitioned to after a round ends
        GR_STATE_BONUS,

        // Game is awaiting the next wave/round of a multi round experience
        GR_STATE_BETWEEN_RNDS,

        GR_NUM_ROUND_STATES
    };

    enum HalloweenScenarioType
    {
        HALLOWEEN_SCENARIO_NONE = 0,
        HALLOWEEN_SCENARIO_MANN_MANOR,
        HALLOWEEN_SCENARIO_VIADUCT,
        HALLOWEEN_SCENARIO_LAKESIDE,
        HALLOWEEN_SCENARIO_HIGHTOWER,
        HALLOWEEN_SCENARIO_DOOMSDAY
    };

    // Stored value, don't re-order
    enum EMatchGroup
    {
        k_nMatchGroup_Invalid = -1,
        k_nMatchGroup_First   = 0,

        k_nMatchGroup_MvM_Practice = 0,
        k_nMatchGroup_MvM_MannUp,

        k_nMatchGroup_Ladder_6v6,
        k_nMatchGroup_Ladder_9v9,
        k_nMatchGroup_Ladder_12v12,

        k_nMatchGroup_Casual_6v6,
        k_nMatchGroup_Casual_9v9,
        k_nMatchGroup_Casual_12v12,

        k_nMatchGroup_Count
    };

    //-----------------------------------------------------------------------------
    // List of holidays. These are sorted by priority. Needs to match static IIsHolidayActive *s_HolidayChecks
    //-----------------------------------------------------------------------------
    enum EHoliday
    {
        kHoliday_None = 0, // must stay at zero for backwards compatibility
        kHoliday_TFBirthday,
        kHoliday_Halloween,
        kHoliday_Christmas,
        kHoliday_CommunityUpdate,
        kHoliday_EOTL,
        kHoliday_Valentines,
        kHoliday_MeetThePyro,
        kHoliday_FullMoon,
        kHoliday_HalloweenOrFullMoon,
        kHoliday_HalloweenOrFullMoonOrValentines,
        kHoliday_AprilFools,
        kHolidayCount
    };

    char pad0[68];                             // 0    | 68 bytes   | 68
    gamerules_roundstate_t m_iRoundState;      // 68   | 4 bytes    | 72
    char pad1[1];                              // 72   | 1 byte     | 73
    bool m_bInSetup;                           // 73   | 1 byte     | 74
    bool m_bSwitchedTeamsThisRound;            // 74   | 1 byte     | 75
    char pad2[1];                              // 75   | 1 byte     | 76
    int m_iWinningTeam;                        // 76   | 4 bytes    | 80
    char pad3[4];                              // 80   | 4 bytes    | 84
    bool m_bInWaitingForPlayers;               // 84   | 1 byte     | 85
    bool bool_pad0[3];                         // 85   | 3 bytes    | 88
    char pad4[1037];                           // 88   | 1037 bytes | 1125
    bool m_bPlayingSpecialDeliveryMode;        // 1125 | 1 byte     | 1126
    bool m_bPlayingMannVsMachine;              // 1126 | 1 byte     | 1127
    char bool_pad1[3];                         // 1127 | 3 bytes    | 1130
    bool m_bCompetitiveMode;                   // 1130 | 1 byte     | 1131
    char bool_pad2[3];                         // 1131 | 3 bytes    | 1134
    char pad5[8];                              // 1134 | 8 bytes    | 1142
    bool m_bIsUsingSpells;                     // 1142 | 1 byte     | 1143
    bool m_bTruceActive;                       // 1143 | 1 byte     | 1144
    char bool_pad3[2];                         // 1144 | 2 bytes    | 1146
    char pad6[10];                             // 1146 | 10 bytes   | 1156
    int m_nMapHolidayType;                     // 1156 | 4 bytes    | 1160
    char pad7[1048];                           // 1160 | 1048 bytes | 2208
    HalloweenScenarioType m_halloweenScenario; // 2208 | 4 bytes    | 2212

    int GetWinningTeam() const
    {
        return m_iWinningTeam;
    }

    inline gamerules_roundstate_t State_Get() const
    {
        return m_iRoundState;
    }

    bool RoundHasBeenWon() const
    {
        return State_Get() == GR_STATE_TEAM_WIN;
    }

    bool InRoundRestart() const
    {
        return State_Get() == GR_STATE_PREROUND;
    }

    bool InStalemate() const
    {
        return State_Get() == GR_STATE_STALEMATE;
    }

    // Return false if players aren't allowed to cap points at this time (i.e. in WaitingForPlayers)
    bool PointsMayBeCaptured() const
    {
        return (State_Get() == GR_STATE_RND_RUNNING || InStalemate()) && !IsInWaitingForPlayers();
    }

    inline bool IsHalloweenScenario(HalloweenScenarioType scenario) const
    {
        return m_halloweenScenario == scenario;
    }

    bool IsHolidayActive(/*EHoliday*/ int eHoliday) const
    {
        return TF_IsHolidayActive(eHoliday);
    }

    bool IsUsingSpells() const
    {
        auto tf_spells_enabled = g_ICvar->FindVar("tf_spells_enabled");
        if (tf_spells_enabled->GetBool())
            return true;

        // Hightower
        if (IsHalloweenScenario(HALLOWEEN_SCENARIO_HIGHTOWER))
            return true;

        return m_bIsUsingSpells;
    }

    bool IsMannVsMachineMode() const
    {
        return m_bPlayingMannVsMachine;
    }

    bool IsPlayingSpecialDeliveryMode() const
    {
        return m_bPlayingSpecialDeliveryMode;
    }

    bool IsHolidayMap(int nHoliday) const
    {
        return m_nMapHolidayType == nHoliday;
    }

    bool IsTruceActive() const
    {
        return m_bTruceActive;
    }

    bool InSetup() const
    {
        return m_bInSetup;
    }

    bool IsInWaitingForPlayers() const
    {
        return m_bInWaitingForPlayers;
    }

    bool SwitchedTeamsThisRound() const
    {
        return m_bSwitchedTeamsThisRound;
    }

    HalloweenScenarioType GetHalloweenScenario() const;
} __attribute__((packed));

//-----------------------------------------------------------------------------
// Gets us the team fortress game rules
//-----------------------------------------------------------------------------
inline CGameRules *TFGameRules()
{
    return static_cast<CGameRules *>(g_pGameRules);
}
