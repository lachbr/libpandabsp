/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsp_render.h
 * @author Brian Lach
 * @date August 07, 2018
 */

#ifndef BSPRENDER_H
#define BSPRENDER_H

#include "config_bsp.h"

#include <pandaNode.h>
#include <modelNode.h>
#include <cullTraverser.h>
#include <cullableObject.h>

// Appends onto ShaderAttrib flags which are already 0-3
enum
{
        BSPSHADERFLAG_AUTO = 4,
};

class BSPLoader;
struct nodeshaderinput_t;

class BSPCullableObject : public CullableObject
{
public:
        INLINE BSPCullableObject() :
                CullableObject(),
                _bsp_node_input( nullptr )
        {
        }

        INLINE BSPCullableObject( CPT( Geom ) geom, CPT( RenderState ) state,
                                  CPT( TransformState ) internal_transform,
                                  nodeshaderinput_t *bsp_node_input ) :
                CullableObject( std::move(geom), std::move(state), internal_transform ),
                _bsp_node_input( bsp_node_input )
        {
        }

        INLINE BSPCullableObject( const BSPCullableObject &copy ) :
                CullableObject( copy ),
                _bsp_node_input( copy._bsp_node_input )
        {
        }

        INLINE void operator = ( const BSPCullableObject &copy )
        {
                CullableObject::operator = ( copy );
                _bsp_node_input = copy._bsp_node_input;
        }

        virtual CullableObject *make_copy();

        virtual void ensure_generated_shader( GraphicsStateGuardianBase *gsgbase );

        PT( nodeshaderinput_t ) _bsp_node_input;

public:
        ALLOC_DELETED_CHAIN( BSPCullableObject );
};

class BSPCullTraverser : public CullTraverser
{
        TypeDecl( BSPCullTraverser, CullTraverser );

PUBLISHED:
        BSPCullTraverser( CullTraverser *trav, BSPLoader *loader );

        virtual void traverse_below( CullTraverserData &data );

protected:
        virtual bool is_in_view( CullTraverserData &data );

private:
        INLINE void add_geomnode_for_draw( GeomNode *node, CullTraverserData &data, nodeshaderinput_t *shinput );
        static CPT( RenderState ) get_depth_offset_state();

private:
        BSPLoader *_loader;
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