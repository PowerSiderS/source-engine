#ifndef CS_WEAPON_PRESENTATION_H
#define CS_WEAPON_PRESENTATION_H
#include "cs_weapon_parse.h"
struct CSWeaponPresentation { CSWeaponID id; const char *view; const char *world; };
inline const CSWeaponPresentation *CSDefaultWeaponPresentation(CSWeaponID id)
{
 static const CSWeaponPresentation models[]={
  { WEAPON_AK47, "models/sourceadvanced/cs2/v_rif_ak47.mdl", "models/sourceadvanced/w_weapon_ak-47_ak-47.mdl" },
  { WEAPON_AUG, "models/sourceadvanced/cs2/v_rif_aug.mdl", "models/sourceadvanced/w_weapon_aug_aug.mdl" },
  { WEAPON_AWP, "models/sourceadvanced/cs2/v_snip_awp.mdl", "models/sourceadvanced/w_weapon_awp_awp.mdl" },
  { WEAPON_DEAGLE, "models/sourceadvanced/cs2/v_pist_deagle.mdl", "models/sourceadvanced/w_weapon_desert_eagle_desert_eagle.mdl" },
  { WEAPON_ELITE, "models/sourceadvanced/cs2/v_pist_elite.mdl", "models/sourceadvanced/w_weapon_dual_elites_dual_berettas.mdl" },
  { WEAPON_FAMAS, "models/sourceadvanced/cs2/v_rif_famas.mdl", "models/sourceadvanced/w_weapon_famas_famas.mdl" },
  { WEAPON_FIVESEVEN, "models/sourceadvanced/cs2/v_pist_fiveseven.mdl", "models/sourceadvanced/w_weapon_five-seven_five_seven.mdl" },
  { WEAPON_G3SG1, "models/sourceadvanced/cs2/v_snip_g3sg1.mdl", "models/sourceadvanced/w_weapon_g3sg-1_g3sg1.mdl" },
  { WEAPON_GALIL, "models/sourceadvanced/cs2/v_rif_galil.mdl", "models/sourceadvanced/w_weapon_galil_galil_ar.mdl" },
  { WEAPON_GLOCK, "models/sourceadvanced/cs2/v_pist_glock18.mdl", "models/sourceadvanced/w_weapon_glock_18_glock-18.mdl" },
  { WEAPON_M249, "models/sourceadvanced/cs2/v_mach_m249para.mdl", "models/sourceadvanced/w_weapon_m249_m249.mdl" },
  { WEAPON_M3, "models/sourceadvanced/cs2/v_shot_m3super90.mdl", "models/sourceadvanced/w_weapon_m3_nova.mdl" },
  { WEAPON_M4A1, "models/sourceadvanced/cs2/v_rif_m4a1.mdl", "models/sourceadvanced/w_weapon_m4a1_m4a1-s.mdl" },
  { WEAPON_MAC10, "models/sourceadvanced/cs2/v_smg_mac10.mdl", "models/sourceadvanced/w_weapon_mac10_mac-10.mdl" },
  { WEAPON_MP5NAVY, "models/sourceadvanced/cs2/v_smg_mp5.mdl", "models/sourceadvanced/w_weapon_mp5_mp5-sd.mdl" },
  { WEAPON_P228, "models/sourceadvanced/cs2/v_pist_p228.mdl", "models/sourceadvanced/w_weapon_p228_p250.mdl" },
  { WEAPON_P90, "models/sourceadvanced/cs2/v_smg_p90.mdl", "models/sourceadvanced/w_weapon_p90_p90.mdl" },
  { WEAPON_SCOUT, "models/sourceadvanced/cs2/v_snip_scout.mdl", "models/sourceadvanced/w_weapon_scout_ssg_08.mdl" },
  { WEAPON_SG550, "models/sourceadvanced/cs2/v_snip_sg550.mdl", "models/sourceadvanced/w_weapon_sg-550_scar-20.mdl" },
  { WEAPON_SG552, "models/sourceadvanced/cs2/v_rif_sg552.mdl", "models/sourceadvanced/w_weapon_sg-552_sg_553.mdl" },
  { WEAPON_TMP, "models/sourceadvanced/cs2/v_smg_tmp.mdl", "models/sourceadvanced/w_weapon_tmp_mp9.mdl" },
  { WEAPON_UMP45, "models/sourceadvanced/cs2/v_smg_ump45.mdl", "models/sourceadvanced/w_weapon_ump_ump-45.mdl" },
  { WEAPON_USP, "models/sourceadvanced/cs2/v_pist_usp.mdl", "models/sourceadvanced/w_weapon_usp_usp-s.mdl" },
  { WEAPON_XM1014, "models/sourceadvanced/cs2/v_shot_xm1014.mdl", "models/sourceadvanced/w_weapon_xm1014_xm1014.mdl" },
  { WEAPON_KNIFE, "models/sourceadvanced/c_weapon_knife_ct_knife.mdl", "models/sourceadvanced/w_weapon_knife_ct_knife.mdl" }
 };
 for(unsigned int i=0;i<sizeof(models)/sizeof(models[0]);++i) if(models[i].id==id)return &models[i];
 return NULL;
}
inline bool CSIsUnportedFirearm(CSWeaponID id)
{
 switch(id) { case WEAPON_CZ75: case WEAPON_TEC9: case WEAPON_REVOLVER: case WEAPON_P2000: case WEAPON_M4A4: case WEAPON_MP7: case WEAPON_BIZON: case WEAPON_MAG7: case WEAPON_SAWEDOFF: case WEAPON_NEGEV: return true; default: return false; }
}
#endif
