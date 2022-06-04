/*
 * entitycache.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include <time.h>
#include <settings/Float.hpp>
#include "soundcache.hpp"

bool IsProjectileACrit(CachedEntity *ent)
{
    if (ent->m_bGrenadeProjectile())
        return CE_BYTE(ent, netvar.Grenade_bCritical);
    return CE_BYTE(ent, netvar.Rocket_bCritical);
}
// This method of const'ing the index is weird.
CachedEntity::CachedEntity() : m_IDX(int(((unsigned) this - (unsigned) &entity_cache::array) / sizeof(CachedEntity))), hitboxes(hitbox_cache::Get(unsigned(m_IDX)))
{
#if PROXY_ENTITY != true
    m_pEntity = nullptr;
#endif
    m_fLastUpdate = 0.0f;
}

void CachedEntity::Reset()
{
    m_lLastSeen         = 0;
    m_lSeenTicks        = 0;
    memset(&player_info, 0, sizeof(player_info_s));
    m_vecAcceleration.Zero();
    m_vecVOrigin.Zero();
    m_vecVelocity.Zero();
    m_fLastUpdate = 0;
}

CachedEntity::~CachedEntity()
{
}

static settings::Float ve_window{ "debug.ve.window", "0" };
static settings::Boolean ve_smooth{ "debug.ve.smooth", "true" };
static settings::Int ve_averager_size{ "debug.ve.averaging", "0" };

void CachedEntity::Update()
{
    auto raw = RAW_ENT(this);

    if (!raw)
        return;
#if PROXY_ENTITY != true
    m_pEntity = g_IEntityList->GetClientEntity(idx);
    if (!m_pEntity)
    {
        return;
    }
#endif
    m_lSeenTicks = 0;
    m_lLastSeen  = 0;

    hitboxes.Update();

    if (m_Type() == EntityType::ENTITY_PLAYER)
        GetPlayerInfo(m_IDX, &player_info);
}

int IsVisible(CachedEntity* current_ent)
{
   
        if(CE_BAD(LOCAL_W)){
            return -1;
        }
        int player_weapon_mode = g_pLocalPlayer->weapon_mode;
        int player_weapon = g_pLocalPlayer->weapon()->m_iClassID();
        int determine_optimal_hitboxes = determine_hitboxes(player_weapon_mode, player_weapon);
        if(determine_optimal_hitboxes == -1){
            return -1;
        }
        int determine_index;
        
      auto optimal_hitboxes = optimal_array_switch(determine_optimal_hitboxes, &determine_index);
        const int size_of_arr = sizeof(optimal_hitboxes);
        if(determine_index == NULL){
            return -1;
        }
        for (int i = 0; i < determine_index; i++)
        {
            logging::Info("YOOO INDEX %d", determine_index);
            logging::Info("YOOO INDEX of ARR %d", optimal_hitboxes[i]); 
          
          Vector get_vec= current_ent->hitboxes.GetHitbox(optimal_hitboxes[i])->center;
            if ( IsEntityVectorVisible(current_ent, get_vec))
            {
                return i;
            }
            
        }
        return -1;
}


const int* optimal_array_switch(int determine_optimal, int *index_size){
    switch(determine_optimal){
        case 0:{
            static const int optimal_hitboxes[] = { hitbox_t::head, hitbox_t::spine_1, hitbox_t::hand_R, hitbox_t::lowerArm_R };
            *index_size =sizeof(optimal_hitboxes)/4;
            return optimal_hitboxes;
        }
        case 1:{
            static const int optimal_hitboxes[]= {hitbox_t::spine_1, hitbox_t::pelvis, hitbox_t::lowerArm_L, hitbox_t::lowerArm_R, hitbox_t::head, hitbox_t::spine_2};
            *index_size =sizeof(optimal_hitboxes)/4;
            return optimal_hitboxes;
        }
        case 2:{
            static const int optimal_hitboxes[] = {hitbox_t::foot_L, hitbox_t::foot_R,hitbox_t::spine_1, hitbox_t::hip_L, hitbox_t::hip_R };
            *index_size =sizeof(optimal_hitboxes)/4;
            return optimal_hitboxes;
        }
        case 3:{
            static const int optimal_hitboxes[] = {hitbox_t::head, hitbox_t::spine_1, hitbox_t::upperArm_L, hitbox_t::spine_3};
            *index_size =sizeof(optimal_hitboxes)/4;
            return optimal_hitboxes;
        }
        case 4:{
            static const int optimal_hitboxes[]= {hitbox_t::spine_1, hitbox_t::spine_0, hitbox_t::spine_3};
            *index_size =sizeof(optimal_hitboxes)/4;
            return optimal_hitboxes;
        }
        default:{
            *index_size=NULL;
            return NULL;
        }



    }



}
int determine_hitboxes(int weapon_mode, int player_weapon){
    switch(weapon_mode){
        case weapon_hitscan:{
            if(IsAmbassador(g_pLocalPlayer->weapon()) ||g_pLocalPlayer->holding_sniper_rifle){
                return 0;
            }
            else{
                return 1;
            }
        }
        case weapon_throwable:
        case weapon_projectile:{
        if( player_weapon == CL_CLASS(CTFCompoundBow)){
            return 3;
        }    
        else{
            return 2;
        }



        }

        case weapon_melee:{
            return 4;
        }
    default:{
        return -1;
    }    


    }



}

std::optional<Vector> CachedEntity::m_vecDormantOrigin()
{
    if (!RAW_ENT(this)->IsDormant())
        return m_vecOrigin();
    auto vec = soundcache::GetSoundLocation(this->m_IDX);
    if (vec)
        return *vec;
    return std::nullopt;
}
int GetScoreForEntity(CachedEntity *entity)
{
    if (!entity)
        return 0;

    if (entity->m_Type() == ENTITY_BUILDING)
    {
        if (entity->m_iClassID() == CL_CLASS(CObjectSentrygun))
        {
            bool is_strong_class = g_pLocalPlayer->clazz == tf_heavy || g_pLocalPlayer->clazz == tf_soldier;

            if (is_strong_class)
            {
                float distance = (g_pLocalPlayer->v_Origin - entity->m_vecOrigin()).Length();
                if (distance < 400.0f)
                    return 120;
                else if (distance < 1100.0f)
                    return 60;
                return 30;
            }
            return 1;
        }
        return 0;
    }
    int clazz      = CE_INT(entity, netvar.iClass);
    int health     = CE_INT(entity, netvar.iHealth);
    float distance = (g_pLocalPlayer->v_Origin - entity->m_vecOrigin()).Length();
    bool zoomed    = HasCondition<TFCond_Zoomed>(entity);
    bool pbullet   = HasCondition<TFCond_SmallBulletResist>(entity);
    bool special   = false;
    bool kritz     = IsPlayerCritBoosted(entity);
    int total      = 0;
    switch (clazz)
    {
    case tf_sniper:
        total += 25;
        if (zoomed)
        {
            total += 50;
        }
        special = true;
        break;
    case tf_medic:
        total += 50;
        if (pbullet)
            return 100;
        break;
    case tf_spy:
        total += 20;
        if (distance < 400)
            total += 60;
        special = true;
        break;
    case tf_soldier:
        if (HasCondition<TFCond_BlastJumping>(entity))
            total += 30;
        break;
    }
    if (!special)
    {
        if (pbullet)
        {
            total += 50;
        }
        if (kritz)
        {
            total += 99;
        }
        if (distance != 0)
        {
            int distance_factor = (4096 / distance) * 4;
            total += distance_factor;
            if (health != 0)
            {
                int health_factor = (450 / health) * 4;
                if (health_factor > 30)
                    health_factor = 30;
                total += health_factor;
            }
        }
    }
    if (total > 99)
        total = 99;
    if (playerlist::AccessData(entity).state == playerlist::k_EState::RAGE)
        total = 999;
    if (IsSentryBuster(entity))
        total = 0;
    if (clazz == tf_medic && g_pGameRules->isPVEMode)
        total = 999;
    return total;
}
entity_linked_list* global_head;
namespace entity_cache
{
CachedEntity array[MAX_ENTITIES]{};

void Update()
{
    max = g_IEntityList->GetHighestEntityIndex();
    for (int i = 0; i <= max; i++)
    {
        array[i].Update();
        if (CE_GOOD((&array[i]))){
            array[i].hitboxes.UpdateBones();
        }

    }
}

void Invalidate()
{
    for (auto &ent : array)
    {
        // pMuch useless line!
        // ent.m_pEntity = nullptr;
        ent.Reset();
    }
}

void Shutdown()
{
    for (auto &ent : array)
    {
        ent.Reset();
        ent.hitboxes.Reset();
    }
}
void* cached_entity_linked(void* args){
    entity_linked_list* head = NULL; 
    head = (struct entity_linked_list*)malloc(sizeof(struct entity_linked_list));
    head->target_score=-1;
    entity_linked_list **tail = &head;
    add_players(tail,0, MAX_ENTITIES);
    
  while(true){
      for(int i=0; i<MAX_ENTITIES; i++){
          
          auto current_int = ENTITY(i);
          if (current_int->m_Type() == ENTITY_PLAYER){
            logging::Info("%d", i);
          int current_hitbox = IsVisible(current_int);
          if(current_hitbox!=-1){
              int score_for_ent = GetScoreForEntity(current_int);
              head = sort_linked_list(head, score_for_ent,current_hitbox);

              }
          }
      }
    logging::Info("%d", head->target_score);



  }
}
entity_linked_list* sort_linked_list(entity_linked_list* head, int score_For_ent, int current_hitbox){

entity_linked_list* temp = head;
if(head->target_score==NULL){
    head->target_score=score_For_ent;
    head->best_hitbox=current_hitbox;
    return head;
}   
else if(head->target_score<score_For_ent){
    int curr_head_score= head->target_score;
    int curr_hitbox = head->best_hitbox;
    head->target_score=score_For_ent;
    head->best_hitbox=current_hitbox;
    if(temp->next->target_score==NULL){
        temp->next->target_score=curr_head_score;
        temp->next->best_hitbox=curr_hitbox;
    }
    while(temp->next->target_score != NULL){
    int move_this = temp->next->target_score;
    int local_hitbox = temp->next->best_hitbox;
    temp->next->target_score=curr_head_score;
    temp->next->best_hitbox=curr_hitbox;
    curr_hitbox=local_hitbox;
    curr_head_score=move_this;
   temp=temp->next;
   }

}
else{
while(temp->next->target_score!=NULL && score_For_ent<temp->next->target_score){
    temp=temp->next;
};
int less_than = temp->next->target_score;
  int local_hitbox = temp->next->best_hitbox;
temp->next->target_score=score_For_ent;
temp->next->best_hitbox=current_hitbox;
temp = temp->next;
 while(temp->next->target_score != NULL){
    int move_this = temp->next->target_score;
    int ref_hitbox = temp->next->best_hitbox;
    temp->next->target_score=less_than;
    temp->next->best_hitbox=local_hitbox;
    local_hitbox=ref_hitbox;
    less_than=move_this;
   temp=temp->next;
}
}

return temp;
}
void add_players(entity_linked_list** tail, int local_max, int current_max){
    while(local_max<current_max){
       entity_linked_list* new_node = (struct entity_linked_list*)malloc(sizeof(struct entity_linked_list));
        new_node->next = NULL;
         *tail = new_node;
     tail = &(*tail)->next;
     local_max++;
    }
}

int max = 0;
} // namespace entity_cache
