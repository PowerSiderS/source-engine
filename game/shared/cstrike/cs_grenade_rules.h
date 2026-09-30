#ifndef CS_GRENADE_RULES_H
#define CS_GRENADE_RULES_H
#include <math.h>
inline float CSGrenadeStrength(int mode) {return mode==1 ? 0.0f : mode==2 ? .5f : 1.0f;}
inline float CSGrenadeThrowSpeed(float strength) {return 750.0f*.9f*(.3f+.7f*strength);}
inline float CSGrenadePitch(float pitch) {
    pitch=fmodf(pitch+180,360);if(pitch<0)pitch+=360;pitch-=180;
    pitch=fmaxf(-90,fminf(90,pitch));
    return pitch-(90-fabsf(pitch))*(10.0f/90.0f);
}
inline float CSGrenadeSourceHeight(float strength) {return strength*12.0f-12.0f;}
static const float CS_GRENADE_GRAVITY_SCALE=.4f;
static const float CS_GRENADE_PLAYER_VELOCITY_SCALE=1.25f;
static const float CS_GRENADE_REST_SPEED=20.0f;
#endif
