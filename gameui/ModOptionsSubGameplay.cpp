//========= Copyright Valve Corporation, All rights reserved. ============//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "ModOptionsSubGameplay.h"

#include <vgui_controls/Label.h>
#include <vgui/IVGui.h>

#include "CvarToggleCheckButton.h"
#include "cvarslider.h"
#include "tier1/KeyValues.h"

// memdbgon must be the last include file in this file.
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Gameplay options property page.
//-----------------------------------------------------------------------------
CModOptionsSubGameplay::CModOptionsSubGameplay( vgui::Panel *parent )
	: vgui::PropertyPage( parent, "ModOptionsSubGameplay" )
{
	new Label( this, "ViewModelFOVLabel", "#GameUI_Main_ViewModelFOV" );
	new Label( this, "ViewModelRightLabel", "#GameUI_Main_ViewModelRightLeft" );
	new Label( this, "ViewModelUpLabel", "#GameUI_Main_ViewModelUpDown" );
	new Label( this, "RadarPanelScaleLabel", "#GameUI_Main_RadarPanelSize" );
	new Label( this, "RadarScaleLabel", "#GameUI_Main_RadarScale" );
	new Label( this, "RadarAlphaLabel", "#GameUI_Main_RadarOpacity" );

	m_pViewModelFOV = new CCvarSlider(
		this, "ViewModelFOVSlider", "#GameUI_Main_ViewModelFOV",
		54.0f, 120.0f, "e_viewmodel_fov" );
	m_pViewModelRight = new CCvarSlider(
		this, "ViewModelRightSlider", "#GameUI_Main_ViewModelRightLeft",
		-12.0f, 12.0f, "e_viewmodel_right" );
	m_pViewModelUp = new CCvarSlider(
		this, "ViewModelUpSlider", "#GameUI_Main_ViewModelUpDown",
		-12.0f, 12.0f, "e_viewmodel_up" );
	m_pRadarPanelScale = new CCvarSlider(
		this, "RadarPanelScaleSlider", "#GameUI_Main_RadarPanelSize",
		0.5f, 2.5f, "cl_radar_panel_scale" );
	m_pRadarScale = new CCvarSlider(
		this, "RadarScaleSlider", "#GameUI_Main_RadarScale",
		1.0f, 3.0f, "cl_radar_scale" );
	m_pRadarAlpha = new CCvarSlider(
		this, "RadarAlphaSlider", "#GameUI_Main_RadarOpacity",
		0.0f, 255.0f, "cl_radaralpha" );

	m_pRadarRotate = new CCvarToggleCheckButton(
		this, "RadarRotateCheckBox", "#GameUI_Main_RadarRotation",
		"cl_radar_rotate" );
	m_pRadarSquare = new CCvarToggleCheckButton(
		this, "RadarSquareCheckBox", "#GameUI_Main_RadarSquare",
		"cl_radar_square_with_scoreboard" );
	m_pDisplayC4Timer = new CCvarToggleCheckButton(
		this, "DisplayC4TimerCheckBox", "#GameUI_Main_DisplayC4Timer",
		"hud_display_c4_time" );
	m_pDisableTouchOnBuyMenu = new CCvarToggleCheckButton(
		this, "DisableTouchOnBuyMenuCheckBox", "#GameUI_Main_DisableTouchOnBuyMenu",
		"touch_disabled_on_buymenu" );
	m_pDrawTracers = new CCvarToggleCheckButton(
		this, "DrawTracersCheckBox", "#GameUI_Main_DrawTracers",
		"r_drawtracers" );
	m_pUseNewHeadbob = new CCvarToggleCheckButton(
		this, "UseNewHeadbobCheckBox", "#GameUI_Main_CSGOBobbing",
		"cl_use_new_headbob" );

	m_pViewModelFOV->AddActionSignalTarget( this );
	m_pViewModelRight->AddActionSignalTarget( this );
	m_pViewModelUp->AddActionSignalTarget( this );
	m_pRadarPanelScale->AddActionSignalTarget( this );
	m_pRadarScale->AddActionSignalTarget( this );
	m_pRadarAlpha->AddActionSignalTarget( this );
	m_pRadarRotate->AddActionSignalTarget( this );
	m_pDisplayC4Timer->AddActionSignalTarget( this );
	m_pDisableTouchOnBuyMenu->AddActionSignalTarget( this );
	m_pDrawTracers->AddActionSignalTarget( this );
	m_pUseNewHeadbob->AddActionSignalTarget( this );

	LoadControlSettings( "Resource/ModOptionsSubGameplay.res" );
}

//-----------------------------------------------------------------------------
CModOptionsSubGameplay::~CModOptionsSubGameplay()
{
}

//-----------------------------------------------------------------------------
void CModOptionsSubGameplay::OnControlModified()
{
	PostMessage( GetParent(), new KeyValues( "ApplyButtonEnable" ) );
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
void CModOptionsSubGameplay::OnTextChanged( vgui::Panel *panel )
{
	OnControlModified();
}

//-----------------------------------------------------------------------------
void CModOptionsSubGameplay::OnSliderMoved( KeyValues *data )
{
	OnControlModified();
}

//-----------------------------------------------------------------------------
void CModOptionsSubGameplay::OnCheckButtonChecked()
{
	OnControlModified();
}

//-----------------------------------------------------------------------------
void CModOptionsSubGameplay::OnResetData()
{
	m_pViewModelFOV->Reset();
	m_pViewModelRight->Reset();
	m_pViewModelUp->Reset();
	m_pRadarPanelScale->Reset();
	m_pRadarScale->Reset();
	m_pRadarAlpha->Reset();

	m_pRadarRotate->Reset();
	m_pRadarSquare->Reset();
	m_pDisplayC4Timer->Reset();
	m_pDisableTouchOnBuyMenu->Reset();
	m_pDrawTracers->Reset();
	m_pUseNewHeadbob->Reset();
}

//-----------------------------------------------------------------------------
void CModOptionsSubGameplay::OnApplyChanges()
{
	m_pViewModelFOV->ApplyChanges();
	m_pViewModelRight->ApplyChanges();
	m_pViewModelUp->ApplyChanges();
	m_pRadarPanelScale->ApplyChanges();
	m_pRadarScale->ApplyChanges();
	m_pRadarAlpha->ApplyChanges();

	m_pRadarRotate->ApplyChanges();
	m_pRadarSquare->ApplyChanges();
	m_pDisplayC4Timer->ApplyChanges();
	m_pDisableTouchOnBuyMenu->ApplyChanges();
	m_pDrawTracers->ApplyChanges();
	m_pUseNewHeadbob->ApplyChanges();
}