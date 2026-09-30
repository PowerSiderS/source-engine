#include "cbase.h"
#include "player.h"
#include "filesystem.h"
#include "eiface.h"
#include "convar.h"
#include "map_manager.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "tier1/strtools.h"
#include <ctype.h>

ConVar sm_map_enabled( "sm_map_enabled", "1", FCVAR_GAMEDLL, "Enable or disable in-game chat map switcher commands (!map, !maps, !rtv)." );
ConVar sm_map_delay( "sm_map_delay", "3.0", FCVAR_GAMEDLL, "Delay in seconds before changing to the new map." );
ConVar sm_rtv_ratio( "sm_rtv_ratio", "0.5", FCVAR_GAMEDLL, "Ratio of human players required to trigger Rock The Vote (0.1 to 1.0)." );

static CUtlVector<CUtlString> s_MapList;
static bool s_bMapListInitialized = false;

// Pending changelevel state
static bool s_bMapChangePending = false;
static float s_flMapChangeTime = 0.0f;
static char s_szTargetMap[128] = { 0 };
static int s_nLastCountdownSecond = -1;

// RTV state
static CUtlVector<int> s_RTVVotes;

static bool MapManager_IsAllowedCompetitiveOrCasual( const char *pszMap )
{
	if ( !pszMap || !pszMap[0] )
		return false;

	// Strictly exclude Zombie Escape, Aim training, and Hardware tests
	if ( !Q_strnicmp( pszMap, "ze_", 3 ) || !Q_strnicmp( pszMap, "aim", 3 ) || !Q_strnicmp( pszMap, "test_", 5 ) )
		return false;

	// Exclude duplicates / second variants like _scb or _new if main version exists
	if ( V_stristr( pszMap, "_scb" ) )
		return false;
	if ( !Q_stricmp( pszMap, "de_mirage_csgo_new" ) )
		return false;

	// Only allow standard defusal (de_) and hostage rescue (cs_)
	if ( !Q_strnicmp( pszMap, "de_", 3 ) || !Q_strnicmp( pszMap, "cs_", 3 ) )
		return true;

	return false;
}

// Scan all available maps from filesystem (filtered for competitive & casual standard)
static void MapManager_ScanMaps()
{
	s_MapList.RemoveAll();

	// Primary Competitive Active Duty Pool first
	const char *priorityPool[] = {
		"de_mirage_csgo",
		"de_dust2_go",
		"de_inferno_csgo_cssold_fix",
		"de_anubis_go",
		"de_ancient_go",
		"de_nuke_go",
		"de_overpass_csgo",
		"de_vertigo_csgo_new",
		"de_cache_rework",
		"de_train",
		"de_dust2",
		"de_inferno",
		"de_nuke",
		"de_cbble",
		"de_aztec",
		"cs_office",
		"cs_italy",
		"cs_assault"
	};

	for ( int p = 0; p < (int)ARRAYSIZE( priorityPool ); p++ )
	{
		char szBspPath[128];
		Q_snprintf( szBspPath, sizeof( szBspPath ), "maps/%s.bsp", priorityPool[p] );
		if ( filesystem->FileExists( szBspPath, "GAME" ) )
		{
			s_MapList.AddToTail( CUtlString( priorityPool[p] ) );
		}
	}

	// Add any other standard de_ / cs_ maps found in maps/*.bsp
	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx( "maps/*.bsp", "GAME", &findHandle );
	while ( pFilename )
	{
		if ( V_stristr( pFilename, ".bsp" ) )
		{
			char mapBase[128];
			V_FileBase( pFilename, mapBase, sizeof( mapBase ) );

			if ( MapManager_IsAllowedCompetitiveOrCasual( mapBase ) )
			{
				bool bExists = false;
				for ( int m = 0; m < s_MapList.Count(); m++ )
				{
					if ( !Q_stricmp( s_MapList[m].String(), mapBase ) )
					{
						bExists = true;
						break;
					}
				}

				if ( !bExists )
				{
					s_MapList.AddToTail( CUtlString( mapBase ) );
				}
			}
		}
		pFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );

	s_bMapListInitialized = true;
	Msg( "[Map Manager] Scanned %d competitive and casual maps available on server.\n", s_MapList.Count() );
}


void MapManager_Init()
{
	MapManager_ScanMaps();
	s_bMapChangePending = false;
	s_szTargetMap[0] = '\0';
	s_RTVVotes.RemoveAll();
}

