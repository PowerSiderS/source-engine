//========= Copyright Valve Corporation, All rights reserved. ============//

#include "tier0/dbg.h"
#include "unitlib/unitlib.h"
#include "cstrike/cs_legacy_gameplay.h"
#include "tier1/strtools.h"
#include <math.h>

DEFINE_TESTSUITE( CStrikeGameplayTestSuite )

static bool NearlyEqual( float flActual, float flExpected, float flTolerance = 0.0001f )
{
	return fabsf( flActual - flExpected ) <= flTolerance;
}

DEFINE_TESTCASE( UsePickupSlotPolicyTest, CStrikeGameplayTestSuite )
{
	Shipping_Assert( CSUsePickupShouldReplaceSlot( WEAPON_SLOT_RIFLE ) );
	Shipping_Assert( CSUsePickupShouldReplaceSlot( WEAPON_SLOT_PISTOL ) );
	Shipping_Assert( !CSUsePickupShouldReplaceSlot( WEAPON_SLOT_KNIFE ) );
	Shipping_Assert( !CSUsePickupShouldReplaceSlot( WEAPON_SLOT_GRENADES ) );
	Shipping_Assert( !CSUsePickupShouldReplaceSlot( WEAPON_SLOT_C4 ) );
}

DEFINE_TESTCASE( LegacyMovementInaccuracyTest, CStrikeGameplayTestSuite )
{
	const float flMaxSpeed = 250.0f;
	const float flDuckSpeedModifier = 0.34f;
	const float flStart = flMaxSpeed * flDuckSpeedModifier;
	const float flEnd = flMaxSpeed * 0.95f;
	const float flMiddle = ( flStart + flEnd ) * 0.5f;

	Shipping_Assert( NearlyEqual( CSLegacyMovementInaccuracyScale( flStart - 1.0f, flMaxSpeed, flDuckSpeedModifier, false ), 0.0f ) );
	Shipping_Assert( NearlyEqual( CSLegacyMovementInaccuracyScale( flMiddle, flMaxSpeed, flDuckSpeedModifier, true ), 0.5f ) );
	Shipping_Assert( NearlyEqual( CSLegacyMovementInaccuracyScale( flMiddle, flMaxSpeed, flDuckSpeedModifier, false ), powf( 0.5f, 0.25f ) ) );
	Shipping_Assert( CSLegacyMovementInaccuracyScale( flMiddle, flMaxSpeed, flDuckSpeedModifier, false ) > CSLegacyMovementInaccuracyScale( flMiddle, flMaxSpeed, flDuckSpeedModifier, true ) );
	Shipping_Assert( NearlyEqual( CSLegacyMovementInaccuracyScale( flEnd + 1.0f, flMaxSpeed, flDuckSpeedModifier, false ), 1.0f ) );
	Shipping_Assert( NearlyEqual( CSLegacyMovementInaccuracyScale( 200.0f, 0.0f, flDuckSpeedModifier, false ), 0.0f ) );
}

DEFINE_TESTCASE( LegacyAirInaccuracyTest, CStrikeGameplayTestSuite )
{
	const float flJumpPenalty = 0.4f;
	const float flLegacyJumpImpulse = sqrtf( 2.0f * 800.0f * 57.0f );

	Shipping_Assert( NearlyEqual( CSLegacyAirSpeedInaccuracy( 0.0f, flJumpPenalty ), 0.0f ) );
	Shipping_Assert( NearlyEqual( CSLegacyAirSpeedInaccuracy( flLegacyJumpImpulse, flJumpPenalty ), flJumpPenalty ) );
	Shipping_Assert( NearlyEqual( CSLegacyAirSpeedInaccuracy( 100000.0f, flJumpPenalty ), flJumpPenalty * 2.0f ) );
	Shipping_Assert( NearlyEqual( CSLegacyAirSpeedInaccuracy( flLegacyJumpImpulse, 0.0f ), 0.0f ) );
}
