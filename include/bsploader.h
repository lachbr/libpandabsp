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
#include "raytrace.h"
#include "bsp_trace.h"

NotifyCategoryDeclNoExport(bspfile);

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
class BSPShaderGenerator;

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

struct dface_lightmap_info_t
{
	float s_scale, t_scale;
	float s_offset, t_offset;
	int texmins[2];
	int texsize[2];
	bool flipped;
	LightmapPaletteDirectory::LightmapFacePaletteEntry *palette_entry;
};

/**
 * Loads and handles the operations of PBSP files.
 */
class EXPCL_PANDABSP BSPLoader
{
PUBLISHED:
	BSPLoader();

	void add_dynamic_entity( PyObject *pyent );
	void remove_dynamic_entity( PyObject *pyent );
	void mark_entity_preserved( int n, bool preserved = true );
	INLINE int get_num_entities() const
	{
		return _entities.size();
	}
	PyObject *get_entity( int n ) const;

	INLINE void set_transition_landmark( const std::string &name,
					     const LVector3 &origin,
					     const LVector3 &angles )
	{
		// Preserved entity transforms are stored relative to this node,
		// and applied relative to the destination landmark in the new level.
		_transition_source_landmark = NodePath( name );
		_transition_source_landmark.set_pos( origin );
		_transition_source_landmark.set_hpr( angles );
	}
	INLINE void clear_transition_landmark()
	{
		_transition_source_landmark = NodePath();
	}

        bool read( const Filename &file, bool is_transition = false );
	void do_optimizations();

	void set_gamma( PN_stdfloat gamma, int overbright = 1 );
        INLINE PN_stdfloat get_gamma() const
        {
                return _gamma;
        }
	void set_win( GraphicsWindow *win );
        void set_camera( const NodePath &camera );
	void set_render( const NodePath &render );
        void set_shader_generator( BSPShaderGenerator *shgen );
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

	INLINE void trace_decal( const std::string &decal_material, const LPoint2 &decal_scale,
		float rotate, const LPoint3 &start, const LPoint3 &end, const LColorf &decal_color = LColorf( 1 ) )
        {
                _decal_mgr.decal_trace( decal_material, decal_scale, rotate, start, end, decal_color );
        }

        Texture *get_closest_cubemap_texture( const LPoint3 &pos );

        void build_cubemaps();

        void set_want_shadows( bool flag );
        void set_shadow_dir( const LVector3 &dir );

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
        void remove_py_entity( PyObject *ent );
#endif

	NodePath get_model( int modelnum ) const;

        CBaseEntity *get_c_entity( const int entnum ) const;

	INLINE int find_leaf( const NodePath &np )
        {
                return find_leaf( np.get_pos( _result ) );
        }
        
	int find_leaf( const LPoint3 &pos, int headnode = 0 );
        int find_node( const LPoint3 &pos );
        bool is_cluster_visible( int curr_cluster, int cluster ) const;

        bool pvs_bounds_test( const GeometricBoundingVolume *bounds, unsigned int required_leaf_flags = 0u );
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

	void cleanup( bool is_transition = false );

        INLINE NodePath get_result() const
        {
                return _result;
        }

	INLINE void set_current_leaf( int leaf )
	{
		update_leaf( leaf );
	}
	INLINE int get_current_leaf() const
	{
		return _curr_leaf_idx;
	}
	INLINE int get_num_visleafs() const
	{
		return _bspdata->dmodels[0].visleafs + 1;
	}
	INLINE LPoint3 get_leaf_center( int leaf ) const
	{
		BoundingBox *bbox = _leaf_bboxs[leaf];
		return bbox->get_approx_center();
	}

	LTexCoord get_lightcoords( int facenum, const LVector3 &point );

	void update();

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
        INLINE bspdata_t *get_bspdata() const
        {
                return _bspdata;
        }
        INLINE collbspdata_t *get_colldata() const
        {
                return _colldata;
        }
	INLINE entity_t *get_light_environment() const
	{
		return _light_environment;
	}
        INLINE AmbientProbeManager *get_ambient_probe_mgr()
        {
                return &_amb_probe_mgr;
        }
	INLINE BSPTrace *get_trace() const
	{
		return _trace;
	}
	INLINE LightmapPaletteDirectory *get_lightmap_dir()
	{
		return &_lightmap_dir;
	}

	INLINE const dmodel_t *dmodel_for_dface( const dface_t *dface ) const
	{
		auto itr = _dface_dmodels.find( dface );
		if ( itr != _dface_dmodels.end() )
			return itr->second;

		return nullptr;
	}

private:
	void spawn_entities();

	void setup_raytrace_environment();

	void update_leaf( int leaf );
        
        void make_faces();

        void make_faces_ai();
        NodePath make_faces_ai_base( const std::string &name, const vector_string &include_entities );

	void load_entities();
        void load_static_props();
        void load_cubemaps();

	void read_materials_file();

        void remove_model( int modelnum );

#ifdef HAVE_PYTHON
	PyObject *make_pyent( PyObject *pyent, const string &classname );
#endif

	LTexCoord get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert, bool lightmap = false ) const;

        CPT( BSPMaterial ) try_load_texref( texref_t *tref );

        PT( EggVertex ) make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                     dedge_t *edge, texinfo_t *texinfo,
                                     dface_t *face, int k, Texture *tex );
        PT( EggVertex ) make_vertex_ai( EggVertexPool *vpool, EggPolygon *poly, dedge_t *edge, int k );

        cubemap_t *find_closest_cubemap( const LPoint3 &pos );

	void init_dface_lightmap_info( dface_lightmap_info_t *info, int facenum );

        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
        bspdata_t *_bspdata;
        BSPShaderGenerator *_shgen;
        collbspdata_t *_colldata;
	NodePath _result;
        NodePath _camera;
	NodePath _render;
        NodePath _fake_dl;
	NodePath _transition_source_landmark;
	NodePath _transition_dest_landmark;
        LVector3 _shadow_dir;
        bool _want_shadows;
        entity_t *_light_environment;
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

	PT( BSPTrace ) _trace;

	struct visibleleafdata_t
	{
		BoundingBox *bbox;
		int flags;
	};
	pvector<visibleleafdata_t> _visible_leaf_bboxs;
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

	std::unordered_map<const dface_t *, const dmodel_t *> _dface_dmodels;
        pmap<texref_t *, CPT( BSPMaterial )> _texref_materials;
        vector<uint8_t *> _leaf_pvs;
	pvector<NodePath> _leaf_visnp;
	pvector<PT( BoundingBox )> _leaf_bboxs;
	pvector<entitydef_t> _entities;
	pvector<NodePath> _model_roots;
        pmap<NodePath, LPoint3> _model_origins;
        LightmapPaletteDirectory _lightmap_dir;
	pvector<dface_lightmap_info_t> _face_lightmap_info;
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

extern LColor color_from_value( const std::string &value, bool scale = true, bool gamma = false );

#endif // BSPLOADER_H