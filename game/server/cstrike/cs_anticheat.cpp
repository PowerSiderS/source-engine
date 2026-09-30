#include "cbase.h"
#include "cs_player.h"
#include "cs_anticheat.h"
#include "cs_anticheat_rules.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "igamesystem.h"
#include "util.h"

ConVar sv_simple_ac("sv_simple_ac","1",FCVAR_GAMEDLL|FCVAR_NOTIFY,"Validate commands and log repeated behavioral anomalies.");
ConVar sv_simple_ac_action("sv_simple_ac_action","1",FCVAR_GAMEDLL,"Malformed input: 0=log/sanitize, 1=discard, 2=kick after repeated malformed commands.",true,0,true,2);
struct CSACState
{
    int userId,lastCommand,invalid,rate,snaps,hops,consecutiveHops,snapWindowCount;
    double lastLog,snapWindow,grace,lastSample,lastJump;
    bool initialized,lastGround,lastJumpHeld;
    QAngle angles;CSCommandRateBudget budget;
    CSACState():userId(0),lastCommand(0),invalid(0),rate(0),snaps(0),hops(0),consecutiveHops(0),snapWindowCount(0),lastLog(-10),snapWindow(0),grace(0),lastSample(0),lastJump(0),initialized(false),lastGround(false),lastJumpHeld(false) {angles.Init();}
};
static CSACState s_state[MAX_PLAYERS+1];
class CSACSystem:public CAutoGameSystem
{
public:CSACSystem():CAutoGameSystem("SimpleAntiCheat"){}
    void LevelInitPreEntity() {for(int i=0;i<ARRAYSIZE(s_state);++i)s_state[i]=CSACState();}
};
static CSACSystem s_system;
void CSAntiCheat_ResetPlayer(CCSPlayer *player)
{
    if(!player || player->entindex()<1 || player->entindex()>MAX_PLAYERS)return;
    CSACState &s=s_state[player->entindex()];s=CSACState();
    s.userId=player->GetUserID();s.grace=gpGlobals->realtime+5;
}
static void Report(CCSPlayer *player,CSACState &s,const char *reason)
{
    double now=gpGlobals->realtime;if(now-s.lastLog<5)return;s.lastLog=now;
    // User-supplied names never become console commands.
    UTIL_LogPrintf("[simple-ac] userid=%d reason=%s invalid=%d rate=%d snaps=%d hops=%d\n",s.userId,reason,s.invalid,s.rate,s.snaps,s.hops);
}
bool CSAntiCheat_CheckCommand(CCSPlayer *player,CUserCmd *cmd)
{
    if(!player || !cmd)return false;
    if(!sv_simple_ac.GetBool())return true;
    const int index=player->entindex();if(index<1 || index>MAX_PLAYERS)return false;
    CSACState &s=s_state[index];if(s.userId!=player->GetUserID())CSAntiCheat_ResetPlayer(player);
    bool valid=CSCommandScalarValid(cmd->forwardmove,10000) && CSCommandScalarValid(cmd->sidemove,10000) && CSCommandScalarValid(cmd->upmove,10000);
    for(int i=0;i<3;++i)valid=valid && CSCommandScalarValid(cmd->viewangles[i],36000);
    if(!valid)
    {
        ++s.invalid;Report(player,s,"malformed_command");
        // Sanitize even in monitor mode: NaN/Inf must not reach physics/bones.
        cmd->forwardmove=cmd->sidemove=cmd->upmove=0;cmd->buttons=0;cmd->viewangles=player->EyeAngles();
        if(sv_simple_ac_action.GetInt()==2 && s.invalid>=8 && !player->IsBot())
        {char kick[128];Q_snprintf(kick,sizeof(kick),"kickid %d \"Repeated malformed commands\"\n",s.userId);engine->ServerCommand(kick);}
        return sv_simple_ac_action.GetInt()==0;
    }
    if(player->IsBot() || !player->IsAlive() || player->IsObserver()) {s.initialized=false;return true;}
    // Replayed backup/null commands do not count as new input samples.
    if(cmd->command_number<=s.lastCommand)return true;
    double now=gpGlobals->realtime;
    const bool ground=player->GetGroundEntity()!=NULL,jump=(cmd->buttons&IN_JUMP)!=0;
    if(s.initialized && now>s.grace && player->GetMoveType()==MOVETYPE_WALK)
    {
        if(!s.budget.Consume(now,gpGlobals->interval_per_tick)) {++s.rate;Report(player,s,"command_rate_observation");}
        if(now-s.snapWindow>10){s.snapWindow=now;s.snapWindowCount=0;}
        if(now-s.lastSample<.1 && (cmd->buttons&IN_ATTACK) &&
           fabsf(AngleDiff(cmd->viewangles.y,s.angles.y))>45 && ++s.snapWindowCount>=8)
        {++s.snaps;s.snapWindowCount=0;Report(player,s,"repeated_aim_snap_observation");}
        if(ground && !s.lastGround)
        {
            if(jump && !s.lastJumpHeld && now-s.lastJump<2)
            {if(++s.consecutiveHops>=8){++s.hops;s.consecutiveHops=0;Report(player,s,"repeated_landing_jump_observation");}}
            else s.consecutiveHops=0;
        }
        if(ground && jump && !s.lastJumpHeld)s.lastJump=now;
    }
    else {s.budget=CSCommandRateBudget();s.consecutiveHops=0;}
    // Flicks, packet loss and skilled hops are observations, never auto bans.
    s.lastGround=ground;s.lastJumpHeld=jump;s.angles=cmd->viewangles;
    s.lastCommand=cmd->command_number;s.lastSample=now;s.initialized=true;
    return true;
}
CON_COMMAND(sv_simple_ac_status,"Print per-player anti-cheat counters to the server console.")
{
    if(!UTIL_IsCommandIssuedByServerAdmin())return;
    for(int i=1;i<=gpGlobals->maxClients && i<=MAX_PLAYERS;++i)
    {CCSPlayer *p=ToCSPlayer(UTIL_PlayerByIndex(i));if(!p)continue;const CSACState &s=s_state[i];Msg("[simple-ac] userid=%d bot=%d invalid=%d rate=%d snaps=%d hops=%d\n",p->GetUserID(),p->IsBot(),s.invalid,s.rate,s.snaps,s.hops);}
    Msg("[simple-ac] enabled=%d action=%d behavioral_action=observe_only\n",sv_simple_ac.GetInt(),sv_simple_ac_action.GetInt());
}
