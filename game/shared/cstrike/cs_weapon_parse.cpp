//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "cs_weapon_presentation.h"
#include <KeyValues.h>
#include "cs_weapon_parse.h"
#include "cs_legacy_weapon_tuning.h"
#include "cs_2015_weapon_profiles.h"
#include "cs2_weapon_profiles.h"
#include "cs_economy.h"
#include "cs_shareddefs.h"
#include "weapon_csbase.h"
#include "icvar.h"
#include "cs_gamerules.h"
#include "cs_blackmarket.h"
#include "filesystem.h"


//--------------------------------------------------------------------------------------------------------
struct WeaponTypeInfo
{
	CSWeaponType type;
	const char * name;
};


//--------------------------------------------------------------------------------------------------------
WeaponTypeInfo s_weaponTypeInfo[] =
{
	{ WEAPONTYPE_KNIFE,			"Knife" },
	{ WEAPONTYPE_PISTOL,		"Pistol" },
	{ WEAPONTYPE_SUBMACHINEGUN,	"Submachine Gun" },	// First match is printable
	{ WEAPONTYPE_SUBMACHINEGUN,	"submachinegun" },
	{ WEAPONTYPE_SUBMACHINEGUN,	"smg" },
	{ WEAPONTYPE_RIFLE,			"Rifle" },
	{ WEAPONTYPE_SHOTGUN,		"Shotgun" },
	{ WEAPONTYPE_SNIPER_RIFLE,	"Sniper Rifle" },	// First match is printable
	{ WEAPONTYPE_SNIPER_RIFLE,	"SniperRifle" },
	{ WEAPONTYPE_MACHINEGUN,	"Machine Gun" },		// First match is printable
	{ WEAPONTYPE_MACHINEGUN,	"machinegun" },
	{ WEAPONTYPE_MACHINEGUN,	"mg" },
	{ WEAPONTYPE_C4,			"C4" },
	{ WEAPONTYPE_GRENADE,		"Grenade" },
};


struct WeaponNameInfo
{
	CSWeaponID id;
	const char *name;
};

WeaponNameInfo s_weaponNameInfo[] =
{
	{ WEAPON_P228,				"weapon_p228" },
	{ WEAPON_GLOCK,				"weapon_glock" },
	{ WEAPON_SCOUT,				"weapon_scout" },
	{ WEAPON_HEGRENADE,			"weapon_hegrenade" },
	{ WEAPON_XM1014,			"weapon_xm1014" },
	{ WEAPON_C4,				"weapon_c4" },
	{ WEAPON_MAC10,				"weapon_mac10" },
	{ WEAPON_AUG,				"weapon_aug" },
	{ WEAPON_SMOKEGRENADE,		"weapon_smokegrenade" },
	{ WEAPON_ELITE,				"weapon_elite" },
	{ WEAPON_FIVESEVEN,			"weapon_fiveseven" },
	{ WEAPON_UMP45,				"weapon_ump45" },
	{ WEAPON_SG550,				"weapon_sg550" },

	{ WEAPON_GALIL,				"weapon_galil" },
	{ WEAPON_FAMAS,				"weapon_famas" },
	{ WEAPON_USP,				"weapon_usp" },
	{ WEAPON_AWP,				"weapon_awp" },
	{ WEAPON_MP5NAVY,			"weapon_mp5navy" },
	{ WEAPON_M249,				"weapon_m249" },
	{ WEAPON_M3,				"weapon_m3" },
	{ WEAPON_M4A1,				"weapon_m4a1" },
	{ WEAPON_TMP,				"weapon_tmp" },
	{ WEAPON_G3SG1,				"weapon_g3sg1" },
	{ WEAPON_FLASHBANG,			"weapon_flashbang" },
	{ WEAPON_DEAGLE,			"weapon_deagle" },
	{ WEAPON_SG552,				"weapon_sg552" },
	{ WEAPON_AK47,				"weapon_ak47" },
	{ WEAPON_KNIFE,				"weapon_knife" },
	{ WEAPON_P90,				"weapon_p90" },

	// not sure any of these are needed
	{ WEAPON_SHIELDGUN,			"weapon_shieldgun" },
	{ WEAPON_CZ75, "weapon_cz75" }, { WEAPON_TEC9, "weapon_tec9" },
	{ WEAPON_REVOLVER, "weapon_revolver" }, { WEAPON_P2000, "weapon_p2000" },
	{ WEAPON_M4A4, "weapon_m4a4" }, { WEAPON_MP7, "weapon_mp7" },
	{ WEAPON_BIZON, "weapon_bizon" }, { WEAPON_MAG7, "weapon_mag7" },
	{ WEAPON_SAWEDOFF, "weapon_sawedoff" }, { WEAPON_NEGEV, "weapon_negev" },
	{ WEAPON_KEVLAR,			"weapon_kevlar" },
	{ WEAPON_ASSAULTSUIT,		"weapon_assaultsuit" },
	{ WEAPON_NVG,				"weapon_nvg" },

	{ WEAPON_NONE,				"weapon_none" },
};



