/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsploader.h
 * @author Brian Lach
 * @date March 27, 2018
 */

#ifndef BSPLOADER_H
#define BSPLOADER_H

#include "config_bsp.h"
#include "bsp_material.h"

#include <filename.h>
#include <lvector3.h>
#include <pnmImage.h>
#include <nodePath.h>
#include <genericAsyncTask.h>
#include <geom.h>
#include <graphicsStateGuardian.h>
#include <texture.h>
#ifdef HAVE_PYTHON
#include <py_panda.h>
#endif
#include "entity.h"
#include <renderAttrib.h>
#include <boundingBox.h>
#include <lightReMutex.h>
#include <graphicsWindow.h>

#include "lightmap_palettes.h"
#include "ambient_probes.h"
#include "vifparser.h"
#include "cubemaps.h"
#include "decals.h"

NotifyCategoryDeclNoExport(bspfile);

struct FaceLightmapData
{
	double mins[2], maxs[2];
	int texmins[2], texmaxs[2];
	int texsize[2];
	double midpolys[2], midtexs[2];
        LightmapPaletteDirectory::LightmapFacePaletteEntry *faceentry;

        FaceLightmapData() :
                faceentry( nullptr )
        {
        }
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
struct bspdata_t;

class EggVertex;
class EggVertexPool;
class EggPolygon;
class Geom;
class GeomNode;
class BSPLoader;
class PSSMShaderGenerator;

/**
 * An attribute applied to each face Geom from a BSP file.
 * All it does right now is indicate the material of the face
 * and if it's a wall or a floor (depending on the face normal).
 */
class EXPCL_PANDABSP BSPFaceAttrib : public RenderAttrib
{
private:
	INLINE BSPFaceAttrib() :
                RenderAttrib(),
                _ignore_pvs( false )
        {
        }

PUBLISHED:
        enum
        {
                FACETYPE_WALL,
                FACETYPE_FLOOR,
        };

	static CPT( RenderAttrib ) make( const string &face_material, int face_type );
	static CPT( RenderAttrib ) make_default();
        static CPT( RenderAttrib ) make_ignore_pvs();

	INLINE string get_material() const
        {
                return _material;
        }
        INLINE int get_face_type() const
        {
                return _face_type;
        }
        INLINE bool get_ignore_pvs() const
        {
                return _ignore_pvs;
        }

public:
	virtual bool has_cull_callback() const;

	virtual size_t get_hash_impl() const;
	virtual int compare_to_impl( const RenderAttrib *other ) const;

private:
	string _material;
        int _face_type;
        bool _ignore_pvs;

PUBLISHED:
	static int get_class_slot()
	{
		return _attrib_slot;
	}
	virtual int get_slot() const
	{
		return get_class_slot();
	}
	MAKE_PROPERTY( class_slot, get_class_slot );

public:
	static TypeHandle get_class_type()
	{
		return _type_handle;
	}
	static void init_type()
	{
		RenderAttrib::init_type();
		register_type( _type_handle, "BSPFaceAttrib",
			       RenderAttrib::get_class_type() );
		_attrib_slot = register_slot( _type_handle, 100, new BSPFaceAttrib );
	}
	virtual TypeHandle get_type() const
	{
		return get_class_type();
	}
	virtual TypeHandle force_init_type()
	{
		init_type(); return get_class_type();
	}

private:
	static TypeHandle _type_handle;
	static int _attrib_slot;

};

struct collbspdata_t;

/**
 * Loads and handles the operations of PBSP files.
 */
class EXPCL_PANDABSP BSPLoader
{
PUBLISHED:
	BSPLoader();

        bool read( const Filename &file );
	void do_optimizations();

