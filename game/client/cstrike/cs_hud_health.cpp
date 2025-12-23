//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Combined Health and Armor HUD (CS:GO style)
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>

using namespace vgui;

#include "hudelement.h"
#include "c_cs_player.h"

#include "convar.h"
#include "cs_hud_color.h"

ConVar cl_hud_healthammo_style( "cl_hud_healthammo_style", "0", FCVAR_ARCHIVE, "HUD style: 0 = progress bars, 1 = simple" );
ConVar cl_hud_background_alpha( "cl_hud_background_alpha", "0.5", FCVAR_ARCHIVE, "HUD background alpha" );


//-----------------------------------------------------------------------------
// Purpose: Overriding Paint method to allow for correct border rendering
//-----------------------------------------------------------------------------
class CHudHealthArmorProgress: public ContinuousProgressBar
{
        DECLARE_CLASS_SIMPLE( CHudHealthArmorProgress, ContinuousProgressBar );

public:
        CHudHealthArmorProgress( Panel *parent, const char *panelName );
        virtual void Paint();
};

CHudHealthArmorProgress::CHudHealthArmorProgress( Panel *parent, const char *panelName ): ContinuousProgressBar( parent, panelName )
{
}

void CHudHealthArmorProgress::Paint()
{
        int x = 1, y = 1;
        int wide, tall;
        GetSize(wide, tall);
        wide -= 2;
        tall -= 2;

        surface()->DrawSetColor( GetFgColor() );

        bool bUsePrev = _prevProgress >= 0.f;
        bool bGain = _progress > _prevProgress;

        switch( m_iProgressDirection )
        {
        case PROGRESS_EAST:
                if ( bUsePrev )
                {
                        if ( bGain )
                        {
                                surface()->DrawFilledRect( x, y, x + (int)( wide * _prevProgress ), y + tall );

                                surface()->DrawSetColor( m_colorGain );
                                surface()->DrawFilledRect( x + (int)( wide * _prevProgress ), y, x + (int)( wide * _progress ), y + tall );
                                break;
                        }
                        else
                        {
                                surface()->DrawSetColor( m_colorLoss );
                                surface()->DrawFilledRect( x + (int)( wide * _progress ), y, x + (int)( wide * _prevProgress ), y + tall );
                        }
                }
                surface()->DrawSetColor( GetFgColor() );
                surface()->DrawFilledRect( x, y, x + (int)( wide * _progress ), y + tall );
                break;

        case PROGRESS_WEST:
                if ( bUsePrev )
                {
                        if ( bGain )
                        {
                                surface()->DrawFilledRect( x + (int)( wide * ( 1.0f - _prevProgress ) ), y, x + wide, y + tall );

                                surface()->DrawSetColor( m_colorGain );
                                surface()->DrawFilledRect( x + (int)( wide * ( 1.0f - _progress ) ), y, x + (int)( wide * ( 1.0f - _prevProgress ) ), y + tall );
                                break;
                        }
                        else
                        {
                                surface()->DrawSetColor( m_colorLoss );
                                surface()->DrawFilledRect( x + (int)( wide * ( 1.0f - _prevProgress ) ), y, x + (int)( wide * ( 1.0f - _progress ) ), y + tall );
                        }
                }
                surface()->DrawSetColor( GetFgColor() );
                surface()->DrawFilledRect( x + (int)( wide * ( 1.0f - _progress ) ), y, x + wide, y + tall );
                break;

        case PROGRESS_NORTH:
                if ( bUsePrev )
                {
                        if ( bGain )
                        {
                                surface()->DrawFilledRect( x, y + (int)( tall * ( 1.0f - _prevProgress ) ), x + wide, y + tall );

                                surface()->DrawSetColor( m_colorGain );
                                surface()->DrawFilledRect( x, y + (int)( tall * ( 1.0f - _progress ) ), x + wide, y + (int)( tall * ( 1.0f - _prevProgress ) ) );
                                break;
                        }
                        else
                        {
                                surface()->DrawSetColor( m_colorLoss );
                                surface()->DrawFilledRect( x, y + (int)( tall * ( 1.0f - _prevProgress ) ), x + wide, y + (int)( tall * ( 1.0f - _progress ) ) );
                        }
                }
                surface()->DrawSetColor( GetFgColor() );
                surface()->DrawFilledRect( x, y + (int)( tall * ( 1.0f - _progress ) ), x + wide, y + tall );
                break;

        case PROGRESS_SOUTH:
                if ( bUsePrev )
                {
                        if ( bGain )
                        {
                                surface()->DrawFilledRect( x, y, x + wide, y + (int)( tall * ( 1.0f - _progress ) ) );

                                surface()->DrawSetColor( m_colorGain );
                                surface()->DrawFilledRect( x, y + (int)( tall * ( 1.0f - _progress ) ), x + wide, y + (int)( tall * ( 1.0f - _prevProgress ) ) );
                                break;
                        }
                        else
                        {
                                surface()->DrawSetColor( m_colorLoss );
                                surface()->DrawFilledRect( x, y + (int)( tall * ( 1.0f - _prevProgress ) ), x + wide, y + (int)( tall * ( 1.0f - _progress ) ) );
                        }
                }
                surface()->DrawSetColor( GetFgColor() );
                surface()->DrawFilledRect( x, y, x + wide, y + (int)( tall * _progress ) );
                break;
        }
}