//--------------------------------------------------------------------------------------------------------------


CCSWeaponInfo g_EquipmentInfo[MAX_EQUIPMENT];

void PrepareEquipmentInfo( void )
{
    // MoeMod : dont use memset here
    for(int i = 0; i < MAX_EQUIPMENT; ++i)
        g_EquipmentInfo[i] = {};

	g_EquipmentInfo[2].SetWeaponPrice( CSGameRules()->GetBlackMarketPriceForWeapon( WEAPON_KEVLAR ) );
	g_EquipmentInfo[2].SetDefaultPrice( KEVLAR_PRICE );
	g_EquipmentInfo[2].SetPreviousPrice( CSGameRules()->GetBlackMarketPreviousPriceForWeapon( WEAPON_KEVLAR ) );
	g_EquipmentInfo[2].m_iTeam = TEAM_UNASSIGNED;
	Q_strcpy( g_EquipmentInfo[2].szClassName, "weapon_vest" );

#ifdef CLIENT_DLL
	g_EquipmentInfo[2].iconActive = new CHudTexture;
	g_EquipmentInfo[2].iconActive->cCharacterInFont = 't';
#endif

	g_EquipmentInfo[1].SetWeaponPrice( CSGameRules()->GetBlackMarketPriceForWeapon( WEAPON_ASSAULTSUIT ) );
	g_EquipmentInfo[1].SetDefaultPrice( ASSAULTSUIT_PRICE );
	g_EquipmentInfo[1].SetPreviousPrice( CSGameRules()->GetBlackMarketPreviousPriceForWeapon( WEAPON_ASSAULTSUIT ) );
	g_EquipmentInfo[1].m_iTeam = TEAM_UNASSIGNED;
	Q_strcpy( g_EquipmentInfo[1].szClassName, "weapon_vesthelm" );

#ifdef CLIENT_DLL
	g_EquipmentInfo[1].iconActive = new CHudTexture;
	g_EquipmentInfo[1].iconActive->cCharacterInFont = 'u';
#endif

	g_EquipmentInfo[0].SetWeaponPrice( CSGameRules()->GetBlackMarketPriceForWeapon( WEAPON_NVG ) );
	g_EquipmentInfo[0].SetPreviousPrice( CSGameRules()->GetBlackMarketPreviousPriceForWeapon( WEAPON_NVG ) );
	g_EquipmentInfo[0].SetDefaultPrice( NVG_PRICE );
	g_EquipmentInfo[0].m_iTeam = TEAM_UNASSIGNED;
	Q_strcpy( g_EquipmentInfo[0].szClassName, "weapon_nvgs" );

#ifdef CLIENT_DLL
	g_EquipmentInfo[0].iconActive = new CHudTexture;
	g_EquipmentInfo[0].iconActive->cCharacterInFont = 's';
#endif

}

//--------------------------------------------------------------------------------------------------------------
CCSWeaponInfo * GetWeaponInfo( CSWeaponID weaponID )
{
	if ( weaponID == WEAPON_NONE )
		return NULL;

	if ( weaponID >= WEAPON_KEVLAR )
	{
		int iIndex = (WEAPON_MAX - weaponID) - 1;

		return &g_EquipmentInfo[iIndex];

	}

	const char *weaponName = WeaponIdAsString(weaponID);
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( weaponName );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return NULL;
	}

	FileWeaponInfo_t *pFileInfo=GetFileWeaponInfoFromHandle(hWpnInfo);
	// The generic null weapon descriptor is smaller than CCSWeaponInfo. With
	// RTTI disabled, casting it would read outside the object on missing content.
	if(!pFileInfo || !pFileInfo->bParsedScript)return NULL;
	return static_cast<CCSWeaponInfo *>(pFileInfo);
}

