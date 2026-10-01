#include "cbase.h"
#include "cs_feedback_sound_events.h"
#include "cs_inventory.h"
#include "weapon_csbase.h"
#include "baseviewmodel_shared.h"
#include "filesystem.h"
#include "tier1/KeyValues.h"
#include "tier1/utlbuffer.h"
#include "studio.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#ifdef CLIENT_DLL
#include "c_cs_player.h"
#include "materialsystem/imaterialsystem.h"
#include "functionproxy.h"
#else
#include "cs_player.h"
#endif
#include "tier0/memdbgon.h"
extern ISoundEmitterSystemBase *soundemitterbase;

static CInventoryManager s_Inventory;
CInventoryManager &CSInventory() { return s_Inventory; }
#ifdef CLIENT_DLL
// The imported AUG/SG lenses fade with the existing 55-degree zoom.
class CInventoryIronSightProxy : public CResultProxy
{
public:
	bool Init(IMaterial *material, KeyValues *keys)
	{
		m_Invert=keys->GetBool("invert",false);
		return CResultProxy::Init(material,keys);
	}
	void OnBind(void *)
	{
		C_CSPlayer *player=C_CSPlayer::GetLocalCSPlayer();
		CWeaponCSBase *weapon=player ? dynamic_cast<CWeaponCSBase *>(player->GetActiveWeapon()) : NULL;
		float amount=0.0f;
		if (weapon && (weapon->GetWeaponID()==WEAPON_AUG || weapon->GetWeaponID()==WEAPON_SG552))
		{
			const float normal=player->GetDefaultFOV();
			amount=clamp((normal-player->GetFOV())/MAX(1.0f,normal-55.0f),0.0f,1.0f);
		}
		SetFloatResult(m_Invert ? 1.0f-amount : amount);
	}
private:
	bool m_Invert;
};
EXPOSE_INTERFACE(CInventoryIronSightProxy,IMaterialProxy,"IronSightAmount" IMATERIAL_PROXY_INTERFACE_VERSION);
static ConVar cl_inventory_gloves( "cl_inventory_gloves", "0", FCVAR_ARCHIVE, "CSSO first-person glove inventory item; 0 selects the default CSSO Sporty gloves." );
const CGloveItem *CSSelectedGlove()
{
	const CGloveItem *selected = CSInventory().FindGlove(cl_inventory_gloves.GetInt());
	return selected ? selected : CSInventory().FindGlove(4019);
}
#endif
static void RegisterInventoryModel( const char *path )
{
#ifdef CLIENT_DLL
	CBaseEntity::PrecacheModel( path );
#else
	// Register every choice before signon, but load a cosmetic only when used.
	// The string-table entry still permits switching models during the match.
	// BaseEntity also scans model components, which immediately loads the
	// model even with bPreload=false. These cosmetic meshes have client-side
	// animation sounds from the manifest, so register directly with engine.
	engine->PrecacheModel( path, false );
#endif
}

static bool ValidModel( const char *path, int skin = 0 )
{
	const char *extension = Q_GetFileExtension( path );
	if ( !extension || Q_strnicmp( path, "models/", 7 ) || Q_strstr( path, ".." ) || Q_strstr( path, "\\" ) || Q_strstr( path, ":" ) || Q_stricmp( extension, "mdl" ) )
		return false;
	// Validation needs the header, not the entire model payload.
	FileHandle_t file = filesystem->Open( path, "rb", "GAME" );
	if ( file == FILESYSTEM_INVALID_HANDLE ) { DevWarning("[inventory-model] open failed: %s\n",path); return false; }
	studiohdr_t storage;
	const unsigned int length = filesystem->Size( file );
	const int read = filesystem->Read( &storage, sizeof(storage), file );
	filesystem->Close( file );
	const studiohdr_t *header = &storage;
	if ( read != sizeof(storage) || header->id != 0x54534449 || header->version < 44 || header->version > 49 || header->length < sizeof(studiohdr_t) || header->length > length || skin < 0 || skin >= header->numskinfamilies )
	{
		DevWarning("[inventory-model] invalid header: %s read=%d expected=%u id=%x version=%d length=%d/%u skin=%d/%d\n",path,read,(unsigned int)sizeof(storage),header->id,header->version,header->length,length,skin,header->numskinfamilies);
		return false;
	}
	char sibling[256]; Q_StripExtension( path, sibling, sizeof( sibling ) ); Q_strncat( sibling, ".vvd", sizeof( sibling ) );
	if ( !filesystem->FileExists( sibling, "GAME" ) ) { DevWarning("[inventory-model] missing companion: %s\n",sibling); return false; }
	Q_StripExtension( path, sibling, sizeof( sibling ) ); Q_strncat( sibling, ".dx90.vtx", sizeof( sibling ) );
	return filesystem->FileExists( sibling, "GAME" );
}

