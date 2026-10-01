#include "OptionsSubInventory.h"
#include "EngineInterface.h"
#include "filesystem.h"
#include "tier1/KeyValues.h"
#include "tier1/strtools.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ScrollBar.h>
#include <tier0/memdbgon.h>
using namespace vgui;

COptionsSubInventory::COptionsSubInventory( Panel *parent ) : PropertyPage( parent, "OptionsSubInventory" ), m_Selected( 0 )
{
	SetBgColor( Color( 18, 21, 25, 245 ) );
	SetPaintBackgroundEnabled(true);
	m_Grid = new Panel( this, "InventoryGrid" ); m_Grid->SetBgColor( Color( 25, 29, 34, 235 ) );
	m_Category = new ComboBox( this, "Category", 8, false ); m_Category->AddActionSignalTarget( this );
	m_Category->AddItem( "Todos", new KeyValues( "Category", "filter", "" ) );
	m_Scroll = new ScrollBar( this, "InventoryScroll", true ); m_Scroll->AddActionSignalTarget( this );
	m_Preview = new ImagePanel( this, "Preview" ); m_Preview->SetShouldScaleImage( true );
	m_Name = new Label( this, "SelectedName", "Selecione um item" );
	m_Name->SetFgColor( Color( 225, 232, 240, 255 ) );
	m_Status = new Label( this, "Status", "Equipar altera o visual durante a partida." );
	m_Status->SetFgColor( Color( 244, 128, 201, 255 ) );
	m_Equip = new Button( this, "Equip", "Equipar", this, "equip" ); m_Equip->SetEnabled( false );
	new Button( this, "Unequip", "Restaurar arma ativa", this, "unequip" );
	KeyValues *root = new KeyValues( "SkinsManifest" );
	if ( root->LoadFromFile( g_pFullFileSystem, "scripts/skins_manifest.txt", "MOD" ) )
	{
		for ( KeyValues *key = root->GetFirstTrueSubKey(); key && m_Cards.Count() < 512; key = key->GetNextTrueSubKey() )
		{
			Card card = {}; card.id = Q_atoi( key->GetName() );
			Q_strncpy( card.name, key->GetString( "name" ), sizeof( card.name ) );
			Q_strncpy( card.category, key->GetString( "category", "Other" ), sizeof( card.category ) );
			Q_strncpy( card.icon, key->GetString( "icon" ), sizeof( card.icon ) );
			if ( card.id <= 0 || card.id > 65535 ) continue;
			bool categoryExists = false;
			for ( int i = 0; i < m_Cards.Count(); ++i ) if ( !Q_stricmp( m_Cards[i].category, card.category ) ) categoryExists = true;
			if ( !categoryExists ) m_Category->AddItem( card.category, new KeyValues( "Category", "filter", card.category ) );
			char command[32]; Q_snprintf( command, sizeof( command ), "select_%d", card.id );
			card.button = new Button( m_Grid, command, card.name, this, command );
			card.button->SetContentAlignment( Label::a_south ); card.button->SetDefaultColor( Color( 235, 240, 245, 255 ), Color( 46, 53, 61, 255 ) );
			card.button->SetArmedColor( Color( 255, 255, 255, 255 ), Color( 105, 49, 84, 255 ) );
			card.image = new ImagePanel( card.button, "Icon" ); card.image->SetMouseInputEnabled( false );
			card.image->SetShouldScaleImage( true ); if ( card.icon[0] ) card.image->SetImage( card.icon );
			m_Cards.AddToTail( card );
		}
	}
	root->deleteThis(); m_Category->ActivateItem( 0 );
	for ( int i = 0; i < m_Category->GetItemCount(); ++i )
	{
		KeyValues *data = m_Category->GetItemUserData( i );
		if ( data && !Q_stricmp( data->GetString( "filter" ), "Facas" ) ) m_Category->ActivateItem( i );
	}
	for ( int i = 0; i < m_Cards.Count(); ++i ) if ( !Q_stricmp( m_Cards[i].name, "Karambit" ) )
	{
		m_Selected = m_Cards[i].id; m_Name->SetText( m_Cards[i].name ); m_Preview->SetImage( m_Cards[i].icon ); m_Equip->SetEnabled( true ); break;
	}
}