static const char* MapManager_FindBestMapMatch( const char *pQuery )
{
	if ( !s_bMapListInitialized || s_MapList.Count() == 0 )
	{
		MapManager_ScanMaps();
	}

	if ( !pQuery || !pQuery[0] )
		return NULL;

	// 1. Exact match
	for ( int i = 0; i < s_MapList.Count(); i++ )
	{
		if ( !Q_stricmp( s_MapList[i].String(), pQuery ) )
			return s_MapList[i].String();
	}

	// 2. Prefix match (e.g. "de_mir" -> "de_mirage_csgo")
	for ( int i = 0; i < s_MapList.Count(); i++ )
	{
		if ( !Q_strnicmp( s_MapList[i].String(), pQuery, Q_strlen( pQuery ) ) )
			return s_MapList[i].String();
	}

	// 3. Substring match (e.g. "mirage" -> "de_mirage_csgo", "dust2" -> "de_dust2_go")
	for ( int i = 0; i < s_MapList.Count(); i++ )
	{
		if ( V_stristr( s_MapList[i].String(), pQuery ) )
			return s_MapList[i].String();
	}

	return NULL;
}

void MapManager_ChangeMap( const char *pszMapName, float flDelay, const char *pszInitiator )
{
	if ( !pszMapName || !pszMapName[0] )
		return;

	const char *pszMatched = MapManager_FindBestMapMatch( pszMapName );
	if ( !pszMatched )
	{
		Msg( "[Map Manager] Error: Map '%s' not found!\n", pszMapName );
		return;
	}

	Q_strncpy( s_szTargetMap, pszMatched, sizeof( s_szTargetMap ) );
	s_bMapChangePending = true;
	s_flMapChangeTime = gpGlobals->curtime + flDelay;
	s_nLastCountdownSecond = (int)flDelay;

	char msg[256];
	if ( pszInitiator && pszInitiator[0] )
	{
		Q_snprintf( msg, sizeof( msg ), "\x04[MAP MANAGER]\x01 \x03%s\x01 iniciou troca para \x04%s\x01 em %.0f segundos...\n", pszInitiator, s_szTargetMap, flDelay );
	}
	else
	{
		Q_snprintf( msg, sizeof( msg ), "\x04[MAP MANAGER]\x01 Mudando para o mapa \x04%s\x01 em %.0f segundos...\n", s_szTargetMap, flDelay );
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, "%s", msg );

	char centerMsg[128];
	Q_snprintf( centerMsg, sizeof( centerMsg ), "Trocando mapa para:\n%s", s_szTargetMap );
	UTIL_CenterPrintAll( centerMsg );
}

void MapManager_Update()
{
	if ( !s_bMapChangePending )
		return;

	float flRemaining = s_flMapChangeTime - gpGlobals->curtime;

	if ( flRemaining <= 0.0f )
	{
		s_bMapChangePending = false;
		Msg( "[Map Manager] Executing ChangeLevel to '%s' now...\n", s_szTargetMap );
		engine->ChangeLevel( s_szTargetMap, NULL );
		return;
	}

	int nSecondsLeft = (int)ceil( flRemaining );
	if ( nSecondsLeft > 0 && nSecondsLeft != s_nLastCountdownSecond )
	{
		s_nLastCountdownSecond = nSecondsLeft;
		char centerMsg[128];
		Q_snprintf( centerMsg, sizeof( centerMsg ), "Trocando para %s em %d...", s_szTargetMap, nSecondsLeft );
		UTIL_CenterPrintAll( centerMsg );
	}
}

static int MapManager_GetHumanPlayerCount()
{
	int count = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pP = UTIL_PlayerByIndex( i );
		if ( pP && pP->IsConnected() && !pP->IsBot() )
		{
			count++;
		}
	}
	return count;
}

