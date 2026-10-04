#ifndef CS_CAPSULE_GEOMETRY_H
#define CS_CAPSULE_GEOMETRY_H
#include "mathlib/vector.h"
#include <math.h>
#include <cmath>
#include <float.h>

// Bone-local segment with one circular radius. Zero-length segments are spheres.
struct CSCapsuleHitbox
{
    Vector start, end;
    float radius;
    bool IsValid() const
    {
        for (int i=0;i<3;++i)
        if(!std::isfinite(start[i]) || !std::isfinite(end[i])) return false;
        return std::isfinite(radius) && radius>0.0f;
    }
};
// Compatibility adapter for existing MDLs: keep the original longitudinal
// extent and fit the radius inside both perpendicular extents. This is our
// geometry, not a claim to reproduce the original Valve 2015 coordinates.
inline bool CSCapsuleFromAuthoredBox(const Vector &mins,const Vector &maxs,CSCapsuleHitbox &shape)
{
    Vector extent=(maxs-mins)*.5f,center=(maxs+mins)*.5f;
    for(int i=0;i<3;++i) if(!std::isfinite(extent[i]) || !std::isfinite(center[i]) || extent[i]<=.0001f) return false;
    int axis=extent.y>extent.x ? 1 : 0;if(extent.z>extent[axis]) axis=2;
    shape.radius=fminf(extent[(axis+1)%3],extent[(axis+2)%3]);
    shape.start=shape.end=center;
    shape.start[axis]-=extent[axis]-shape.radius;
    shape.end[axis]+=extent[axis]-shape.radius;
    return shape.IsValid();
}
inline bool CSCapsuleBoundsOverlapRay(const CSCapsuleHitbox &shape,const Vector &start,const Vector &delta)
{
    double enter=0,exit=1;
    for(int i=0;i<3;++i) {
        double lo=fmin(shape.start[i],shape.end[i])-shape.radius;
        double hi=fmax(shape.start[i],shape.end[i])+shape.radius;
        if(delta[i]==0) {if(start[i]<lo || start[i]>hi) return false;continue;}
        double a=(lo-start[i])/delta[i],b=(hi-start[i])/delta[i];
        enter=fmax(enter,fmin(a,b));exit=fmin(exit,fmax(a,b));
        if(enter>exit) return false;
    }
    return true;
}
struct CSCapsuleIntersection
{
    float fraction, exitFraction;
    bool startSolid;
    Vector normal;
};
inline double CSCapsuleDot(const Vector &a,const Vector &b)
{
    return double(a.x)*b.x+double(a.y)*b.y+double(a.z)*b.z;
}
inline Vector CSCapsuleClosestPoint(const CSCapsuleHitbox &shape,const Vector &p)
{
    Vector segment=shape.end-shape.start;
    double length=CSCapsuleDot(segment,segment);
    double t=length>0 ? CSCapsuleDot(p-shape.start,segment)/length : 0;
    t=fmax(0.0,fmin(1.0,t));
    return shape.start+segment*float(t);
}
inline void CSCapsuleRetainRoots(double a,double b,double c,const Vector &origin,
    const Vector &delta,const Vector &axis,double length,int cap,double &entry,double &exit)
{
    if(a<=DBL_MIN) return;
    double determinant=b*b-a*c;
    // Permit only round-off-scale negative discriminants at exact tangencies.
    const double tolerance=16.0*DBL_EPSILON*(fabs(b*b)+fabs(a*c));
    if(determinant < -tolerance) return;
    determinant=fmax(0.0,determinant);
    double root=sqrt(determinant),q=-b-copysign(root,b);
    double roots[2]={q/a,q!=0 ? c/q : -b/a};
    for(int i=0;i<2;++i)
    {
        double projection=CSCapsuleDot(origin,axis)+CSCapsuleDot(delta,axis)*roots[i];
        if(cap==0 && (projection<0 || projection>length)) continue;
        if(cap<0 && projection>0) continue;
        if(cap>0 && projection<length) continue;
        entry=fmin(entry,roots[i]); exit=fmax(exit,roots[i]);
    }
}
inline bool CSIntersectCapsule(const CSCapsuleHitbox &shape,const Vector &rayStart,
    const Vector &rayDelta,CSCapsuleIntersection &hit)
{
    if(!shape.IsValid()) return false;
    for(int i=0;i<3;++i) if(!std::isfinite(rayStart[i]) || !std::isfinite(rayDelta[i])) return false;
    if(!CSCapsuleBoundsOverlapRay(shape,rayStart,rayDelta)) return false;
    Vector segment=shape.end-shape.start;
    double length=sqrt(CSCapsuleDot(segment,segment));
    Vector axis=length>0 ? segment/float(length) : Vector(0,0,1);
    Vector origin=rayStart-shape.start;
    double radiusSqr=double(shape.radius)*shape.radius;
    Vector nearest=CSCapsuleClosestPoint(shape,rayStart);
    hit.startSolid=CSCapsuleDot(rayStart-nearest,rayStart-nearest)<=radiusSqr;
    double entry=DBL_MAX,exit=-DBL_MAX;
    double da=CSCapsuleDot(rayDelta,axis),oa=CSCapsuleDot(origin,axis);
    if(length>0)
        CSCapsuleRetainRoots(CSCapsuleDot(rayDelta,rayDelta)-da*da,
            CSCapsuleDot(origin,rayDelta)-oa*da,
            CSCapsuleDot(origin,origin)-oa*oa-radiusSqr,
            origin,rayDelta,axis,length,0,entry,exit);
    for(int cap=-1;cap<=1;cap+=2)
    {
        Vector local=rayStart-(cap<0 ? shape.start : shape.end);
        CSCapsuleRetainRoots(CSCapsuleDot(rayDelta,rayDelta),CSCapsuleDot(local,rayDelta),
            CSCapsuleDot(local,local)-radiusSqr,origin,rayDelta,axis,length,cap,entry,exit);
    }
    if(!hit.startSolid && (entry<0 || entry>1 || exit<0)) return false;
    hit.fraction=hit.startSolid ? 0.0f : float(entry);
    hit.exitFraction=exit==-DBL_MAX ? 1.0f : float(exit);
    Vector p=rayStart+rayDelta*hit.fraction;
    Vector normal=p-CSCapsuleClosestPoint(shape,p);
    double magnitude=sqrt(CSCapsuleDot(normal,normal));
    hit.normal=magnitude>0 ? normal/float(magnitude) : Vector(0,0,1);
    return true;
}
#endif
