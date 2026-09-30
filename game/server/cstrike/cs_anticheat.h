#ifndef CS_SIMPLE_ANTICHEAT_H
#define CS_SIMPLE_ANTICHEAT_H
class CCSPlayer;class CUserCmd;
bool CSAntiCheat_CheckCommand(CCSPlayer *player,CUserCmd *command);
void CSAntiCheat_ResetPlayer(CCSPlayer *player);
#endif
