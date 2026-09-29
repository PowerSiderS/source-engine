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
	{ "default",        "Default Knife",   "models/weapons/v_knife_t.mdl",                     "models/weapons/w_knife_t.mdl" },
	{ "bayonet",        "Bayonet",         "models/codex_knives/bayoneta/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" },
	{ "bowie",          "Bowie Knife",     "models/codex_knives/bowie/v_knife_t.mdl",          "models/weapons/w_knife_t.mdl" },
	{ "butterfly",      "Butterfly Knife", "models/codex_knives/butterfly/v_knife_t.mdl",      "models/weapons/w_knife_t.mdl" },
	{ "classic",        "Classic Knife",   "models/codex_knives/classic/v_knife_t.mdl",        "models/weapons/w_knife_t.mdl" },
	{ "falchion",       "Falchion Knife",  "models/codex_knives/falchion/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" },
	{ "flip",           "Flip Knife",      "models/codex_knives/flip/v_knife_t.mdl",           "models/weapons/w_knife_t.mdl" },
	{ "gut",            "Gut Knife",       "models/codex_knives/gut/v_knife_t.mdl",            "models/weapons/w_knife_t.mdl" },
	{ "huntsman",       "Huntsman Knife",  "models/codex_knives/huntsman/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" },
	{ "karambit",       "Karambit",        "models/codex_knives/karambit/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" },
	{ "kukri",          "Kukri Knife",     "models/codex_knives/kukri/v_knife_t.mdl",          "models/weapons/w_knife_t.mdl" },
	{ "m9",             "M9 Bayonet",      "models/codex_knives/m9/v_knife_t.mdl",             "models/weapons/w_knife_t.mdl" },
	{ "shadow_daggers", "Shadow Daggers",  "models/codex_knives/shadow_daggers/v_knife_t.mdl", "models/weapons/w_knife_t.mdl" },
	{ "skeleton",       "Skeleton Knife",  "models/codex_knives/skeleton/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" },
	{ "stiletto",       "Stiletto Knife",  "models/codex_knives/stiletto/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" },
	{ "survival",       "Survival Knife",  "models/codex_knives/survival/v_knife_t.mdl",       "models/weapons/w_knife_t.mdl" },
	{ "talon",          "Talon Knife",     "models/codex_knives/talon/v_knife_t.mdl",          "models/weapons/w_knife_t.mdl" }
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
