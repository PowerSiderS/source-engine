#ifndef CS_CAPSULE_MODEL_H
#define CS_CAPSULE_MODEL_H
#include "studio.h"
#include "cs_capsule_geometry.h"
#include <string.h>
// Optional extension carried in the eight reserved MDL hitbox words. Keeping
// authored box bounds intact preserves broad-phase and old-client compatibility.
static const int CS_CAPSULE_MDL_MARKER=0x53434150;
inline bool CSReadModelCapsule(const mstudiobbox_t &box,CSCapsuleHitbox &shape)
{
    if(box.unused[0]!=CS_CAPSULE_MDL_MARKER)return CSCapsuleFromAuthoredBox(box.bbmin,box.bbmax,shape);
    memcpy(&shape.start,&box.unused[1],sizeof(float)*3);
    memcpy(&shape.end,&box.unused[4],sizeof(float)*3);
    memcpy(&shape.radius,&box.unused[7],sizeof(float));
    if(!shape.IsValid())return false;
    for(int i=0;i<3;++i)
        if(fminf(shape.start[i],shape.end[i])-shape.radius<box.bbmin[i]-.01f ||
           fmaxf(shape.start[i],shape.end[i])+shape.radius>box.bbmax[i]+.01f)return false;
    return true;
}
#endif
