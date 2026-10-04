//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "weapon_csbase.h"
#include "decals.h"
#include "cs_gamerules.h"
#include "weapon_c4.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"
#include "bone_setup.h"
#include "cs_hitbox_geometry.h"
#include "cs_capsule_geometry.h"
#include "cs_capsule_model.h"
#include "cs_ballistics.h"
#include "cs_gameplay_audit.h"
#include <cmath>

#ifdef CLIENT_DLL
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
	#include "soundent.h"
	#include "bot/cs_bot.h"
	#include "KeyValues.h"
	#include "triggers.h"
	#include "cs_gamestats.h"
#endif

#include "cs_playeranimstate.h"
#include "basecombatweapon_shared.h"
#include "util_shared.h"
#include "takedamageinfo.h"
#include "effect_dispatch_data.h"
#include "engine/ivdebugoverlay.h"
#include "obstacle_pushaway.h"
#include "props_shared.h"

ConVar sv_gameplay_audit("sv_gameplay_audit","0",FCVAR_REPLICATED|FCVAR_CHEAT,"Opt-in diagnostic mask: 1=movement, 2=shots, 4=grenades. Disabled by default.",true,0,true,7);

ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Shows client (red) and server (blue) bullet impact point (1=both, 2=client-only, 3=server-only)" );
ConVar sv_showplayerhitboxes( "sv_showplayerhitboxes", "0", FCVAR_REPLICATED, "Show lag compensated hitboxes for the specified player index whenever a player fires." );

#define	CS_MASK_SHOOT (MASK_SOLID|CONTENTS_DEBRIS)

ConVar sv_player_hitbox_capsules( "sv_player_hitbox_capsules", "2", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Player hitboxes: 0=authored boxes, 1=legacy rounded ellipses, 2=circular bone-local capsules.", true, 0.0f, true, 2.0f );

bool CCSPlayer::TestHitboxes( const Ray_t &ray, unsigned int contentsMask, trace_t &trace )
{
	// Swept melee hulls retain the existing box collision path.
	if ( !sv_player_hitbox_capsules.GetBool() || !ray.m_IsRay )
		return BaseClass::TestHitboxes( ray, contentsMask, trace );
	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr || hdr->numbones() <= 0 || hdr->numbones() > MAXSTUDIOBONES ||
		GetHitboxSet() < 0 || GetHitboxSet() >= hdr->numhitboxsets() ) return false;
	mstudiohitboxset_t *set = hdr->pHitboxSet( GetHitboxSet() );
	if ( !set || !set->numhitboxes ) return false;
#ifdef CLIENT_DLL
	CBoneCache *cache = GetBoneCache( hdr );
#else
	CBoneCache *cache = GetBoneCache();
#endif
	if ( !cache ) return false;
	matrix3x4_t *bones[MAXSTUDIOBONES] = {};
	cache->ReadCachedBonePointers( bones, hdr->numbones() );
	trace.fraction = 1.0f;
	trace.startsolid = trace.allsolid = false;
	trace.startpos = ray.m_Start;
	trace.endpos = ray.m_Start + ray.m_Delta;
	for ( int i = 0; i < set->numhitboxes; ++i )
	{
		mstudiobbox_t *box = set->pHitbox(i);
		if ( box->bone < 0 || box->bone >= hdr->numbones() || !bones[box->bone] ) continue;
		mstudiobone_t *bone = hdr->pBone(box->bone);
		if ( !( bone->contents & contentsMask ) ) continue;
		CSRoundedHitbox shape;
		if ( !shape.Init( box->bbmin, box->bbmax ) ) continue;
		const matrix3x4_t &matrix = *bones[box->bone];
		Vector axes[3], localStart, localDelta;
		const Vector origin(matrix[0][3],matrix[1][3],matrix[2][3]);
		bool valid = true;
		for ( int j = 0; j < 3; ++j )
		{
			axes[j].Init(matrix[0][j],matrix[1][j],matrix[2][j]);
			float lengthSqr = axes[j].LengthSqr();
			if ( !std::isfinite(lengthSqr) || lengthSqr <= 0.000001f ) { valid = false; break; }
			// Inverse transform for orthogonal bone axes, including model scale.
			axes[j] /= lengthSqr;
			localStart[j] = DotProduct(ray.m_Start - origin,axes[j]);
			localDelta[j] = DotProduct(ray.m_Delta,axes[j]);
		}
		if ( !valid ) continue;
		CSRoundedHit hit;
		if ( sv_player_hitbox_capsules.GetInt() == 2 )
		{
			CSCapsuleHitbox capsule;CSCapsuleIntersection result;
			if(!CSReadModelCapsule(*box,capsule) || !CSIntersectCapsule(capsule,localStart,localDelta,result)) continue;
			hit.fraction=result.fraction;hit.exitFraction=result.exitFraction;hit.startSolid=result.startSolid;hit.normal=result.normal;
		}
		else if(!CSIntersectRoundedHitbox(shape,localStart,localDelta,hit)) continue;
		if(hit.fraction >= trace.fraction) continue;
		trace.fraction = hit.fraction;
		trace.startsolid = hit.startSolid;
		trace.allsolid = hit.startSolid && hit.exitFraction >= 1.0f;
		trace.endpos = ray.m_Start + ray.m_Delta * hit.fraction;
		trace.hitbox = i;
		trace.hitgroup = box->group;
		trace.contents = bone->contents | CONTENTS_HITBOX;
		trace.physicsbone = bone->physicsbone;
		trace.surface.name = "**studio**";
		trace.surface.flags = SURF_HITBOX;
		trace.surface.surfaceProps = physprops->GetSurfaceIndex(bone->pszSurfaceProp());
		trace.plane.normal = axes[0]*hit.normal.x + axes[1]*hit.normal.y + axes[2]*hit.normal.z;
		VectorNormalize(trace.plane.normal);
		trace.plane.dist = DotProduct(trace.endpos,trace.plane.normal);
		trace.plane.type = 3;
	}
	// A tested miss must not fall back to the player's movement hull.
	return true;
}