//--------------------------------------------------------------------------------------------------------
const char* WeaponClassAsString( CSWeaponType weaponType )
{
	for ( int i = 0; i < ARRAYSIZE(s_weaponTypeInfo); ++i )
	{
		if ( s_weaponTypeInfo[i].type == weaponType )
		{
			return s_weaponTypeInfo[i].name;
		}
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromString( const char* weaponType )
{
	for ( int i = 0; i < ARRAYSIZE(s_weaponTypeInfo); ++i )
	{
		if ( !Q_stricmp( s_weaponTypeInfo[i].name, weaponType ) )
		{
			return s_weaponTypeInfo[i].type;
		}
	}

	return WEAPONTYPE_UNKNOWN;
}


//--------------------------------------------------------------------------------------------------------
CSWeaponType WeaponClassFromWeaponID( CSWeaponID weaponID )
{
	const char *weaponStr = WeaponIDToAlias( weaponID );
	const char *translatedAlias = GetTranslatedWeaponAlias( weaponStr );

	char wpnName[128];
	Q_snprintf( wpnName, sizeof( wpnName ), "weapon_%s", translatedAlias );
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( wpnName );
	if ( hWpnInfo != GetInvalidWeaponInfoHandle() )
	{
		CCSWeaponInfo *pWeaponInfo = dynamic_cast< CCSWeaponInfo* >( GetFileWeaponInfoFromHandle( hWpnInfo ) );
		if ( pWeaponInfo )
		{
			return pWeaponInfo->m_WeaponType;
		}
	}

	return WEAPONTYPE_UNKNOWN;
}


//--------------------------------------------------------------------------------------------------------
const char * WeaponIdAsString( CSWeaponID weaponID )
{
	for ( int i = 0; i < ARRAYSIZE(s_weaponNameInfo); ++i )
	{
		if (s_weaponNameInfo[i].id == weaponID )
			return s_weaponNameInfo[i].name;
	}

	return NULL;
}


//--------------------------------------------------------------------------------------------------------
CSWeaponID WeaponIdFromString( const char *szWeaponName )
{
	for ( int i = 0; i < ARRAYSIZE(s_weaponNameInfo); ++i )
	{
		if ( Q_stricmp(s_weaponNameInfo[i].name, szWeaponName) == 0 )
			return s_weaponNameInfo[i].id;
	}

	return WEAPON_NONE;
}


//--------------------------------------------------------------------------------------------------------
void ParseVector( KeyValues *keyValues, const char *keyName, Vector& vec )
{
	vec.x = vec.y = vec.z = 0.0f;

	if ( !keyValues || !keyName )
		return;

	const char *vecString = keyValues->GetString( keyName, "0 0 0" );
	if ( vecString && *vecString )
	{
		float x = 0.0f, y = 0.0f, z = 0.0f;
		if ( 3 == sscanf( vecString, "%f %f %f", &x, &y, &z ) )
		{
			vec.x = x;
			vec.y = y;
			vec.z = z;
		}
	}
}


FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CCSWeaponInfo;
}


CCSWeaponInfo::CCSWeaponInfo()
{
	m_flMaxSpeed = 1; // This should always be set in the script.
	m_flMaxSpeedAlt = 1;
	m_flHeadshotMultiplier = 4.0f;
	m_szAddonModel[0] = 0;

	m_fRecoilAngle[0] = m_fRecoilAngle[1] = 0.0f;
	m_fRecoilAngleVariance[0] = m_fRecoilAngleVariance[1] = 0.0f;
	m_fRecoilMagnitude[0] = m_fRecoilMagnitude[1] = 0.0f;
	m_fRecoilMagnitudeVariance[0] = m_fRecoilMagnitudeVariance[1] = 0.0f;
	m_iRecoilSeed = 0;
	m_fRecoveryTimeStand = m_fRecoveryTimeStandFinal = 1.0f;
	m_fRecoveryTimeCrouch = m_fRecoveryTimeCrouchFinal = 1.0f;
	m_iRecoveryTransitionStartBullet = 0;
	m_iRecoveryTransitionEndBullet = 0;
}

int	CCSWeaponInfo::GetWeaponPrice( void ) const
{
	return m_iWeaponPrice;
}

int	CCSWeaponInfo::GetDefaultPrice( void )
{
	return m_iDefaultPrice;
}

int	CCSWeaponInfo::GetPrevousPrice( void )
{
	return m_iPreviousPrice;
}


void CCSWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_flMaxSpeed = (float)pKeyValuesData->GetInt( "MaxPlayerSpeed", 1 );
	m_flMaxSpeedAlt = (float)pKeyValuesData->GetInt( "MaxPlayerSpeedAlt", (int)m_flMaxSpeed );

	m_iDefaultPrice = m_iWeaponPrice = pKeyValuesData->GetInt( "WeaponPrice", -1 );
	if ( m_iWeaponPrice == -1 )
	{
		// This weapon should have the price in its script.
		Assert( false );
	}

	if ( CSGameRules()->IsBlackMarket() )
	{
		CSWeaponID iWeaponID = AliasToWeaponID( GetTranslatedWeaponAlias ( szWeaponName ) );

		m_iDefaultPrice = m_iWeaponPrice;
		m_iPreviousPrice = CSGameRules()->GetBlackMarketPreviousPriceForWeapon( iWeaponID );
		m_iWeaponPrice = CSGameRules()->GetBlackMarketPriceForWeapon( iWeaponID );
	}
		
	m_flArmorRatio				= pKeyValuesData->GetFloat( "WeaponArmorRatio", 1 );
	m_iCrosshairMinDistance		= pKeyValuesData->GetInt( "CrosshairMinDistance", 4 );
	m_iCrosshairDeltaDistance	= pKeyValuesData->GetInt( "CrosshairDeltaDistance", 3 );
	m_bCanUseWithShield			= !!pKeyValuesData->GetInt( "CanEquipWithShield", false );
	m_flMuzzleScale				= pKeyValuesData->GetFloat( "MuzzleFlashScale", 1 );

	const char *pMuzzleFlashStyle = pKeyValuesData->GetString( "MuzzleFlashStyle", "CS_MUZZLEFLASH_NORM" );
	
	if( pMuzzleFlashStyle )
	{
		if ( Q_stricmp( pMuzzleFlashStyle, "CS_MUZZLEFLASH_X" ) == 0 )
		{
			m_iMuzzleFlashStyle = CS_MUZZLEFLASH_X;
		}
		else if ( Q_stricmp( pMuzzleFlashStyle, "CS_MUZZLEFLASH_NONE" ) == 0 )
		{
			m_iMuzzleFlashStyle = CS_MUZZLEFLASH_NONE;
		}
		else
		{
			m_iMuzzleFlashStyle = CS_MUZZLEFLASH_NORM;
		}
	}
	else
	{
		Assert( false );
	}

	m_iPenetration		= pKeyValuesData->GetInt( "Penetration", 1 );
	m_iDamage			= pKeyValuesData->GetInt( "Damage", 42 ); // Douglas Adams 1952 - 2001
	m_flHeadshotMultiplier = pKeyValuesData->GetFloat("HeadshotMultiplier",4.0f);
	m_flRange			= pKeyValuesData->GetFloat( "Range", 8192.0f );
	m_flRangeModifier	= pKeyValuesData->GetFloat( "RangeModifier", 0.98f );
	m_iBullets			= pKeyValuesData->GetInt( "Bullets", 1 );
	m_flCycleTime[0]		= pKeyValuesData->GetFloat( "CycleTime", 0.15 );
	m_flCycleTime[1]		= pKeyValuesData->GetFloat( "CycleTimeAlt", m_flCycleTime[0] );
	m_bAccuracyQuadratic= pKeyValuesData->GetInt( "AccuracyQuadratic", 0 );
	m_flAccuracyDivisor	= pKeyValuesData->GetFloat( "AccuracyDivisor", -1 ); // -1 = off
	m_flAccuracyOffset	= pKeyValuesData->GetFloat( "AccuracyOffset", 0 );
	m_flMaxInaccuracy	= pKeyValuesData->GetFloat( "MaxInaccuracy", 0 );

	// new accuracy model parameters
	m_fSpread[0]				= pKeyValuesData->GetFloat("Spread", 0.0f);
	m_fInaccuracyCrouch[0]		= pKeyValuesData->GetFloat("InaccuracyCrouch", 0.0f);
	m_fInaccuracyStand[0]		= pKeyValuesData->GetFloat("InaccuracyStand", 0.0f);
	m_fInaccuracyJump[0]		= pKeyValuesData->GetFloat("InaccuracyJump", 0.0f);
	m_fInaccuracyLand[0]		= pKeyValuesData->GetFloat("InaccuracyLand", 0.0f);
	m_fInaccuracyLadder[0]		= pKeyValuesData->GetFloat("InaccuracyLadder", 0.0f);
	m_fInaccuracyImpulseFire[0]	= pKeyValuesData->GetFloat("InaccuracyFire", 0.0f);
	m_fInaccuracyMove[0]		= pKeyValuesData->GetFloat("InaccuracyMove", 0.0f);

	m_fSpread[1]				= pKeyValuesData->GetFloat("SpreadAlt", 0.0f);
	m_fInaccuracyCrouch[1]		= pKeyValuesData->GetFloat("InaccuracyCrouchAlt", 0.0f);
	m_fInaccuracyStand[1]		= pKeyValuesData->GetFloat("InaccuracyStandAlt", 0.0f);
	m_fInaccuracyJump[1]		= pKeyValuesData->GetFloat("InaccuracyJumpAlt", 0.0f);
	m_fInaccuracyLand[1]		= pKeyValuesData->GetFloat("InaccuracyLandAlt", 0.0f);
	m_fInaccuracyLadder[1]		= pKeyValuesData->GetFloat("InaccuracyLadderAlt", 0.0f);
	m_fInaccuracyImpulseFire[1]	= pKeyValuesData->GetFloat("InaccuracyFireAlt", 0.0f);
	m_fInaccuracyMove[1]		= pKeyValuesData->GetFloat("InaccuracyMoveAlt", 0.0f);

	m_fInaccuracyReload			= pKeyValuesData->GetFloat("InaccuracyReload", 0.0f);
	m_fInaccuracyAltSwitch		= pKeyValuesData->GetFloat("InaccuracyAltSwitch", 0.0f);

	m_fInaccuracyJumpInitial[0]	= pKeyValuesData->GetFloat("InaccuracyJumpInitial", 0.0f);
	m_fInaccuracyJumpInitial[1]	= pKeyValuesData->GetFloat("InaccuracyJumpInitialAlt", m_fInaccuracyJumpInitial[0]);

	m_fRecoveryTimeCrouch	= pKeyValuesData->GetFloat("RecoveryTimeCrouch", 1.0f);
	m_fRecoveryTimeCrouchFinal = pKeyValuesData->GetFloat("RecoveryTimeCrouchFinal", m_fRecoveryTimeCrouch);
	m_fRecoveryTimeStand	= pKeyValuesData->GetFloat("RecoveryTimeStand", 1.0f);
	m_fRecoveryTimeStandFinal = pKeyValuesData->GetFloat("RecoveryTimeStandFinal", m_fRecoveryTimeStand);

	// recoil pattern parameters (per weapon mode: primary / secondary)
	m_fRecoilAngle[0]		= pKeyValuesData->GetFloat("RecoilAngle", 0.0f);
	m_fRecoilAngleVariance[0]		= pKeyValuesData->GetFloat("RecoilAngleVariance", 0.0f);
	m_fRecoilMagnitude[0]		= pKeyValuesData->GetFloat("RecoilMagnitude", 0.0f);
	m_fRecoilMagnitudeVariance[0]	= pKeyValuesData->GetFloat("RecoilMagnitudeVariance", 0.0f);

	m_fRecoilAngle[1]		= pKeyValuesData->GetFloat("RecoilAngleAlt", m_fRecoilAngle[0]);
	m_fRecoilAngleVariance[1]		= pKeyValuesData->GetFloat("RecoilAngleVarianceAlt", m_fRecoilAngleVariance[0]);
	m_fRecoilMagnitude[1]		= pKeyValuesData->GetFloat("RecoilMagnitudeAlt", m_fRecoilMagnitude[0]);
	m_fRecoilMagnitudeVariance[1]	= pKeyValuesData->GetFloat("RecoilMagnitudeVarianceAlt", m_fRecoilMagnitudeVariance[0]);

	m_iRecoilSeed	= pKeyValuesData->GetInt("RecoilSeed", 0);

	m_iRecoveryTransitionStartBullet	= pKeyValuesData->GetInt("RecoveryTransitionStartBullet", 0);
	m_iRecoveryTransitionEndBullet		= pKeyValuesData->GetInt("RecoveryTransitionEndBullet", 0);

	m_flTimeToIdleAfterFire	= pKeyValuesData->GetFloat( "TimeToIdle", 2 );
	m_flIdleInterval	= pKeyValuesData->GetFloat( "IdleInterval", 20 );

	// Figure out what team can have this weapon.
	m_iTeam = TEAM_UNASSIGNED;
	const char *pTeam = pKeyValuesData->GetString( "Team", NULL );
	if ( pTeam )
	{
		if ( Q_stricmp( pTeam, "CT" ) == 0 )
		{
			m_iTeam = TEAM_CT;
		}
		else if ( Q_stricmp( pTeam, "TERRORIST" ) == 0 )
		{
			m_iTeam = TEAM_TERRORIST;
		}
		else if ( Q_stricmp( pTeam, "ANY" ) == 0 )
		{
			m_iTeam = TEAM_UNASSIGNED;
		}
		else
		{
			Assert( false );
		}
	}
	else
	{
		Assert( false );
	}

	
	const char *pWrongTeamMsg = pKeyValuesData->GetString( "WrongTeamMsg", "" );
	Q_strncpy( m_WrongTeamMsg, pWrongTeamMsg, sizeof( m_WrongTeamMsg ) );

	const char *pShieldViewModel = pKeyValuesData->GetString( "shieldviewmodel", "" );
	Q_strncpy( m_szShieldViewModel, pShieldViewModel, sizeof( m_szShieldViewModel ) );
	
	const char *pAnimEx = pKeyValuesData->GetString( "PlayerAnimationExtension", "m4" );
	Q_strncpy( m_szAnimExtension, pAnimEx, sizeof( m_szAnimExtension ) );

	// Default is 2000.
	m_flBotAudibleRange = pKeyValuesData->GetFloat( "BotAudibleRange", 2000.0f );
	
	const char *pTypeString = pKeyValuesData->GetString( "WeaponType", "" );
	m_WeaponType = WeaponClassFromString(pTypeString);

	m_bFullAuto = pKeyValuesData->GetBool("FullAuto");

	// Gameplay balance is compiled into both DLLs. Content packs may replace
	// models, materials and sounds without silently changing recoil or damage.
	const char *balanceAlias = !Q_strnicmp( szWeaponName, "weapon_", 7 ) ? szWeaponName+7 : szWeaponName;
	const CSWeaponID balanceID=AliasToWeaponID(GetTranslatedWeaponAlias(balanceAlias));
	CSApplyLegacyWeaponTuning(*this,balanceID);
	CSApply2015WeaponProfile(*this,balanceID);
	CSApplyCS2WeaponProfile(*this,balanceID);
	// Default presentation is compiled, so empty/stale loadouts cannot restore CS:S viewmodels.
	if(const CSWeaponPresentation *models=CSDefaultWeaponPresentation(balanceID))
	{ Q_strncpy(szViewModel,models->view,sizeof(szViewModel)); Q_strncpy(szWorldModel,models->world,sizeof(szWorldModel)); }
	if (const CSEconomyWeapon *economy = CSEconomyWeaponForID(balanceID))
	{
		SetWeaponPrice(economy->price); SetDefaultPrice(economy->price); SetPreviousPrice(economy->price);
	}

	// Read the addon model.
	Q_strncpy( m_szAddonModel, pKeyValuesData->GetString( "AddonModel" ), sizeof( m_szAddonModel ) );

	// Read the dropped model.
	Q_strncpy( m_szDroppedModel, pKeyValuesData->GetString( "DroppedModel" ), sizeof( m_szDroppedModel ) );

	// Read the silencer model.
	Q_strncpy( m_szSilencerModel, pKeyValuesData->GetString( "SilencerModel" ), sizeof( m_szSilencerModel ) );

#ifndef CLIENT_DLL
	// Enforce consistency for the weapon here, since that way we don't need to save off the model bounds
	// for all time.
	// Moved to pure_server_minimal.txt
//	engine->ForceExactFile( UTIL_VarArgs("scripts/%s.ctx", szWeaponName ) );

	// Model bounds are rounded to the nearest integer, then extended by 1
	engine->ForceModelBounds( szWorldModel, Vector( -15, -12, -18 ), Vector( 44, 16, 19 ) );
	if ( m_szAddonModel[0] )
	{
		engine->ForceModelBounds( m_szAddonModel, Vector( -5, -5, -6 ), Vector( 13, 5, 7 ) );
	}
	if ( m_szSilencerModel[0] )
	{
		engine->ForceModelBounds( m_szSilencerModel, Vector( -15, -12, -18 ), Vector( 44, 16, 19 ) );
	}
#endif // !CLIENT_DLL
}


