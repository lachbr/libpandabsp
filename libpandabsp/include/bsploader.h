#ifndef BSPLOADER_H
#define BSPLOADER_H

#include "config_bsp.h"

#include <filename.h>
#include <lvector3.h>
#include <pnmImage.h>
#include <nodePath.h>
#include <genericAsyncTask.h>
#include <graphicsStateGuardian.h>
#include <texture.h>

NotifyCategoryDeclNoExport(bspfile);

struct FaceLightmapData
{
	double mins[2], maxs[2];
	int texmins[2], texmaxs[2];
	int texsize[2];
	double midpolys[2], midtexs[2];
};

struct texinfo_s;
struct dvertex_s;
struct texref_s;
struct dface_s;
struct dedge_s;
typedef texinfo_s texinfo_t;
typedef dvertex_s dvertex_t;
typedef texref_s texref_t;
typedef dface_s dface_t;
typedef dedge_s dedge_t;

class EggVertex;
class EggVertexPool;
class EggPolygon;
class Geom;
class GeomNode;

class EXPCL_PANDABSP BSPLoader
{
PUBLISHED:
	BSPLoader();

        bool read( const Filename &file );

	void set_gamma( PN_stdfloat gamma, int overbright = 1 );
	void set_gsg( GraphicsStateGuardian *gsg );
        void set_camera( const NodePath &camera );
	void set_render( const NodePath &render );
	void set_want_visibility( bool flag );
	void set_physics_type( int type );

	int get_num_entities() const;
	string get_entity_value( int entnum, const char *key ) const;
	float get_entity_value_float( int entnum, const char *key ) const;
	int get_entity_value_int( int entnum, const char *key ) const;
	LVector3 get_entity_value_vector( int entnum, const char *key ) const;
	LColor get_entity_value_color( int entnum, const char *key, bool scale = true ) const;
	NodePath get_entity( int entnum ) const;
	NodePath get_model( int modelnum ) const;

	void cleanup();

        NodePath get_result() const;

PUBLISHED:
	enum PhysicsType
	{
		PT_none,
		PT_panda,
		PT_bullet,
		PT_ode,
		PT_physx,
	};

private:
        void make_faces();
	void group_models();
	void load_entities();

	LTexCoord get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert ) const;

        PT( Texture ) try_load_texref( texref_t *tref );
        

        PT( EggVertex ) make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                     dedge_t *edge, texinfo_t *texinfo,
                                     dface_t *face, int k, FaceLightmapData *ld, Texture *tex );

        PNMImage lightmap_image_from_face( dface_t *face, FaceLightmapData *ld );

        int find_leaf( const LPoint3 &pos );
        bool is_cluster_visible( int curr_cluster, int cluster ) const;

        void update();
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
	NodePath _result;
	NodePath _brushroot;
        NodePath _camera;
	NodePath _render;
	GraphicsStateGuardian *_gsg;
	bool _has_pvs_data;
	bool _want_visibility;
	int _physics_type;

        pmap<texref_t *, PT( Texture )> _texref_textures;
        vector<vector<uint8_t>> _leaf_pvs;
	pvector<GeomNode *> _face_geomnodes;
	pvector<NodePath> _entities;
	
        PT( GenericAsyncTask ) _update_task;
};

#endif // BSPLOADER_H