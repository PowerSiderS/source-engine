// Generated from Valve scripts archived 2015-11-30 and Valve items_game dated 2015-11-11.
// See references/csgo2015/provenance.json. No modern recovery-final/jump-apex values.
#ifndef CS_2015_WEAPON_PROFILES_H
#define CS_2015_WEAPON_PROFILES_H
#include "cs_weapon_parse.h"
struct CS2015WeaponMode { float cycle, spread, crouch, stand, jump, land, ladder, fire, move, angle, angleVariance, magnitude, magnitudeVariance; };
struct CS2015WeaponProfile { CSWeaponID id; const char *name; int seed; bool fullAuto; float maxSpeed, maxSpeedAlt, recoveryCrouch, recoveryStand; CS2015WeaponMode modes[2]; };
static const CS2015WeaponProfile g_CS2015WeaponProfiles[] = {
    { WEAPON_P228, "weapon_p250", 9788, false, 240.0f, 240.0f, 0.287823f, 0.345388f, { { 0.15f, 0.002f, 0.00683f, 0.0091f, 0.000633f, 0.00019f, 0.138f, 0.05245f, 0.01341f, 0.0f, 10.0f, 26.0f, 3.0f }, { 0.15f, 0.002f, 0.00683f, 0.0091f, 0.000633f, 0.00019f, 0.138f, 0.05245f, 0.01341f, 0.0f, 10.0f, 26.0f, 3.0f } } },
    { WEAPON_GLOCK, "weapon_glock", 4484, false, 240.0f, 240.0f, 0.27631f, 0.331572f, { { 0.15f, 0.002f, 0.0042f, 0.0056f, 0.000616f, 0.000185f, 0.137f, 0.056f, 0.012f, 0.0f, 20.0f, 18.0f, 0.0f }, { 0.15f, 0.015f, 0.003f, 0.0056f, 0.00015f, 0.000185f, 0.11925f, 0.045f, 0.01295f, 0.0f, 20.0f, 30.0f, 5.0f } } },
    { WEAPON_SCOUT, "weapon_ssg08", 1278, false, 230.0f, 230.0f, 0.055783f, 0.142096f, { { 1.25f, 0.00028f, 0.02378f, 0.0317f, 0.000716f, 0.000215f, 0.09549f, 0.02292f, 0.12345f, 0.0f, 20.0f, 33.0f, 15.0f }, { 1.25f, 0.00023f, 0.0028f, 0.003f, 0.000716f, 0.000215f, 0.09549f, 0.02292f, 0.12345f, 0.0f, 20.0f, 33.0f, 15.0f } } },
    { WEAPON_XM1014, "weapon_xm1014", 24862, true, 215.0f, 215.0f, 0.361835f, 0.506569f, { { 0.35f, 0.038f, 0.00525f, 0.007f, 0.000772f, 0.000232f, 0.07721f, 0.00883f, 0.03603f, 0.0f, 20.0f, 80.0f, 20.0f }, { 0.35f, 0.038f, 0.00525f, 0.007f, 0.000772f, 0.000232f, 0.07721f, 0.00883f, 0.03603f, 0.0f, 20.0f, 80.0f, 20.0f } } },
    { WEAPON_MAC10, "weapon_mac10", 34079, true, 240.0f, 240.0f, 0.285521f, 0.399729f, { { 0.075f, 0.0006f, 0.00998f, 0.0133f, 0.000228f, 6.9e-05f, 0.03426f, 0.00476f, 0.01399f, 0.0f, 70.0f, 18.0f, 1.0f }, { 0.075f, 0.0006f, 0.00998f, 0.0133f, 0.000228f, 6.9e-05f, 0.03426f, 0.00476f, 0.01399f, 0.0f, 70.0f, 18.0f, 1.0f } } },
    { WEAPON_AUG, "weapon_aug", 24204, true, 220.0f, 150.0f, 0.30552f, 0.429727f, { { 0.09f, 0.0005f, 0.00288f, 0.00385f, 0.000693f, 0.000208f, 0.11004f, 0.00616f, 0.13545f, 0.0f, 60.0f, 26.0f, 0.0f }, { 0.09f, 0.0003f, 0.00101f, 0.00212f, 0.000693f, 0.000208f, 0.10004f, 0.00616f, 0.10545f, 0.0f, 60.0f, 18.0f, 0.0f } } },
    { WEAPON_ELITE, "weapon_elite", 24563, false, 240.0f, 240.0f, 0.437491f, 0.524989f, { { 0.12f, 0.002f, 0.00525f, 0.007f, 0.000849f, 0.000255f, 0.102f, 0.01116f, 0.01785f, 0.0f, 20.0f, 27.0f, 4.0f }, { 0.12f, 0.002f, 0.0075f, 0.01f, 0.000849f, 0.000255f, 0.102f, 0.01196f, 0.01785f, 0.0f, 20.0f, 27.0f, 4.0f } } },
    { WEAPON_FIVESEVEN, "weapon_fiveseven", 33244, false, 240.0f, 240.0f, 0.273844f, 0.332613f, { { 0.15f, 0.002f, 0.00683f, 0.0091f, 0.000633f, 0.00019f, 0.138f, 0.03245f, 0.01341f, 0.0f, 5.0f, 25.0f, 4.0f }, { 0.15f, 0.002f, 0.00683f, 0.0091f, 0.000633f, 0.00019f, 0.138f, 0.03245f, 0.01341f, 0.0f, 5.0f, 25.0f, 4.0f } } },
    { WEAPON_UMP45, "weapon_ump45", 59299, true, 230.0f, 230.0f, 0.249995f, 0.349993f, { { 0.09f, 0.001f, 0.01007f, 0.01343f, 0.000282f, 8.5e-05f, 0.04235f, 0.00342f, 0.02876f, 0.0f, 40.0f, 23.0f, 1.0f }, { 0.09f, 0.001f, 0.01007f, 0.01343f, 0.000282f, 8.5e-05f, 0.04235f, 0.00342f, 0.02876f, 0.0f, 40.0f, 23.0f, 1.0f } } },
    { WEAPON_SG550, "weapon_scar20", 19364, true, 215.0f, 120.0f, 0.388808f, 0.544331f, { { 0.25f, 0.0003f, 0.01935f, 0.0258f, 0.000873f, 0.000262f, 0.11639f, 0.01861f, 0.15048f, 0.0f, 30.0f, 31.0f, 4.0f }, { 0.25f, 0.0003f, 0.0015f, 0.002f, 0.000873f, 0.000262f, 0.11639f, 0.01861f, 0.15048f, 0.0f, 30.0f, 31.0f, 4.0f } } },
    { WEAPON_GALIL, "weapon_galilar", 51191, true, 215.0f, 215.0f, 0.384861f, 0.538805f, { { 0.09f, 0.0006f, 0.00658f, 0.00877f, 0.000852f, 0.000256f, 0.11358f, 0.00878f, 0.12356f, 0.0f, 70.0f, 21.0f, 1.0f }, { 0.09f, 0.0006f, 0.00484f, 0.00778f, 0.000852f, 0.000256f, 0.11358f, 0.00585f, 0.10652f, 0.0f, 70.0f, 21.0f, 1.0f } } },
    { WEAPON_FAMAS, "weapon_famas", 39623, true, 220.0f, 220.0f, 0.336177f, 0.470648f, { { 0.09f, 0.0006f, 0.00739f, 0.00985f, 0.000685f, 0.000205f, 0.118716f, 0.0067f, 0.09934f, 0.0f, 60.0f, 20.0f, 1.0f }, { 0.09f, 0.0006f, 0.00395f, 0.00369f, 0.000685f, 0.000205f, 0.118716f, 0.00335f, 0.09934f, 0.0f, 50.0f, 20.0f, 1.0f } } },
    { WEAPON_USP, "weapon_usp_silencer", 5426, false, 240.0f, 240.0f, 0.291277f, 0.349532f, { { 0.17f, 0.0025f, 0.00368f, 0.0049f, 0.000638f, 0.000191f, 0.13832f, 0.071f, 0.01387f, 0.0f, 0.0f, 29.0f, 0.0f }, { 0.17f, 0.0015f, 0.00368f, 0.0049f, 0.00066f, 0.000198f, 0.1199f, 0.052f, 0.01387f, 0.0f, 0.0f, 23.0f, 0.0f } } },
    { WEAPON_AWP, "weapon_awp", 4100, false, 200.0f, 100.0f, 0.24671f, 0.34539f, { { 1.455f, 0.0002f, 0.0606f, 0.0808f, 0.001024f, 0.000307f, 0.1365f, 0.05385f, 0.17648f, 0.0f, 20.0f, 78.0f, 15.0f }, { 1.455f, 0.0002f, 0.0015f, 0.002f, 0.001024f, 0.0001f, 0.1365f, 0.05385f, 0.17648f, 0.0f, 20.0f, 78.0f, 15.0f } } },
    { WEAPON_MP5NAVY, "weapon_mp7", 61649, true, 220.0f, 220.0f, 0.312494f, 0.437491f, { { 0.08f, 0.0006f, 0.00592f, 0.01f, 0.000384f, 0.000115f, 0.05756f, 0.00218f, 0.01986f, 0.0f, 70.0f, 16.0f, 1.0f }, { 0.08f, 0.0006f, 0.00592f, 0.01f, 0.000384f, 0.000115f, 0.05756f, 0.00218f, 0.01986f, 0.0f, 70.0f, 16.0f, 1.0f } } },
    { WEAPON_M249, "weapon_m249", 50310, true, 195.0f, 195.0f, 0.592093f, 0.828931f, { { 0.08f, 0.002f, 0.00534f, 0.0077f, 0.001328f, 0.000398f, 0.13281f, 0.00356f, 0.15625f, 0.0f, 50.0f, 25.0f, 2.0f }, { 0.08f, 0.002f, 0.00534f, 0.0077f, 0.001328f, 0.000398f, 0.13281f, 0.00356f, 0.15625f, 0.0f, 50.0f, 25.0f, 2.0f } } },
    { WEAPON_M3, "weapon_nova", 7763, true, 220.0f, 220.0f, 0.328941f, 0.460517f, { { 0.88f, 0.04f, 0.00525f, 0.007f, 0.000788f, 0.000236f, 0.07875f, 0.00972f, 0.03675f, 0.0f, 20.0f, 143.0f, 22.0f }, { 0.88f, 0.04f, 0.00525f, 0.007f, 0.000788f, 0.000236f, 0.07875f, 0.00972f, 0.03675f, 0.0f, 20.0f, 143.0f, 22.0f } } },
    { WEAPON_M4A1, "weapon_m4a1_silencer", 38965, true, 225.0f, 225.0f, 0.302625f, 0.423676f, { { 0.1f, 0.0006f, 0.00368f, 0.0049f, 0.000656f, 0.000197f, 0.110994003f, 0.012f, 0.092879997f, 0.0f, 65.0f, 25.0f, 3.0f }, { 0.1f, 0.0005f, 0.00368f, 0.0049f, 0.000656f, 0.000197f, 0.113671997f, 0.007f, 0.122f, 0.0f, 65.0f, 21.0f, 0.0f } } },
    { WEAPON_TMP, "weapon_mp9", 50729, true, 240.0f, 240.0f, 0.184207f, 0.25789f, { { 0.07f, 0.0006f, 0.0055f, 0.009f, 0.000186f, 5.6e-05f, 0.1489125f, 0.0037f, 0.02904f, 0.0f, 70.0f, 19.0f, 1.0f }, { 0.07f, 0.0006f, 0.0055f, 0.009f, 0.000186f, 5.6e-05f, 0.1489125f, 0.0037f, 0.02904f, 0.0f, 70.0f, 19.0f, 1.0f } } },
    { WEAPON_G3SG1, "weapon_g3sg1", 29908, true, 215.0f, 120.0f, 0.388808f, 0.544331f, { { 0.25f, 0.0003f, 0.01935f, 0.0258f, 0.000873f, 0.000262f, 0.11639f, 0.01861f, 0.15048f, 0.0f, 30.0f, 30.0f, 4.0f }, { 0.25f, 0.0003f, 0.0015f, 0.002f, 0.000873f, 0.000262f, 0.11639f, 0.01861f, 0.15048f, 0.0f, 30.0f, 30.0f, 4.0f } } },
    { WEAPON_DEAGLE, "weapon_deagle", 1454, false, 230.0f, 230.0f, 0.449927f, 0.8112f, { { 0.225f, 0.002f, 0.00218f, 0.0042f, 0.001966f, 0.00073f, 0.152f, 0.07223f, 0.0481f, 0.0f, 60.0f, 48.2f, 18.0f }, { 0.225f, 0.002f, 0.00218f, 0.0042f, 0.001966f, 0.00073f, 0.152f, 0.07223f, 0.0481f, 0.0f, 60.0f, 48.2f, 18.0f } } },
    { WEAPON_SG552, "weapon_sg556", 43500, true, 210.0f, 150.0f, 0.379204f, 0.452886f, { { 0.09f, 0.0005f, 0.00284f, 0.00378f, 0.000627f, 0.000188f, 0.08366f, 0.00668f, 0.13601f, 0.0f, 60.0f, 28.0f, 2.0f }, { 0.09f, 0.0003f, 0.00104f, 0.00218f, 0.000627f, 0.000188f, 0.138758f, 0.00668f, 0.13601f, 0.0f, 60.0f, 19.0f, 2.0f } } },
    { WEAPON_AK47, "weapon_ak47", 223, true, 215.0f, 215.0f, 0.381571f, 0.46f, { { 0.1f, 0.0006f, 0.00481f, 0.00641f, 0.000807f, 0.000242f, 0.14f, 0.0078f, 0.17506f, 0.0f, 70.0f, 30.0f, 0.0f }, { 0.1f, 0.0006f, 0.00481f, 0.00641f, 0.000807f, 0.000242f, 0.14f, 0.0078f, 0.17506f, 0.0f, 70.0f, 30.0f, 0.0f } } },
    { WEAPON_P90, "weapon_p90", 6213, true, 230.0f, 230.0f, 0.265784f, 0.372098f, { { 0.07f, 0.001f, 0.01024f, 0.01365f, 0.00065f, 8.2e-05f, 0.13217f, 0.00285f, 0.031f, 0.0f, 70.0f, 16.0f, 1.0f }, { 0.07f, 0.001f, 0.01024f, 0.01365f, 0.00065f, 8.2e-05f, 0.13217f, 0.00285f, 0.031f, 0.0f, 70.0f, 16.0f, 1.0f } } },
    { WEAPON_CZ75, "weapon_cz75a", 9788, true, 240.0f, 240.0f, 0.287823f, 0.345388f, { { 0.1f, 0.003f, 0.0076f, 0.01043f, 0.000633f, 0.00019f, 0.138f, 0.025f, 0.01341f, 0.0f, 180.0f, 27.0f, 12.0f }, { 0.1f, 0.003f, 0.0076f, 0.01043f, 0.000633f, 0.00019f, 0.138f, 0.025f, 0.01341f, 0.0f, 180.0f, 27.0f, 12.0f } } },
    { WEAPON_TEC9, "weapon_tec9", 789, false, 240.0f, 240.0f, 0.322362f, 0.386834f, { { 0.12f, 0.002f, 0.00757f, 0.00943f, 0.000504f, 0.000211f, 0.1206f, 0.03688f, 0.00381f, 0.0f, 60.0f, 23.0f, 3.0f }, { 0.12f, 0.002f, 0.00757f, 0.00943f, 0.000504f, 0.000211f, 0.1206f, 0.03688f, 0.00381f, 0.0f, 60.0f, 23.0f, 3.0f } } },
    { WEAPON_P2000, "weapon_hkp2000", 5426, false, 240.0f, 240.0f, 0.291277f, 0.349532f, { { 0.17f, 0.002f, 0.00368f, 0.0049f, 0.000638f, 0.000191f, 0.13832f, 0.05f, 0.013f, 0.0f, 0.0f, 26.0f, 0.0f }, { 0.17f, 0.0015f, 0.00368f, 0.0049f, 0.00066f, 0.000198f, 0.1199f, 0.01315f, 0.01387f, 0.0f, 0.0f, 26.0f, 0.0f } } },
    { WEAPON_M4A4, "weapon_m4a1", 38965, true, 225.0f, 225.0f, 0.302625f, 0.423676f, { { 0.09f, 0.0006f, 0.00368f, 0.0049f, 0.00064f, 0.000192f, 0.110994f, 0.007f, 0.13788f, 0.0f, 70.0f, 23.0f, 0.0f }, { 0.09f, 0.00045f, 0.00368f, 0.0049f, 0.000656f, 0.000197f, 0.113672f, 0.00634f, 0.122f, 0.0f, 70.0f, 23.0f, 0.0f } } },
    { WEAPON_MP7, "weapon_mp7", 61649, true, 220.0f, 220.0f, 0.312494f, 0.437491f, { { 0.08f, 0.0006f, 0.00592f, 0.01f, 0.000384f, 0.000115f, 0.05756f, 0.00218f, 0.01986f, 0.0f, 70.0f, 16.0f, 1.0f }, { 0.08f, 0.0006f, 0.00592f, 0.01f, 0.000384f, 0.000115f, 0.05756f, 0.00218f, 0.01986f, 0.0f, 70.0f, 16.0f, 1.0f } } },
    { WEAPON_BIZON, "weapon_bizon", 36387, true, 240.0f, 240.0f, 0.236837f, 0.331572f, { { 0.08f, 0.001f, 0.0105f, 0.014f, 0.000265f, 8e-05f, 0.16965f, 0.00288f, 0.02757f, 0.0f, 70.0f, 18.0f, 1.0f }, { 0.08f, 0.001f, 0.0105f, 0.014f, 0.000265f, 8e-05f, 0.16965f, 0.00288f, 0.02757f, 0.0f, 70.0f, 18.0f, 1.0f } } },
    { WEAPON_MAG7, "weapon_mag7", 12518, true, 225.0f, 225.0f, 0.285521f, 0.399729f, { { 0.85f, 0.04f, 0.00525f, 0.007f, 0.000343f, 0.000103f, 0.13426f, 0.01119f, 0.01599f, 0.0f, 20.0f, 165.0f, 25.0f }, { 0.85f, 0.04f, 0.00525f, 0.007f, 0.000343f, 0.000103f, 0.13426f, 0.01119f, 0.01599f, 0.0f, 20.0f, 165.0f, 25.0f } } },
    { WEAPON_SAWEDOFF, "weapon_sawedoff", 1089, false, 210.0f, 210.0f, 0.328941f, 0.460517f, { { 0.85f, 0.062f, 0.00525f, 0.007f, 0.00036f, 0.000108f, 0.036f, 0.00972f, 0.0168f, 0.0f, 20.0f, 143.0f, 22.0f }, { 0.85f, 0.062f, 0.00525f, 0.007f, 0.00036f, 0.000108f, 0.036f, 0.00972f, 0.0168f, 0.0f, 20.0f, 143.0f, 22.0f } } },
    { WEAPON_NEGEV, "weapon_negev", 57966, true, 195.0f, 195.0f, 0.624987f, 0.874982f, { { 0.06f, 0.002f, 0.00763f, 0.01017f, 0.001364f, 0.000409f, 0.13643f, 0.00337f, 0.15914f, 0.0f, 50.0f, 22.0f, 2.0f }, { 0.06f, 0.002f, 0.00763f, 0.01017f, 0.001364f, 0.000409f, 0.13643f, 0.00337f, 0.15914f, 0.0f, 50.0f, 22.0f, 2.0f } } },
};
inline const CS2015WeaponProfile *CSFind2015WeaponProfile(CSWeaponID id) {
    for (int i=0; i<ARRAYSIZE(g_CS2015WeaponProfiles); ++i) if(g_CS2015WeaponProfiles[i].id==id) return &g_CS2015WeaponProfiles[i];
    return NULL;
}
inline void CSApply2015WeaponProfile(CCSWeaponInfo &info, CSWeaponID id) {
    const CS2015WeaponProfile *p=CSFind2015WeaponProfile(id); if(!p) return;
    info.m_flMaxSpeed=p->maxSpeed; info.m_flMaxSpeedAlt=p->maxSpeedAlt;
    info.m_iRecoilSeed=p->seed; info.m_bFullAuto=p->fullAuto;
    info.m_fRecoveryTimeCrouch=info.m_fRecoveryTimeCrouchFinal=p->recoveryCrouch;
    info.m_fRecoveryTimeStand=info.m_fRecoveryTimeStandFinal=p->recoveryStand;
    info.m_iRecoveryTransitionStartBullet=info.m_iRecoveryTransitionEndBullet=0;
    for(int mode=0;mode<2;++mode) {
        const CS2015WeaponMode &m=p->modes[mode];
        info.m_flCycleTime[mode]=m.cycle; info.m_fSpread[mode]=m.spread;
        info.m_fInaccuracyCrouch[mode]=m.crouch; info.m_fInaccuracyStand[mode]=m.stand;
        info.m_fInaccuracyJump[mode]=m.jump; info.m_fInaccuracyJumpInitial[mode]=0;
        info.m_fInaccuracyLand[mode]=m.land; info.m_fInaccuracyLadder[mode]=m.ladder;
        info.m_fInaccuracyImpulseFire[mode]=m.fire; info.m_fInaccuracyMove[mode]=m.move;
        info.m_fRecoilAngle[mode]=m.angle; info.m_fRecoilAngleVariance[mode]=m.angleVariance;
        info.m_fRecoilMagnitude[mode]=m.magnitude; info.m_fRecoilMagnitudeVariance[mode]=m.magnitudeVariance;
    }
}
#endif
