//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef CS_KNIFE_MODELS_H
#define CS_KNIFE_MODELS_H
#ifdef _WIN32
#pragma once
#endif

struct KnifeModelInfo_t
{
	const char *name;
	const char *displayName;
	const char *v_model;
	const char *w_model;
};

static const KnifeModelInfo_t s_KnifeModels[] =
{
	{ "default",  "Default Knife", "models/weapons/v_knife_t.mdl",               "models/weapons/w_knife_t.mdl" },
	{ "gut",      "Gut Knife",     "models/codex_knives/gut/v_knife_t.mdl",      "models/weapons/w_knife_t.mdl" },
	{ "karambit", "Karambit",      "models/codex_knives/karambit/v_knife_t.mdl", "models/weapons/w_knife_t.mdl" },
	{ "m9",       "M9 Bayonet",    "models/codex_knives/m9/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" }
};

enum { CS_KNIFE_MODEL_COUNT = sizeof( s_KnifeModels ) / sizeof( s_KnifeModels[0] ) };

inline int CSClampKnifeChoice( int nChoice )
{
	if ( nChoice < 0 )
		return 0;
	if ( nChoice >= CS_KNIFE_MODEL_COUNT )
		return CS_KNIFE_MODEL_COUNT - 1;
	return nChoice;
}

#endif // CS_KNIFE_MODELS_H