#ifndef CLIENT_DLL
#include "tier1/utlbuffer.h"
#include "filesystem.h"
CON_COMMAND_F(cs_export_player_hitboxes,"Export currently mounted player MDLs and companions for VPK construction.",FCVAR_CHEAT)
{
    int exported=0,failed=0;
    const char *extensions[]={".mdl",".vvd",".dx90.vtx",".phy",".ani"};
    for(int team=0;team<2;++team) {
        const CUtlVectorInitialized<const char*> &models=team ? TerroristPlayerModels : CTPlayerModels;
        for(int i=0;i<models.Count();++i) {
            char stem[256];Q_StripExtension(models[i],stem,sizeof(stem));
            for(int ext=0;ext<ARRAYSIZE(extensions);++ext) {
                char input[256],output[384],folder[384];
                Q_snprintf(input,sizeof(input),"%s%s",stem,extensions[ext]);
                CUtlBuffer data;
                if(!filesystem->ReadFile(input,"GAME",data)) {if(ext<3)++failed;continue;}
                Q_snprintf(output,sizeof(output),"sourceadvanced_gameplay/%s",input);
                Q_strncpy(folder,output,sizeof(folder));Q_StripFilename(folder);
                filesystem->CreateDirHierarchy(folder,"MOD");
                if(filesystem->WriteFile(output,"MOD",data))++exported;else++failed;
            }
        }
    }
    Msg("[model-export] files=%d failures=%d destination=sourceadvanced_gameplay\n",exported,failed);
}
CON_COMMAND_F(cs_test_bot,"Private QA only: cs_test_bot index x y z yaw health armor helmet. Requires server administration and sv_cheats; only fake clients can be changed.",FCVAR_CHEAT)
{
    if(!UTIL_IsCommandIssuedByServerAdmin() || args.ArgC()!=9) {Msg("[test-bot] rejected\n");return;}
    CCSPlayer *p=ToCSPlayer(UTIL_PlayerByIndex(V_atoi(args[1])));
    if(!p || !p->IsBot() || !p->IsAlive()) {Msg("[test-bot] invalid fake client\n");return;}
    float values[7];for(int i=0;i<7;++i) {values[i]=V_atof(args[i+2]);if(!std::isfinite(values[i]))return;}
    Vector position(values[0],values[1],values[2]);
    if(fabsf(position.x)>16384 || fabsf(position.y)>16384 || fabsf(position.z)>16384)return;
    QAngle angles(0,AngleNormalize(values[3]),0);Vector velocity(0,0,0);
    p->Teleport(&position,&angles,&velocity);p->SetHealth(clamp(int(values[4]),1,10000));
    p->SetArmorValue(clamp(int(values[5]),0,100));p->m_bHasHelmet=values[6]>=.5f;
    p->InvalidateBoneCache();
    CStudioHdr *hdr=p->GetModelPtr();CBoneCache *cache=p->GetBoneCache();if(!hdr || !cache || hdr->numbones()>MAXSTUDIOBONES)return;
    matrix3x4_t *bones[MAXSTUDIOBONES]={};cache->ReadCachedBonePointers(bones,hdr->numbones());
    mstudiohitboxset_t *set=hdr->pHitboxSet(p->GetHitboxSet());
    for(int i=0;i<set->numhitboxes;++i) {mstudiobbox_t *box=set->pHitbox(i);if(box->bone<0 || box->bone>=hdr->numbones() || !bones[box->bone])continue;CSCapsuleHitbox capsule;if(!CSReadModelCapsule(*box,capsule))continue;Vector center;VectorTransform((capsule.start+capsule.end)*.5f,*bones[box->bone],center);Msg("[test-bot] player=%d box=%d group=%d x=%.6f y=%.6f z=%.6f\n",p->entindex(),i,box->group,center.x,center.y,center.z);}
}
CON_COMMAND_F(cs_gameplay_status,"Report player health, armor, weapon accuracy and grenade entities.",FCVAR_CHEAT)
{
	for(int i=1;i<=gpGlobals->maxClients;++i)
	{
		CCSPlayer *p=ToCSPlayer(UTIL_PlayerByIndex(i));if(!p)continue;
		CWeaponCSBase *w=dynamic_cast<CWeaponCSBase *>(p->GetActiveWeapon());
		Msg("[gameplay-status] player=%d team=%d alive=%d hp=%d armor=%d helmet=%d ground=%d speed=%.6f stamina=%.6f weapon=%s clip=%d inaccuracy=%.9f recovery=%.9f\n",i,p->GetTeamNumber(),p->IsAlive(),p->GetHealth(),p->ArmorValue(),int(p->m_bHasHelmet),p->GetGroundEntity()!=NULL,p->GetAbsVelocity().Length2D(),float(p->m_flStamina),w?w->GetClassname():"none",w?w->Clip1():-1,w?w->GetInaccuracy():0,w?w->GetRecoveryTime():0);
	}
	const char *names[]={"hegrenade_projectile","flashbang_projectile","smokegrenade_projectile","env_particlesmokegrenade"};
	for(int i=0;i<ARRAYSIZE(names);++i) {int count=0;CBaseEntity *p=NULL;while((p=gEntList.FindEntityByClassname(p,names[i]))!=NULL)++count;Msg("[grenade-status] type=%s count=%d\n",names[i],count);}
}
CON_COMMAND_F(cs_audit_player_scale,"Report model scale, hull, eye height and weapon speed.",FCVAR_CHEAT)
{
    for(int i=1;i<=gpGlobals->maxClients;++i) {
        CCSPlayer *p=ToCSPlayer(UTIL_PlayerByIndex(i));if(!p || !p->IsAlive())continue;
        CStudioHdr *hdr=p->GetModelPtr();int authored=0,total=0;
        if(hdr && p->GetHitboxSet()>=0 && p->GetHitboxSet()<hdr->numhitboxsets()) {
            mstudiohitboxset_t *set=hdr->pHitboxSet(p->GetHitboxSet());total=set->numhitboxes;
            for(int j=0;j<total;++j)if(set->pHitbox(j)->unused[0]==CS_CAPSULE_MDL_MARKER)++authored;
        }
        Msg("[scale-audit] player=%d model=%s scale=%.3f hull=%.3f eye=%.3f fov=%d maxspeed=%.3f actualspeed=%.3f mdl_capsules=%d/%d\n",
            i,STRING(p->GetModelName()),p->GetModelScale(),p->CollisionProp()->OBBSize().z,p->GetViewOffset().z,p->GetFOV(),p->GetPlayerMaxSpeed(),p->GetAbsVelocity().Length2D(),authored,total);
    }
    Msg("[scale-audit] standing_hull=%.1f crouched_hull=%.1f standing_eye=%.1f crouched_eye=%.1f\n",
        CSGameRules()->GetViewVectors()->m_vHullMax.z,CSGameRules()->GetViewVectors()->m_vDuckHullMax.z,
        CSGameRules()->GetViewVectors()->m_vView.z,CSGameRules()->GetViewVectors()->m_vDuckView.z);
}
#endif