//--------------------------------------------------------------------------------------------------------
// CS:GO recoil pattern data
//--------------------------------------------------------------------------------------------------------
ConVar weapon_recoil_suppression_shots( "weapon_recoil_suppression_shots", "4", FCVAR_CHEAT | FCVAR_REPLICATED, "Number of shots before weapon uses full recoil" );
ConVar weapon_recoil_suppression_factor( "weapon_recoil_suppression_factor", "0.75", FCVAR_CHEAT | FCVAR_REPLICATED, "Initial recoil suppression factor (first suppressed shot will use this factor * standard recoil, lerping to 1 for later shots" );
ConVar weapon_recoil_variance( "weapon_recoil_variance", "0.55", FCVAR_CHEAT | FCVAR_REPLICATED, "Amount of variance per recoil impulse", true, 0.0f, true, 1.0f );

WeaponRecoilData::WeaponRecoilData()
{
	m_mapRecoilTables.SetLessFunc( DefLessFunc( CSWeaponID ) );
}

WeaponRecoilData::~WeaponRecoilData()
{
	m_mapRecoilTables.PurgeAndDeleteElements();
}

void WeaponRecoilData::GenerateRecoilTable( RecoilData *data )
{
	const int iSuppressionShots = weapon_recoil_suppression_shots.GetInt();
	const float fBaseSuppressionFactor = weapon_recoil_suppression_factor.GetFloat();
	const float fRecoilVariance = weapon_recoil_variance.GetFloat();
	CUniformRandomStream recoilRandom;

	if ( !data )
		return;
	data->suppressionShots = iSuppressionShots;
	data->suppressionFactor = fBaseSuppressionFactor;
	data->variance = fRecoilVariance;

	CCSWeaponInfo *pWeaponInfo = GetWeaponInfo( data->iWeaponID );

	// Walk the attributes to determine all things that we need
	int iSeed = 0;
	bool bFullAuto = false;
	float flRecoilAngle[2] = {};
	float flRecoilAngleVariance[2] = {};
	float flRecoilMagnitude[2] = {};
	float flRecoilMagnitudeVariance[2] = {};

	if ( pWeaponInfo )
	{
		iSeed = pWeaponInfo->m_iRecoilSeed;
		bFullAuto = pWeaponInfo->m_bFullAuto;
		for ( int iMode = 0; iMode < 2; ++iMode )
	{
			flRecoilAngle[iMode] = pWeaponInfo->m_fRecoilAngle[iMode];
			flRecoilAngleVariance[iMode] = pWeaponInfo->m_fRecoilAngleVariance[iMode];
			flRecoilMagnitude[iMode] = pWeaponInfo->m_fRecoilMagnitude[iMode];
			flRecoilMagnitudeVariance[iMode] = pWeaponInfo->m_fRecoilMagnitudeVariance[iMode];
	}
	}

	for ( int iMode = 0; iMode < 2; ++iMode )
	{
		CSGenerateRecoilPattern(recoilRandom, iSeed, bFullAuto,
			flRecoilAngle[iMode], flRecoilAngleVariance[iMode],
			flRecoilMagnitude[iMode], flRecoilMagnitudeVariance[iMode],
			iSuppressionShots, fBaseSuppressionFactor, fRecoilVariance,
			data->recoilTable[iMode], ARRAYSIZE(data->recoilTable[iMode]));
	}
}

