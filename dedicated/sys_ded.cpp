//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include <stdio.h>
#include <stdlib.h>
#include "isys.h"
#include "console/conproc.h"
#include "dedicated.h"
#include "engine_hlds_api.h"
#include "checksum_md5.h"
#include "mathlib/mathlib.h"
#include "tier0/vcrmode.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include "tier0/icommandline.h"
#include "idedicatedexports.h"
#include "vgui/vguihelpers.h"
#include "appframework/AppFramework.h"
#include "filesystem_init.h"
#include "tier2/tier2.h"
#include "dedicated.h"
#include "vstdlib/cvar.h"
#include "inputsystem/iinputsystem.h"

#ifdef _WIN32
#include <windows.h> 
#include <direct.h>
#include "KeyValues.h"
#else
#define _chdir chdir
#include <unistd.h>
#endif

void* FileSystemFactory( const char *pName, int *pReturnCode );
bool InitInstance( );
int ProcessConsoleInput( void );
bool NET_Init( void );
void NET_Shutdown( void );
const char *UTIL_GetBaseDir( void );
#ifdef _WIN32
bool g_bVGui = false;
#endif

#if defined ( _WIN32 )
#include "console/TextConsoleWin32.h"
CTextConsoleWin32 console;
#else
#include "console/TextConsoleUnix.h"
CTextConsoleUnix console;
#endif

#ifdef _WIN32
extern char *gpszCvars;
#endif

IDedicatedServerAPI *engine = NULL;

//-----------------------------------------------------------------------------
// Implementation of IVCRHelpers.
//-----------------------------------------------------------------------------
class CVCRHelpers : public IVCRHelpers
{
public:
	virtual void	ErrorMessage( const char *pMsg )
	{
		printf( "ERROR: %s\n", pMsg );
	}

	virtual void*	GetMainWindow()
	{
		return 0;
	}
};
CVCRHelpers g_VCRHelpers;

SpewRetval_t DedicatedSpewOutputFunc( SpewType_t spewType, char const *pMsg ); // in sys_common.cpp

