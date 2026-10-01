#ifndef CLIENT_LOAD_PROFILE_H
#define CLIENT_LOAD_PROFILE_H
#include "tier1/convar.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"

// Disabled timers do not read the clock or print in normal gameplay.
class CClientLoadTimer
{
public:
	CClientLoadTimer(const char *phase, const char *map) : m_Phase(phase), m_Map(map)
	{
		static ConVarRef enabled("cl_profile_map_load");
		m_Enabled=enabled.IsValid() && enabled.GetBool();
		m_Start=m_Enabled ? Plat_FloatTime() : 0;
		if(m_Enabled) ConMsg("[load-profile] event=begin phase=%s map=%s t=%.6f\n",m_Phase,m_Map,m_Start);
	}
	~CClientLoadTimer()
	{
		if(m_Enabled) ConMsg("[load-profile] event=end phase=%s map=%s ms=%.2f t=%.6f\n",
			m_Phase,m_Map,(Plat_FloatTime()-m_Start)*1000,Plat_FloatTime());
	}
private:
	const char *m_Phase,*m_Map;
	bool m_Enabled;
	double m_Start;
};
#endif