//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudHealthArmor : public CHudElement, public EditablePanel
{
        DECLARE_CLASS_SIMPLE( CHudHealthArmor, EditablePanel );

public:
        CHudHealthArmor( const char *pElementName );
        virtual void Init( void );
        virtual void ApplySettings( KeyValues *inResourceData );
        virtual void ApplySchemeSettings( IScheme *pScheme );
        virtual void Reset( void );
        virtual void OnThink();
        virtual void Paint();

private:
        float   m_flBackgroundAlpha;

        int             m_iHealth;
        int             m_iArmor;

        CHudTexture     *m_pHealthIcon;
        CHudTexture     *m_pArmorIcon;
        CHudTexture     *m_pHelmetIcon;

        Label           *m_pHealthLabel;
        Label           *m_pArmorLabel;
        Label           *m_pSimpleArmorLabel;

        CHudHealthArmorProgress *m_pHealthProgress;
        CHudHealthArmorProgress *m_pArmorProgress;

        CPanelAnimationVarAliasType( int, simple_wide, "simple_wide", "75", "proportional_int" );
        CPanelAnimationVarAliasType( int, simple_tall, "simple_tall", "22", "proportional_int" );

        CPanelAnimationVarAliasType( int, health_icon_xpos, "health_icon_xpos", "4", "proportional_int" );
        CPanelAnimationVarAliasType( int, health_icon_ypos, "health_icon_ypos", "4", "proportional_int" );
        CPanelAnimationVarAliasType( int, armor_icon_xpos, "armor_icon_xpos", "4", "proportional_int" );
        CPanelAnimationVarAliasType( int, armor_icon_ypos, "armor_icon_ypos", "26", "proportional_int" );
        CPanelAnimationVarAliasType( int, icon_wide, "icon_wide", "16", "proportional_int" );
        CPanelAnimationVarAliasType( int, icon_tall, "icon_tall", "16", "proportional_int" );

        CPanelAnimationVarAliasType( int, simple_health_icon_xpos, "simple_health_icon_xpos", "4", "proportional_int" );
        CPanelAnimationVarAliasType( int, simple_health_icon_ypos, "simple_health_icon_ypos", "2", "proportional_int" );
        CPanelAnimationVarAliasType( int, simple_armor_icon_xpos, "simple_armor_icon_xpos", "55", "proportional_int" );
        CPanelAnimationVarAliasType( int, simple_armor_icon_ypos, "simple_armor_icon_ypos", "2", "proportional_int" );

        int m_iStyle;
        int m_iOriginalWide;
        int m_iOriginalTall;

        int m_nHealthIconX, m_nHealthIconY;
        int m_nArmorIconX, m_nArmorIconY;
        bool m_bShowHelmet;
};