bool MapManager_HandleChat( CBasePlayer *pPlayer, const char *pChatText )
{
	if ( !sm_map_enabled.GetBool() || !pChatText )
		return false;

	// Trim leading whitespace
	while ( *pChatText == ' ' || *pChatText == '\t' )
		pChatText++;

	if ( *pChatText != '!' && *pChatText != '/' )
		return false;

	const char *pCmd = pChatText + 1;

	// !map <name> or /map <name>
	if ( !Q_strnicmp( pCmd, "map ", 4 ) || !Q_strnicmp( pCmd, "map", 3 ) && ( pCmd[3] == ' ' || pCmd[3] == '\0' ) )
	{
		const char *pArg = pCmd + 3;
		while ( *pArg == ' ' ) pArg++;

		if ( !pArg[0] )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "\x04[MAP MANAGER]\x01 Uso: \x03!map <nome_do_mapa>\x01 (Exemplo: \x04!map mirage\x01 ou \x04!map dust2\x01)\n" );
			return true;
		}

		const char *pBestMap = MapManager_FindBestMapMatch( pArg );
		if ( pBestMap )
		{
			MapManager_ChangeMap( pBestMap, sm_map_delay.GetFloat(), pPlayer ? pPlayer->GetPlayerName() : "Console" );
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, UTIL_VarArgs( "\x04[MAP MANAGER]\x01 Mapa '\x03%s\x01' nao encontrado! Digite \x04!maps\x01 no chat para ver a lista.\n", pArg ) );
		}
		return true;
	}

	// !maps or /maps
	if ( !Q_stricmp( pCmd, "maps" ) || !Q_stricmp( pCmd, "maplist" ) )
	{
		if ( !s_bMapListInitialized )
			MapManager_ScanMaps();

		ClientPrint( pPlayer, HUD_PRINTTALK, UTIL_VarArgs( "\x04[MAP MANAGER]\x01 Enviando lista de \x03%d mapas\x01 para seu console (~). Digite \x04!map <nome>\x01 para trocar!\n", s_MapList.Count() ) );

		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "\n==================== MAPAS DO SERVIDOR ====================\n" );
		for ( int i = 0; i < s_MapList.Count(); i++ )
		{
			char line[128];
			Q_snprintf( line, sizeof( line ), " [%02d] %s\n", i + 1, s_MapList[i].String() );
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "%s", line );
		}
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "===========================================================\n" );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Dica: Digite !map <nome> no chat ou map_change <nome> no console.\n\n" );
		return true;
	}

	// !rtv or /rtv (Rock The Vote)
	if ( !Q_stricmp( pCmd, "rtv" ) || !Q_stricmp( pCmd, "rockthevote" ) )
	{
		if ( !pPlayer )
			return true;

		int entIndex = pPlayer->entindex();
		if ( s_RTVVotes.Find( entIndex ) != -1 )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "\x04[RTV]\x01 Voce ja votou para trocar de mapa!\n" );
			return true;
		}

		s_RTVVotes.AddToTail( entIndex );

		int nHumans = MapManager_GetHumanPlayerCount();
		if ( nHumans < 1 ) nHumans = 1;

		int nRequired = (int)ceil( (float)nHumans * sm_rtv_ratio.GetFloat() );
		if ( nRequired < 1 ) nRequired = 1;

		char rtvMsg[256];
		Q_snprintf( rtvMsg, sizeof( rtvMsg ), "\x04[RTV]\x01 \x03%s\x01 votou para trocar de mapa (%d/%d necessarios - digite \x04!rtv\x01).\n",
			pPlayer->GetPlayerName(), s_RTVVotes.Count(), nRequired );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "%s", rtvMsg );

		if ( s_RTVVotes.Count() >= nRequired )
		{
			UTIL_ClientPrintAll( HUD_PRINTTALK, "\x04[RTV]\x01 Votacao alcancada! Selecionando proximo mapa...\n" );
			s_RTVVotes.RemoveAll();

			// Pick a random different map
			if ( s_MapList.Count() > 1 )
			{
				int nextIdx = RandomInt( 0, s_MapList.Count() - 1 );
				if ( !Q_stricmp( s_MapList[nextIdx].String(), STRING( gpGlobals->mapname ) ) )
				{
					nextIdx = ( nextIdx + 1 ) % s_MapList.Count();
				}
				MapManager_ChangeMap( s_MapList[nextIdx].String(), sm_map_delay.GetFloat(), "Votacao RTV" );
			}
		}

		return true;
	}

	// !currentmap
	if ( !Q_stricmp( pCmd, "currentmap" ) || !Q_stricmp( pCmd, "mapatual" ) )
	{
		char curMsg[128];
		Q_snprintf( curMsg, sizeof( curMsg ), "\x04[MAP MANAGER]\x01 Mapa atual: \x03%s\x01 (128 Tickrate)\n", STRING( gpGlobals->mapname ) );
		ClientPrint( pPlayer, HUD_PRINTTALK, "%s", curMsg );
		return true;
	}

	return false;
}

// Server Console Commands
CON_COMMAND( map_change, "Change dedicated server to specified map immediately or with delay: map_change <mapname>" )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: map_change <mapname>\n" );
		return;
	}

	MapManager_ChangeMap( args[1], 1.0f, "Console RCON" );
}

CON_COMMAND( sm_map, "SourceMod compatible map changer: sm_map <mapname>" )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: sm_map <mapname>\n" );
		return;
	}

	MapManager_ChangeMap( args[1], sm_map_delay.GetFloat(), "Console Admin" );
}

CON_COMMAND( map_list, "List all available maps on the server" )
{
	if ( !s_bMapListInitialized )
		MapManager_ScanMaps();

	Msg( "==================== MAPAS DO SERVIDOR (%d) ====================\n", s_MapList.Count() );
	for ( int i = 0; i < s_MapList.Count(); i++ )
	{
		Msg( " [%02d] %s\n", i + 1, s_MapList[i].String() );
	}
	Msg( "=================================================================\n" );
}

CON_COMMAND( map_reload, "Rescan all maps from the maps folder" )
{
	MapManager_ScanMaps();
}
