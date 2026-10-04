#include "tier0/dbg.h"
#include "unitlib/unitlib.h"
#include "tier1/utlvector.h"
#include "tier1/utlmap.h"
#include "cstrike/cs_capsule_geometry.h"
#include "cstrike/cs_2015_weapon_profiles.h"
#include "cstrike/cs_recoil_pattern.h"
#include <math.h>
#include <cmath>
#include <limits>
#include "cstrike/cs_grenade_rules.h"
#include "cstrike/cs_anticheat_rules.h"
#include "cstrike/cs_map_rules.h"
DEFINE_TESTSUITE(CStrikeCoreTestSuite)
static bool CSCoreNear(float a,float b,float epsilon=0.0001f) {return fabsf(a-b)<=epsilon;}
DEFINE_TESTCASE(AuthoredCapsuleTest,CStrikeCoreTestSuite)
{
    CSCapsuleHitbox shape={Vector(0,0,-2),Vector(0,0,2),1};
    CSCapsuleIntersection hit;
    Shipping_Assert(CSIntersectCapsule(shape,Vector(-4,0,0),Vector(8,0,0),hit));
    Shipping_Assert(CSCoreNear(hit.fraction,.375f));
    Shipping_Assert(CSCoreNear(hit.exitFraction,.625f));
    Shipping_Assert(CSCoreNear(hit.normal.x,-1));
    Shipping_Assert(!CSIntersectCapsule(shape,Vector(-4,1.01f,0),Vector(8,0,0),hit));
    Shipping_Assert(CSIntersectCapsule(shape,Vector(0,0,8),Vector(0,0,-16),hit));
    Shipping_Assert(CSCoreNear(hit.fraction,5.0f/16));
    Shipping_Assert(CSIntersectCapsule(shape,Vector(0,0,0),Vector(0,0,16),hit));
    Shipping_Assert(hit.startSolid && hit.fraction==0 && CSCoreNear(hit.exitFraction,3.0f/16));
    Shipping_Assert(CSIntersectCapsule(shape,Vector(-4,1,0),Vector(8,0,0),hit));
    Shipping_Assert(CSCoreNear(hit.fraction,.5f));
}
DEFINE_TESTCASE(CapsuleBoxAdapterTest,CStrikeCoreTestSuite)
{
    CSCapsuleHitbox shape;CSCapsuleIntersection hit;
    Shipping_Assert(CSCapsuleFromAuthoredBox(Vector(-5,-2,-1),Vector(5,2,1),shape));
    Shipping_Assert(CSCoreNear(shape.radius,1) && CSCoreNear(shape.start.x,-4) && CSCoreNear(shape.end.x,4));
    Shipping_Assert(CSIntersectCapsule(shape,Vector(-10,0,0),Vector(20,0,0),hit) && CSCoreNear(hit.fraction,.25f));
    Shipping_Assert(!CSIntersectCapsule(shape,Vector(0,-10,1.01f),Vector(0,20,0),hit));
    Shipping_Assert(!CSCapsuleFromAuthoredBox(Vector(0,0,0),Vector(0,2,3),shape));
}
DEFINE_TESTCASE(GrenadeLaunchRulesTest,CStrikeCoreTestSuite)
{
    Shipping_Assert(CSCoreNear(CSGrenadeThrowSpeed(CSGrenadeStrength(0)),675));
    Shipping_Assert(CSCoreNear(CSGrenadeThrowSpeed(CSGrenadeStrength(1)),202.5f));
    Shipping_Assert(CSCoreNear(CSGrenadeThrowSpeed(CSGrenadeStrength(2)),438.75f));
    Shipping_Assert(CSCoreNear(CSGrenadePitch(0),-10) && CSCoreNear(CSGrenadePitch(-90),-90));
    Shipping_Assert(CSCoreNear(CSGrenadePitch(270),-90) && CSCoreNear(CSGrenadePitch(90),90));
    Shipping_Assert(CSCoreNear(CSGrenadeSourceHeight(0),-12) && CSCoreNear(CSGrenadeSourceHeight(1),0));
    Shipping_Assert(CSCoreNear(CS_GRENADE_GRAVITY_SCALE*800,320));
}
DEFINE_TESTCASE(CommandValidationAndLagToleranceTest,CStrikeCoreTestSuite)
{
    Shipping_Assert(CSCommandScalarValid(450,10000));
    Shipping_Assert(!CSCommandScalarValid(std::numeric_limits<float>::quiet_NaN(),10000));
    Shipping_Assert(!CSCommandScalarValid(std::numeric_limits<float>::infinity(),10000));
    Shipping_Assert(!CSCommandScalarValid(10001,10000));
    CSCommandRateBudget normal;
    for(int i=0;i<1000;++i)Shipping_Assert(normal.Consume(i/128.0,1/128.0));
    for(int i=0;i<24;++i)Shipping_Assert(normal.Consume(1000/128.0,1/128.0));
    CSCommandRateBudget flood;int excess=0;
    for(int i=0;i<256;++i)if(!flood.Consume(10,1/128.0))++excess;
    Shipping_Assert(excess>128 && flood.Consume(12,1/128.0));
    Shipping_Assert(flood.Consume(0,1/128.0));
}
DEFINE_TESTCASE(MapNameSafetyAndVariantsTest,CStrikeCoreTestSuite)
{
    Shipping_Assert(CSMapNameValid("de_mirage_csgo_new") && CSMapNameValid("de_dust2_scb"));
    Shipping_Assert(CSMapNameValid("cs_office") && CSMapNameValid("de_dust2"));
    Shipping_Assert(!CSMapNameValid("aim_map") && !CSMapNameValid("ze_escape"));
    Shipping_Assert(!CSMapNameValid("de_a;quit") && !CSMapNameValid("de_../test") && !CSMapNameValid("de_foo.bsp"));
    Shipping_Assert(!CSMapNameValid(NULL) && !CSMapNameValid("de_"));
}
DEFINE_TESTCASE(CapsuleSphereAndLongRaysTest,CStrikeCoreTestSuite)
{
    CSCapsuleHitbox sphere={Vector(3,2,1),Vector(3,2,1),2};CSCapsuleIntersection hit;
    Shipping_Assert(CSIntersectCapsule(sphere,Vector(-8192,2,1),Vector(16384,0,0),hit));
    Shipping_Assert(CSCoreNear(hit.fraction,8193.0f/16384));
    Shipping_Assert(CSIntersectCapsule(sphere,Vector(3,2,1),Vector(0,0,0),hit));
    Shipping_Assert(hit.startSolid && hit.exitFraction==1);
    Shipping_Assert(!CSIntersectCapsule(sphere,Vector(9,2,1),Vector(0,0,0),hit));
    sphere.radius=-1;Shipping_Assert(!CSIntersectCapsule(sphere,Vector(0,0,0),Vector(8,0,0),hit));
}
DEFINE_TESTCASE(CapsuleArbitraryBoneDirectionsTest,CStrikeCoreTestSuite)
{
    // Expected wall entry is obtained directly from the radius, independently
    // of the cylinder/hemisphere quadratic intersection implementation.
    for(int i=0;i<1000;++i) {
        float angle=float(i)*.037f;
        Vector axis(cosf(angle),sinf(angle),0),radial(-sinf(angle),cosf(angle),0);
        Vector center(3,-7,11);
        CSCapsuleHitbox shape={center-axis*5,center+axis*5,2};CSCapsuleIntersection hit;
        Shipping_Assert(CSIntersectCapsule(shape,center+radial*8192,radial*-16384,hit));
        Shipping_Assert(CSCoreNear(hit.fraction,(8192.0f-2)/16384));
        Vector normal=hit.normal;
        Shipping_Assert(CSCoreNear(normal.LengthSqr(),1,.001f));
    }
}
DEFINE_TESTCASE(HistoricalWeaponAnchorsTest,CStrikeCoreTestSuite)
{
    // Values from original 2015 scripts, not from the previous tuning table.
    const CS2015WeaponProfile *ak=CSFind2015WeaponProfile(WEAPON_AK47);
    Shipping_Assert(ak && ak->seed==223 && CSCoreNear(ak->recoveryStand,.46f));
    Shipping_Assert(CSCoreNear(ak->recoveryCrouch,.381571f));
    Shipping_Assert(CSCoreNear(ak->modes[0].magnitude,30) && CSCoreNear(ak->modes[0].spread,.0006f));
    const CS2015WeaponProfile *glock=CSFind2015WeaponProfile(WEAPON_GLOCK);
    Shipping_Assert(glock && glock->seed==4484 && CSCoreNear(glock->modes[0].magnitude,18));
    const CS2015WeaponProfile *mp9=CSFind2015WeaponProfile(WEAPON_TMP);
    Shipping_Assert(mp9 && mp9->seed==50729 && CSCoreNear(mp9->modes[0].magnitude,19));
    Shipping_Assert(ARRAYSIZE(g_CS2015WeaponProfiles)==33);
}
DEFINE_TESTCASE(SharedSprayPredictionTest,CStrikeCoreTestSuite)
{
    for(int i=0;i<ARRAYSIZE(g_CS2015WeaponProfiles);++i) {
        const CS2015WeaponProfile &p=g_CS2015WeaponProfiles[i];
        for(int mode=0;mode<2;++mode) {
            CUniformRandomStream client,server;CSRecoilOffset a[64],b[64];
            const CS2015WeaponMode &m=p.modes[mode];
            CSGenerateRecoilPattern(client,p.seed,p.fullAuto,m.angle,m.angleVariance,m.magnitude,m.magnitudeVariance,4,.75f,.55f,a,64);
            CSGenerateRecoilPattern(server,p.seed,p.fullAuto,m.angle,m.angleVariance,m.magnitude,m.magnitudeVariance,4,.75f,.55f,b,64);
            for(int shot=0;shot<64;++shot) {
                Shipping_Assert(a[shot].fAngle==b[shot].fAngle && a[shot].fMagnitude==b[shot].fMagnitude);
                Shipping_Assert(std::isfinite(a[shot].fAngle) && std::isfinite(a[shot].fMagnitude));
            }
        }
    }
}
