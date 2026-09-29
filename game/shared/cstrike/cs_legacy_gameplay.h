//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Shared, deterministic CS gameplay rules used by the game and unit tests.
//
//=============================================================================//

#ifndef CS_LEGACY_GAMEPLAY_H
#define CS_LEGACY_GAMEPLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "cs_weapon_slots.h"
#include <math.h>

inline bool CSUsePickupShouldReplaceSlot( int iSlot )
{
	return iSlot == WEAPON_SLOT_RIFLE || iSlot == WEAPON_SLOT_PISTOL;
}

inline bool CSShouldCancelDefuse( bool bUseHeld, bool bOnGround, bool bHeartbeatExpired )
{
	return !bUseHeld || !bOnGround || bHeartbeatExpired;
}

inline float CSLegacyClamp01( float flValue )
{
	if ( flValue < 0.0f )
		return 0.0f;
	if ( flValue > 1.0f )
		return 1.0f;
	return flValue;
}

inline float CSLegacyStaminaRatio( float flStaminaMilliseconds, float flRecoverRate, float flStaminaMax )
{
	if ( flStaminaMax <= 0.0f )
		return 1.0f;

	const float flPenalty = ( flStaminaMilliseconds / 1000.0f ) * flRecoverRate;
	return CSLegacyClamp01( ( flStaminaMax - flPenalty ) / flStaminaMax );
}

inline float CSLegacyFlashFacingScale( float flViewDot )
{
	flViewDot = CSLegacyClamp01( ( flViewDot + 1.0f ) * 0.5f ) * 2.0f - 1.0f;
	if ( flViewDot >= 0.5f )
		return 1.0f;
	if ( flViewDot >= 0.0f )
		return 0.35f + flViewDot * 1.30f;
	return 0.10f + ( flViewDot + 1.0f ) * 0.25f;
}

inline float CSLegacyMovementInaccuracyScale( float flSpeed, float flMaxSpeed, float flStartSpeedModifier, bool bWalking )
{
	if ( flMaxSpeed <= 0.0f )
		return 0.0f;

	const float flStartSpeed = flMaxSpeed * flStartSpeedModifier;
	const float flEndSpeed = flMaxSpeed * 0.95f;
	const float flSpeedRange = flEndSpeed - flStartSpeed;
	if ( flSpeedRange <= 0.0f )
		return 0.0f;

	float flScale = CSLegacyClamp01( ( flSpeed - flStartSpeed ) / flSpeedRange );
	if ( flScale > 0.0f && !bWalking )
		flScale = powf( flScale, 0.25f );

	return flScale;
}

inline float CSLegacyAirSpeedInaccuracy( float flVerticalSpeed, float flInitialJumpPenalty )
{
	if ( flVerticalSpeed <= 0.0f || flInitialJumpPenalty <= 0.0f )
		return 0.0f;

	const float flLegacyJumpImpulse = sqrtf( 2.0f * 800.0f * 57.0f );
	const float flSqrtJumpSpeed = sqrtf( flLegacyJumpImpulse );
	const float flStartSpeed = flSqrtJumpSpeed * 0.25f;
	const float flSpeedRange = flSqrtJumpSpeed - flStartSpeed;
	const float flScale = ( sqrtf( flVerticalSpeed ) - flStartSpeed ) / flSpeedRange;
	float flInaccuracy = flScale * flInitialJumpPenalty;

	if ( flInaccuracy < 0.0f )
		return 0.0f;
	if ( flInaccuracy > flInitialJumpPenalty * 2.0f )
		return flInitialJumpPenalty * 2.0f;
	return flInaccuracy;
}

#endif // CS_LEGACY_GAMEPLAY_H