//-----------------------------------------------------------------------------
// Run a single VGUI frame. if bFinished is true, run VGUIFinishedConfig() first.
//-----------------------------------------------------------------------------
static bool DoRunVGUIFrame( bool bFinished = false )
{
#ifdef _WIN32
	if ( g_bVGui )
	{
		if ( bFinished )
			VGUIFinishedConfig();
		RunVGUIFrame();
		return true;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Handle the VCRHook PeekMessage loop.
//   Return true if WM_QUIT received.
//-----------------------------------------------------------------------------
static bool HandleVCRHook()
{
#if defined ( _WIN32 )
	MSG msg;

	bool bDone = false;
	while( VCRHook_PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		//if (!GetMessage( &msg, NULL, 0, 0))
		if ( msg.message == WM_QUIT )
		{
			bDone = true;
			break;
		}

		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	if ( IsPC() )
	{
		// NOTE: Under some implementations of Win9x, 
		// dispatching messages can cause the FPU control word to change
		SetupFPUControlWord();
	}

	if ( bDone /*|| gbAppHasBeenTerminated*/ )
		return true;
#endif // _WIN32

	return false;
}

//-----------------------------------------------------------------------------
//
//  Server loop
//
//-----------------------------------------------------------------------------
void RunServer( void )
{
	FILE *fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[RunServer] entered\n" ); fclose( fpDbg ); }

#ifdef _WIN32
	if(gpszCvars)
	{
		engine->AddConsoleText(gpszCvars);
	}
#endif

	// Run 2 engine frames first to get the engine to load its resources.
	for ( int i = 0; i < 2; i++ )
	{
		DoRunVGUIFrame();
		if ( !engine->RunFrame() )
		{
			fpDbg = fopen( "dedicated_trace.log", "a" );
			if ( fpDbg ) { fprintf( fpDbg, "[RunServer] early engine->RunFrame() returned false at iteration %d\n", i ); fclose( fpDbg ); }
			return;
		}
	}

	// Run final VGUI frame.
	DoRunVGUIFrame( true );

	int bDone = false;
	while ( !bDone )
	{
		// Check on VCRHook_PeekMessage...
		if ( HandleVCRHook() )
			break;

		if ( !DoRunVGUIFrame() )
			ProcessConsoleInput();

		if ( !engine->RunFrame() )
			bDone = true;

		sys->UpdateStatus( 0 /* don't force */ );
	}
}

//-----------------------------------------------------------------------------
//
// initialize the console or wait for vgui to start the server
//
//-----------------------------------------------------------------------------
static bool ConsoleStartup( )
{
#ifdef _WIN32
	if ( g_bVGui )
	{
		RunVGUIFrame();

		// Run the config screen
		while (VGUIIsInConfig()	&& VGUIIsRunning())
			RunVGUIFrame();

		if ( VGUIIsStopping() )
			return false;

		return true;
	}
	else
	{
		if ( !console.Init() )
		{
			return false;	 
		}
	}
#endif // _WIN32

	return true;
}


//-----------------------------------------------------------------------------
// Instantiate all main libraries
//-----------------------------------------------------------------------------
bool CDedicatedAppSystemGroup::Create( )
{
#ifndef _WIN32
	if ( !console.Init() )
		return false;
#endif		

	// Hook the debug output stuff (override the spew func in the appframework)
	SpewOutputFunc( DedicatedSpewOutputFunc );

	// Added the dedicated exports module for the engine to grab
	AppModule_t dedicatedModule = LoadModule( Sys_GetFactoryThis() );
	IAppSystem *pSystem = AddSystem( dedicatedModule, VENGINE_DEDICATEDEXPORTS_API_VERSION );
	if ( !pSystem )
		return false;

	if ( sys->LoadModules( this ) )
	{
		// Find the input system and tell it to skip Steam Controller initialization (we have to set this flag before Init gets called on the
		// input system). Dedicated server should skip controller initialization to avoid initializing Steam, because we don't want the user to be
		// flagged as "playing" the game.
		auto inputsystem = ( IInputSystem* )FindSystem( INPUTSYSTEM_INTERFACE_VERSION );
		if ( inputsystem )
		{
			inputsystem->SetSkipControllerInitialization( true );
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool CDedicatedAppSystemGroup::PreInit( )
{
	extern IFileSystem *g_pFileSystem;
	// A little hack needed because dedicated links directly to filesystem .cpp files
	g_pFullFileSystem = g_pFileSystem;

	FILE *fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] entered, g_pFullFileSystem=%p\n", g_pFullFileSystem ); fclose( fpDbg ); }

	if ( !BaseClass::PreInit() )
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] BaseClass::PreInit failed\n" ); fclose( fpDbg ); }
		return false;
	}

	CFSSteamSetupInfo steamInfo;
	steamInfo.m_pDirectoryName = NULL;
	steamInfo.m_bOnlyUseDirectoryName = false;
	steamInfo.m_bToolsMode = false;
	steamInfo.m_bSetSteamDLLPath = false;
	steamInfo.m_bSteam = g_pFullFileSystem ? g_pFullFileSystem->IsSteam() : false;
	steamInfo.m_bNoGameInfo = steamInfo.m_bSteam;
	if ( FileSystem_SetupSteamEnvironment( steamInfo ) != FS_OK )
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] FileSystem_SetupSteamEnvironment failed\n" ); fclose( fpDbg ); }
		return false;
	}

	CFSMountContentInfo fsInfo;
	fsInfo.m_pFileSystem = g_pFullFileSystem;
	fsInfo.m_bToolsMode = false;
	fsInfo.m_pDirectoryName = steamInfo.m_GameInfoPath;

	if ( FileSystem_MountContent( fsInfo ) != FS_OK )
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] FileSystem_MountContent failed for %s\n", steamInfo.m_GameInfoPath ); fclose( fpDbg ); }
		return false;
	}

	fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] FileSystem_MountContent succeeded for %s\n", steamInfo.m_GameInfoPath ); fclose( fpDbg ); }

	fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] calling NET_Init()\n" ); fclose( fpDbg ); }

	if ( !NET_Init() )
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] NET_Init failed\n" ); fclose( fpDbg ); }
		return false;
	}

	fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] NET_Init succeeded\n" ); fclose( fpDbg ); }

#ifdef _WIN32
	g_bVGui = CommandLine()->CheckParm( "-vgui" );
