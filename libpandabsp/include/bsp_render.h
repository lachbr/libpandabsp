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
#include <modelRoot.h>
#include <cullTraverser.h>
#include <cullableObject.h>

#include "bspfile.h"
#include "shader_generator.h"

class BSPLoader;
class nodeshaderinput_t;

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

	INLINE bool has_camera_bits( unsigned int bits ) const
	{
		return ( get_camera_mask() & bits ) != 0u;
	}

	INLINE bool needs_lighting() const
	{
		return has_camera_bits( CAMERA_MASK_LIGHTING );
	}
	INLINE bool needs_culling() const
	{
		return has_camera_bits( CAMERA_MASK_CULLING );
	}

	/**
	 * Returns the flags that must be set on a leaf for it to be
	 * rendered by the current camera.
	 */
	INLINE unsigned int get_required_leaf_flags() const
	{
		//if ( has_camera_bits( CAMERA_SHADOW ) )
		//	return LEAF_FLAGS_SKY2D;
		return 0u;
	}

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

class BSPProp : public ModelRoot
{
        TypeDecl( BSPProp, ModelRoot );

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