/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file glow_node.cpp
 * @author Brian Lach
 * @date July 20, 2019
 */

#include "glow_node.h"

#include <mathlib.h>

#include <geomVertexData.h>
#include <geomVertexWriter.h>
#include <geomVertexFormat.h>
#include <geomPoints.h>

#include <graphicsStateGuardian.h>
#include <geomDrawCallbackData.h>

#include <glgsg.h>

#include <depthTestAttrib.h>
#include <depthWriteAttrib.h>
#include <colorWriteAttrib.h>
#include <clockObject.h>
#include <configVariableDouble.h>
#include <renderModeAttrib.h>

static ConfigVariableDouble r_glow_fadetime( "r_glow_fadetime", 0.1 );
ConfigVariableDouble r_glow_querysize( "r_glow_querysize", 16.0 );

static int calc_num_pixels( float point_size )
{
	return (int)roundf( powf( point_size, 2 ) );
}

IMPLEMENT_CLASS( GlowNode );

GlowNode::GlowNode( const std::string &name, float query_size ) :
	GeomNode( name ),
	_ctx( nullptr ),
	_occlusion_query_point( nullptr ),
	_occlusion_query_pixels( 0 ),
	_occlusion_query_point_pixels( 0 ),
	_query_size( query_size )
{
	setup_occlusion_query_geom();
}

GlowNode::GlowNode( const GeomNode &copy, float query_size ) :
	GeomNode( copy ),
	_ctx( nullptr ),
	_occlusion_query_point( nullptr ),
	_occlusion_query_pixels( 0 ),
	_occlusion_query_point_pixels( 0 ),
	_query_size( query_size )
{
	setup_occlusion_query_geom();
}

void GlowNode::setup_occlusion_query_geom()
{
	// Sets up a geom that renders as a single pixel on screen.
	// This point will be used in the occlusion query to determine glow visibility.

	PT( GeomVertexData ) data = new GeomVertexData( "occlusion_query_point", GeomVertexFormat::get_v3(), GeomEnums::UH_static );
	data->set_num_rows( 1 );

	GeomVertexWriter vtx_writer( data, InternalName::get_vertex() );
	vtx_writer.add_data3f( LVecBase3f( 0, 0, 0 ) );

	PT( GeomPoints ) points = new GeomPoints( GeomEnums::UH_static );
	points->add_vertex( 0 );
	points->close_primitive();

	PT(Geom) geom = new Geom( data );
	geom->add_primitive( points );
	_occlusion_query_point = geom;

	_occlusion_query_point_state = RenderState::make( DepthTestAttrib::make_default(),
							  DepthWriteAttrib::make( DepthWriteAttrib::M_off ),
							  ColorWriteAttrib::make( ColorWriteAttrib::C_off ),
							  RenderModeAttrib::make( RenderModeAttrib::M_point, _query_size ) );

	_occlusion_query_point_pixels = calc_num_pixels( _query_size );
	
}

void GlowNode::add_for_draw( CullTraverser *trav, CullTraverserData &data )
{
	std::cout << "ERROR: GlowNode::add_for_draw() was called!" << std::endl;
}

void GlowNode::draw_callback( CallbackData *data )
{
	if ( _ctx )
	{
		if ( _ctx->is_of_type( CLP( OcclusionQueryContext )::get_class_type() ) )
		{
			CLP( OcclusionQueryContext ) *glctx = DCAST( CLP( OcclusionQueryContext ), _ctx );
			if ( glctx->is_answer_ready() )
			{
				_occlusion_query_pixels = glctx->get_num_fragments();
				_ctx = nullptr;
			}
		}
		else
		{
			_occlusion_query_pixels = _ctx->get_num_fragments();
			_ctx = nullptr;
		}
	}

	GeomDrawCallbackData *geom_cbdata;
	DCAST_INTO_V( geom_cbdata, data );
	GraphicsStateGuardian *gsg;
	DCAST_INTO_V( gsg, geom_cbdata->get_gsg() );

	CullableObject *obj = geom_cbdata->get_object();

	if ( _occlusion_query_pixels > 0 )
	{
		float fraction = _occlusion_query_pixels / (float)_occlusion_query_point_pixels;
		if ( fraction > 1.0f - FLT_EPSILON )
		{
			// Glow is fully visible, render as-is
			obj->draw_inline( gsg, geom_cbdata->get_force(), Thread::get_current_thread() );
		}
		else
		{
			// Glow is somewhat occluded
			CPT( RenderState ) scale_state = RenderState::make( ColorScaleAttrib::make( LVector4f( fraction, fraction, fraction, 1.0f ) ), 1 );
			CPT( RenderState ) new_state = obj->_state->compose( scale_state );
			new_state->_generated_shader = obj->_state->_generated_shader;

			gsg->set_state_and_transform( new_state, obj->_internal_transform );

			obj->draw_inline( gsg, geom_cbdata->get_force(), Thread::get_current_thread() );
		}
	}

	if ( !_ctx )
	{
		// Render a point at the location of the glow with an occlusion query.

		CPT( GeomVertexData ) munged_data = _occlusion_query_point->get_vertex_data();
		CPT( Geom ) munged_geom = _occlusion_query_point;

		gsg->get_geom_munger( _occlusion_query_point_state, Thread::get_current_thread() )->
			munge_geom( munged_geom, munged_data, true, Thread::get_current_thread() );

		gsg->set_state_and_transform( _occlusion_query_point_state, obj->_internal_transform );

	        gsg->begin_occlusion_query();
		munged_geom->draw( gsg, munged_data, true, Thread::get_current_thread() );
		_ctx = gsg->end_occlusion_query();
	}

	geom_cbdata->set_lost_state( false );
}

IMPLEMENT_CLASS( GlowNodeDrawCallback );

GlowNodeDrawCallback::GlowNodeDrawCallback( GlowNode *node ) :
	CallbackObject(),
	_node( node )
{
}

void GlowNodeDrawCallback::do_callback( CallbackData *data )
{
	_node->draw_callback( data );
}