	void set_gamma( PN_stdfloat gamma, int overbright = 1 );
        INLINE PN_stdfloat get_gamma() const
        {
                return _gamma;
        }
	void set_win( GraphicsWindow *win );
        void set_camera( const NodePath &camera );
	void set_render( const NodePath &render );
        void set_shader_generator( PSSMShaderGenerator *shgen );
	void set_want_visibility( bool flag );
	void set_want_lightmaps( bool flag );
	void set_physics_type( int type );
	void set_visualize_leafs( bool flag );
	void set_materials_file( const Filename &file );
        void set_wireframe( bool flag );
        INLINE bool get_wireframe() const
        {
                return _wireframe;
        }

        INLINE NodePath get_camera() const
        {
                return _camera;
        }

        INLINE NodePath trace_decal( const std::string &decal_material, const LPoint2 &decal_scale,
                float rotate, const LPoint3 &start, const LPoint3 &end )
        {
                return _decal_mgr.decal_trace( decal_material, decal_scale, rotate, start, end );
        }

        Texture *get_closest_cubemap_texture( const LPoint3 &pos );

        void build_cubemaps();

        void set_want_shadows( bool flag );
        void set_shadow_dir( const LVector3 &dir );
        void set_shadow_color( const LColor &color );

        int extract_modelnum( int entnum );
        void get_model_bounds( int modelnum, LPoint3 &mins, LPoint3 &maxs );

#ifdef HAVE_PYTHON
        void set_server_entity_dispatcher( PyObject *dispatcher );
        void link_server_entity_to_class( const string &name, PyTypeObject *type );
#endif

        void set_ai( bool ai );
        INLINE bool is_ai() const
        {
                return _ai;
        }

        bool trace_line( const LPoint3 &start, const LPoint3 &end );
        LPoint3 clip_line( const LPoint3 &start, const LPoint3 &end );

#ifdef HAVE_PYTHON
	void link_entity_to_class( const string &entname, PyTypeObject *type );
	PyObject *get_py_entity_by_target_name( const string &targetname ) const;
        PyObject *find_all_entities( const string &classname );
        void get_entity_keyvalues( PyObject *list, const int entnum );
        void link_cent_to_pyent( int entum, PyObject *pyent );
#endif

	INLINE int get_num_entities() const
        {
                return _bspdata->numentities;
        }
	string get_entity_value( int entnum, const char *key ) const;
	float get_entity_value_float( int entnum, const char *key ) const;
	int get_entity_value_int( int entnum, const char *key ) const;
	LVector3 get_entity_value_vector( int entnum, const char *key ) const;
	LColor get_entity_value_color( int entnum, const char *key, bool scale = true ) const;
	NodePath get_entity( int entnum ) const;
	NodePath get_model( int modelnum ) const;

        CBaseEntity *get_c_entity( const int entnum ) const;

	INLINE int find_leaf( const NodePath &np )
        {
                return find_leaf( np.get_pos( _result ) );
        }
        
	int find_leaf( const LPoint3 &pos );
        int find_node( const LPoint3 &pos );
        bool is_cluster_visible( int curr_cluster, int cluster ) const;

        bool pvs_bounds_test( const GeometricBoundingVolume *bounds );
        CPT( GeometricBoundingVolume ) make_net_bounds( const TransformState *net_transform,
                                                        const GeometricBoundingVolume *original );

        INLINE bool has_active_level() const
        {
                return _active_level;
        }
        INLINE bool has_visibility() const
        {
                return _active_level && _want_visibility && _has_pvs_data;
        }

	void cleanup();

        INLINE NodePath get_result() const
        {
                return _result;
        }

        static BSPLoader *get_global_ptr();

PUBLISHED:
	enum PhysicsType
	{
		PT_none,
		PT_panda,
		PT_bullet,
		PT_ode,
		PT_physx,
	};

public:
	INLINE pvector<BoundingBox *> get_visible_leaf_bboxs() const
        {
                LightReMutexHolder holder( _leaf_aabb_lock );
                return _visible_leaf_bboxs;
        }
        INLINE bspdata_t *get_bspdata() const
        {
                return _bspdata;
        }
        INLINE collbspdata_t *get_colldata() const
        {
                return _colldata;
        }

private:
        
