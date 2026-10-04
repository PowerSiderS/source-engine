#ifndef CS_HITBOX_GEOMETRY_H
#define CS_HITBOX_GEOMETRY_H
#include "mathlib/vector.h"
#include <math.h>
#include <cmath>
#include <float.h>

// A bone-local capsule with an elliptical cross section, contained in the
// authored box. This adapts old CS:S rigs without changing the MDL format.
struct CSRoundedHitbox
{
	Vector center, radii;
	int axis;
	float halfSegment;

	bool Init( const Vector &mins, const Vector &maxs )
	{
		center = ( mins + maxs ) * 0.5f;
		radii = ( maxs - mins ) * 0.5f;
		axis = radii.y > radii.x ? 1 : 0;
		if ( radii.z > radii[axis] ) axis = 2;
		for ( int i = 0; i < 3; ++i )
		if ( !std::isfinite( center[i] ) || !std::isfinite( radii[i] ) || radii[i] <= 0.0001f ) return false;
		const float cap = fminf( radii[(axis+1)%3], radii[(axis+2)%3] );
		halfSegment = ( radii[axis] - cap ) / cap;
		radii[axis] = cap;
		return true;
	}
};

struct CSRoundedHit
{
	float fraction, exitFraction;
	bool startSolid;
	Vector normal;
};

inline float CSCapsuleDistanceSqr( const Vector &p, int axis, float halfSegment )
{
	Vector closest = p;
	closest[axis] -= fmaxf( -halfSegment, fminf( halfSegment, p[axis] ) );
	return closest.LengthSqr();
}

// Retain roots on the cylinder or the outward hemisphere, never the inward
// half of a cap. The ray parameter is unchanged by nonuniform scaling.
inline double CSHitboxDot( const Vector &a, const Vector &b )
{
	return double(a.x)*b.x + double(a.y)*b.y + double(a.z)*b.z;
}

inline void CSCapsuleRoots( double a, double b, double c, const Vector &start,
	const Vector &delta, int axis, float halfSegment, int cap, float &entry, float &exit )
{
	if ( a <= FLT_MIN ) return;
	const double discriminant = b*b - a*c;
	if ( discriminant < 0.0f ) return;
	const double root = sqrt( discriminant );
	// Stable quadratic roots avoid cancellation for long bullet rays.
	const double q = -b - ( b >= 0.0f ? root : -root );
	float roots[2];
	roots[0] = float(q / a);
	roots[1] = float(q != 0.0 ? c / q : -b / a);
	for ( int i = 0; i < 2; ++i )
	{
		const float t = roots[i];
		const float axial = start[axis] + delta[axis] * t;
		if ( cap == 0 && fabsf( axial ) > halfSegment + 0.00001f ) continue;
		if ( cap < 0 && axial > -halfSegment + 0.00001f ) continue;
		if ( cap > 0 && axial < halfSegment - 0.00001f ) continue;
		entry = fminf( entry, t );
		exit = fmaxf( exit, t );
	}
}

inline bool CSIntersectRoundedHitbox( const CSRoundedHitbox &shape,
	const Vector &rayStart, const Vector &rayDelta, CSRoundedHit &hit )
{
	Vector start, delta;
	for ( int i = 0; i < 3; ++i )
	{
		start[i] = ( rayStart[i] - shape.center[i] ) / shape.radii[i];
		delta[i] = rayDelta[i] / shape.radii[i];
	}
	hit.startSolid = CSCapsuleDistanceSqr( start, shape.axis, shape.halfSegment ) <= 1.0f;
	float entry = FLT_MAX, exit = -FLT_MAX;
	Vector radialStart = start, radialDelta = delta;
	radialStart[shape.axis] = radialDelta[shape.axis] = 0.0f;
	CSCapsuleRoots( CSHitboxDot(radialDelta,radialDelta), CSHitboxDot(radialStart,radialDelta),
		CSHitboxDot(radialStart,radialStart)-1.0, start, delta, shape.axis, shape.halfSegment, 0, entry, exit );
	for ( int cap = -1; cap <= 1; cap += 2 )
	{
		Vector p = start;
		p[shape.axis] -= cap * shape.halfSegment;
		CSCapsuleRoots( CSHitboxDot(delta,delta), CSHitboxDot(p,delta), CSHitboxDot(p,p)-1.0,
			start, delta, shape.axis, shape.halfSegment, cap, entry, exit );
	}
	if ( !hit.startSolid && ( entry < 0.0f || entry > 1.0f || exit < 0.0f ) ) return false;
	hit.fraction = hit.startSolid ? 0.0f : entry;
	hit.exitFraction = exit == -FLT_MAX ? 1.0f : exit;
	Vector normal = start + delta * hit.fraction;
	normal[shape.axis] -= fmaxf(-shape.halfSegment, fminf(shape.halfSegment,normal[shape.axis]));
	for ( int i = 0; i < 3; ++i ) normal[i] /= shape.radii[i];
	const float length = sqrtf( normal.LengthSqr() );
	hit.normal = length > 0.000001f ? normal / length : Vector(0,0,1);
	return true;
}

// Interpolate wrapped angles/pose parameters by the shortest arc.
inline float CSInterpolateLoop( float fraction, float from, float to, float loop )
{
	float delta = to - from;
	if ( loop > 0.0f )
	{
		delta = fmodf( delta, loop );
		if ( delta > loop*0.5f ) delta -= loop;
		if ( delta < -loop*0.5f ) delta += loop;
	}
	return from + fraction * delta;
}
#endif
