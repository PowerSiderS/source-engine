//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Centralized HUD color system for CS:S Android
// Author: PowerSiderS
//
//=============================================================================//

#ifndef CS_HUD_COLOR_H
#define CS_HUD_COLOR_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include <Color.h>
#include "igamesystem.h"

// HUD Color Modes
enum HudColorMode_t
{
        HUD_COLOR_RED = 0,
        HUD_COLOR_BLUE = 1,
        HUD_COLOR_CYAN = 2,
        HUD_COLOR_LIME = 3,
        HUD_COLOR_RAINBOW = 4,
        
        HUD_COLOR_COUNT
};

// External reference to the cl_hud_color ConVar (defined in clientmode_csnormal.cpp)
extern ConVar cl_hud_color;

// Get rainbow color that cycles over time
inline Color GetRainbowColor( float timeOffset = 0.0f )
{
        float time = gpGlobals->curtime + timeOffset;
        float frequency = 2.0f;
        
        int r = (int)(sin(frequency * time + 0) * 127 + 128);
        int g = (int)(sin(frequency * time + 2) * 127 + 128);
        int b = (int)(sin(frequency * time + 4) * 127 + 128);
        
        return Color( r, g, b, 255 );
}

// Get the current HUD color based on cl_hud_color ConVar
inline Color GetHudColor( int alpha = 255 )
{
        int colorMode = cl_hud_color.GetInt();
        
        switch( colorMode )
        {
                case HUD_COLOR_RED:
                        return Color( 255, 50, 50, alpha );      // Red
                case HUD_COLOR_BLUE:
                        return Color( 50, 100, 255, alpha );     // Blue
                case HUD_COLOR_CYAN:
                        return Color( 0, 255, 255, alpha );      // Cyan
                case HUD_COLOR_LIME:
                        return Color( 50, 255, 50, alpha );      // Lime Green
                case HUD_COLOR_RAINBOW:
                        return GetRainbowColor();                // Rainbow (cycling)
                default:
                        return Color( 255, 50, 50, alpha );      // Default: Red
        }
}

// Get HUD color with custom alpha (for transparency effects)
inline Color GetHudColorWithAlpha( int alpha )
{
        Color clr = GetHudColor( 255 );
        clr[3] = alpha;
        return clr;
}

// Get secondary HUD color (slightly darker version for contrast)
inline Color GetHudColorSecondary( int alpha = 255 )
{
        int colorMode = cl_hud_color.GetInt();
        
        switch( colorMode )
        {
                case HUD_COLOR_RED:
                        return Color( 180, 30, 30, alpha );
                case HUD_COLOR_BLUE:
                        return Color( 30, 70, 180, alpha );
                case HUD_COLOR_CYAN:
                        return Color( 0, 180, 180, alpha );
                case HUD_COLOR_LIME:
                        return Color( 30, 180, 30, alpha );
                case HUD_COLOR_RAINBOW:
                        return GetRainbowColor( 1.0f );  // Offset for variation
                default:
                        return Color( 180, 30, 30, alpha );
        }
}

#endif // CS_HUD_COLOR_H
