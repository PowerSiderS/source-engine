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
	char display_name[96];
	char category[32];
	char view_model[192];
	char world_model[192];
	char sound_script[128];
	char shoot_sound[96];
	char silenced_sound[96];
};

class CInventoryManager : public CAutoGameSystem
{
public:
	CInventoryManager();
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();
	const CSkinItem *Find( int itemId ) const;
	const CSkinItem *FindForWeapon( int itemId, CSWeaponID id ) const;
	void PrecacheForWeapon( CSWeaponID id );
	int GetPlayerSelection( CBasePlayer *player, CSWeaponID id ) const;
	int Count() const { return m_Items.Count(); }
private:
	void Load();
	CUtlVector<CSkinItem> m_Items;
	bool m_Loaded;
};
CInventoryManager &CSInventory();
#endif