void WeaponRecoilData::GetRecoilOffsets( CWeaponCSBase *pWeapon, int iMode, int iIndex, float& fAngle, float &fMagnitude )
{
	// Recoil offset tables are indexed by a weapon's definition index.
	// Look for the existing table, otherwise generate it.

	CSWeaponID id = pWeapon->GetWeaponID();

	RecoilData *wepData = NULL;
	CUtlMap< CSWeaponID, RecoilData* >::IndexType_t iMapLocation = m_mapRecoilTables.Find( id );
	if ( iMapLocation == m_mapRecoilTables.InvalidIndex() )
	{
		wepData = new RecoilData;
		wepData->iWeaponID = id;
		iMapLocation = m_mapRecoilTables.InsertOrReplace( id, wepData );
	GenerateRecoilTable( wepData );
	}
	else
	{
		wepData = m_mapRecoilTables.Element( iMapLocation );
	Assert( wepData );
	}

	if ( wepData->suppressionShots != weapon_recoil_suppression_shots.GetInt() ||
		wepData->suppressionFactor != weapon_recoil_suppression_factor.GetFloat() ||
		wepData->variance != weapon_recoil_variance.GetFloat() ) GenerateRecoilTable(wepData);
	iMode = iMode == Secondary_Mode ? Secondary_Mode : Primary_Mode;
	const int count = ARRAYSIZE( wepData->recoilTable[iMode] );
	iIndex = CSRecoilTableIndex( iIndex, count );
	fAngle = wepData->recoilTable[iMode][iIndex].fAngle;
	fMagnitude = wepData->recoilTable[iMode][iIndex].fMagnitude;
}

