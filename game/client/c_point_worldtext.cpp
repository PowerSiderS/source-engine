// PC CS:S networking compatibility. World-text rendering is not implemented yet.
#include "cbase.h"
#include "tier0/memdbgon.h"

class C_PointWorldText : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PointWorldText, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	// This point entity has no model. Keep its network state without inventing
	// geometry or allowing an unimplemented class to break the entity stream.
	bool ShouldDraw() { return false; }
	char m_szText[1024];
	int m_colTextColor, m_nOrientation, m_nFont;
	float m_flTextSize, m_flTextSpacingX, m_flTextSpacingY;
	bool m_bRainbow;
};

IMPLEMENT_CLIENTCLASS_DT( C_PointWorldText, DT_PointWorldText, CPointWorldText )
	RecvPropString( RECVINFO( m_szText ) ),
	RecvPropInt( RECVINFO( m_colTextColor ) ),
	RecvPropFloat( RECVINFO( m_flTextSize ) ),
	RecvPropFloat( RECVINFO( m_flTextSpacingX ) ),
	RecvPropFloat( RECVINFO( m_flTextSpacingY ) ),
	RecvPropInt( RECVINFO( m_nOrientation ) ),
	RecvPropInt( RECVINFO( m_nFont ) ),
	RecvPropBool( RECVINFO( m_bRainbow ) ),
END_RECV_TABLE()
