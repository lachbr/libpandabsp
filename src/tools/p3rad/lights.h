#pragma once

#include "mathlib.h"
#include "bspfile.h"

typedef enum
{
        emit_surface,
        emit_point,
        emit_spotlight,
        emit_skylight,
        emit_skyambient,
}
emittype_t;

typedef struct directlight_s
{
        struct directlight_s* next;
        emittype_t      type;
        int style;

        LVector3          origin;
        LVector3          intensity;
        LVector3          normal;                                // for surfaces and spotlights

        float           stopdot;                               // for spotlights
        float           stopdot2;                              // for spotlights

        //=======================================================================================//

        int index;
        byte *pvs;
        int leaf;
        int facenum;

        float exponent;

        float start_fade_distance;
        float end_fade_distance;
        float cap_distance;

        float quadratic_atten;
        float linear_atten;
        float constant_atten;

        float radius;

        int flags;

} directlight_t;

class Lights
{
public:

        static int numdlights;
        static directlight_t *directlights[MAX_MAP_LEAFS];
        static directlight_t *activelights;
        static entity_t *face_texlights[MAX_MAP_FACES];
        static float sun_angular_extent;
        static directlight_t *skylight;
        static directlight_t *ambientlight;

        static void CreateDirectLights();
        static void DeleteDirectLights();

        static INLINE bool HasLightEnvironment();
        static INLINE bool HasSkyLight();
        static INLINE bool HasAmbientLight();

};
