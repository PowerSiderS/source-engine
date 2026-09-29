//========= Copyright Valve Corporation, All rights reserved. ============//

#include "OptionsSubInventory.h"
#include "EngineInterface.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Panel.h>
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "tier1/KeyValues.h"

#include <tier0/memdbgon.h>

using namespace vgui;

COptionsSubInventory::COptionsSubInventory( Panel *parent )
	: PropertyPage( parent, "OptionsSubInventory" ), m_nSelectedKnife( 0 )
{
	m_pSidebar = new Panel( this, "InventorySidebar" );
	m_pSidebar->SetBgColor( Color( 31, 35, 39, 255 ) );
	m_pGallery = new Panel( this, "InventoryGallery" );
	m_pGallery->SetBgColor( Color( 39, 43, 47, 255 ) );

	Label *pInventoryTitle = new Label( m_pSidebar, "InventoryTitle", "INVENTARIO" );
	pInventoryTitle->SetBounds( 16, 14, 150, 24 );
	Label *pKnifeCategory = new Label( m_pSidebar, "KnifeCategory", "FACAS" );
	pKnifeCategory->SetBounds( 16, 52, 150, 28 );
	pKnifeCategory->SetFgColor( Color( 74, 163, 255, 255 ) );
	Label *pHint = new Label( m_pSidebar, "InventoryHint", "Escolha um item da\ngaleria. A troca e\naplicada na hora." );
	pHint->SetBounds( 16, 98, 150, 70 );
	pHint->SetFgColor( Color( 170, 177, 184, 255 ) );

	m_pSelectionTitle = new Label( m_pGallery, "SelectionTitle", "Default Knife" );
	m_pSelectionTitle->SetFgColor( Color( 238, 242, 245, 255 ) );
	m_pSelectionPath = new Label( m_pGallery, "SelectionPath", "models/weapons/v_knife_t.mdl" );
	m_pSelectionPath->SetFgColor( Color( 150, 158, 166, 255 ) );
	m_pLiveStatus = new Label( m_pGallery, "LiveStatus", "PRONTO PARA TROCA EM TEMPO REAL" );
	m_pLiveStatus->SetFgColor( Color( 95, 205, 130, 255 ) );

	for ( int i = 0; i < CS_KNIFE_MODEL_COUNT; ++i )
	{
		char szCommand[32];
		Q_snprintf( szCommand, sizeof( szCommand ), "knife_%d", i );
		m_pKnifeButtons[i] = new Button( m_pGallery, s_KnifeModels[i].name, s_KnifeModels[i].displayName, this, szCommand );
		m_pKnifeButtons[i]->SetDefaultColor( Color( 220, 224, 228, 255 ), Color( 49, 54, 59, 255 ) );
		m_pKnifeButtons[i]->SetArmedColor( Color( 255, 255, 255, 255 ), Color( 53, 128, 196, 255 ) );
	}

	SetBgColor( Color( 24, 27, 30, 255 ) );
}

void COptionsSubInventory::OnResetData()
{
	ConVarRef knifeChoice( "cl_knife_choice" );
	SelectKnife( knifeChoice.IsValid() ? CSClampKnifeChoice( knifeChoice.GetInt() ) : 0, false );
}

void COptionsSubInventory::OnApplyChanges()
{
	SelectKnife( m_nSelectedKnife, true );
}

void COptionsSubInventory::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "knife_", 6 ) )
	{
		SelectKnife( Q_atoi( command + 6 ), true );
		PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
		return;
	}

	BaseClass::OnCommand( command );
}

void COptionsSubInventory::SelectKnife( int nChoice, bool bApplyToGame )
{
	m_nSelectedKnife = CSClampKnifeChoice( nChoice );
	m_pSelectionTitle->SetText( s_KnifeModels[m_nSelectedKnife].displayName );
	m_pSelectionPath->SetText( s_KnifeModels[m_nSelectedKnife].v_model );

	if ( bApplyToGame && engine )
	{
		char szCommand[64];
		Q_snprintf( szCommand, sizeof( szCommand ), "knife_select %d", m_nSelectedKnife );
		engine->ExecuteClientCmd( szCommand );
	}

	RefreshCards();
}

void COptionsSubInventory::RefreshCards()
{
	for ( int i = 0; i < CS_KNIFE_MODEL_COUNT; ++i )
	{
		char szLabel[64];
		if ( i == m_nSelectedKnife )
			Q_snprintf( szLabel, sizeof( szLabel ), "[*] %s", s_KnifeModels[i].displayName );
		else
			Q_strncpy( szLabel, s_KnifeModels[i].displayName, sizeof( szLabel ) );
		m_pKnifeButtons[i]->SetText( szLabel );
		m_pKnifeButtons[i]->SetDefaultColor(
			i == m_nSelectedKnife ? Color( 255, 255, 255, 255 ) : Color( 220, 224, 228, 255 ),
			i == m_nSelectedKnife ? Color( 37, 111, 175, 255 ) : Color( 49, 54, 59, 255 ) );
	}
}

void COptionsSubInventory::PerformLayout()
{
	BaseClass::PerformLayout();
	int wide, tall;
	GetSize( wide, tall );
	m_pSidebar->SetBounds( 0, 0, 176, tall );
	m_pGallery->SetBounds( 184, 0, MAX( 1, wide - 184 ), tall );

	m_pSelectionTitle->SetBounds( 16, 12, wide - 220, 24 );
	m_pSelectionPath->SetBounds( 16, 36, wide - 220, 20 );
	m_pLiveStatus->SetBounds( 16, tall - 28, wide - 220, 20 );

	const int nColumns = 3;
	const int nGap = 8;
	const int nStartY = 68;
	const int nButtonHeight = 40;
	const int nGalleryWidth = MAX( 1, wide - 184 );
	const int nButtonWidth = MAX( 90, ( nGalleryWidth - 32 - ( nColumns - 1 ) * nGap ) / nColumns );
	for ( int i = 0; i < CS_KNIFE_MODEL_COUNT; ++i )
	{
		const int x = 16 + ( i % nColumns ) * ( nButtonWidth + nGap );
		const int y = nStartY + ( i / nColumns ) * ( nButtonHeight + nGap );
		m_pKnifeButtons[i]->SetBounds( x, y, nButtonWidth, nButtonHeight );
	}
}
