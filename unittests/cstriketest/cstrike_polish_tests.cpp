#include "tier0/dbg.h"
#include "unitlib/unitlib.h"
#include "cstrike/cs_ballistics.h"
#include "cstrike/cs_legacy_gameplay.h"
#include "cstrike/cs_anticheat_rules.h"
#include <limits>
DEFINE_TESTSUITE(CStrikePolishTestSuite)

DEFINE_TESTCASE(MaterialThicknessAndPowerTest,CStrikePolishTestSuite)
{
	// Independently derived concrete: 36*.16 + 11.25/(2*.4) + 16*16/(24*.4).
	Shipping_Assert(fabsf(CSPenetrationDamageLoss(36,16,2,'C','C',false)-36)<.0001f);
	const float glass=CSPenetrationDamageLoss(36,4,2,'G','G',false);
	Shipping_Assert(fabsf(glass-(1.8f+1.875f+16.0f/72))<.0001f);
	for(int i=1;i<=64;++i)
	{
		float thin=CSPenetrationDamageLoss(115,float(i),2,'W','W',false);
		float thick=CSPenetrationDamageLoss(115,float(i+1),2,'W','W',false);
		Shipping_Assert(thick>=thin && thick<=115 && thin>=0);
		Shipping_Assert(CSPenetrationDamageLoss(115,float(i),3,'M','M',false)<=CSPenetrationDamageLoss(115,float(i),1,'M','M',false));
	}
	Shipping_Assert(CSPenetrationDamageLoss(36,4,0,'W','W',false)==36);
	Shipping_Assert(CSPenetrationDamageLoss(36,std::numeric_limits<float>::quiet_NaN(),2,'W','W',false)==36);
}
DEFINE_TESTCASE(AccuracyRecoveryTickrateTest,CStrikePolishTestSuite)
{
	// A recovery interval removes exactly 90% of excess penalty.
	Shipping_Assert(fabsf(CSAccuracyDecay(.21f,.01f,.4f,.4f)-.03f)<.00001f);
	float a=.21f,b=.21f;
	for(int i=0;i<64;++i)a=CSAccuracyDecay(a,.01f,.4f,1.0f/64);
	for(int i=0;i<128;++i)b=CSAccuracyDecay(b,.01f,.4f,1.0f/128);
	Shipping_Assert(fabsf(a-b)<.000001f);
	Shipping_Assert(CSAccuracyDecay(.2f,.01f,0,0)==.01f);
	Shipping_Assert(CSAccuracyDecay(.2f,.01f,.4f,0)==.2f);
	Shipping_Assert(CSAccuracyDecay(std::numeric_limits<float>::quiet_NaN(),.01f,.4f,.01f)==.01f);
}
DEFINE_TESTCASE(CommandBudgetLowFpsAndReconnectTest,CStrikePolishTestSuite)
{
	for(int tickrate=64;tickrate<=128;tickrate*=2)
	{
		CSCommandRateBudget budget;
		for(int frame=0;frame<1000;++frame)
			for(int queued=0;queued<tickrate/16;++queued) Shipping_Assert(budget.Consume(frame/16.0,1.0/tickrate));
		Shipping_Assert(budget.Consume(0,1.0/tickrate)); // map/reconnect clock resets
	}
}
