// Penetration powers from installed CS2 weapons.vdata_c, 2026-10-01.
#ifndef CS_WEAPON_PENETRATION_H
#define CS_WEAPON_PENETRATION_H
#include "cs_weapon_parse.h"
inline float CSWeaponPenetrationPower(CSWeaponID id)
{
 switch(id)
 {
 case WEAPON_P228: return 1.0f;
 case WEAPON_GLOCK: return 1.0f;
 case WEAPON_SCOUT: return 2.5f;
 case WEAPON_XM1014: return 1.0f;
 case WEAPON_MAC10: return 1.0f;
 case WEAPON_AUG: return 2.0f;
 case WEAPON_ELITE: return 1.0f;
 case WEAPON_FIVESEVEN: return 1.0f;
 case WEAPON_UMP45: return 1.0f;
 case WEAPON_SG550: return 2.5f;
 case WEAPON_GALIL: return 2.0f;
 case WEAPON_FAMAS: return 2.0f;
 case WEAPON_USP: return 1.0f;
 case WEAPON_AWP: return 2.5f;
 case WEAPON_MP5NAVY: return 1.0f;
 case WEAPON_M249: return 2.0f;
 case WEAPON_M3: return 1.0f;
 case WEAPON_M4A1: return 2.0f;
 case WEAPON_TMP: return 1.0f;
 case WEAPON_G3SG1: return 2.5f;
 case WEAPON_DEAGLE: return 2.0f;
 case WEAPON_SG552: return 2.0f;
 case WEAPON_AK47: return 2.0f;
 case WEAPON_P90: return 1.0f;
 case WEAPON_CZ75: return 1.0f;
 case WEAPON_TEC9: return 1.0f;
 case WEAPON_REVOLVER: return 2.0f;
 case WEAPON_P2000: return 1.0f;
 case WEAPON_M4A4: return 2.0f;
 case WEAPON_MP7: return 1.0f;
 case WEAPON_BIZON: return 1.0f;
 case WEAPON_MAG7: return 1.0f;
 case WEAPON_SAWEDOFF: return 1.0f;
 case WEAPON_NEGEV: return 2.0f;
 default: return 0;
 }
}
#endif