DECLARE_HUDELEMENT( CHudHealthArmor );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealthArmor::CHudHealthArmor( const char *pElementName ) : CHudElement( pElementName ), EditablePanel(NULL, "HudHealthArmor")
{
        vgui::Panel *pParent = g_pClientMode->GetViewport();
        SetParent( pParent );

        SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

        m_iOriginalWide = 0;
        m_iOriginalTall = 0;

        m_pHealthIcon = NULL;
        m_pArmorIcon = NULL;
        m_pHelmetIcon = NULL;

        m_pHealthLabel = new Label( this, "HealthLabel", "" );
        m_pArmorLabel = new Label( this, "ArmorLabel", "" );
        m_pSimpleArmorLabel = new Label( this, "SimpleArmorLabel", "" );

        m_pHealthProgress = new CHudHealthArmorProgress( this, "HealthProgress" );
        m_pArmorProgress = new CHudHealthArmorProgress( this, "ArmorProgress" );

        m_nHealthIconX = 0;
        m_nHealthIconY = 0;
        m_nArmorIconX = 0;
        m_nArmorIconY = 0;
        m_bShowHelmet = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudHealthArmor::Init()
{
        m_flBackgroundAlpha     = 0.0f;
        m_iStyle                        = -1;

        m_iHealth                       = -1;
        m_iArmor                        = -1;
}

void CHudHealthArmor::ApplySettings( KeyValues *inResourceData )
{
        BaseClass::ApplySettings( inResourceData );

        GetSize( m_iOriginalWide, m_iOriginalTall );
}

void CHudHealthArmor::ApplySchemeSettings( IScheme *pScheme )
{
        BaseClass::ApplySchemeSettings( pScheme );

        if ( !m_pHealthIcon )
                m_pHealthIcon = gHUD.GetIcon( "health_icon" );
        if ( !m_pArmorIcon )
                m_pArmorIcon = gHUD.GetIcon( "shield_bright" );
        if ( !m_pHelmetIcon )
                m_pHelmetIcon = gHUD.GetIcon( "shield_kevlar_bright" );

        int panelWide = XRES( 200 );
        int panelTall = YRES( 50 );
        SetPos( XRES( 16 ), ScreenHeight() - panelTall - YRES( 16 ) );
        SetSize( panelWide, panelTall );
        m_iOriginalWide = panelWide;
        m_iOriginalTall = panelTall;

        SetPaintBackgroundEnabled( false );
        SetBgColor( Color( 0, 0, 0, 0 ) );

        m_pHealthLabel->SetPos( XRES( 24 ), YRES( 4 ) );
        m_pHealthLabel->SetSize( XRES( 40 ), YRES( 16 ) );
        m_pHealthLabel->SetFont( pScheme->GetFont( "HudNumbers", true ) );

        m_pArmorLabel->SetPos( XRES( 24 ), YRES( 26 ) );
        m_pArmorLabel->SetSize( XRES( 40 ), YRES( 16 ) );
        m_pArmorLabel->SetFont( pScheme->GetFont( "HudNumbers", true ) );

        m_pSimpleArmorLabel->SetPos( XRES( 75 ), YRES( 4 ) );
        m_pSimpleArmorLabel->SetSize( XRES( 40 ), YRES( 16 ) );
        m_pSimpleArmorLabel->SetFont( pScheme->GetFont( "HudNumbers", true ) );
        m_pSimpleArmorLabel->SetVisible( false );

        m_pHealthProgress->SetPos( XRES( 70 ), YRES( 6 ) );
        m_pHealthProgress->SetSize( XRES( 120 ), YRES( 14 ) );
        m_pHealthProgress->SetBgColor( Color( 40, 40, 40, 200 ) );

        m_pArmorProgress->SetPos( XRES( 70 ), YRES( 28 ) );
        m_pArmorProgress->SetSize( XRES( 120 ), YRES( 14 ) );
        m_pArmorProgress->SetBgColor( Color( 40, 40, 40, 200 ) );

        m_nHealthIconX = health_icon_xpos;
        m_nHealthIconY = health_icon_ypos;
        m_nArmorIconX = armor_icon_xpos;
        m_nArmorIconY = armor_icon_ypos;
}

//-----------------------------------------------------------------------------
// Purpose: reset health to normal color at round restart
//-----------------------------------------------------------------------------
void CHudHealthArmor::Reset()
{
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthRestored");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudHealthArmor::OnThink()
{
        if ( m_iStyle != cl_hud_healthammo_style.GetInt() )
        {
                m_iStyle = cl_hud_healthammo_style.GetInt();

                switch ( m_iStyle )
                {
                        case 0:
                                SetSize( m_iOriginalWide, m_iOriginalTall );

                                m_pHealthProgress->SetVisible( true );
                                m_pArmorProgress->SetVisible( true );

                                m_pArmorLabel->SetVisible( true );
                                m_pSimpleArmorLabel->SetVisible( false );

                                m_nHealthIconX = health_icon_xpos;
                                m_nHealthIconY = health_icon_ypos;
                                m_nArmorIconX = armor_icon_xpos;
                                m_nArmorIconY = armor_icon_ypos;
                                break;

                        case 1:
                                SetSize( simple_wide, simple_tall );

                                m_pHealthProgress->SetVisible( false );
                                m_pArmorProgress->SetVisible( false );

                                m_pArmorLabel->SetVisible( false );
                                m_pSimpleArmorLabel->SetVisible( true );

                                m_nHealthIconX = simple_health_icon_xpos;
                                m_nHealthIconY = simple_health_icon_ypos;
                                m_nArmorIconX = simple_armor_icon_xpos;
                                m_nArmorIconY = simple_armor_icon_ypos;
                                break;
                }
        }

        if ( m_flBackgroundAlpha != cl_hud_background_alpha.GetFloat() )
        {
                m_flBackgroundAlpha = cl_hud_background_alpha.GetFloat();
                Color oldColor = GetBgColor();
                Color newColor( oldColor.r(), oldColor.g(), oldColor.b(), (int)(cl_hud_background_alpha.GetFloat() * 255) );
                SetBgColor( newColor );
        }

        int realHealth = 0;
        int realArmor = 0;
        C_CSPlayer *local = C_CSPlayer::GetLocalCSPlayer();
        if ( !local )
                return;
        
        realHealth = MAX( local->GetHealth(), 0 );
        realArmor = MAX( local->ArmorValue(), 0 );

        m_bShowHelmet = local->HasHelmet();

        wchar_t unicode[8];
        if ( realHealth != m_iHealth )
        {
                if ( realHealth > m_iHealth && m_iHealth != -1 )
                {
                        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthRestored" );
                }
                else if ( realHealth <= 20 )
                {
                        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthLow" );
                }
                else if ( realHealth < m_iHealth )
                {
                        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthTookDamage" );
                }

                m_iHealth = realHealth;

                V_snwprintf( unicode, ARRAYSIZE( unicode ), L"%d", m_iHealth );
                m_pHealthLabel->SetText( unicode );
                m_pHealthProgress->SetProgress( clamp( m_iHealth / 100.0f, 0.0f, 1.0f ) );
        }

        if ( realArmor != m_iArmor )
        {
                m_iArmor = realArmor;

                V_snwprintf( unicode, ARRAYSIZE( unicode ), L"%d", m_iArmor );
                m_pArmorLabel->SetText( unicode );
                m_pSimpleArmorLabel->SetText( unicode );
                m_pArmorProgress->SetProgress( clamp( m_iArmor / 100.0f, 0.0f, 1.0f ) );
        }
}

void CHudHealthArmor::Paint()
{
        int wide, tall;
        GetSize( wide, tall );
        
        Color hudColor = GetHudColor();
        Color bgColor( 0, 0, 0, (int)(cl_hud_background_alpha.GetFloat() * 255) );

        // Draw rounded background rectangle
        surface()->DrawSetColor( bgColor );
        
        // Draw main rectangle with slight inset for border
        surface()->DrawFilledRect( 2, 2, wide - 2, tall - 2 );
        
        // Draw border with HUD color
        surface()->DrawSetColor( GetHudColorSecondary( 200 ) );
        surface()->DrawOutlinedRect( 1, 1, wide - 1, tall - 1 );

        // Update label colors (for rainbow to work in real-time)
        m_pHealthLabel->SetFgColor( hudColor );
        m_pArmorLabel->SetFgColor( hudColor );
        m_pSimpleArmorLabel->SetFgColor( hudColor );
        m_pHealthProgress->SetFgColor( hudColor );
        m_pArmorProgress->SetFgColor( hudColor );

        if ( m_pHealthIcon )
        {
                m_pHealthIcon->DrawSelf( m_nHealthIconX, m_nHealthIconY, icon_wide, icon_tall, hudColor );
        }

        if ( m_bShowHelmet && m_pHelmetIcon )
        {
                m_pHelmetIcon->DrawSelf( m_nArmorIconX, m_nArmorIconY, icon_wide, icon_tall, hudColor );
        }
        else if ( m_pArmorIcon )
        {
                m_pArmorIcon->DrawSelf( m_nArmorIconX, m_nArmorIconY, icon_wide, icon_tall, hudColor );
        }

        BaseClass::Paint();
}