#ifndef CLIENT_DLL
CON_COMMAND_F( cs_validate_hitboxes, "Validate current player bones and rounded hitboxes without drawing overlays.", FCVAR_CHEAT )
{
	int players = 0, tested = 0, failures = 0;
	MDLCACHE_CRITICAL_SECTION();
	for ( int playerIndex = 1; playerIndex <= gpGlobals->maxClients; ++playerIndex )
	{
		CCSPlayer *player = ToCSPlayer(UTIL_PlayerByIndex(playerIndex));
		if ( !player || !player->IsAlive() ) continue;
		++players;
		CStudioHdr *hdr = player->GetModelPtr();
		if ( !hdr || hdr->numbones() > MAXSTUDIOBONES || player->GetHitboxSet() < 0 ||
			player->GetHitboxSet() >= hdr->numhitboxsets() ) { ++failures; continue; }
		CBoneCache *cache = player->GetBoneCache();
		if ( !cache ) { ++failures; continue; }
		matrix3x4_t *bones[MAXSTUDIOBONES] = {};
		cache->ReadCachedBonePointers(bones,hdr->numbones());
		mstudiohitboxset_t *set = hdr->pHitboxSet(player->GetHitboxSet());
		int playerFailures = 0;
		for ( int boxIndex = 0; boxIndex < set->numhitboxes; ++boxIndex )
		{
			mstudiobbox_t *box = set->pHitbox(boxIndex);
			CSRoundedHitbox shape;
			if ( box->bone < 0 || box->bone >= hdr->numbones() || !bones[box->bone] || !shape.Init(box->bbmin,box->bbmax) )
			{ ++playerFailures; continue; }
			Vector center;
			VectorTransform(shape.center,*bones[box->bone],center);
			for ( int axis = 0; axis < 3; ++axis )
				for ( int sign = -1; sign <= 1; sign += 2 )
				{
					Vector start = center;
					start[axis] += sign*8192.0f;
					Ray_t ray; ray.Init(start,center);
					trace_t trace;
					++tested;
					if ( !player->TestHitboxes(ray,MASK_SHOT,trace) || trace.fraction >= 1.0f || trace.hitgroup <= 0 ) ++playerFailures;
				}
		}
		failures += playerFailures;
		Msg("[hitbox-audit] player=%d model=%s bones=%d boxes=%d failures=%d\n",playerIndex,hdr->pszName(),hdr->numbones(),set->numhitboxes,playerFailures);
	}
	Msg("[hitbox-audit] players=%d rays=%d failures=%d rounded=%d\n",players,tested,failures,sv_player_hitbox_capsules.GetInt());
}
#endif

void DispatchEffect( const char *pName, const CEffectData &data );


#ifdef _DEBUG

	// This is some extra code to collect weapon accuracy stats:

	struct bulletdata_s
	{
		float	timedelta;	// time delta since first shot of this round
		float	derivation;	// derivation for first shoot view angle
		int		count;
	};

	#define STATS_MAX_BULLETS	50

	static bulletdata_s s_bullet_stats[STATS_MAX_BULLETS];

	Vector	s_firstImpact = Vector(0,0,0);
	float	s_firstTime = 0;
	float	s_LastTime = 0;
	int		s_bulletCount = 0;

	void ResetBulletStats()
	{
		s_firstTime = 0;
		s_LastTime = 0;
		s_bulletCount = 0;
		s_firstImpact = Vector(0,0,0);
		Q_memset( s_bullet_stats, 0, sizeof(s_bullet_stats) );
	}

	void PrintBulletStats()
	{
		for (int i=0; i<STATS_MAX_BULLETS; i++ )
		{
			if (s_bullet_stats[i].count == 0)
				break;

			Msg("%3i;%3i;%.4f;%.4f\n", i, s_bullet_stats[i].count,
				s_bullet_stats[i].timedelta, s_bullet_stats[i].derivation );
		}
	}

	void AddBulletStat( float time, float dist, Vector &impact )
	{
		if ( time > s_LastTime + 2.0f )
		{
			// time delta since last shoot is bigger than 2 seconds, start new row
			s_LastTime = s_firstTime = time;
			s_bulletCount = 0;
			s_firstImpact = impact;

		}
		else
		{
			s_LastTime = time;
			s_bulletCount++;
		}

		if ( s_bulletCount >= STATS_MAX_BULLETS )
			s_bulletCount = STATS_MAX_BULLETS -1;

		if ( dist < 1 )
			dist = 1;

		int i = s_bulletCount;

		float offset = VectorLength( s_firstImpact - impact );

		float timedelta = time - s_firstTime;
		float derivation = offset / dist;

		float weight = (float)s_bullet_stats[i].count/(float)(s_bullet_stats[i].count+1);

		s_bullet_stats[i].timedelta *= weight;
		s_bullet_stats[i].timedelta += (1.0f-weight) * timedelta;

		s_bullet_stats[i].derivation *= weight;
		s_bullet_stats[i].derivation += (1.0f-weight) * derivation;

		s_bullet_stats[i].count++;
	}

	CON_COMMAND( stats_bullets_reset, "Reset bullet stats")
	{
		ResetBulletStats();
	}

	CON_COMMAND( stats_bullets_print, "Print bullet stats")
	{
		PrintBulletStats();
	}

