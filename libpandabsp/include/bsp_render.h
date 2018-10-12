#ifndef BSPRENDER_H
#define BSPRENDER_H

#include "config_bsp.h"

#include <pandaNode.h>
#include <modelNode.h>
#include <cullTraverser.h>
#include <cullableObject.h>

class BSPLoader;
struct nodeshaderinput_t;

class BSPCullTraverser : public CullTraverser
{
        TypeDecl( BSPCullTraverser, CullTraverser );

PUBLISHED:
        BSPCullTraverser( CullTraverser *trav, BSPLoader *loader );

        virtual void traverse_below( CullTraverserData &data );

protected:
        virtual bool is_in_view( CullTraverserData &data );

private:
        INLINE void add_geomnode_for_draw( GeomNode *node, CullTraverserData &data );
        static CPT( RenderState ) get_depth_offset_state();

private:
        BSPLoader *_loader;
};

/**
 * Forces the node/geom to not be tested against the PVS, just frustum culled.
 */
class EXPCL_PANDABSP IgnorePVSAttrib : public RenderAttrib
{
PUBLISHED:
        INLINE IgnorePVSAttrib();

        static CPT( RenderAttrib ) make();

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
                register_type( _type_handle, "IgnorePVSAttrib",
                               RenderAttrib::get_class_type() );
                _attrib_slot = register_slot( _type_handle, 100, new IgnorePVSAttrib );
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
 * Top of the scene graph when a BSP level is in effect.
 * Culls nodes against the PVS, operates ambient cubes, etc.
 */
class BSPRender : public PandaNode
{
        TypeDecl( BSPRender, PandaNode );

PUBLISHED:
        BSPRender( const std::string &name, BSPLoader *loader );

public:
        virtual bool cull_callback( CullTraverser *trav, CullTraverserData &data );

private:
        BSPLoader * _loader;
};

class BSPRoot : public PandaNode
{
        TypeDecl( BSPRoot, PandaNode );

PUBLISHED:
        BSPRoot( const std::string &name );

public:
        virtual bool safe_to_combine() const;
        virtual bool safe_to_flatten() const;
};

class BSPProp : public ModelNode
{
        TypeDecl( BSPProp, ModelNode );

PUBLISHED:
        BSPProp( const std::string &name );

public:
        virtual bool safe_to_combine() const;
        virtual bool safe_to_flatten() const;
};

class BSPModel : public ModelNode
{
        TypeDecl( BSPModel, ModelNode );

PUBLISHED:
        BSPModel( const std::string &name );

public:
        virtual bool safe_to_combine() const;
        virtual bool safe_to_flatten() const;
};

#endif // BSPRENDER_H