void WeaponRecoilData::GenerateRecoilPattern( CSWeaponID id )
{
	CUtlMap< CSWeaponID, RecoilData* >::IndexType_t iMapLocation = m_mapRecoilTables.Find( id );
	if ( iMapLocation == m_mapRecoilTables.InvalidIndex() )
	{
	RecoilData *wepData = new RecoilData;
		wepData->iWeaponID = id;
		iMapLocation = m_mapRecoilTables.InsertOrReplace( id, wepData );
	GenerateRecoilTable( wepData );
	}
}

WeaponRecoilData g_WeaponRecoilData;

#ifndef CLIENT_DLL
CON_COMMAND_F( cs_validate_economy, "Audit compiled prices and loss-bonus transitions.", FCVAR_CHEAT )
{
    int failed=0, checked=0;
    for (int i=0; i<ARRAYSIZE(g_CSEconomyWeapons); ++i)
    {
        const CSEconomyWeapon &entry=g_CSEconomyWeapons[i];
        const CCSWeaponInfo *info=GetWeaponInfo(entry.id);
        const bool valid=info && info->GetWeaponPrice()==entry.price;
        ++checked; if (!valid) ++failed;
        Msg("[economy-audit] %s %s reference=%s price=%d kill=%d\n",valid ? "PASS" : "FAIL",WeaponIdAsString(entry.id),entry.reference,info ? info->GetWeaponPrice() : -1,entry.killAward);
    }
    const int awards[]={1400,1900,2400,2900,3400};
    for (int i=0; i<5; ++i)
        if (CSEconomyLossAward(i)!=awards[i] || CSEconomyNextLossLevel(i,false)!=MIN(i+1,4) || CSEconomyNextLossLevel(i,true)!=MAX(i-1,0)) ++failed;
    Msg("[economy-audit] checked=%d failures=%d loss_steps=1400,1900,2400,2900,3400\n",checked,failed);
}

