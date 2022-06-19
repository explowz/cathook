#include <mlpack/core.hpp>
#include <mlpack/prereqs.hpp>
#include <mlpack/methods/linear_regression/linear_regression.hpp>
#include "playerresource.h"
#include "entitycache.hpp"

void* mlpack_harvest(void* args);

void* mlpack_harvest(void* args){
    
        g_pPlayerResource->Update();
        int *blue_team_vars[20];
        int *red_team_vars[20];
        int loop_int_blue=0;
        int loop_int_red =0;
        for(int i=1; i<64;i++){
            CachedEntity* ent= ENTITY(i);
            if(CE_BAD(ent) || ent->m_Type()!= ENTITY_PLAYER){continue;}
            
        int curr_class= g_pPlayerResource->GetClass(ent);
        int curr_team=  g_pPlayerResource->GetTeam(i);
        int curr_score= g_pPlayerResource->GetScore(i);
        int curr_kills= g_pPlayerResource->GetKills(i);
        int curr_deaths=g_pPlayerResource->GetDeaths(i);
        int curr_level=  g_pPlayerResource->GetLevel(i);
        int curr_damage= g_pPlayerResource->GetDamage(i);
        int curr_account_id = g_pPlayerResource->GetAccountID(i);
        int curr_ping= g_pPlayerResource->GetPing(i);
        int arr_of_vars[10] = {curr_class, curr_team, curr_score, curr_kills, curr_deaths, curr_level, curr_damage, curr_account_id, curr_ping};
        switch(curr_team){
            case 2:{
                blue_team_vars[loop_int_blue]=arr_of_vars;
                loop_int_blue++;
            }
            case 3:{
                red_team_vars[loop_int_red]=arr_of_vars;
                loop_int_red++;

            }
        }
        }
    
        
}       