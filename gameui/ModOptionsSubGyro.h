//========= Copyright Valve Corporation, All rights reserved. ============//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MODOPTIONSSUBGYRO_H
#define MODOPTIONSSUBGYRO_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Label.h>

class CCvarToggleCheckButton;
class CCvarSlider;

//-----------------------------------------------------------------------------
// Purpose: gyroscope options property page
//-----------------------------------------------------------------------------
class CModOptionsSubGyro : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CModOptionsSubGyro, vgui::PropertyPage );

public:
	CModOptionsSubGyro( vgui::Panel *parent );
	~CModOptionsSubGyro();

protected:
	virtual void OnResetData();
	virtual void OnApplyChanges();
	MESSAGE_FUNC( OnControlModified,    "ControlModified"    );
	MESSAGE_FUNC_PTR( OnTextChanged,    "TextChanged", panel );
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data  );
	MESSAGE_FUNC( OnCheckButtonChecked, "CheckButtonChecked" );

private:
	CCvarToggleCheckButton	*m_pGyroEnable;
	CCvarToggleCheckButton	*m_pGyroReversePitch;
	CCvarToggleCheckButton	*m_pGyroReverseYaw;

	CCvarSlider				*m_pGyroPitchSensitivity;
	CCvarSlider				*m_pGyroYawSensitivity;
	CCvarSlider				*m_pGyroScopedSensitivity;
	CCvarSlider				*m_pGyroDeadzone;
};

#endif // MODOPTIONSSUBGYRO_H