CON_COMMAND_F( cs_validate_weapon_balance, "Compare effective weapon data with the compiled balance table.", FCVAR_CHEAT )
{
	int checked=0,failed=0;
	for (int i=0;i<ARRAYSIZE(g_CSLegacyWeaponTuning);++i)
	{
		CSWeaponID id=g_CSLegacyWeaponTuning[i].id; const CCSWeaponInfo *loaded=GetWeaponInfo(id);
		if (!loaded) { ++failed; Warning("[balance-audit] missing %s\n",WeaponIdAsString(id)); continue; }
		CCSWeaponInfo expected=*loaded; CSApplyLegacyWeaponTuning(expected,id); CSApply2015WeaponProfile(expected,id); CSApplyCS2WeaponProfile(expected,id);
		bool valid=loaded->iMaxClip1==expected.iMaxClip1 && loaded->iDefaultClip1==expected.iDefaultClip1 && loaded->m_iDamage==expected.m_iDamage && loaded->m_iPenetration==expected.m_iPenetration && loaded->m_flRange==expected.m_flRange && loaded->m_flRangeModifier==expected.m_flRangeModifier && loaded->m_iRecoilSeed==expected.m_iRecoilSeed;
		for (int mode=0;mode<2;++mode) valid=valid && loaded->m_flCycleTime[mode]==expected.m_flCycleTime[mode] && loaded->m_fSpread[mode]==expected.m_fSpread[mode] && loaded->m_fInaccuracyStand[mode]==expected.m_fInaccuracyStand[mode] && loaded->m_fInaccuracyMove[mode]==expected.m_fInaccuracyMove[mode] && loaded->m_fRecoilMagnitude[mode]==expected.m_fRecoilMagnitude[mode];
		++checked; if (!valid) ++failed;
		Msg("[balance-audit] %s %s damage=%d clip=%d cycle=%.6f recoil_seed=%d\n",valid ? "PASS" : "FAIL",WeaponIdAsString(id),loaded->m_iDamage,loaded->iMaxClip1,loaded->m_flCycleTime[0],loaded->m_iRecoilSeed);
	}
	Msg("[balance-audit] checked=%d failed=%d\n",checked,failed);
}
CON_COMMAND_F( cs_export_gunplay, "Export loaded weapon profiles and seeded recoil impulses to gunplay_audit.csv.", FCVAR_CHEAT )
{
	FileHandle_t file = filesystem->Open("gunplay_audit.csv","wt","MOD");
	if ( file == FILESYSTEM_INVALID_HANDLE ) { Warning("Cannot write gunplay_audit.csv\n"); return; }
	filesystem->FPrintf(file,"weapon,mode,shot,seed,angle,magnitude,cycle,spread,crouch,stand,move,jump,fire,recovery_crouch,recovery_crouch_final,recovery_stand,recovery_stand_final,land,ladder,jump_initial,recoil_angle,recoil_angle_variance,recoil_magnitude,recoil_magnitude_variance,transition_start,transition_end,maxspeed,damage,armor_ratio,headshot_multiplier,range,range_modifier,clip\n");
	int rows = 0;
	for ( int index = 0; index < ARRAYSIZE(g_CSLegacyWeaponTuning); ++index )
	{
		CSWeaponID id = g_CSLegacyWeaponTuning[index].id;
		const CCSWeaponInfo *info = GetWeaponInfo(id);
		if ( !info ) continue;
		for ( int mode = 0; mode < 2; ++mode )
		{
			CUniformRandomStream random;
			CSRecoilOffset offsets[64];
			CSGenerateRecoilPattern(random,info->m_iRecoilSeed,info->m_bFullAuto,
				info->m_fRecoilAngle[mode],info->m_fRecoilAngleVariance[mode],info->m_fRecoilMagnitude[mode],info->m_fRecoilMagnitudeVariance[mode],
				weapon_recoil_suppression_shots.GetInt(),weapon_recoil_suppression_factor.GetFloat(),weapon_recoil_variance.GetFloat(),offsets,64);
			for ( int shot = 0; shot < 64; ++shot )
			{
				filesystem->FPrintf(file,"%s,%d,%d,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%d,%.9g,%d,%.9g,%.9g,%.9g,%.9g,%d\n",
					WeaponIdAsString(id),mode,shot,info->m_iRecoilSeed,offsets[shot].fAngle,offsets[shot].fMagnitude,
					info->m_flCycleTime[mode],info->m_fSpread[mode],info->m_fInaccuracyCrouch[mode],info->m_fInaccuracyStand[mode],info->m_fInaccuracyMove[mode],info->m_fInaccuracyJump[mode],info->m_fInaccuracyImpulseFire[mode],
					info->m_fRecoveryTimeCrouch,info->m_fRecoveryTimeCrouchFinal,info->m_fRecoveryTimeStand,info->m_fRecoveryTimeStandFinal,
					info->m_fInaccuracyLand[mode],info->m_fInaccuracyLadder[mode],info->m_fInaccuracyJumpInitial[mode],info->m_fRecoilAngle[mode],info->m_fRecoilAngleVariance[mode],info->m_fRecoilMagnitude[mode],info->m_fRecoilMagnitudeVariance[mode],
					info->m_iRecoveryTransitionStartBullet,info->m_iRecoveryTransitionEndBullet,mode ? info->m_flMaxSpeedAlt : info->m_flMaxSpeed,info->m_iDamage,info->m_flArmorRatio,info->m_flHeadshotMultiplier,info->m_flRange,info->m_flRangeModifier,info->iMaxClip1);
				++rows;
			}
		}
	}
	filesystem->Close(file);
	Msg("[gunplay-audit] Exported %d rows to gunplay_audit.csv\n",rows);
}
#endif

void GenerateWeaponRecoilPattern( CSWeaponID idx )
{
	g_WeaponRecoilData.GenerateRecoilPattern( idx );
}
