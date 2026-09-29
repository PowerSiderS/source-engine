//========= Copyright Valve Corporation, All rights reserved. ============//

#include "tier0/dbg.h"
#include "unitlib/unitlib.h"
#include "appframework/IAppSystem.h"

class CCStrikeTestAppSystem : public CTier0AppSystem< IAppSystem >
{
	typedef CTier0AppSystem< IAppSystem > BaseClass;

public:
	virtual bool Connect( CreateInterfaceFn factory )
	{
		return BaseClass::Connect( factory );
	}

	virtual InitReturnVal_t Init()
	{
		return INIT_OK;
	}

	virtual void Shutdown()
	{
		BaseClass::Shutdown();
	}
};

USE_UNITTEST_APPSYSTEM( CCStrikeTestAppSystem )
