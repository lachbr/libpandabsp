/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file glow_node.h
 * @author Brian Lach
 * @date July 20, 2019
 */

#ifndef GLOWNODE_H
#define GLOWNODE_H

#include "config_bsp.h"

#include <callbackObject.h>
#include <geomNode.h>
#include <occlusionQueryContext.h>
#include <geom.h>
#include <renderState.h>

extern ConfigVariableDouble r_glow_querysize;

class GlowNode;

class EXPCL_PANDABSP GlowNodeDrawCallback : public CallbackObject
{
	DECLARE_CLASS( GlowNodeDrawCallback, CallbackObject );

public:
	ALLOC_DELETED_CHAIN( GlowNodeDrawCallback );
	GlowNodeDrawCallback( GlowNode *node );
	virtual void do_callback( CallbackData *cbdata );

private:
	GlowNode *_node;
};

/**
 * This is a specialization on GeomNode that uses a pixel occlusion query
 * to determine if the Geoms on the node should be rendered.
 *
 * This is useful for things like light glows or lens flares.
 */
class EXPCL_PANDABSP GlowNode : public GeomNode
{
	DECLARE_CLASS( GlowNode, GeomNode );

PUBLISHED:
	GlowNode( const std::string &name, float query_size = r_glow_querysize );
	GlowNode( const GeomNode &copy, float query_size = r_glow_querysize );

public:
	virtual void add_for_draw( CullTraverser *trav, CullTraverserData &data );

private:
	void draw_callback( CallbackData *data );
	void setup_occlusion_query_geom();

	float _query_size;

	CPT( Geom ) _occlusion_query_point;
	CPT( RenderState ) _occlusion_query_point_state;
	// Number of pixels the point takes up
	int _occlusion_query_point_pixels;

	PT( OcclusionQueryContext ) _ctx;
	// Number of pixels that passed the query
	int _occlusion_query_pixels;

	friend class GlowNodeDrawCallback;
};

#endif // GLOWNODE_H
