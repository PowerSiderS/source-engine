//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "predicted_viewmodel.h"

#ifdef CLIENT_DLL 
#include "prediction.h" 
#ifdef CSTRIKE_DLL
#include "c_cs_player.h" 
#endif 
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( predicted_viewmodel, CPredictedViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( PredictedViewModel, DT_PredictedViewModel )

BEGIN_NETWORK_TABLE( CPredictedViewModel, DT_PredictedViewModel )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
CPredictedViewModel::CPredictedViewModel() : m_LagAnglesHistory("CPredictedViewModel::m_LagAnglesHistory")
{
        m_vLagAngles.Init();
        m_LagAnglesHistory.Setup( &m_vLagAngles, 0 );
}
#else
CPredictedViewModel::CPredictedViewModel()
{
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPredictedViewModel::~CPredictedViewModel()
{
}

#ifdef CLIENT_DLL
ConVar cl_wpn_sway_interp( "cl_wpn_sway_interp", "0.1", FCVAR_CLIENTDLL );
ConVar cl_wpn_sway_scale( "cl_wpn_sway_scale", "1.0", FCVAR_CLIENTDLL|FCVAR_CHEAT );
#endif

//----------------------------------------------------------------------------- 
// Purpose:  Adds head bob for off hand models (simplified version for dual elites)
//----------------------------------------------------------------------------- 
void CPredictedViewModel::AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles ) 
{ 
#ifdef CSTRIKE_DLL
#ifdef CLIENT_DLL 
        // Only apply bobbing to off-hand viewmodel (index 1, e.g. dual elites left hand)
        if ( ViewModelIndex() != 1 ) 
                return;

        if ( !owner || !gpGlobals->frametime )
                return;

        // Simplified bobbing calculation for off-hand model
        float speed = owner->GetLocalVelocity().Length2D();
        speed = clamp( speed, -320.0f, 320.0f );

        float bob_offset = RemapVal( speed, 0.0f, 320.0f, 0.0f, 1.0f );
        
        m_BobState.m_flBobTime += (gpGlobals->curtime - m_BobState.m_flLastBobTime) * bob_offset;
        m_BobState.m_flLastBobTime = gpGlobals->curtime;

        float flBobCycle = 0.5f;
        float cycle = m_BobState.m_flBobTime - (int)(m_BobState.m_flBobTime / flBobCycle) * flBobCycle;
        cycle /= flBobCycle;

        if ( cycle < 0.5f )
                cycle = M_PI * cycle / 0.5f;
        else
                cycle = M_PI + M_PI * (cycle - 0.5f) / 0.5f;

        float flBobMultiplier = (owner->GetGroundEntity() == NULL) ? 0.00125f : 0.00625f;

        // Use fixed bob amounts for off-hand
        m_BobState.m_flVerticalBob = speed * (flBobMultiplier * 0.3f);
        m_BobState.m_flVerticalBob = m_BobState.m_flVerticalBob * 0.3f + m_BobState.m_flVerticalBob * 0.7f * sin( cycle );
        m_BobState.m_flVerticalBob = clamp( m_BobState.m_flVerticalBob, -7.0f, 4.0f );

        cycle = m_BobState.m_flBobTime - (int)(m_BobState.m_flBobTime / flBobCycle * 2) * flBobCycle * 2;
        cycle /= flBobCycle * 2;

        if ( cycle < 0.5f )
                cycle = M_PI * cycle / 0.5f;
        else
                cycle = M_PI + M_PI * (cycle - 0.5f) / 0.5f;

        m_BobState.m_flLateralBob = speed * (flBobMultiplier * 0.5f);
        m_BobState.m_flLateralBob = m_BobState.m_flLateralBob * 0.3f + m_BobState.m_flLateralBob * 0.7f * sin( cycle );
        m_BobState.m_flLateralBob = clamp( m_BobState.m_flLateralBob, -8.0f, 8.0f );

        // Apply the bob to origin and angles
        Vector forward, right;
        AngleVectors( eyeAngles, &forward, &right, NULL );

        VectorMA( eyePosition, m_BobState.m_flVerticalBob * 0.4f, forward, eyePosition );
        eyePosition[2] += m_BobState.m_flVerticalBob * 0.1f;

        eyeAngles[ROLL] += m_BobState.m_flVerticalBob * 0.5f;
        eyeAngles[PITCH] -= m_BobState.m_flVerticalBob * 0.4f;
        eyeAngles[YAW] -= m_BobState.m_flLateralBob * 0.3f;

        VectorMA( eyePosition, m_BobState.m_flLateralBob * 0.2f, right, eyePosition );
#endif 
#endif 
}

void CPredictedViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
        #ifdef CLIENT_DLL
                // Calculate our drift
                Vector  forward, right, up;
                AngleVectors( angles, &forward, &right, &up );
                
                // Add an entry to the history.
                m_vLagAngles = angles;
                m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat(), false );
                
                // Interpolate back 100ms.
                m_LagAnglesHistory.Interpolate( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat() );
                
                // Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
                Vector vLaggedForward;
                QAngle angleDiff = m_vLagAngles - angles;
                AngleVectors( -angleDiff, &vLaggedForward, 0, 0 );
                Vector vForwardDiff = Vector(1,0,0) - vLaggedForward;

                // Now offset the origin using that.
                vForwardDiff *= cl_wpn_sway_scale.GetFloat();
                origin += forward*vForwardDiff.x + right*-vForwardDiff.y + up*vForwardDiff.z;
        #endif
}
