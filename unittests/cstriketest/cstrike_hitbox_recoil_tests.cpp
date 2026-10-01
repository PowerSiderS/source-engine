#include "tier0/dbg.h"
#include "unitlib/unitlib.h"
#include "tier1/utlmap.h"
#include "tier1/utlvector.h"
#include "cstrike/cs_hitbox_geometry.h"
#include "cstrike/cs_recoil_pattern.h"
#include "cstrike/cs_legacy_weapon_tuning.h"
#include "cstrike/cs_sha256.h"
#include <limits.h>

DEFINE_TESTSUITE( CStrikeHitboxRecoilTestSuite )

DEFINE_TESTCASE( WeaponFileSha256Test, CStrikeHitboxRecoilTestSuite )
{
	char digest[65]; CSSha256 empty; empty.Finish(digest);
	Shipping_Assert(!strcmp(digest,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
	CSSha256 abc; abc.Update("a",1); abc.Update("bc",2); abc.Finish(digest);
	Shipping_Assert(!strcmp(digest,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
	const char *longText="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
	CSSha256 padded; padded.Update(longText,strlen(longText)); padded.Finish(digest);
	Shipping_Assert(!strcmp(digest,"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
	CSSha256 million; char block[1000]; memset(block,'a',sizeof(block));
	for (int i=0;i<1000;++i) million.Update(block,sizeof(block)); million.Finish(digest);
	Shipping_Assert(!strcmp(digest,"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
}

static bool Close( float a, float b, float tolerance = 0.0001f )
{
	return fabsf(a-b) <= tolerance;
}

DEFINE_TESTCASE( RoundedHitboxRayTest, CStrikeHitboxRecoilTestSuite )
{
	CSRoundedHitbox shape;
	CSRoundedHit hit;
	Shipping_Assert(shape.Init(Vector(-5,-2,-1),Vector(5,2,1)));
	Shipping_Assert(CSIntersectRoundedHitbox(shape,Vector(-10,0,0),Vector(20,0,0),hit));
	Shipping_Assert(Close(hit.fraction,0.25f) && Close(hit.exitFraction,0.75f));
	Shipping_Assert(Close(hit.normal.x,-1.0f) && !hit.startSolid);
	Shipping_Assert(CSIntersectRoundedHitbox(shape,Vector(0,-10,0),Vector(0,20,0),hit));
	Shipping_Assert(Close(hit.fraction,0.4f));
	Shipping_Assert(CSIntersectRoundedHitbox(shape,Vector(0,0,-10),Vector(0,0,20),hit));
	Shipping_Assert(Close(hit.fraction,0.45f));
	// The old box corner is empty, while the rounded cap remains hittable.
	Shipping_Assert(!CSIntersectRoundedHitbox(shape,Vector(4.9f,-10,0.9f),Vector(0,20,0),hit));
	Shipping_Assert(CSIntersectRoundedHitbox(shape,Vector(4.5f,-10,0),Vector(0,20,0),hit));
	Shipping_Assert(!CSIntersectRoundedHitbox(shape,Vector(10,0,0),Vector(10,0,0),hit));
	Shipping_Assert(!CSIntersectRoundedHitbox(shape,Vector(-10,0,0),Vector(4,0,0),hit));
	Shipping_Assert(CSIntersectRoundedHitbox(shape,Vector(0,0,0),Vector(10,0,0),hit));
	Shipping_Assert(hit.startSolid && Close(hit.fraction,0) && Close(hit.exitFraction,0.5f));
	Shipping_Assert(CSIntersectRoundedHitbox(shape,Vector(0,0,0),Vector(0,0,0),hit));
	Shipping_Assert(hit.startSolid && hit.exitFraction >= 1);
	Shipping_Assert(!shape.Init(Vector(0,0,0),Vector(0,1,1)));
}

DEFINE_TESTCASE( RoundedHitboxAxesAndRangeTest, CStrikeHitboxRecoilTestSuite )
{
	for ( int axis = 0; axis < 3; ++axis )
	{
		Vector mins(-1,-1,-1), maxs(1,1,1), start(0,0,0), delta(0,0,0);
		mins[axis] = -5; maxs[axis] = 5; start[axis] = -8192; delta[axis] = 16384;
		CSRoundedHitbox shape; CSRoundedHit hit;
		Shipping_Assert(shape.Init(mins,maxs));
		Shipping_Assert(CSIntersectRoundedHitbox(shape,start,delta,hit));
		Shipping_Assert(Close((start+delta*hit.fraction)[axis],-5,0.002f));
		start[(axis+1)%3] = 1.001f;
		Shipping_Assert(!CSIntersectRoundedHitbox(shape,start,delta,hit));
		start[(axis+1)%3] = 1;
		Shipping_Assert(CSIntersectRoundedHitbox(shape,start,delta,hit));
	}
}

static float Sample( unsigned int &seed )
{
	seed = seed * 1664525u + 1013904223u;
	return float(seed >> 8) / 16777216.0f;
}

DEFINE_TESTCASE( RoundedHitboxSampledReferenceTest, CStrikeHitboxRecoilTestSuite )
{
	// Independent dense point sampling catches false negatives and bad roots
	// across arbitrary ray directions and asymmetric authored bounds.
	unsigned int seed = 317;
	CSRoundedHitbox shape;
	Shipping_Assert(shape.Init(Vector(-7,-3,-2),Vector(11,5,2)));
	for ( int trial = 0; trial < 1500; ++trial )
	{
		Vector start, delta;
		for ( int axis = 0; axis < 3; ++axis )
		{ start[axis] = Sample(seed)*40-20; delta[axis] = Sample(seed)*40-20; }
		CSRoundedHit hit;
		const bool intersects = CSIntersectRoundedHitbox(shape,start,delta,hit);
		bool sampledInside = false;
		for ( int sample = 0; sample <= 256; ++sample )
		{
			Vector p = start + delta*(float(sample)/256);
			for ( int axis = 0; axis < 3; ++axis ) p[axis] = (p[axis]-shape.center[axis])/shape.radii[axis];
			if ( CSCapsuleDistanceSqr(p,shape.axis,shape.halfSegment) < 0.999f ) sampledInside = true;
		}
		Shipping_Assert(!sampledInside || intersects);
		if ( intersects && !hit.startSolid )
		{
			Vector p = start + delta*hit.fraction;
			for ( int axis = 0; axis < 3; ++axis ) p[axis] = (p[axis]-shape.center[axis])/shape.radii[axis];
			Shipping_Assert(Close(CSCapsuleDistanceSqr(p,shape.axis,shape.halfSegment),1,0.0002f));
			Shipping_Assert(Close(hit.normal.LengthSqr(),1,0.0001f));
		}
	}
}

DEFINE_TESTCASE( WrappedBonePoseInterpolationTest, CStrikeHitboxRecoilTestSuite )
{
	Shipping_Assert(Close(CSInterpolateLoop(0.5f,179,-179,360),180));
	Shipping_Assert(Close(CSInterpolateLoop(0.5f,-179,179,360),-180));
	Shipping_Assert(Close(CSInterpolateLoop(0.5f,0,100,0),50));
	Shipping_Assert(Close(CSInterpolateLoop(0.25f,350,10,360),355));
}

DEFINE_TESTCASE( WeaponProfilesRecoilDeterminismTest, CStrikeHitboxRecoilTestSuite )
{
	CUniformRandomStream serverRandom, clientRandom;
	for ( int weapon = 0; weapon < ARRAYSIZE(g_CSLegacyWeaponTuning); ++weapon )
	{
		const float *v = g_CSLegacyWeaponTuning[weapon].v;
		for ( int mode = 0; mode < 2; ++mode )
		{
			CSRecoilOffset server[64], client[64];
			CSGenerateRecoilPattern(serverRandom,int(v[36]),v[1]!=0,v[28+mode],v[30+mode],v[32+mode],v[34+mode],4,0.75f,0.55f,server,64);
			CSGenerateRecoilPattern(clientRandom,int(v[36]),v[1]!=0,v[28+mode],v[30+mode],v[32+mode],v[34+mode],4,0.75f,0.55f,client,64);
			for ( int shot = 0; shot < 64; ++shot )
			{
				Shipping_Assert(server[shot].fAngle == client[shot].fAngle && server[shot].fMagnitude == client[shot].fMagnitude);
				Shipping_Assert(isfinite(server[shot].fAngle) && isfinite(server[shot].fMagnitude) && server[shot].fMagnitude >= 0);
			}
		}
	}
	Shipping_Assert(CSRecoilTableIndex(-1,64)==63);
	Shipping_Assert(CSRecoilTableIndex(INT_MIN,64)==0);
	Shipping_Assert(CSRecoilTableIndex(64,64)==0);
}

DEFINE_TESTCASE( RecoilSuppressionAndModeTest, CStrikeHitboxRecoilTestSuite )
{
	CUniformRandomStream random;
	CSRecoilOffset offsets[64];
	CSGenerateRecoilPattern(random,223,true,0,0,30,0,4,0.75f,0.55f,offsets,64);
	Shipping_Assert(Close(offsets[0].fMagnitude,22.5f));
	Shipping_Assert(Close(offsets[1].fMagnitude,21.6328125f));
	Shipping_Assert(Close(offsets[2].fMagnitude,22.9554199f));
	Shipping_Assert(Close(offsets[3].fMagnitude,25.1530678f));
	Shipping_Assert(Close(offsets[63].fMagnitude,30));
	CSGenerateRecoilPattern(random,223,false,0,0,30,0,4,0.75f,0.55f,offsets,64);
	Shipping_Assert(Close(offsets[0].fMagnitude,30));
	Shipping_Assert(Close(offsets[1].fMagnitude,30));
}

DEFINE_TESTCASE( CS2AK47RecordedImpulseTest, CStrikeHitboxRecoilTestSuite )
{
	// Independent offline CS2 demo, patch 1.41.8.1: recorded base velocities.
	const float referenceVelocity[30][2] = {
		{-19.9884243f, -10.3301945f},
		{-33.3389091f, 0.0371489525f},
		{-44.0956955f, -2.29641318f},
		{-53.2594757f, -2.18210745f},
		{-60.7618408f, 6.06096792f},
		{-67.3836212f, 8.53518295f},
		{-71.0697174f, 14.5996895f},
		{-74.4104919f, 2.85722494f},
		{-68.9622192f, -18.9556313f},
		{-71.6650467f, -23.5188713f},
		{-75.5643463f, -12.3934889f},
		{-73.9596252f, -23.2328854f},
		{-69.7514877f, -34.5458298f},
		{-72.8099747f, -31.8792534f},
		{-74.682663f, -10.2524338f},
		{-77.594902f, -5.32429314f},
		{-79.3531952f, -0.677322388f},
		{-78.2406006f, 11.2245808f},
		{-73.3677979f, 25.8310642f},
		{-75.7364655f, 8.62194633f},
		{-76.7108459f, 15.1075821f},
		{-78.9120407f, 9.87204647f},
		{-80.3141937f, 6.6692934f},
		{-77.5182343f, 18.6715469f},
		{-78.7192917f, 18.3869305f},
		{-79.6890793f, 6.24508333f},
		{-77.9378815f, -8.83150101f},
		{-70.3357544f, -27.4021835f},
		{-72.7478409f, -28.499733f},
		{-76.1285706f, -14.2500381f},
	};
	CUniformRandomStream random;
	CSRecoilOffset offsets[64];
	CSGenerateRecoilPattern(random,223,true,0,70,30,0,4,0.75f,0.55f,offsets,64);
	float pitchVelocity = 0.0f, yawVelocity = 0.0f;
	const float velocityDecay = expf(-4.5f * 0.1f);
	for (int shot = 0; shot < 30; ++shot)
	{
		const float angle = DEG2RAD(offsets[shot].fAngle);
		pitchVelocity = pitchVelocity * velocityDecay - cosf(angle) * offsets[shot].fMagnitude;
		yawVelocity = yawVelocity * velocityDecay - sinf(angle) * offsets[shot].fMagnitude;
		Shipping_Assert(Close(pitchVelocity, referenceVelocity[shot][0], 0.0001f));
		Shipping_Assert(Close(yawVelocity, referenceVelocity[shot][1], 0.0001f));
	}
}