#endif

float CCSPlayer::GetPlayerMaxSpeed()
{
	if ( GetMoveType() == MOVETYPE_NONE )
	{
		return CS_PLAYER_SPEED_STOPPED;
	}

	if ( IsObserver() )
	{
		// Player gets speed bonus in observer mode
		return CS_PLAYER_SPEED_OBSERVER;
	}

	bool bValidMoveState = ( State_Get() == STATE_ACTIVE || State_Get() == STATE_OBSERVER_MODE );
	if ( !bValidMoveState || m_bIsDefusing || CSGameRules()->IsFreezePeriod() )
	{
		// Player should not move during the freeze period
		return CS_PLAYER_SPEED_STOPPED;
	}

	float speed = BaseClass::GetPlayerMaxSpeed();

	if ( IsVIP() == true )  // VIP is slow due to the armour he's wearing
	{
		speed = MIN(speed, CS_PLAYER_SPEED_VIP);
	}
	else
	{

		CWeaponCSBase *pWeapon = dynamic_cast<CWeaponCSBase*>( GetActiveWeapon() );

		if ( pWeapon )
		{
			if ( HasShield() && IsShieldDrawn() )
			{
				speed = MIN(speed, CS_PLAYER_SPEED_SHIELD);
			}
			else
			{
				speed = MIN(speed, pWeapon->GetMaxSpeed());
			}
		}
	}

	return speed;
}


void CCSPlayer::GetBulletTypeParameters(
	int iBulletType,
	float &fPenetrationPower,
	float &flPenetrationDistance )
{
	//MIKETODO: make ammo types come from a script file.
	if ( IsAmmoType( iBulletType, BULLET_PLAYER_50AE ) )
	{
		fPenetrationPower = 30;
		flPenetrationDistance = 1000.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_762MM ) )
	{
		fPenetrationPower = 39;
		flPenetrationDistance = 5000.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_556MM ) ||
			  IsAmmoType( iBulletType, BULLET_PLAYER_556MM_BOX ) )
	{
		fPenetrationPower = 35;
		flPenetrationDistance = 4000.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_338MAG ) )
	{
		fPenetrationPower = 45;
		flPenetrationDistance = 8000.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_9MM ) )
	{
		fPenetrationPower = 21;
		flPenetrationDistance = 800.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_BUCKSHOT ) )
	{
		fPenetrationPower = 0;
		flPenetrationDistance = 0.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_45ACP ) )
	{
		fPenetrationPower = 15;
		flPenetrationDistance = 500.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_357SIG ) )
	{
		fPenetrationPower = 25;
		flPenetrationDistance = 800.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_57MM ) )
	{
		fPenetrationPower = 30;
		flPenetrationDistance = 2000.0;
	}
	else
	{
		// What kind of ammo is this?
		Assert( false );
		fPenetrationPower = 0;
		flPenetrationDistance = 0.0;
	}
}

static void GetMaterialParameters( int iMaterial, float &flPenetrationModifier, float &flDamageModifier )
{
	switch ( iMaterial )
	{
		case CHAR_TEX_METAL :
			flPenetrationModifier = 0.5;  // If we hit metal, reduce the thickness of the brush we can't penetrate
			flDamageModifier = 0.3;
			break;
		case CHAR_TEX_DIRT :
			flPenetrationModifier = 0.5;
			flDamageModifier = 0.3;
			break;
		case CHAR_TEX_CONCRETE :
			flPenetrationModifier = 0.4;
			flDamageModifier = 0.25;
			break;
		case CHAR_TEX_GRATE	:
			flPenetrationModifier = 1.0;
			flDamageModifier = 0.99;
			break;
		case CHAR_TEX_VENT :
			flPenetrationModifier = 0.5;
			flDamageModifier = 0.45;
			break;
		case CHAR_TEX_TILE :
			flPenetrationModifier = 0.65;
			flDamageModifier = 0.3;
			break;
		case CHAR_TEX_COMPUTER :
			flPenetrationModifier = 0.4;
			flDamageModifier = 0.45;
			break;
		case CHAR_TEX_WOOD :
			flPenetrationModifier = 1.0;
			flDamageModifier = 0.6;
			break;
		default :
			flPenetrationModifier = 1.0;
			flDamageModifier = 0.5;
			break;
	}

	Assert( flPenetrationModifier > 0 );
	Assert( flDamageModifier < 1.0f ); // Less than 1.0f for avoiding infinite loops
}