CInventoryManager::CInventoryManager() : CAutoGameSystem( "SourceAdvancedInventory" ), m_Loaded( false ) { ResetPrecache(); }
void CInventoryManager::ResetPrecache()
{
	memset( m_Precached, 0, sizeof(m_Precached) );
	memset( m_DefaultItems, 0, sizeof(m_DefaultItems) );
	m_ArmsPrecached = false;
	m_PrecacheCalls = m_PrecacheHits = m_PrecacheModels = 0;
	m_LoadSeconds = m_PrecacheSeconds = 0;
}
void CInventoryManager::Load()
{
	if ( m_Loaded ) return;
	const double started = Plat_FloatTime();
	m_Loaded = true;
	KeyValues *root = new KeyValues( "SkinsManifest" );
	if ( root->LoadFromFile( filesystem, "scripts/skins_manifest.txt", "MOD" ) )
	{
		const char *animationSounds = root->GetString("animation_sound_script");
		if (animationSounds[0] && !Q_strnicmp(animationSounds,"scripts/game_sounds_sourceadvanced_",34) && !Q_strstr(animationSounds,"..") && !Q_strstr(animationSounds,"\\") && !Q_strstr(animationSounds,":") && filesystem->FileExists(animationSounds,"MOD"))
			{
            soundemitterbase->AddSoundOverrides(animationSounds,true);
#ifndef CLIENT_DLL
            // Footsteps were initially precached by ClientPrecache before these
            // overrides loaded. Register every replacement wave before signon.
            for(int i=0;i<ARRAYSIZE(g_CSFeedbackSoundEvents);++i)
                CBaseEntity::PrecacheScriptSound(g_CSFeedbackSoundEvents[i]);
#endif
        }
		for ( KeyValues *key = root->GetFirstTrueSubKey(); key && m_Items.Count() < 512; key = key->GetNextTrueSubKey() )
		{
			if ( !Q_stricmp(key->GetString("type"),"gloves") )
			{
				CGloveItem glove = {}; glove.item_id=Q_atoi(key->GetName()); glove.skin=key->GetInt("skin",0);
				Q_strncpy(glove.display_name,key->GetString("name"),sizeof(glove.display_name));
				const char *fields[GLOVE_RIG_COUNT]={"native_model","legacy_model","cs2_model"};
				bool valid=glove.item_id>0 && glove.item_id<=65535 && glove.skin>=0 && glove.skin<=255 && !Find(glove.item_id) && !FindGlove(glove.item_id) && m_Gloves.Count()<64;
				for (int rig=0;rig<GLOVE_RIG_COUNT;++rig)
				{
					Q_strncpy(glove.models[rig],key->GetString(fields[rig]),sizeof(glove.models[rig]));
					valid=ValidModel(glove.models[rig],glove.skin) && valid;
				}
				if (valid) m_Gloves.AddToTail(glove);
				else Warning("Inventory: rejected invalid/missing gloves %s\n",key->GetName());
				continue;
			}
			CSkinItem item = {};
			item.item_id = Q_atoi( key->GetName() );
			item.weapon_id = AliasToWeaponID( GetTranslatedWeaponAlias( key->GetString( "weapon_class" ) + ( !Q_strnicmp( key->GetString( "weapon_class" ), "weapon_", 7 ) ? 7 : 0 ) ) );
			item.skin = key->GetInt( "skin", 0 );
			item.use_as_default = key->GetBool("use_as_default",false);
			Q_strncpy( item.display_name, key->GetString( "name" ), sizeof( item.display_name ) );
			Q_strncpy( item.category, key->GetString( "category", "Other" ), sizeof( item.category ) );
			Q_strncpy( item.view_model, key->GetString( "view_model" ), sizeof( item.view_model ) );
			Q_strncpy( item.world_model, key->GetString( "world_model" ), sizeof( item.world_model ) );
			Q_strncpy( item.sound_script, key->GetString( "sound_script" ), sizeof( item.sound_script ) );
			Q_strncpy( item.shoot_sound, key->GetString( "shoot_sound" ), sizeof( item.shoot_sound ) );
			Q_strncpy( item.silenced_sound, key->GetString( "silenced_sound" ), sizeof( item.silenced_sound ) );
			if ( item.item_id <= 0 || item.item_id > 65535 || item.weapon_id <= WEAPON_NONE || item.weapon_id >= WEAPON_KEVLAR || item.skin < 0 || item.skin > 255 || Find( item.item_id ) || FindGlove(item.item_id) || !ValidModel( item.view_model, item.skin ) || !ValidModel( item.world_model, item.skin ) )
			{
				Warning( "Inventory: rejected invalid/missing item %s\n", key->GetName() ); continue;
			}
			m_Items.AddToTail( item );
			if (item.use_as_default && !m_DefaultItems[item.weapon_id]) m_DefaultItems[item.weapon_id]=m_Items.Count();
			if ( item.sound_script[0] && !Q_strnicmp(item.sound_script,"scripts/game_sounds_sourceadvanced_",34) && !Q_strstr(item.sound_script,"..") && !Q_strstr(item.sound_script,"\\") && !Q_strstr(item.sound_script,":") && filesystem->FileExists(item.sound_script,"MOD") )
				soundemitterbase->AddSoundOverrides(item.sound_script,true);
		}
	}
	root->deleteThis();
	m_LoadSeconds = Plat_FloatTime() - started;
	DevMsg( "Inventory: %d validated items loaded\n", m_Items.Count() );
}
void CInventoryManager::LevelInitPreEntity() { m_Loaded = false; m_Items.RemoveAll(); m_Gloves.RemoveAll(); ResetPrecache(); Load(); }
void CInventoryManager::LevelShutdownPostEntity()
{
	m_Items.Purge(); m_Gloves.Purge(); memset(m_DefaultItems,0,sizeof(m_DefaultItems)); m_Loaded = false;
	// CHLClient and the model loader already own material cache teardown.
}
const CSkinItem *CInventoryManager::Find( int itemId ) const
{
	for ( int i = 0; i < m_Items.Count(); ++i ) if ( m_Items[i].item_id == itemId ) return &m_Items[i];
	return NULL;
}
const CSkinItem *CInventoryManager::FindForWeapon( int itemId, CSWeaponID id ) const
{
	if (!itemId && id>WEAPON_NONE && id<WEAPON_KEVLAR && m_DefaultItems[id]>0 && m_DefaultItems[id]<=m_Items.Count()) return &m_Items[m_DefaultItems[id]-1];
	const CSkinItem *item = Find( itemId ); return item && item->weapon_id == id ? item : NULL;
}
const CGloveItem *CInventoryManager::FindGlove( int itemId ) const
{
	for (int i=0;i<m_Gloves.Count();++i) if (m_Gloves[i].item_id==itemId) return &m_Gloves[i];
	return NULL;
}
void CInventoryManager::PrecacheForWeapon( CSWeaponID id )
{
	Load();
	++m_PrecacheCalls;
	if ( id <= WEAPON_NONE || id >= WEAPON_KEVLAR ) return;
	if ( m_Precached[id] ) { ++m_PrecacheHits; return; }
	m_Precached[id] = true;
	const double started = Plat_FloatTime();
	for ( int i = 0; i < m_Items.Count(); ++i ) if ( m_Items[i].weapon_id == id )
	{
		RegisterInventoryModel( m_Items[i].view_model ); RegisterInventoryModel( m_Items[i].world_model );
		m_PrecacheModels += 2;
	}
	if ( !m_ArmsPrecached && m_Items.Count() )
	{
		m_ArmsPrecached = true;
		for (int i=0;i<m_Gloves.Count();++i) for (int rig=0;rig<GLOVE_RIG_COUNT;++rig)
		{ RegisterInventoryModel(m_Gloves[i].models[rig]); ++m_PrecacheModels; }
		if ( ValidModel( "models/sourceadvanced/c_arms_default.mdl" ) )
		{ RegisterInventoryModel( "models/sourceadvanced/c_arms_default.mdl" ); ++m_PrecacheModels; }
		if ( ValidModel( "models/sourceadvanced/c_arms_native.mdl" ) )
		{ RegisterInventoryModel( "models/sourceadvanced/c_arms_native.mdl" ); ++m_PrecacheModels; }
	}
	m_PrecacheSeconds += Plat_FloatTime() - started;
}

