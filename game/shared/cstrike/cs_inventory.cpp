#include "cbase.h"
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
#else
#include "cs_player.h"
#endif
#include "tier0/memdbgon.h"
extern ISoundEmitterSystemBase *soundemitterbase;

static CInventoryManager s_Inventory;
CInventoryManager &CSInventory() { return s_Inventory; }
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
	if ( file == FILESYSTEM_INVALID_HANDLE ) return false;
	studiohdr_t storage;
	const unsigned int length = filesystem->Size( file );
	const int read = filesystem->Read( &storage, sizeof(storage), file );
	filesystem->Close( file );
	const studiohdr_t *header = &storage;
	if ( read != sizeof(storage) || header->id != 0x54534449 || header->version < 44 || header->version > 49 || header->length < sizeof(studiohdr_t) || header->length > length || skin < 0 || skin >= header->numskinfamilies ) return false;
	char sibling[256]; Q_StripExtension( path, sibling, sizeof( sibling ) ); Q_strncat( sibling, ".vvd", sizeof( sibling ) );
	if ( !filesystem->FileExists( sibling, "GAME" ) ) return false;
	Q_StripExtension( path, sibling, sizeof( sibling ) ); Q_strncat( sibling, ".dx90.vtx", sizeof( sibling ) );
	return filesystem->FileExists( sibling, "GAME" );
}

CInventoryManager::CInventoryManager() : CAutoGameSystem( "SourceAdvancedInventory" ), m_Loaded( false ) { ResetPrecache(); }
void CInventoryManager::ResetPrecache()
{
	memset( m_Precached, 0, sizeof(m_Precached) );
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
		for ( KeyValues *key = root->GetFirstTrueSubKey(); key && m_Items.Count() < 512; key = key->GetNextTrueSubKey() )
		{
			CSkinItem item = {};
			item.item_id = Q_atoi( key->GetName() );
			item.weapon_id = AliasToWeaponID( GetTranslatedWeaponAlias( key->GetString( "weapon_class" ) + ( !Q_strnicmp( key->GetString( "weapon_class" ), "weapon_", 7 ) ? 7 : 0 ) ) );
			item.skin = key->GetInt( "skin", 0 );
			Q_strncpy( item.display_name, key->GetString( "name" ), sizeof( item.display_name ) );
			Q_strncpy( item.category, key->GetString( "category", "Other" ), sizeof( item.category ) );
			Q_strncpy( item.view_model, key->GetString( "view_model" ), sizeof( item.view_model ) );
			Q_strncpy( item.world_model, key->GetString( "world_model" ), sizeof( item.world_model ) );
			Q_strncpy( item.sound_script, key->GetString( "sound_script" ), sizeof( item.sound_script ) );
			Q_strncpy( item.shoot_sound, key->GetString( "shoot_sound" ), sizeof( item.shoot_sound ) );
			Q_strncpy( item.silenced_sound, key->GetString( "silenced_sound" ), sizeof( item.silenced_sound ) );
			if ( item.item_id <= 0 || item.item_id > 65535 || item.weapon_id <= WEAPON_NONE || item.weapon_id >= WEAPON_KEVLAR || item.skin < 0 || item.skin > 255 || Find( item.item_id ) || !ValidModel( item.view_model, item.skin ) || !ValidModel( item.world_model, item.skin ) )
			{
				Warning( "Inventory: rejected invalid/missing item %s\n", key->GetName() ); continue;
			}
			m_Items.AddToTail( item );
			if ( item.sound_script[0] && !Q_strnicmp(item.sound_script,"scripts/game_sounds_sourceadvanced_",34) && !Q_strstr(item.sound_script,"..") && !Q_strstr(item.sound_script,"\\") && !Q_strstr(item.sound_script,":") && filesystem->FileExists(item.sound_script,"MOD") )
				soundemitterbase->AddSoundOverrides(item.sound_script,true);
		}
	}
	root->deleteThis();
	m_LoadSeconds = Plat_FloatTime() - started;
	DevMsg( "Inventory: %d validated items loaded\n", m_Items.Count() );
}
void CInventoryManager::LevelInitPreEntity() { m_Loaded = false; m_Items.RemoveAll(); ResetPrecache(); Load(); }
void CInventoryManager::LevelShutdownPostEntity()
{
	m_Items.Purge(); m_Loaded = false;
	// CHLClient and the model loader already own material cache teardown.
}
const CSkinItem *CInventoryManager::Find( int itemId ) const
{
	for ( int i = 0; i < m_Items.Count(); ++i ) if ( m_Items[i].item_id == itemId ) return &m_Items[i];
	return NULL;
}
const CSkinItem *CInventoryManager::FindForWeapon( int itemId, CSWeaponID id ) const
{
	const CSkinItem *item = Find( itemId ); return item && item->weapon_id == id ? item : NULL;
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
		if ( ValidModel( "models/sourceadvanced/c_arms_default.mdl" ) )
		{ RegisterInventoryModel( "models/sourceadvanced/c_arms_default.mdl" ); ++m_PrecacheModels; }
		if ( ValidModel( "models/sourceadvanced/c_arms_native.mdl" ) )
		{ RegisterInventoryModel( "models/sourceadvanced/c_arms_native.mdl" ); ++m_PrecacheModels; }
	}
	m_PrecacheSeconds += Plat_FloatTime() - started;
}

void CInventoryManager::PrintProfile() const
{
	Msg("[inventory-profile] side=%s items=%d header_load_ms=%.2f precache_ms=%.2f calls=%d cached_calls=%d models=%d\n",
#ifdef CLIENT_DLL
		"client",
#else
		"server",
#endif
		m_Items.Count(),m_LoadSeconds*1000,m_PrecacheSeconds*1000,m_PrecacheCalls,m_PrecacheHits,m_PrecacheModels);
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
	Msg("[inventory-audit] item=%d model=%s sequence=%s arms=%s missing_bones=%d invalid_matrices=%d clip=%d/%d alive=%d buttons=%d attack=%.3f now=%.3f looking=%d\n",
		weapon->m_iInventoryItem.Get(),modelinfo->GetModelName(vm->GetModel()),vm->GetSequenceName(vm->GetSequence()),
		arms ? modelinfo->GetModelName(arms->GetModel()) : "none",missing,invalid,weapon->Clip1(),weapon->GetMaxClip1(),player->IsAlive(),player->m_nButtons,weapon->m_flNextPrimaryAttack.Get(),gpGlobals->curtime,player->IsLookingAtWeapon());
}
#endif