void COptionsSubInventory::OnCategoryChanged() { m_Scroll->SetValue( 0 ); InvalidateLayout(); }
void COptionsSubInventory::OnScroll( int position ) { InvalidateLayout(); }
void COptionsSubInventory::PerformLayout()
{
	BaseClass::PerformLayout(); int wide, tall; GetSize( wide, tall );
	const int sidebar = MIN( 280, MAX( 170, wide / 3 ) ), top = 48, gap = 8, cardHeight = 112;
	int gridWidth = MAX( 100, wide-sidebar-32 ), gridHeight = MAX( 112, tall-top-8 );
	m_Category->SetBounds( 8, 10, gridWidth, 28 ); m_Grid->SetBounds( 8, top, gridWidth, gridHeight );
	m_Scroll->SetBounds( 8+gridWidth, top, 16, gridHeight );
	int columns = MAX( 1, gridWidth / 155 ), cardWidth = (gridWidth-gap*(columns+1))/columns;
	KeyValues *selected = m_Category->GetActiveItemUserData(); const char *filter = selected ? selected->GetString( "filter" ) : "";
	int count = 0;
	for ( int i = 0; i < m_Cards.Count(); ++i ) if ( !filter[0] || !Q_stricmp( filter, m_Cards[i].category ) ) ++count;
	int rows = (count+columns-1)/columns, visibleRows = MAX( 1, gridHeight / (cardHeight+gap) );
	m_Scroll->SetRange( 0, MAX( visibleRows, rows ) ); m_Scroll->SetRangeWindow( visibleRows );
	int visibleIndex = 0, first = m_Scroll->GetValue();
	for ( int i = 0; i < m_Cards.Count(); ++i )
	{
		Card &card = m_Cards[i]; bool matches = !filter[0] || !Q_stricmp( filter, card.category );
		card.button->SetDefaultColor( Color( 235, 240, 245, 255 ), card.id == m_Selected ? Color( 89, 46, 74, 255 ) : Color( 36, 42, 48, 255 ) );
		if ( !matches ) { card.button->SetVisible( false ); continue; }
		int row = visibleIndex/columns-first, column = visibleIndex%columns; ++visibleIndex;
		card.button->SetVisible( row >= 0 && row < visibleRows );
		card.button->SetBounds( gap+column*(cardWidth+gap), gap+row*(cardHeight+gap), cardWidth, cardHeight );
		card.image->SetBounds( 4, 4, cardWidth-8, 78 );
	}
	int x = wide-sidebar+8;
	m_Name->SetBounds( x, 18, sidebar-16, 44 ); m_Preview->SetBounds( x, 70, sidebar-16, 154 );
	m_Status->SetBounds( x, 240, sidebar-16, 70 ); m_Status->SetWrap( true );
	m_Equip->SetBounds( x, tall-94, sidebar-16, 32 );
	FindChildByName( "Unequip" )->SetBounds( x, tall-52, sidebar-16, 32 );
}

void COptionsSubInventory::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "select_", 7 ) )
	{
		int id = Q_atoi( command+7 );
		for ( int i = 0; i < m_Cards.Count(); ++i ) if ( m_Cards[i].id == id )
		{
			m_Selected = id; m_Name->SetText( m_Cards[i].name ); m_Preview->SetImage( m_Cards[i].icon ); m_Equip->SetEnabled( true );
			Button *restore=static_cast<Button *>(FindChildByName("Unequip"));
			if (restore) restore->SetText(!Q_stricmp(m_Cards[i].category,"Gloves") ? "Restaurar luvas" : "Restaurar arma ativa");
			InvalidateLayout();
			break;
		}
		return;
	}
	if ( !Q_stricmp( command, "equip" ) && m_Selected )
	{
		if ( engine && engine->IsInGame() ) { char cmd[64]; Q_snprintf( cmd, sizeof( cmd ), "inventory_equip %d", m_Selected ); engine->ExecuteClientCmd( cmd ); m_Status->SetText( "Visual aplicado durante a partida." ); }
		else m_Status->SetText( "Entre em uma partida para equipar." );
		return;
	}
	if ( !Q_stricmp( command, "unequip" ) )
	{
		bool gloves=false;
		for (int i=0;i<m_Cards.Count();++i) if (m_Cards[i].id==m_Selected) gloves=!Q_stricmp(m_Cards[i].category,"Gloves");
		if (engine) engine->ExecuteClientCmd(gloves ? "inventory_gloves 0" : "inventory_unequip");
		return;
	}
	BaseClass::OnCommand( command );
}