void CInventoryManager::PrintProfile() const
{
	Msg("[inventory-profile] side=%s items=%d gloves=%d header_load_ms=%.2f precache_ms=%.2f calls=%d cached_calls=%d models=%d\n",
#ifdef CLIENT_DLL
		"client",
#else
		"server",
#endif
		m_Items.Count(),m_Gloves.Count(),m_LoadSeconds*1000,m_PrecacheSeconds*1000,m_PrecacheCalls,m_PrecacheHits,m_PrecacheModels);
}

#ifdef CLIENT_DLL
CON_COMMAND( cl_inventory_profile, "Report inventory loading and repeated precache savings." )
#else
CON_COMMAND( cs_inventory_profile, "Report inventory loading and repeated precache savings." )
#endif
{ CSInventory().PrintProfile(); }

#ifdef CLIENT_DLL
static ConVar cl_inventory_loadout( "cl_inventory_loadout", "", FCVAR_ARCHIVE | FCVAR_USERINFO, "Validated inventory choices; weaponID:itemID pairs." );
CON_COMMAND( inspectlook, "Inspect the held weapon; attacks and reload interrupt the animation." )
{ engine->ServerCmd( "inspectlook" ); }
#endif
int CInventoryManager::GetPlayerSelection( CBasePlayer *player, CSWeaponID id ) const
{
	if ( !player ) return 0;
#ifdef CLIENT_DLL
	const char *choices = cl_inventory_loadout.GetString();
#else
	const char *choices = engine->GetClientConVarValue( player->entindex(), "cl_inventory_loadout" );
#endif
	char bounded[1024]; Q_strncpy( bounded, choices ? choices : "", sizeof( bounded ) );
	char *context = NULL;
	for ( char *token = strtok_s( bounded, ";", &context ); token; token = strtok_s( NULL, ";", &context ) )
	{
		int weaponId, itemId;
		if ( sscanf( token, "%d:%d", &weaponId, &itemId ) == 2 && weaponId == id && FindForWeapon( itemId, id ) ) return itemId;
	}
	return 0;
}