static bool TraceToExit(Vector &start, Vector &dir, Vector &end, float flStepSize, float flMaxDistance )
{
	float flDistance = 0;
	Vector last = start;

	while ( flDistance < flMaxDistance )
	{
		flDistance = MIN(flDistance + flStepSize, flMaxDistance);

		end = start + flDistance *dir;

		if ( (UTIL_PointContents ( end ) & MASK_SOLID) == 0 )
		{
			// found first free point
			return true;
		}
	}

	return false;
}

inline void UTIL_TraceLineIgnoreTwoEntities( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask,
					 const IHandleEntity *ignore, const IHandleEntity *ignore2, int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CTraceFilterSkipTwoEntities traceFilter( ignore, ignore2, collisionGroup );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
}

void CCSPlayer::FireBullet(
	Vector vecSrc,	// shooting postion
	const QAngle &shootAngles,  //shooting angle
	float flDistance, // max distance
	int iPenetration, // how many obstacles can be penetrated
	int iBulletType, // ammo type
	int iDamage, // base damage
	float flRangeModifier, // damage range modifier
	CBaseEntity *pevAttacker, // shooter
	bool bDoEffects,
	float xSpread, float ySpread, float penetrationPower
	)
{
	float fCurrentDamage = iDamage;   // damage of the bullet at it's current trajectory
	float flCurrentDistance = 0.0;  //distance that the bullet has traveled so far

	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( shootAngles, &vecDirShooting, &vecRight, &vecUp );

	// MIKETODO: put all the ammo parameters into a script file and allow for CS-specific params.
	float flPenetrationPower = 0;		// thickness of a wall that this bullet can penetrate
	float flPenetrationDistance = 0;	// distance at which the bullet is capable of penetrating a wall
	float flDamageModifier = 0.5;		// default modification of bullets power after they go through a wall.
	float flPenetrationModifier = 1.f;

	GetBulletTypeParameters( iBulletType, flPenetrationPower, flPenetrationDistance );
	const bool materialPenetration=std::isfinite(penetrationPower) && penetrationPower>0;
	if(materialPenetration) { iPenetration=4; flPenetrationDistance=3000.0f; }


	if ( !pevAttacker )
		pevAttacker = this;  // the default attacker is ourselves

	// add the spray
	Vector vecDir = vecDirShooting + xSpread * vecRight + ySpread * vecUp;

	VectorNormalize( vecDir );

	//Adrian: visualize server/client player positions
	//This is used to show where the lag compesator thinks the player should be at.
#if 0
	for ( int k = 1; k <= gpGlobals->maxClients; k++ )
	{
		CBasePlayer *clientClass = (CBasePlayer *)CBaseEntity::Instance( k );

		if ( clientClass == NULL )
			 continue;

		if ( k == entindex() )
			 continue;

#ifdef CLIENT_DLL
		debugoverlay->AddBoxOverlay( clientClass->GetAbsOrigin(), clientClass->WorldAlignMins(), clientClass->WorldAlignMaxs(), QAngle( 0, 0, 0), 255,0,0,127, 4 );
#else
		NDebugOverlay::Box( clientClass->GetAbsOrigin(), clientClass->WorldAlignMins(), clientClass->WorldAlignMaxs(), 0,0,255,127, 4 );
#endif

	}

#endif


//=============================================================================
// HPE_BEGIN:
//=============================================================================

#ifndef CLIENT_DLL
	// [pfreese] Track number player entities killed with this bullet
	int iPenetrationKills = 0;

	// [menglish] Increment the shots fired for this player
	CCS_GameStats.Event_ShotFired( this, GetActiveWeapon() );
#endif

//=============================================================================
// HPE_END
//=============================================================================

	bool bFirstHit = true;

	// how many objects (walls/players) the bullet passed through when it hits a target.
	const int iPenetrationMax = iPenetration;

	CBasePlayer *lastPlayerHit = NULL;

	if( sv_showplayerhitboxes.GetInt() > 0 )
	{
		CBasePlayer *lagPlayer = UTIL_PlayerByIndex( sv_showplayerhitboxes.GetInt() );
		if( lagPlayer )
		{
#ifdef CLIENT_DLL
			lagPlayer->DrawClientHitboxes(4, true);
#else
			lagPlayer->DrawServerHitboxes(4, true);
#endif
		}
	}

	MDLCACHE_CRITICAL_SECTION();
	while ( fCurrentDamage > 0 )
	{
	const int iPenetrationBeforeHit = iPenetration;

	Vector vecEnd = vecSrc + vecDir * flDistance;

	trace_t tr; // main enter bullet trace

		UTIL_TraceLineIgnoreTwoEntities( vecSrc, vecEnd, CS_MASK_SHOOT|CONTENTS_HITBOX, this, lastPlayerHit, COLLISION_GROUP_NONE, &tr );
		{
			CTraceFilterSkipTwoEntities filter( this, lastPlayerHit, COLLISION_GROUP_NONE );

			// Check for player hitboxes extending outside their collision bounds
			const float rayExtension = 40.0f;
			UTIL_ClipTraceToPlayers( vecSrc, vecEnd + vecDir * rayExtension, CS_MASK_SHOOT|CONTENTS_HITBOX, &filter, &tr );
		}

		lastPlayerHit = ToBasePlayer(tr.m_pEnt);

		if ( tr.fraction == 1.0f )
			break; // we didn't hit anything, stop tracing shoot

#ifdef _DEBUG
		if ( bFirstHit )
			AddBulletStat( gpGlobals->realtime, VectorLength( vecSrc-tr.endpos), tr.endpos );
#endif

		bFirstHit = false;

#ifndef CLIENT_DLL
		//
		// Propogate a bullet impact event
		// @todo Add this for shotgun pellets (which dont go thru here)
		//
		IGameEvent * event = gameeventmanager->CreateEvent( "bullet_impact" );
		if ( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetFloat( "x", tr.endpos.x );
			event->SetFloat( "y", tr.endpos.y );
			event->SetFloat( "z", tr.endpos.z );
			gameeventmanager->FireEvent( event );
		}
#endif

		/************* MATERIAL DETECTION ***********/
		surfacedata_t *pSurfaceData = physprops->GetSurfaceData( tr.surface.surfaceProps );
		int iEnterMaterial = pSurfaceData->game.material;

		GetMaterialParameters( iEnterMaterial, flPenetrationModifier, flDamageModifier );

		bool hitGrate = tr.contents & CONTENTS_GRATE;

		// since some railings in de_inferno are CONTENTS_GRATE but CHAR_TEX_CONCRETE, we'll trust the
		// CONTENTS_GRATE and use a high damage modifier.
		if ( hitGrate )
		{
			// If we're a concrete grate (TOOLS/TOOLSINVISIBLE texture) allow more penetrating power.
			flPenetrationModifier = 1.0f;
			flDamageModifier = 0.99f;
		}

#ifdef CLIENT_DLL
		if ( sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 2 )
		{
			// draw red client impact markers
			debugoverlay->AddBoxOverlay( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 255,0,0,127, 4 );

			if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
			{
				C_BasePlayer *player = ToBasePlayer( tr.m_pEnt );
				player->DrawClientHitboxes( 4, true );
			}
		}
#else
		if ( sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 3 )
		{
			// draw blue server impact markers
			NDebugOverlay::Box( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 4 );

			if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
			{
				CBasePlayer *player = ToBasePlayer( tr.m_pEnt );
				player->DrawServerHitboxes( 4, true );
			}
		}
#endif

		// Apply falloff only for the segment just traced. Using the accumulated
		// distance here compounds the previous segments after every penetration.
		const float flSegmentDistance = tr.fraction * flDistance;
		flCurrentDistance += flSegmentDistance;
		fCurrentDamage *= pow( flRangeModifier, flSegmentDistance / 500.0f );

		// check if we reach penetration distance, no more penetrations after that
		if (flCurrentDistance > flPenetrationDistance && iPenetration > 0)
			iPenetration = 0;

#ifndef CLIENT_DLL
		// This just keeps track of sounds for AIs (it doesn't play anything).
		CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, tr.endpos, 400, 0.2f, this );
#endif

		int iDamageType = DMG_BULLET | DMG_NEVERGIB;

		if( bDoEffects )
		{
			// See if the bullet ended up underwater + started out of the water
			if ( enginetrace->GetPointContents( tr.endpos ) & (CONTENTS_WATER|CONTENTS_SLIME) )
			{
				trace_t waterTrace;
				UTIL_TraceLine( vecSrc, tr.endpos, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &waterTrace );

				if( waterTrace.allsolid != 1 )
				{
					CEffectData	data;
 					data.m_vOrigin = waterTrace.endpos;
					data.m_vNormal = waterTrace.plane.normal;
					data.m_flScale = random->RandomFloat( 8, 12 );

					if ( waterTrace.contents & CONTENTS_SLIME )
					{
						data.m_fFlags |= FX_WATER_IN_SLIME;
					}

					DispatchEffect( "gunshotsplash", data );
				}
			}
			else
			{
				//Do Regular hit effects

				// Don't decal nodraw surfaces
				if ( !( tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP) ) )
				{
					CBaseEntity *pEntity = tr.m_pEnt;
					if ( !( !friendlyfire.GetBool() && pEntity && pEntity->GetTeamNumber() == GetTeamNumber() ) )
					{
						UTIL_ImpactTrace( &tr, iDamageType );
					}
				}
			}
		} // bDoEffects

		// add damage to entity that we hit