        void make_faces();
        void make_faces_ai();
	void load_entities();
        void load_static_props();
        void load_cubemaps();

	void read_materials_file();

        void remove_model( int modelnum );

#ifdef HAVE_PYTHON
	void make_pyent( CBaseEntity *cent, PyObject *pyent, const string &classname );
#endif

	LTexCoord get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert, bool lightmap = false ) const;

        CPT( BSPMaterial ) try_load_texref( texref_t *tref );
        

        PT( EggVertex ) make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                     dedge_t *edge, texinfo_t *texinfo,
                                     dface_t *face, int k, FaceLightmapData *ld, Texture *tex );
        PT( EggVertex ) make_vertex_ai( EggVertexPool *vpool, EggPolygon *poly, dedge_t *edge, int k );

        cubemap_t *find_closest_cubemap( const LPoint3 &pos );

        void update();
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
        bspdata_t *_bspdata;
        PSSMShaderGenerator *_shgen;
        collbspdata_t *_colldata;
	NodePath _result;
        NodePath _camera;
	NodePath _render;
        NodePath _fake_dl;
        LVector3 _shadow_dir;
        LColor _shadow_color;
        bool _want_shadows;
        entity_t *_light_environment;
        entity_t *_shadow_control;
        bool _wireframe;
	Filename _materials_file;
        PN_stdfloat _gamma;
	typedef pmap<string, string> Tex2Mat;
	Tex2Mat _materials;
	GraphicsWindow *_win;
	bool _has_pvs_data;
	bool _want_visibility;
	bool _want_lightmaps;
	bool _vis_leafs;
        bool _active_level;
        bool _ai;
	int _physics_type;
	pvector<BoundingBox *> _visible_leaf_bboxs;
        pvector<int> _visible_leafs;
	int _curr_leaf_idx;
        Filename _map_file;

        // for purely client-sided, non networked entities
        //
        // utilized only by the client
	pmap<string, PyTypeObject *> _entity_to_class;

        // entity name to distributed object class
        // createServerEntity is called on the dispatcher object
        // to generate the networked entity
        //
        // utilized only by the AI
        PyObject *_sv_ent_dispatch;
        pmap<string, PyTypeObject *> _svent_to_class;

        pmap<texref_t *, CPT( BSPMaterial )> _texref_materials;
        vector<uint8_t *> _leaf_pvs;
	pvector<NodePath> _leaf_visnp;
	pvector<PT( BoundingBox )> _leaf_bboxs;
#ifdef HAVE_PYTHON
	pvector<PyObject *> _py_entities;
	typedef pmap<CBaseEntity *, PyObject *> CEntToPyEnt;
	CEntToPyEnt _cent_to_pyent;
#endif
	pvector<NodePath> _nodepath_entities;
	pvector<NodePath> _model_roots;
        pmap<NodePath, LPoint3> _model_origins;
	pvector<PT( CBaseEntity )> _class_entities;
        LightmapPaletteDirectory _lightmap_dir;
        AmbientProbeManager _amb_probe_mgr;
        DecalManager _decal_mgr;
        pvector<cubemap_t> _cubemaps;
	
        PT( GenericAsyncTask ) _update_task;
        UpdateSeq _generated_shader_seq;

        // A per-leaf list of world Geoms.
        // This list of Geoms will be rendered for the current leaf.
        pvector<GeomNode::Geoms> _leaf_world_geoms;

	friend class BSPFaceAttrib;
        friend class BSPGeomNode;
        friend class AmbientProbeManager;
        friend class BSPCullTraverser;
        friend class BSPRender;
        friend class BSPCullableObject;

        static BSPLoader *_global_ptr;

        // While BSPLoader updates the visible leaf AABBs on the App thread,
        // BSPGeomNode needs to access them from the Cull thread.
        // So we'll use a mutex.
        LightReMutex _leaf_aabb_lock;
};

extern LColor color_from_value( const std::string &value, bool scale = true );

#endif // BSPLOADER_H