#ifdef CLIENT_DLL
CON_COMMAND( inventory_equip, "Equip a validated inventory item instantly." )
{
	if ( args.ArgC() != 2 ) return;
	if (const CGloveItem *glove=CSInventory().FindGlove(Q_atoi(args[1])))
	{ cl_inventory_gloves.SetValue(glove->item_id); Msg("[glove-inventory] equipped=%d\n",glove->item_id); return; }
	const CSkinItem *item = CSInventory().Find( Q_atoi( args[1] ) );
	if ( !item ) { Warning( "Inventory item not available in this map.\n" ); return; }
	char updated[1024] = "";
	for ( int id = WEAPON_NONE+1; id < WEAPON_KEVLAR; ++id )
	{
		int selected = id == item->weapon_id ? item->item_id : CSInventory().GetPlayerSelection( C_CSPlayer::GetLocalCSPlayer(), (CSWeaponID)id );
		if ( selected ) { char pair[32]; Q_snprintf( pair, sizeof( pair ), "%d:%d;", id, selected ); Q_strncat( updated, pair, sizeof( updated ) ); }
	}
	cl_inventory_loadout.SetValue( updated );
	C_CSPlayer *player = C_CSPlayer::GetLocalCSPlayer();
	if ( player ) for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		CWeaponCSBase *weapon = dynamic_cast<CWeaponCSBase *>( player->GetWeapon( i ) );
		if ( weapon && weapon->GetWeaponID() == item->weapon_id ) weapon->SetInventoryItem( item->item_id );
	}
	char command[64]; Q_snprintf( command, sizeof( command ), "inv_equip %d", item->item_id ); engine->ServerCmd( command );
}
CON_COMMAND( inventory_gloves, "Select validated first-person gloves; inventory_gloves 0 restores defaults." )
{
	if (args.ArgC()!=2) return;
	const int id=Q_atoi(args[1]);
	if (id && !CSInventory().FindGlove(id)) { Warning("Glove item not available in this map.\n"); return; }
	cl_inventory_gloves.SetValue(id);
}
CON_COMMAND( inventory_unequip, "Restore the active weapon's default model." )
{
	C_CSPlayer *player = C_CSPlayer::GetLocalCSPlayer();
	CWeaponCSBase *weapon = player ? dynamic_cast<CWeaponCSBase *>( player->GetActiveWeapon() ) : NULL;
	if ( !weapon ) return;
	char updated[1024] = "";
	for ( int id = WEAPON_NONE+1; id < WEAPON_KEVLAR; ++id ) if ( id != weapon->GetWeaponID() )
	{
		int selected = CSInventory().GetPlayerSelection( player, (CSWeaponID)id );
		if ( selected ) { char pair[32]; Q_snprintf( pair, sizeof( pair ), "%d:%d;", id, selected ); Q_strncat( updated, pair, sizeof( updated ) ); }
	}
	cl_inventory_loadout.SetValue( updated ); weapon->SetInventoryItem( 0 ); engine->ServerCmd( "inv_equip 0" );
}
CON_COMMAND_F( cl_inventory_validate, "Log the active model, animation and bonemerged arms. No overlay.", FCVAR_CHEAT )
{
	C_CSPlayer *player=C_CSPlayer::GetLocalCSPlayer(); CBaseViewModel *vm=player ? player->GetViewModel(0) : NULL;
	CWeaponCSBase *weapon=player ? dynamic_cast<CWeaponCSBase *>(player->GetActiveWeapon()) : NULL;
	if (!vm || !vm->GetModel() || !weapon) { Msg("[inventory-audit] No active view model\n"); return; }
	vm->UpdateUnifiedArms(); CBaseAnimating *arms=vm->m_hUnifiedArms.Get(); int missing=0, invalid=0;
	if (arms)
	{
		CBaseAnimating::AutoAllowBoneAccess access(true,true);
		studiohdr_t *header=modelinfo->GetStudiomodel(arms->GetModel()); matrix3x4_t matrices[MAXSTUDIOBONES];
		if (!header || !arms->SetupBones(matrices,MAXSTUDIOBONES,BONE_USED_BY_ANYTHING,gpGlobals->curtime)) ++invalid;
		else for (int i=0;i<header->numbones;++i)
		{
			if (vm->LookupBone(header->pBone(i)->pszName())<0) ++missing;
			for (int row=0;row<3;++row) for (int col=0;col<4;++col) if (!IsFinite(matrices[i][row][col])) ++invalid;
		}
	}
	int embedded=0, hidden=0;
	for (int i=0;i<3;++i)
	{
		char group[32]; Q_snprintf(group,sizeof(group),"sa_embedded_arms_%d",i);
		const int index=vm->FindBodygroupByName(group);
		if (index>=0) { ++embedded; if (vm->GetBodygroup(index)==1) ++hidden; }
	}
	const CGloveItem *glove=CSSelectedGlove();
	Msg("[inventory-audit] item=%d model=%s sequence=%s arms=%s missing_bones=%d invalid_matrices=%d clip=%d/%d alive=%d buttons=%d attack=%.3f now=%.3f looking=%d glove=%d embedded=%d hidden=%d\n",
		weapon->m_iInventoryItem.Get(),modelinfo->GetModelName(vm->GetModel()),vm->GetSequenceName(vm->GetSequence()),
		arms ? modelinfo->GetModelName(arms->GetModel()) : "none",missing,invalid,weapon->Clip1(),weapon->GetMaxClip1(),player->IsAlive(),player->m_nButtons,weapon->m_flNextPrimaryAttack.Get(),gpGlobals->curtime,player->IsLookingAtWeapon(),glove ? glove->item_id : 0,embedded,hidden);
}
#endif