#ifndef CLIENT_DLL
		ClearMultiDamage();

		//=============================================================================
		// HPE_BEGIN:
		// [pfreese] Check if enemy players were killed by this bullet, and if so,
		// add them to the iPenetrationKills count
		//=============================================================================

			CBaseEntity *pEntity = tr.m_pEnt;

		CTakeDamageInfo info( pevAttacker, pevAttacker, fCurrentDamage, iDamageType );
			// number of objects this bullet has penetrated before hitting this entity
		info.SetObjectsPenetrated( iPenetrationMax - iPenetrationBeforeHit );
		CalculateBulletDamageForce( &info, iBulletType, vecDir, tr.endpos );
		pEntity->DispatchTraceAttack( info, vecDir, &tr );

		bool bWasAlive = pEntity->IsAlive();

		TraceAttackToTriggers( info, tr.startpos, tr.endpos, vecDir );

		const int healthBefore=pEntity->GetHealth();
		ApplyMultiDamage();
		if (sv_gameplay_audit.GetInt() & CS_AUDIT_SHOTS)
			CSGameplayAuditPrint("[impact-audit] tick=%d player=%d target=%d hitgroup=%d material=%d distance=%.6f damage=%.6f health_before=%d health_after=%d penetrated=%d x=%.6f y=%.6f z=%.6f\n",
				gpGlobals->tickcount,entindex(),pEntity->entindex(),tr.hitgroup,int(pSurfaceData->game.material),flCurrentDistance,fCurrentDamage,
				healthBefore,pEntity->GetHealth(),iPenetrationMax-iPenetrationBeforeHit,tr.endpos.x,tr.endpos.y,tr.endpos.z);

		if (bWasAlive && !pEntity->IsAlive() && pEntity->IsPlayer() && pEntity->GetTeamNumber() != GetTeamNumber())
		{
			++iPenetrationKills;
		}
		
		//=============================================================================
		// HPE_END
		//=============================================================================

