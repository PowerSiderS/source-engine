#ifndef CS_HUD_THEME_H
#define CS_HUD_THEME_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "filesystem.h"

extern ConVar cl_hud_pink;

// Dedicated resource names avoid replacing another addon's fonts or layout.
inline const char *CSHudResource( const char *original, const char *themed )
{
	return cl_hud_pink.GetBool() && g_pFullFileSystem &&
		g_pFullFileSystem->FileExists( themed, "GAME" ) ? themed : original;
}

#endif
