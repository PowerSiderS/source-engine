#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

class CBasePlayer;

void MapManager_Init();
void MapManager_Update();
bool MapManager_HandleChat( CBasePlayer *pPlayer, const char *pChatText );
void MapManager_ChangeMap( const char *pszMapName, float flDelay = 3.0f, const char *pszInitiator = NULL );

#endif // MAP_MANAGER_H
