#ifndef BSPLOADER_H
#define BSPLOADER_H

#include "config_bsp.h"

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
class BSPLoader;

/**
 * All this class does is override GeomNode's add_for_draw to cull the Geoms
 * against the visible leaf AABBs.
 */
class EXPCL_PANDABSP BSPGeomNode : public GeomNode
{
PUBLISHED:
        explicit BSPGeomNode( const string &name );
public:
        virtual PandaNode *make_copy();
        virtual PandaNode *combine_with( PandaNode *other );
        virtual void add_for_draw( CullTraverser *trav, CullTraverserData &data );

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                GeomNode::init_type();
                register_type( _type_handle, "BSPGeomNode",
                               GeomNode::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
        static TypeHandle _type_handle;
};

/**
 * An attribute applied to each face Geom from a BSP file.
 * All it does right now is indicate the material of the face.
 */
class EXPCL_PANDABSP BSPFaceAttrib : public RenderAttrib
{
private:
	INLINE BSPFaceAttrib();

PUBLISHED:
	static CPT( RenderAttrib ) make( const string &face_material );
	static CPT( RenderAttrib ) make_default();

	INLINE string get_material() const;

public:

	virtual bool has_cull_callback() const;

	virtual size_t get_hash_impl() const;
	virtual int compare_to_impl( const RenderAttrib *other ) const;

private:
	string _material;

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
	void set_gsg( GraphicsStateGuardian *gsg );
        void set_camera( const NodePath &camera );
	void set_render( const NodePath &render );
	void set_want_visibility( bool flag );
	void set_want_lightmaps( bool flag );
	void set_physics_type( int type );
	void set_visualize_leafs( bool flag );
	void set_materials_file( const Filename &file );

#ifdef HAVE_PYTHON
	void link_entity_to_class( const string &entname, PyTypeObject *type );
	PyObject *get_py_entity_by_target_name( const string &targetname ) const;
#endif

	int get_num_entities() const;
	string get_entity_value( int entnum, const char *key ) const;
	float get_entity_value_float( int entnum, const char *key ) const;
	int get_entity_value_int( int entnum, const char *key ) const;
	LVector3 get_entity_value_vector( int entnum, const char *key ) const;
	LColor get_entity_value_color( int entnum, const char *key, bool scale = true ) const;
	NodePath get_entity( int entnum ) const;
	NodePath get_model( int modelnum ) const;

	void cull_node_path_against_leafs( NodePath &np, bool part_of_result = false );

	int find_leaf( const NodePath &np );
	int find_leaf( const LPoint3 &pos );

	void cleanup();

        NodePath get_result() const;

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
	INLINE pvector<BoundingBox *> get_visible_leaf_bboxs( bool render = false ) const;

private:
        void make_faces();
	void load_entities();
        void load_static_props();

	void read_materials_file();

        void remove_model( int modelnum );

        INLINE LRGBColor color_shift_pixel( colorrgbexp32_t *sample );
        INLINE PN_stdfloat gamma_encode( PN_stdfloat linear );

#ifdef HAVE_PYTHON
	void make_pyent( CBaseEntity *cent, PyObject *pyent, const string &classname );
#endif

	LTexCoord get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert, bool lightmap = false ) const;

        PT( Texture ) try_load_texref( texref_t *tref );
        

        PT( EggVertex ) make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                     dedge_t *edge, texinfo_t *texinfo,
                                     dface_t *face, int k, FaceLightmapData *ld, Texture *tex );

        PNMImage lightmap_image_from_face( dface_t *face, FaceLightmapData *ld );

        bool is_cluster_visible( int curr_cluster, int cluster ) const;

        void update();
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

        void swap_geom_nodes( const NodePath &root );

private:
	NodePath _result;
	NodePath _brushroot;
	NodePath _proproot;
        NodePath _camera;
	NodePath _render;
	Filename _materials_file;
        PN_stdfloat _gamma;
	typedef pmap<string, string> Tex2Mat;
	Tex2Mat _materials;
	GraphicsStateGuardian *_gsg;
	bool _has_pvs_data;
	bool _want_visibility;
	bool _want_lightmaps;
	bool _vis_leafs;
	int _physics_type;
	pvector<BoundingBox *> _visible_leaf_bboxs;
	pvector<BoundingBox *> _visible_leaf_render_bboxes;
	int _curr_leaf_idx;
	pmap<string, PyTypeObject *> _entity_to_class;

        pmap<texref_t *, PT( Texture )> _texref_textures;
        vector<uint8_t *> _leaf_pvs;
	pvector<NodePath> _leaf_visnp;
	pvector<PT( BoundingBox )> _leaf_bboxs;
	pvector<PT( BoundingBox )> _leaf_render_bboxes;
#ifdef HAVE_PYTHON
	pvector<PyObject *> _py_entities;
	typedef pmap<CBaseEntity *, PyObject *> CEntToPyEnt;
	CEntToPyEnt _cent_to_pyent;
#endif
	pvector<NodePath> _nodepath_entities;
	pvector<NodePath> _model_roots;
	pvector<PT( CBaseEntity )> _class_entities;
	
        PT( GenericAsyncTask ) _update_task;

	friend class BSPFaceAttrib;
        friend class BSPGeomNode;

        static BSPLoader *_global_ptr;

        // While BSPLoader updates the visible leaf AABBs on the App thread,
        // BSPGeomNode needs to access them from the Cull thread.
        // So we'll use a mutex.
        LightReMutex _leaf_aabb_lock;
};

#endif // BSPLOADER_H