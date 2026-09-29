//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef OPTIONSSUBINVENTORY_H
#define OPTIONSSUBINVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include "cstrike/cs_knife_models.h"

namespace vgui
{
	class Button;
	class Label;
	class Panel;
}

class COptionsSubInventory : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( COptionsSubInventory, vgui::PropertyPage );

public:
	COptionsSubInventory( vgui::Panel *parent );

protected:
	virtual void OnResetData();
	virtual void OnApplyChanges();
	virtual void OnCommand( const char *command );
	virtual void PerformLayout();

private:
	void SelectKnife( int nChoice, bool bApplyToGame );
	void RefreshCards();

	vgui::Panel *m_pSidebar;
	vgui::Panel *m_pGallery;
	vgui::Label *m_pSelectionTitle;
	vgui::Label *m_pSelectionPath;
	vgui::Label *m_pLiveStatus;
	vgui::Button *m_pKnifeButtons[ CS_KNIFE_MODEL_COUNT ];
	int m_nSelectedKnife;
};

#endif // OPTIONSSUBINVENTORY_H
