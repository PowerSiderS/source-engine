#ifndef CS_BALLISTICS_H
#define CS_BALLISTICS_H
#include <math.h>

// Material and thickness based loss, shared by predicted and authoritative traces.
// Surface codes are Source's physics material codes, independent of cosmetic VMTs.
inline float CSMaterialPenetration(int material)
{
	switch(material)
	{
	case 'Y': case 'G': return 3.0f; // glass and grate
	case 'W': case 'U': return 1.5f; // wood and cardboard
	case 'L': return 1.0f;          // plastic
	case 'M': return 0.5f;          // metal
	case 'C': case 'S': return 0.4f;// concrete and stone
	default: return 1.0f;
	}
}
inline float CSPenetrationDamageLoss(float damage,float thickness,float power,int entry,int exit,bool grate)
{
	if(!isfinite(damage) || !isfinite(thickness) || !isfinite(power) || damage<=0 || thickness<0 || power<=0) return damage>0 ? damage : 0;
	float modifier=(CSMaterialPenetration(entry)+CSMaterialPenetration(exit))*0.5f;
	float fraction=0.16f;
	if(grate || entry=='G' || entry=='Y') { modifier=3.0f; fraction=0.05f; }
	else if(entry==exit && (entry=='W' || entry=='U')) modifier=3.0f;
	else if(entry==exit && entry=='L') modifier=2.0f;
	const float loss=damage*fraction + 11.25f/(power*modifier) + thickness*thickness/(24.0f*modifier);
	return fminf(damage,fmaxf(0,loss));
}
#endif
