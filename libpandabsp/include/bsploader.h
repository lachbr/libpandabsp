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

class EXPCL_PANDABSP BSPCullAttrib : public RenderAttrib
{
private:
	INLINE BSPCullAttrib();

PUBLISHED:
	// Used for applying the attrib to a single static geom.
	static CPT( RenderAttrib ) make( CPT(GeometricBoundingVolume) geom_bounds, const string &face_material, BSPLoader *loader );
	// Used for applying the attrib to an entire node.
	static CPT( RenderAttrib ) make( BSPLoader *loader, bool part_of_result = false );

	static CPT( RenderAttrib ) make_default();

	INLINE CPT(GeometricBoundingVolume) get_geom_bounds() const;
	INLINE BSPLoader *get_loader() const;
	INLINE string get_material() const;

PUBLISHED:
	MAKE_PROPERTY( geom_bounds, get_geom_bounds );
	MAKE_PROPERTY( loader, get_loader );

public:

	virtual bool has_cull_callback() const;
	virtual bool cull_callback( CullTraverser *trav, const CullTraverserData &data ) const;

	virtual size_t get_hash_impl() const;
	virtual int compare_to_impl( const RenderAttrib *other ) const;

private:
	CPT( GeometricBoundingVolume ) _geom_bounds;
	BSPLoader *_loader;
	bool _on_geom;
	bool _part_of_result;
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
		register_type( _type_handle, "BSPCullAttrib",
			       RenderAttrib::get_class_type() );
		_attrib_slot = register_slot( _type_handle, 100, new BSPCullAttrib );
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
	pvector<PT( BoundingBox )> get_visible_leaf_bboxs( bool render = false ) const;

private:
        void make_faces();
	void load_entities();

	void read_materials_file();

#ifdef HAVE_PYTHON
	void make_pyent( CBaseEntity *cent, PyObject *pyent, const string &classname );
#endif

	LTexCoord get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert ) const;

        PT( Texture ) try_load_texref( texref_t *tref );
        

        PT( EggVertex ) make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                     dedge_t *edge, texinfo_t *texinfo,
                                     dface_t *face, int k, FaceLightmapData *ld, Texture *tex );

        PNMImage lightmap_image_from_face( dface_t *face, FaceLightmapData *ld );

        bool is_cluster_visible( int curr_cluster, int cluster ) const;

        void update();
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

private:
	NodePath _result;
	NodePath _brushroot;
	NodePath _proproot;
        NodePath _camera;
	NodePath _render;
	Filename _materials_file;
	typedef pmap<string, string> Tex2Mat;
	Tex2Mat _materials;
	GraphicsStateGuardian *_gsg;
	bool _has_pvs_data;
	bool _want_visibility;
	bool _want_lightmaps;
	bool _vis_leafs;
	int _physics_type;
	pvector<PT( BoundingBox )> _visible_leaf_bboxs;
	pvector<PT( BoundingBox )> _visible_leaf_render_bboxes;
	int _curr_leaf_idx;
	pmap<string, PyTypeObject *> _entity_to_class;

	pvector<pvector<const Geom *>> _leaf_geoms;

        pmap<texref_t *, PT( Texture )> _texref_textures;
        vector<uint8_t *> _leaf_pvs;
	pvector<NodePath> _leaf_visnp;
	pvector<GeomNode *> _face_geomnodes;
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

	friend class BSPCullAttrib;
};

#endif // BSPLOADER_H