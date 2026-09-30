#ifndef CS_RECOIL_PATTERN_H
#define CS_RECOIL_PATTERN_H
#include "vstdlib/random.h"

struct CSRecoilOffset
{
	float fAngle, fMagnitude;
};

inline int CSRecoilTableIndex( int index, int count )
{
	return count > 0 ? ( index % count + count ) % count : 0;
}

// The same seeded generator is used in prediction, the server and tests.
// Each weapon supplies its own angle/magnitude and variance profile.
inline void CSGenerateRecoilPattern( IUniformRandomStream &random, int seed, bool fullAuto,
	float angle, float angleVariance, float magnitude, float magnitudeVariance,
	int suppressionShots, float suppressionFactor, float smoothing,
	CSRecoilOffset *offsets, int count )
{
	random.SetSeed(seed);
	float previousAngle = 0.0f, previousMagnitude = 0.0f;
	for ( int shot = 0; shot < count; ++shot )
	{
		const float nextAngle = angle + random.RandomFloat(-angleVariance,angleVariance);
		const float nextMagnitude = magnitude + random.RandomFloat(-magnitudeVariance,magnitudeVariance);
		if ( fullAuto && shot > 0 )
		{
			previousAngle += smoothing * (nextAngle - previousAngle);
			previousMagnitude += smoothing * (nextMagnitude - previousMagnitude);
		}
		else { previousAngle = nextAngle; previousMagnitude = nextMagnitude; }
		offsets[shot].fAngle = previousAngle;
		offsets[shot].fMagnitude = previousMagnitude;
		// Apply suppression to this output impulse, not to the smoothing state.
		// Feeding it back made even a constant profile weaken on shot two.
		if ( fullAuto && shot < suppressionShots )
			offsets[shot].fMagnitude *= suppressionFactor + (1.0f-suppressionFactor)*float(shot)/suppressionShots;
	}
}
#endif
