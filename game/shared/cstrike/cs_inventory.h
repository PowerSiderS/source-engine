#ifndef CS_INVENTORY_H
#define CS_INVENTORY_H
#include "igamesystem.h"
#include "tier1/utlvector.h"
#include "cs_weapon_parse.h"

class CWeaponCSBase;
struct CSkinItem
{
	int item_id;
	CSWeaponID weapon_id;
	int skin;
	bool use_as_default;
	char display_name[96];
	char category[32];
	char view_model[192];
	char world_model[192];
	char sound_script[128];
	char shoot_sound[96];
	char silenced_sound[96];
};

enum CSGloveRig { GLOVE_RIG_NATIVE, GLOVE_RIG_LEGACY, GLOVE_RIG_CS2, GLOVE_RIG_COUNT };
struct CGloveItem
{
	int item_id, skin;
	char display_name[96];
	char models[GLOVE_RIG_COUNT][192];
};

class CInventoryManager : public CAutoGameSystem
{
public:
	CInventoryManager();
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();
	const CSkinItem *Find( int itemId ) const;
	const CSkinItem *FindForWeapon( int itemId, CSWeaponID id ) const;
	const CGloveItem *FindGlove( int itemId ) const;
	void PrecacheForWeapon( CSWeaponID id );
	int GetPlayerSelection( CBasePlayer *player, CSWeaponID id ) const;
	int Count() const { return m_Items.Count(); }
	void PrintProfile() const;
private:
	void Load();
	void ResetPrecache();
	CUtlVector<CSkinItem> m_Items;
	CUtlVector<CGloveItem> m_Gloves;
	int m_DefaultItems[WEAPON_KEVLAR];
	bool m_Loaded;
	bool m_Precached[WEAPON_KEVLAR];
	bool m_ArmsPrecached;
	int m_PrecacheCalls, m_PrecacheHits, m_PrecacheModels;
	double m_LoadSeconds, m_PrecacheSeconds;
};
CInventoryManager &CSInventory();
#ifdef CLIENT_DLL
const CGloveItem *CSSelectedGlove();
#endif
#endif