#endif

	CreateInterfaceFn factory = GetFactory();
	IInputSystem *inputsystem = (IInputSystem *)factory( INPUTSYSTEM_INTERFACE_VERSION, NULL );
	if ( inputsystem )
	{
		inputsystem->SetConsoleTextMode( true );
	}

#ifdef _WIN32
	if ( g_bVGui )
	{
		StartVGUI( GetFactory() );
	}
	else
#endif
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] calling sys->CreateConsoleWindow()\n" ); fclose( fpDbg ); }

		if ( !sys->CreateConsoleWindow() )
		{
			fpDbg = fopen( "dedicated_trace.log", "a" );
			if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] sys->CreateConsoleWindow failed\n" ); fclose( fpDbg ); }
			return false;
		}
	}

	fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::PreInit] returning true\n" ); fclose( fpDbg ); }
	return true;
}

int CDedicatedAppSystemGroup::Main( )
{
	FILE *fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::Main] entered\n" ); fclose( fpDbg ); }

	if ( !ConsoleStartup() )
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::Main] ConsoleStartup failed\n" ); fclose( fpDbg ); }
		return -1;
	}

#ifdef _WIN32
	if ( g_bVGui )
		RunVGUIFrame();
#endif

	// Set up mod information
	ModInfo_t info;
	info.m_pInstance = GetAppInstance();
	info.m_pBaseDirectory = UTIL_GetBaseDir();
	info.m_pInitialMod = CommandLine()->ParmValue( "-game", "hl2" );
	info.m_pInitialGame = CommandLine()->ParmValue( "-defaultgamedir", "hl2" );
	info.m_pParentAppSystemGroup = this;
	info.m_bTextMode = CommandLine()->CheckParm( "-textmode" );

	fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::Main] calling engine->ModInit(mod=%s, base=%s)\n", info.m_pInitialMod, info.m_pBaseDirectory ); fclose( fpDbg ); }

	if ( engine->ModInit( info ) )
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::Main] engine->ModInit succeeded, shutting down\n" ); fclose( fpDbg ); }
		engine->ModShutdown();
	} // if engine->ModInit
	else
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[CDedicatedAppSystemGroup::Main] engine->ModInit returned false\n" ); fclose( fpDbg ); }
	}

	return 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CDedicatedAppSystemGroup::PostShutdown()
{
#ifdef _WIN32
	if ( g_bVGui )
		StopVGUI();
#endif

	sys->DestroyConsoleWindow();
	console.ShutDown();
	NET_Shutdown();
	BaseClass::PostShutdown();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CDedicatedAppSystemGroup::Destroy()
{
}


//-----------------------------------------------------------------------------
// Gets the executable name
//-----------------------------------------------------------------------------
bool GetExecutableName( char *out, int nMaxLen )
{
#ifdef _WIN32
	if ( !::GetModuleFileName( ( HINSTANCE )GetModuleHandle( NULL ), out, nMaxLen ) )
		return false;
	return true;
#elif POSIX
	Q_strncpy( out, g_szEXEName, nMaxLen );
	return true;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Return the directory where this .exe is running from
// Output : char
//-----------------------------------------------------------------------------
void UTIL_ComputeBaseDir( char *pBaseDir, int nMaxLen )
{
	int j;
	char *pBuffer = NULL;

	pBaseDir[ 0 ] = 0;

	if ( GetExecutableName( pBaseDir, nMaxLen ) )
	{
		pBuffer = strrchr( pBaseDir, CORRECT_PATH_SEPARATOR );
		if ( pBuffer && *pBuffer )
		{
			*(pBuffer+1) = '\0';
		}

		j = strlen( pBaseDir );
		if (j > 0)
		{
			if ( ( pBaseDir[ j-1 ] == '\\' ) || 
				 ( pBaseDir[ j-1 ] == '/' ) )
			{
				pBaseDir[ j-1 ] = 0;
			}
		}
	}

	char const *pOverrideDir = CommandLine()->CheckParm( "-basedir" );
	if ( pOverrideDir )
	{
		strcpy( pBaseDir, pOverrideDir );
	}

	Q_strlower( pBaseDir );
	Q_FixSlashes( pBaseDir );
}


//-----------------------------------------------------------------------------
// This class is a helper class used for steam-based applications.
// It loads up the file system in preparation for using it to load other
// required modules from steam.
//
// I couldn't use the one in appframework because the dedicated server
// inlines all the filesystem code.
//-----------------------------------------------------------------------------
class CDedicatedSteamApplication : public CSteamApplication
{
public:
	CDedicatedSteamApplication( CSteamAppSystemGroup *pAppSystemGroup );
	virtual bool Create( );
};


//-----------------------------------------------------------------------------
// This class is a helper class used for steam-based applications.
// It loads up the file system in preparation for using it to load other
// required modules from steam.
//
// I couldn't use the one in appframework because the dedicated server
// inlines all the filesystem code.
//-----------------------------------------------------------------------------
CDedicatedSteamApplication::CDedicatedSteamApplication( CSteamAppSystemGroup *pAppSystemGroup ) : CSteamApplication( pAppSystemGroup )
{
}


//-----------------------------------------------------------------------------
// Implementation of IAppSystemGroup
//-----------------------------------------------------------------------------
bool CDedicatedSteamApplication::Create( )
{
	// Add in the cvar factory
	AppModule_t cvarModule = LoadModule( VStdLib_GetICVarFactory() );
	AddSystem( cvarModule, CVAR_INTERFACE_VERSION );

	AppModule_t fileSystemModule = LoadModule( FileSystemFactory );
	m_pFileSystem = (IFileSystem*)AddSystem( fileSystemModule, FILESYSTEM_INTERFACE_VERSION );

	if ( !m_pFileSystem )
	{
		Warning( "Unable to load the file system!\n" );
		return false;
	}

	return true;
}



//-----------------------------------------------------------------------------
//
// Main entry point for dedicated server, shared between win32 and linux
//
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
#if !defined( POSIX ) && !defined( PLATFORM_64BITS )
	_asm
	{
		fninit
	}
#endif

	SetupFPUControlWord();

#ifdef POSIX
	Q_strncpy( g_szEXEName, *argv, ARRAYSIZE( g_szEXEName ) );
	// Store off command line for argument searching
	BuildCmdLine( argc, argv );
#endif

	MathLib_Init( 2.2f, 2.2f, 0.0f, 1.0f );

	// Store off command line for argument searching
	CommandLine()->CreateCmdLine( VCRHook_GetCommandLine() );
#ifndef _WIN32
	Plat_SetCommandLine( CommandLine()->GetCmdLine() );
#endif

	// Start VCR mode?
	const char *filename;
	if( CommandLine()->CheckParm( "-vcrrecord", &filename ) )
	{
		if ( !VCRStart( filename, true, &g_VCRHelpers ) )
		{
			Error( "-vcrrecord: can't open '%s' for writing.\n", filename );
			return -1;
		}
	}
	else if( CommandLine()->CheckParm( "-vcrplayback", &filename ) )
	{
		if ( !VCRStart( filename, false, &g_VCRHelpers ) )
		{
			Error( "-vcrplayback: can't open '%s' for reading.\n", filename );
			return -1;
		}
	}

	// Figure out the directory the executable is running from
	// and make that be the current working directory
	char pBasedir[ MAX_PATH ];
	UTIL_ComputeBaseDir( pBasedir, MAX_PATH );
	_chdir( pBasedir );

	// Rehook the command line through VCR mode.
	CommandLine()->CreateCmdLine( VCRHook_GetCommandLine() );

	FILE *fpDbg = fopen( "dedicated_trace.log", "w" );
	if ( fpDbg ) { fprintf( fpDbg, "[main] entered argc=%d basedir=%s cmdline=%s\n", argc, pBasedir, CommandLine()->GetCmdLine() ); fclose( fpDbg ); }

	if ( !InitInstance() )
	{
		fpDbg = fopen( "dedicated_trace.log", "a" );
		if ( fpDbg ) { fprintf( fpDbg, "[main] InitInstance failed\n" ); fclose( fpDbg ); }
		return -1;
	}

	fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[main] launching steamApplication.Run()\n" ); fclose( fpDbg ); }

	CDedicatedAppSystemGroup dedicatedSystems;
	CDedicatedSteamApplication steamApplication( &dedicatedSystems );
	int nRet = steamApplication.Run( );

	fpDbg = fopen( "dedicated_trace.log", "a" );
	if ( fpDbg ) { fprintf( fpDbg, "[main] steamApplication.Run() returned %d\n", nRet ); fclose( fpDbg ); }
	return nRet;
}
