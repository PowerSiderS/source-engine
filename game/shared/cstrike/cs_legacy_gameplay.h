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

inline float CSLegacyClamp01( float flValue )
{
	if ( flValue < 0.0f )
		return 0.0f;
	if ( flValue > 1.0f )
		return 1.0f;
	return flValue;
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
