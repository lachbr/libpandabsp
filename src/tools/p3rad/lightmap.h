#ifndef LIGHTMAP_H
#define LIGHTMAP_H

#include "qrad.h"
#include "mathlib/ssemath.h"

#define MAX_SINGLEMAP ((MAX_LIGHTMAP_DIM+1)*(MAX_LIGHTMAP_DIM+1)) //#define	MAX_SINGLEMAP	(18*18*4) //--vluzacn

typedef enum
{
        WALLFLAG_NONE = 0,
        WALLFLAG_NUDGED = 0x1,
        WALLFLAG_BLOCKED = 0x2, // this only happens when the entire face and its surroundings are covered by solid or opaque entities
        WALLFLAG_SHADOWED = 0x4,
}
wallflag_t;

typedef struct
{
        vec_t*          light;
        vec_t           facedist;
        vec3_t          facenormal;
        int normal_count;
        vec3_t          normals[NUM_BUMP_VECTS + 1];
        bool			translucent_b;
        vec3_t			translucent_v;
        int				texref;

        int             numsurfpt;
        vec3_t          surfpt[MAX_SINGLEMAP];
        vec3_t*			surfpt_position; //[MAX_SINGLEMAP] // surfpt_position[] are valid positions for light tracing, while surfpt[] are positions for getting phong normal and doing patch interpolation
        int*			surfpt_surface; //[MAX_SINGLEMAP] // the face that owns this position
        bool			surfpt_lightoutside[MAX_SINGLEMAP];
        vec_t                   surfpt_bounds[MAX_SINGLEMAP][2][2];

        vec3_t          texorg;
        vec3_t          worldtotex[2];                         // s = (world - texorg) . worldtotex[0]
        vec3_t          textoworld[2];                         // world = texorg + s * textoworld[0]
        vec3_t			texnormal;

        vec_t           exactmins[2], exactmaxs[2];

        int             texmins[2], texsize[2];
        int             lightstyles[256];
        int             surfnum;
        dface_t*        face;
        int				lmcache_density; // shared by both s and t direction
        int				lmcache_offset; // shared by both s and t direction
        int				lmcache_side;
        bumpsample_t( *lmcache )[ALLSTYLES]; // lm: short for lightmap // don't forget to free!
        bumpnormal_t			*lmcache_normal; // record the phong normals
        int				*lmcache_wallflags; // wallflag_t
        int				lmcachewidth;
        int				lmcacheheight;

        bool bumped;
        bool isflat;
}
lightinfo_t;

typedef struct
{
        vec3_t normal;
        vec3_t          pos;
        vec_t mins[2];
        vec_t maxs[2];
        bumpsample_t light;
        int				surface; // this sample can grow into another face
        int s, t;
        float area;
        int coord[2];
        Winding *winding;
}
sample_t;

typedef struct
{
        int normal_count;
        int bumped;
        vec3_t normals[NUM_BUMP_VECTS + 1];
        int             numsamples;
        sample_t*       samples[MAXLIGHTMAPS];
        sample_t *sample;

        float world_area_per_luxel;
        int numluxels;
        vec3_t *luxel_points;
        bumpsample_t *light[MAXLIGHTMAPS];
	bumpsample_t *sunlight[MAXLIGHTMAPS];
}
facelight_t;

extern facelight_t facelight[MAX_MAP_FACES];
extern lightinfo_t *lightinfo;

typedef struct
{
        dface_t		*faces[2];
        LVector3	interface_normal;
        bool	        coplanar;
} edgeshare_t;

extern edgeshare_t	edgeshare[MAX_MAP_EDGES];

extern int vertexref[MAX_MAP_VERTS];
extern int *vertexface[MAX_MAP_VERTS];

struct faceneighbor_t
{
        int numneighbors; // neighboring faces that share vertices
        int *neighbor; // neighboring face list (max of 64)

        LVector3 *normal; // adjusted normal per vertex
        LVector3 facenormal; // face normal
};
extern faceneighbor_t faceneighbor[MAX_MAP_FACES];

struct SSE_SampleInfo_t
{
        int facenum;
        dface_t *face;
        facelight_t *facelight;
        int lightmap_width;
        int lightmap_height;
        int lightmap_size;
        int normal_count;
        int thread;
        texinfo_t *texinfo;
        int num_samples;
        int num_sample_groups;
        int clusters[4];
        FourVectors points;
        FourVectors point_normals[NUM_BUMP_VECTS + 1];
};

extern void GatherSampleLightSSE( SSE_sampleLightOutput_t &output, directlight_t *dl, int facenum,
                                  const FourVectors &pos, FourVectors *normals, int normal_count,
                                  int thread, int lightflags = 0, float epsilon = 0 );

extern void SaveVertexNormals();

extern int EdgeVertex( dface_t *f, int edge );

#endif // LIGHTMAP_H