#endif

		// check if bullet can penetrate another entity
		if ( iPenetration == 0 && ( materialPenetration || !hitGrate ) )
			break; // no, stop

		// If we hit a grate with iPenetration == 0, stop on the next thing we hit
		if ( iPenetration < 0 )
			break;

		Vector penetrationEnd;

		// try to penetrate object, maximum penetration is 128 inch
		if ( !TraceToExit( tr.endpos, vecDir, penetrationEnd, materialPenetration ? 4.0f : 24.0f, materialPenetration ? 90.0f : 128.0f ) )
			break;

		// find exact penetration exit
		trace_t exitTr;
		UTIL_TraceLine( penetrationEnd, tr.endpos, CS_MASK_SHOOT|CONTENTS_HITBOX, NULL, &exitTr );

		if( exitTr.m_pEnt != tr.m_pEnt && exitTr.m_pEnt != NULL )
		{
			// something was blocking, trace again
			UTIL_TraceLine( penetrationEnd, tr.endpos, CS_MASK_SHOOT|CONTENTS_HITBOX, exitTr.m_pEnt, COLLISION_GROUP_NONE, &exitTr );
		}

		// Never accept a start-solid or missing exit as a traversable wall.
		if(exitTr.allsolid || exitTr.startsolid || exitTr.fraction==1.0f) break;

		// get material at exit point
		pSurfaceData = physprops->GetSurfaceData( exitTr.surface.surfaceProps );
		int iExitMaterial = pSurfaceData->game.material;

		hitGrate = hitGrate && ( exitTr.contents & CONTENTS_GRATE );

		// if enter & exit point is wood or metal we assume this is
		// a hollow crate or barrel and give a penetration bonus
		if ( iEnterMaterial == iExitMaterial )
		{
			if( iExitMaterial == CHAR_TEX_WOOD ||
				iExitMaterial == CHAR_TEX_METAL )
			{
				flPenetrationModifier *= 2;
			}
		}

		float flTraceDistance = VectorLength( exitTr.endpos - tr.endpos );

		// check if bullet has enough power to penetrate this distance for this material
		if ( !materialPenetration && flTraceDistance > ( flPenetrationPower * flPenetrationModifier ) )
			break; // bullet hasn't enough power to penetrate this distance

		if(materialPenetration)
		{
			fCurrentDamage-=CSPenetrationDamageLoss(fCurrentDamage,flTraceDistance,penetrationPower,iEnterMaterial,iExitMaterial,hitGrate);
			if(fCurrentDamage<1.0f) break;
		}

		// penetration was successful

		// bullet did penetrate object, exit Decal
		if ( bDoEffects )
		{
			UTIL_ImpactTrace( &exitTr, iDamageType );
		}

		//setup new start end parameters for successive trace

		if(!materialPenetration) flPenetrationPower -= flTraceDistance / flPenetrationModifier;
		flCurrentDistance += flTraceDistance;

		// NDebugOverlay::Box( exitTr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,255,0,127, 8 );

		vecSrc = exitTr.endpos;
		flDistance = MAX( 0.0f, flDistance - flSegmentDistance - flTraceDistance );
		if ( flDistance <= 0.0f )
			break;

		// reduce damage power each time we hit something other than a grate
		if(!materialPenetration) fCurrentDamage *= flDamageModifier;

		// reduce penetration counter
		iPenetration--;
	}

#ifndef CLIENT_DLL
	//=============================================================================
	// HPE_BEGIN:
	// [pfreese] If we killed at least two enemies with a single bullet, award the
	// TWO_WITH_ONE_SHOT achievement
	//=============================================================================
	
	if (iPenetrationKills >= 2)
	{
		AwardAchievement(CSKillTwoWithOneShot);
	}
	
	//=============================================================================
	// HPE_END
	//=============================================================================
#endif
}


void CCSPlayer::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity  )
{
	float speedSqr = vecVelocity.AsVector2D().LengthSqr();

	// the fastest walk is 135 ( scout ), see CCSGameMovement::CheckParameters()
	if ( speedSqr < 150.0 * 150.0 ) 
		return; // player is not running, no footsteps

	BaseClass::UpdateStepSound( psurface, vecOrigin, vecVelocity  );
}


ConVar weapon_recoil_view_punch_extra( "weapon_recoil_view_punch_extra", "0.055", FCVAR_CHEAT | FCVAR_REPLICATED, "Additional (non-aim) punch added to view from recoil" );
ConVar weapon_recoil_scale( "weapon_recoil_scale", "2.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Overall recoil scale factor for the aim punch" );

QAngle CCSPlayer::GetAimPunchAngle()
{
	return m_Local.m_aimPunchAngle.Get() * weapon_recoil_scale.GetFloat();
}

QAngle CCSPlayer::GetRawAimPunchAngle() const
{
	return m_Local.m_aimPunchAngle.Get();
}

// Table-driven aim punch (see CWeaponCSBase::Recoil)
void CCSPlayer::KickBack( float fAngle, float fMagnitude )
{
	QAngle angleVelocity( 0, 0, 0 );
	angleVelocity[YAW] = -sinf( DEG2RAD( fAngle ) ) * fMagnitude;
	angleVelocity[PITCH] = -cosf( DEG2RAD( fAngle ) ) * fMagnitude;
	angleVelocity += m_Local.m_aimPunchAngleVel.Get();
	SetAimPunchAngleVelocity( angleVelocity );

	// this bit gives additional punch to the view (screen shake) to make the kick back a bit more visceral
	QAngle viewPunch = GetPunchAngle();
	float fViewPunchMagnitude = fMagnitude * weapon_recoil_view_punch_extra.GetFloat();
	viewPunch[YAW] -= sinf( DEG2RAD( fAngle ) ) * fViewPunchMagnitude;
	viewPunch[PITCH] -= cosf( DEG2RAD( fAngle ) ) * fViewPunchMagnitude;
	SetPunchAngle( viewPunch );
}


