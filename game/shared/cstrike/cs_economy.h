#ifndef CS_ECONOMY_H
#define CS_ECONOMY_H
#include "cs_weapon_parse.h"

// Source: installed Valve CS2 scripts/weapons.vdata_c (CRC verified), 2026-10-01.
// Source 1 classes without a CS2 counterpart use the closest existing weapon.
struct CSEconomyWeapon { CSWeaponID id; const char *reference; int price; int killAward; };
static const CSEconomyWeapon g_CSEconomyWeapons[] =
{
    { WEAPON_P228, "p250", 300, 300 },
    { WEAPON_GLOCK, "glock", 200, 300 },
    { WEAPON_SCOUT, "ssg08", 1700, 300 },
    { WEAPON_HEGRENADE, "hegrenade", 300, 300 },
    { WEAPON_XM1014, "xm1014", 2000, 600 },
    { WEAPON_MAC10, "mac10", 1050, 600 },
    { WEAPON_AUG, "aug", 3300, 300 },
    { WEAPON_SMOKEGRENADE, "smokegrenade", 300, 300 },
    { WEAPON_ELITE, "elite", 300, 300 },
    { WEAPON_FIVESEVEN, "fiveseven", 500, 300 },
    { WEAPON_UMP45, "ump45", 1200, 600 },
    { WEAPON_SG550, "scar20", 5000, 300 },
    { WEAPON_GALIL, "galilar", 1800, 300 },
    { WEAPON_FAMAS, "famas", 1950, 300 },
    { WEAPON_USP, "usp_silencer", 200, 300 },
    { WEAPON_AWP, "awp", 4750, 100 },
    { WEAPON_MP5NAVY, "mp5sd", 1400, 600 },
    { WEAPON_M249, "m249", 5200, 300 },
    { WEAPON_M3, "nova", 1050, 900 },
    { WEAPON_M4A1, "m4a1_silencer", 2900, 300 },
    { WEAPON_TMP, "mp9", 1250, 600 },
    { WEAPON_G3SG1, "g3sg1", 5000, 300 },
    { WEAPON_FLASHBANG, "flashbang", 200, 300 },
    { WEAPON_DEAGLE, "deagle", 700, 300 },
    { WEAPON_SG552, "sg556", 3000, 300 },
    { WEAPON_AK47, "ak47", 2700, 300 },
    { WEAPON_KNIFE, "knife", 0, 1500 },
    { WEAPON_P90, "p90", 2350, 300 },
    { WEAPON_CZ75, "cz75a", 500, 300 },
    { WEAPON_TEC9, "tec9", 500, 300 },
    { WEAPON_REVOLVER, "revolver", 600, 300 },
    { WEAPON_P2000, "hkp2000", 200, 300 },
    { WEAPON_M4A4, "m4a1", 2900, 300 },
    { WEAPON_MP7, "mp7", 1400, 600 },
    { WEAPON_BIZON, "bizon", 1300, 600 },
    { WEAPON_MAG7, "mag7", 1300, 900 },
    { WEAPON_SAWEDOFF, "sawedoff", 1100, 900 },
    { WEAPON_NEGEV, "negev", 1700, 300 },
};

inline const CSEconomyWeapon *CSEconomyWeaponForID( CSWeaponID id )
{
    for (unsigned int i=0; i<sizeof(g_CSEconomyWeapons)/sizeof(g_CSEconomyWeapons[0]); ++i)
        if (g_CSEconomyWeapons[i].id == id) return &g_CSEconomyWeapons[i];
    return NULL;
}
inline int CSEconomyLossLevel( int level ) { return level < 0 ? 0 : level > 4 ? 4 : level; }
inline int CSEconomyLossAward( int level ) { return 1400 + 500 * CSEconomyLossLevel(level); }
inline int CSEconomyNextLossLevel( int level, bool won )
{
    return CSEconomyLossLevel( CSEconomyLossLevel(level) + (won ? -1 : 1) );
}
#endif
