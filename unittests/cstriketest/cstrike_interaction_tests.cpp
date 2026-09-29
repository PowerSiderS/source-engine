//========= Copyright Valve Corporation, All rights reserved. ============//

#include "tier0/dbg.h"
#include "unitlib/unitlib.h"
#include "cstrike/cs_legacy_gameplay.h"

DEFINE_TESTSUITE( CStrikeInteractionTestSuite )

DEFINE_TESTCASE( DefuseCancellationPolicyTest, CStrikeInteractionTestSuite )
{
	Shipping_Assert( !CSShouldCancelDefuse( true, true, false ) );
	Shipping_Assert( CSShouldCancelDefuse( false, true, false ) );
	Shipping_Assert( CSShouldCancelDefuse( true, false, false ) );
	Shipping_Assert( CSShouldCancelDefuse( true, true, true ) );
}

DEFINE_TESTCASE( LegacyStaminaRatioIsAlwaysSafe, CStrikeInteractionTestSuite )
{
	Shipping_Assert( CSLegacyStaminaRatio( 0.0f, 19.0f, 100.0f ) == 1.0f );
	Shipping_Assert( CSLegacyStaminaRatio( 100000.0f, 19.0f, 100.0f ) == 0.0f );
	Shipping_Assert( CSLegacyStaminaRatio( -1000.0f, 19.0f, 100.0f ) == 1.0f );
	Shipping_Assert( CSLegacyStaminaRatio( 1000.0f, 19.0f, 0.0f ) == 1.0f );
}

DEFINE_TESTCASE( LegacyFlashFacingCurveTest, CStrikeInteractionTestSuite )
{
	Shipping_Assert( CSLegacyFlashFacingScale( 1.0f ) == 1.0f );
	Shipping_Assert( CSLegacyFlashFacingScale( 0.0f ) == 0.35f );
	Shipping_Assert( CSLegacyFlashFacingScale( -1.0f ) == 0.10f );
	Shipping_Assert( CSLegacyFlashFacingScale( -0.5f ) < CSLegacyFlashFacingScale( 0.5f ) );
}