bool CCSPlayer::CanMove() const
{
	// When we're in intro camera mode, it's important to return false here
	// so our physics object doesn't fall out of the world.
	if ( GetMoveType() == MOVETYPE_NONE )
		return false;

	if ( IsObserver() )
		return true; // observers can move all the time

	bool bValidMoveState = (State_Get() == STATE_ACTIVE || State_Get() == STATE_OBSERVER_MODE);

	if ( m_bIsDefusing || !bValidMoveState || CSGameRules()->IsFreezePeriod() )
	{
		return false;
	}
	else
	{
		// Can't move while planting C4.
		CC4 *pC4 = dynamic_cast< CC4* >( GetActiveWeapon() );
		if ( pC4 && pC4->m_bStartedArming )
			return false;

		return true;
	}
}


void CCSPlayer::OnJump( float fImpulse )
{
	CWeaponCSBase* pActiveWeapon = GetActiveCSWeapon();
	if ( pActiveWeapon != NULL )
		pActiveWeapon->OnJump(fImpulse);
}


void CCSPlayer::OnLand( float fVelocity )
{
	CWeaponCSBase* pActiveWeapon = GetActiveCSWeapon();
	if ( pActiveWeapon != NULL )
		pActiveWeapon->OnLand(fVelocity);
}


//-------------------------------------------------------------------------------------------------------------------------------
/**
* Track the last time we were on a ladder, along with the ladder's normal and where we
* were grabbing it, so we don't reach behind us and grab it again as we are trying to
* dismount.
*/
void CCSPlayer::SurpressLadderChecks( const Vector& pos, const Vector& normal )
{
	m_ladderSurpressionTimer.Start( 1.0f );
	m_lastLadderPos = pos;
	m_lastLadderNormal = normal;
}


//-------------------------------------------------------------------------------------------------------------------------------
/**
* Prevent us from re-grabbing the same ladder we were just on:
*  - if the timer is elapsed, let us grab again
*  - if the normal is different, let us grab
*  - if the 2D pos is very different, let us grab, since it's probably a different ladder
*/
bool CCSPlayer::CanGrabLadder( const Vector& pos, const Vector& normal )
{
	if ( m_ladderSurpressionTimer.GetRemainingTime() <= 0.0f )
	{
		return true;
	}

	const float MaxDist = 64.0f;
	if ( pos.AsVector2D().DistToSqr( m_lastLadderPos.AsVector2D() ) < MaxDist * MaxDist )
	{
		return false;
	}

	if ( normal != m_lastLadderNormal )
	{
		return true;
	}

	return false;
}


void CCSPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	// In CS, its CPlayerAnimState object manages ALL the animation state.
	return;
}


CWeaponCSBase* CCSPlayer::CSAnim_GetActiveWeapon()
{
	return GetActiveCSWeapon();
}


bool CCSPlayer::CSAnim_CanMove()
{
	return CanMove();
}

//--------------------------------------------------------------------------------------------------------------

#define MATERIAL_NAME_LENGTH 16

#ifdef GAME_DLL

class CFootstepControl : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFootstepControl, CBaseTrigger );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual int UpdateTransmitState( void );
	virtual void Spawn( void );

	CNetworkVar( string_t, m_source );
	CNetworkVar( string_t, m_destination );
};

LINK_ENTITY_TO_CLASS( func_footstep_control, CFootstepControl );


BEGIN_DATADESC( CFootstepControl )
	DEFINE_KEYFIELD( m_source, FIELD_STRING, "Source" ),
	DEFINE_KEYFIELD( m_destination, FIELD_STRING, "Destination" ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFootstepControl, DT_FootstepControl )
	SendPropStringT( SENDINFO(m_source) ),
	SendPropStringT( SENDINFO(m_destination) ),
END_SEND_TABLE()

int CFootstepControl::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CFootstepControl::Spawn( void )
{
	InitTrigger();
}

#else

//--------------------------------------------------------------------------------------------------------------

class C_FootstepControl : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_FootstepControl, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_FootstepControl( void );
	~C_FootstepControl();

	char m_source[MATERIAL_NAME_LENGTH];
	char m_destination[MATERIAL_NAME_LENGTH];
};

IMPLEMENT_CLIENTCLASS_DT(C_FootstepControl, DT_FootstepControl, CFootstepControl)
	RecvPropString( RECVINFO(m_source) ),
	RecvPropString( RECVINFO(m_destination) ),
END_RECV_TABLE()

CUtlVector< C_FootstepControl * > s_footstepControllers;

C_FootstepControl::C_FootstepControl( void )
{
	s_footstepControllers.AddToTail( this );
}

C_FootstepControl::~C_FootstepControl()
{
	s_footstepControllers.FindAndRemove( this );
}

surfacedata_t * CCSPlayer::GetFootstepSurface( const Vector &origin, const char *surfaceName )
{
	for ( int i=0; i<s_footstepControllers.Count(); ++i )
	{
		C_FootstepControl *control = s_footstepControllers[i];

		if ( FStrEq( control->m_source, surfaceName ) )
		{
			if ( control->CollisionProp()->IsPointInBounds( origin ) )
			{
				return physprops->GetSurfaceData( physprops->GetSurfaceIndex( control->m_destination ) );
			}
		}
	}

	return physprops->GetSurfaceData( physprops->GetSurfaceIndex( surfaceName ) );
}

#endif


