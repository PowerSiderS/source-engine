#include "cbase.h"
#include "weapon_csbasegun.h"
#include "baseviewmodel_shared.h"
#include "in_buttons.h"
#ifdef CLIENT_DLL
#include "c_cs_player.h"
#define CImportedArsenalGun C_ImportedArsenalGun
#define CArsenalCZ75 C_ArsenalCZ75
#define CArsenalTec9 C_ArsenalTec9
#define CArsenalRevolver C_ArsenalRevolver
#define CArsenalP2000 C_ArsenalP2000
#define CArsenalM4A4 C_ArsenalM4A4
#define CArsenalMP7 C_ArsenalMP7
#define CArsenalBizon C_ArsenalBizon
#define CArsenalMAG7 C_ArsenalMAG7
#define CArsenalNegev C_ArsenalNegev
#else
#include "cs_player.h"
#endif
#include "tier0/memdbgon.h"

// Independent gameplay entities. Changing a cosmetic never changes these IDs.
class CImportedArsenalGun : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS(CImportedArsenalGun,CWeaponCSBaseGun);
	DECLARE_NETWORKCLASS(); DECLARE_PREDICTABLE();
	CImportedArsenalGun() { m_CockReady=0; }
	void PrimaryAttack()
	{
		if (GetWeaponID()==WEAPON_REVOLVER && Clip1()>0)
		{
			if (m_CockReady==0)
			{
				m_CockReady=gpGlobals->curtime+0.2f;
				CCSPlayer *p=GetPlayerOwner(); CBaseViewModel *vm=p ? p->GetViewModel(0) : NULL;
				int cock=vm ? vm->LookupSequence("cock") : -1;
				if (cock>=0) CBaseCombatWeapon::SendViewModelAnim(cock);
				return;
			}
			if (gpGlobals->curtime<m_CockReady) return;
		}
		m_weaponMode=Primary_Mode;
		if (CSBaseGunFire(GetCSWpnData().m_flCycleTime[Primary_Mode],Primary_Mode)) m_CockReady=0;
	}
	void SecondaryAttack()
	{
		if (GetWeaponID()!=WEAPON_REVOLVER) return;
		m_CockReady=0; m_weaponMode=Secondary_Mode;
		if (CSBaseGunFire(GetCSWpnData().m_flCycleTime[Secondary_Mode],Secondary_Mode)) m_flNextSecondaryAttack=m_flNextPrimaryAttack;
	}
	void ItemPostFrame()
	{
		CCSPlayer *p=GetPlayerOwner(); if (!p || !(p->m_nButtons&IN_ATTACK)) m_CockReady=0;
		BaseClass::ItemPostFrame();
	}
	bool Reload() { m_CockReady=0; return BaseClass::Reload(); }
	float GetInaccuracy() const
	{
		float inaccuracy=BaseClass::GetInaccuracy();
		// Custom Negev spool-up profile: early rounds are less accurate.
		if (GetWeaponID()==WEAPON_NEGEV && m_flRecoilIndex<15) inaccuracy*=1.0f+3.0f*(15.0f-m_flRecoilIndex)/15.0f;
		return inaccuracy;
	}
private:
	CNetworkVar(float,m_CockReady);
};
IMPLEMENT_NETWORKCLASS_ALIASED(ImportedArsenalGun,DT_ImportedArsenalGun)
BEGIN_NETWORK_TABLE(CImportedArsenalGun,DT_ImportedArsenalGun)
#ifdef CLIENT_DLL
	RecvPropFloat(RECVINFO(m_CockReady)),
#else
	SendPropFloat(SENDINFO(m_CockReady)),
#endif
END_NETWORK_TABLE()
#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CImportedArsenalGun)
	DEFINE_PRED_FIELD(m_CockReady,FIELD_FLOAT,FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

#define ARSENAL_GUN(Name,Entity,ID) \
class C##Name : public CImportedArsenalGun { public: DECLARE_CLASS(C##Name,CImportedArsenalGun); DECLARE_NETWORKCLASS(); DECLARE_PREDICTABLE(); CSWeaponID GetWeaponID() const {return ID;} }; \
IMPLEMENT_NETWORKCLASS_ALIASED(Name,DT_##Name) \
BEGIN_NETWORK_TABLE(C##Name,DT_##Name) END_NETWORK_TABLE() \
BEGIN_PREDICTION_DATA(C##Name) END_PREDICTION_DATA() \
LINK_ENTITY_TO_CLASS(Entity,C##Name); PRECACHE_WEAPON_REGISTER(Entity);

ARSENAL_GUN(ArsenalCZ75,weapon_cz75,WEAPON_CZ75)
ARSENAL_GUN(ArsenalTec9,weapon_tec9,WEAPON_TEC9)
ARSENAL_GUN(ArsenalRevolver,weapon_revolver,WEAPON_REVOLVER)
ARSENAL_GUN(ArsenalP2000,weapon_p2000,WEAPON_P2000)
ARSENAL_GUN(ArsenalM4A4,weapon_m4a4,WEAPON_M4A4)
ARSENAL_GUN(ArsenalMP7,weapon_mp7,WEAPON_MP7)
ARSENAL_GUN(ArsenalBizon,weapon_bizon,WEAPON_BIZON)
ARSENAL_GUN(ArsenalMAG7,weapon_mag7,WEAPON_MAG7)
ARSENAL_GUN(ArsenalNegev,weapon_negev,WEAPON_NEGEV)
