#include "custom_steamid.h"
#include "tier1/checksum_md5.h"
#include "tier1/strtools.h"
#include "tier0/dbg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#if defined( USE_SDL )
#include "SDL.h"
#if defined( ANDROID )
#include "SDL_system.h"
#endif
#endif

#if defined( _WIN32 )
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

static char s_szDeviceUUID[64] = { 0 };

static void GetInternalStorageFilePath( char *pszOut, size_t nMaxLen )
{
	pszOut[0] = '\0';

#if defined( ANDROID )
	const char *pszInternal = NULL;
#if defined( USE_SDL )
	pszInternal = SDL_AndroidGetInternalStoragePath();
#endif
	if ( pszInternal && pszInternal[0] )
	{
		V_snprintf( pszOut, nMaxLen, "%s/.device_id", pszInternal );
		return;
	}

	const char *pszEnv = getenv( "INTERNAL_STORAGE" );
	if ( pszEnv && pszEnv[0] )
	{
		V_snprintf( pszOut, nMaxLen, "%s/.device_id", pszEnv );
		return;
	}
#elif defined( _WIN32 )
	char szAppData[MAX_PATH];
	if ( GetEnvironmentVariableA( "LOCALAPPDATA", szAppData, sizeof(szAppData) ) > 0 )
	{
		char szDir[MAX_PATH];
		V_snprintf( szDir, sizeof(szDir), "%s\\SourceEngine", szAppData );
		CreateDirectoryA( szDir, NULL );
		V_snprintf( pszOut, nMaxLen, "%s\\SourceEngine\\.device_id", szAppData );
		return;
	}
#else
	const char *pszHome = getenv( "HOME" );
	if ( pszHome && pszHome[0] )
	{
		V_snprintf( pszOut, nMaxLen, "%s/.source_device_id", pszHome );
		return;
	}
#endif

	V_strncpy( pszOut, ".device_id", nMaxLen );
}

static bool ReadStringFromFile( const char *pszFilePath, char *pszOut, size_t nMaxLen )
{
	if ( !pszFilePath || !pszFilePath[0] )
		return false;

	FILE *f = fopen( pszFilePath, "r" );
	if ( !f )
		return false;

	char szBuffer[128] = { 0 };
	if ( fgets( szBuffer, sizeof(szBuffer) - 1, f ) == NULL )
	{
		fclose( f );
		return false;
	}
	fclose( f );

	char *p = szBuffer;
	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' )
		p++;

	size_t len = V_strlen( p );
	while ( len > 0 && ( p[len - 1] == ' ' || p[len - 1] == '\t' || p[len - 1] == '\r' || p[len - 1] == '\n' ) )
	{
		p[len - 1] = '\0';
		len--;
	}

	if ( len < 8 )
		return false;

	V_strncpy( pszOut, p, nMaxLen );
	return true;
}

static bool WriteStringToFile( const char *pszFilePath, const char *pszData )
{
	if ( !pszFilePath || !pszFilePath[0] || !pszData || !pszData[0] )
		return false;

	FILE *f = fopen( pszFilePath, "w" );
	if ( !f )
		return false;

	fputs( pszData, f );
	fputc( '\n', f );
	fclose( f );
	return true;
}

static void GenerateRandomUUID( char *pszOut, size_t nMaxLen )
{
	static bool s_bSeeded = false;
	if ( !s_bSeeded )
	{
		unsigned int seed = (unsigned int)time( NULL );
#if !defined( _WIN32 )
		seed ^= ( (unsigned int)getpid() << 16 );
#endif
		srand( seed );
		s_bSeeded = true;
	}

	unsigned int r1 = ( (unsigned int)rand() << 16 ) | ( (unsigned int)rand() & 0xFFFF );
	unsigned int r2 = ( (unsigned int)rand() << 16 ) | ( (unsigned int)rand() & 0xFFFF );
	unsigned int r3 = ( (unsigned int)rand() << 16 ) | ( (unsigned int)rand() & 0xFFFF );
	unsigned int r4 = ( (unsigned int)rand() << 16 ) | ( (unsigned int)rand() & 0xFFFF );

	r2 = ( r2 & 0xFFFF0FFF ) | 0x00004000;
	r3 = ( r3 & 0x3FFFFFFF ) | 0x80000000;

	V_snprintf( pszOut, nMaxLen, "%08x-%04x-%04x-%04x-%04x%08x",
		r1,
		( r2 >> 16 ) & 0xFFFF,
		r2 & 0xFFFF,
		( r3 >> 16 ) & 0xFFFF,
		r3 & 0xFFFF,
		r4 );
}

const char *GetDeviceUUID()
{
	if ( s_szDeviceUUID[0] != '\0' )
		return s_szDeviceUUID;

	char szStoragePath[512];
	GetInternalStorageFilePath( szStoragePath, sizeof(szStoragePath) );

	if ( ReadStringFromFile( szStoragePath, s_szDeviceUUID, sizeof(s_szDeviceUUID) ) )
	{
		return s_szDeviceUUID;
	}

#if defined( _WIN32 )
	HKEY hKey;
	if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey ) == ERROR_SUCCESS ||
	     RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
	{
		char szGuid[128] = { 0 };
		DWORD dwSize = sizeof(szGuid);
		DWORD dwType = REG_SZ;
		if ( RegQueryValueExA( hKey, "MachineGuid", NULL, &dwType, (LPBYTE)szGuid, &dwSize ) == ERROR_SUCCESS && dwSize > 8 )
		{
			RegCloseKey( hKey );
			V_strncpy( s_szDeviceUUID, szGuid, sizeof(s_szDeviceUUID) );
			WriteStringToFile( szStoragePath, s_szDeviceUUID );
			return s_szDeviceUUID;
		}
		RegCloseKey( hKey );
	}
#elif !defined( ANDROID )
	if ( ReadStringFromFile( "/etc/machine-id", s_szDeviceUUID, sizeof(s_szDeviceUUID) ) ||
	     ReadStringFromFile( "/var/lib/dbus/machine-id", s_szDeviceUUID, sizeof(s_szDeviceUUID) ) )
	{
		WriteStringToFile( szStoragePath, s_szDeviceUUID );
		return s_szDeviceUUID;
	}
#endif

	GenerateRandomUUID( s_szDeviceUUID, sizeof(s_szDeviceUUID) );
	WriteStringToFile( szStoragePath, s_szDeviceUUID );
	return s_szDeviceUUID;
}

CSteamID GenerateSteamIDFromUUID( const char *pszUUID )
{
	if ( !pszUUID || !pszUUID[0] )
		return CSteamID();

	MD5Context_t ctx;
	unsigned char digest[16];
	memset( &ctx, 0, sizeof(ctx) );
	MD5Init( &ctx );
	MD5Update( &ctx, (const unsigned char *)pszUUID, V_strlen(pszUUID) );
	MD5Final( digest, &ctx );

	uint32 rawID = *(uint32 *)digest;
	uint32 accountID = ( rawID & 0x7FFFFFFF );
	if ( accountID == 0 )
		accountID = 1;

	return CSteamID( accountID, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual );
}

CSteamID GenerateSteamIDFromAddress( const netadr_t &adr )
{
	char szBuf[64];
	V_snprintf( szBuf, sizeof(szBuf), "ADDR_%s", adr.ToString( true ) );
	return GenerateSteamIDFromUUID( szBuf );
}

CSteamID GetLocalDeviceSteamID()
{
	return GenerateSteamIDFromUUID( GetDeviceUUID() );
}
