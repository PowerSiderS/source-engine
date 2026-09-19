//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_csbasegun.h"


#if defined( CLIENT_DLL )

	#define CWeaponM249 C_WeaponM249
	#include "c_cs_player.h"

#else

	#include "cs_player.h"

#endif


class CWeaponM249 : public CWeaponCSBaseGun
{
public:
	DECLARE_CLASS( CWeaponM249, CWeaponCSBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponM249();

	virtual void PrimaryAttack();

 	virtual float GetInaccuracy() const;

	virtual CSWeaponID GetWeaponID( void ) const		{ return WEAPON_M249; }


private:
	CWeaponM249( const CWeaponM249 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM249, DT_WeaponM249 )

BEGIN_NETWORK_TABLE( CWeaponM249, DT_WeaponM249 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM249 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m249, CWeaponM249 );
PRECACHE_WEAPON_REGISTER( weapon_m249 );



CWeaponM249::CWeaponM249()
{
}

float CWeaponM249::GetInaccuracy() const
{
	if ( weapon_accuracy_model.GetInt() == 1 )
	{
		CCSPlayer *pPlayer = GetPlayerOwner();
		if ( !pPlayer )
			return 0.0f;

		if ( !FBitSet( pPlayer->GetFlags(), FL_ONGROUND ) )
			return 0.045f + 0.5f * m_flAccuracy;
		else if (pPlayer->GetAbsVelocity().Length2D() > 140)
			return 0.045f + 0.095f * m_flAccuracy;
		else
			return 0.03f * m_flAccuracy;
	}
	else
		return BaseClass::GetInaccuracy();
}

void CWeaponM249::PrimaryAttack( void )
{
	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CSBaseGunFire( GetCSWpnData().m_flCycleTime[Primary_Mode], Primary_Mode ) )
		return;
	
	pPlayer = GetPlayerOwner();

	// CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
	if ( !pPlayer )
		return;

	
	
}
