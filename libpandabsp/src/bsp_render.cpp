/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file bsp_render.cpp
 * @author Brian Lach
 * @date August 07, 2018
 */

#include "bsp_render.h"
#include "bsploader.h"

#include <depthOffsetAttrib.h>
#include <cullableObject.h>
#include <cullHandler.h>
#include <characterJointEffect.h>
#include <renderModeAttrib.h>
#include <modelRoot.h>

#include <bitset>

#include "shader_generator.h"

static PStatCollector pvs_test_geom_collector( "Cull:BSP:AddForDraw:Geom_LeafBoundsIntersect" );
static PStatCollector pvs_test_node_collector( "Cull:BSP:Node_LeafBoundsIntersect" );
static PStatCollector pvs_xform_collector( "Cull:BSP:AddForDraw:Geom_LeafBoundsXForm" );
static PStatCollector pvs_node_xform_collector( "Cull:BSP:Node_LeafBoundsXForm" );
static PStatCollector addfordraw_collector( "Cull:BSP:AddForDraw" );
static PStatCollector findgeomshader_collector( "Cull:BSP:FindGeomShader" );
static PStatCollector ambientprobe_nodes_collector( "BSP Dynamic Nodes" );
static PStatCollector applyshaderattrib_collector( "Cull:BSP:ApplyShaderAttrib" );
static PStatCollector makecullable_geomnode_collector( "Cull:BSP:AddForDraw:MakeCullableObject" );

static ConfigVariableColor dynamic_wf_color( "bsp-dynamic-wireframe-color", LColor( 0, 1.0, 1.0, 1.0 ) );
static ConfigVariableColor brush_wf_color( "bsp-brush-wireframe-color", LColor( 231 / 255.0, 129 / 255.0, 129 / 255.0, 1.0 ) );

TypeDef( BSPCullTraverser );

BSPCullTraverser::BSPCullTraverser( CullTraverser *trav, BSPLoader *loader ) :
        CullTraverser( *trav ),
        _loader( loader )
{
}

bool BSPCullTraverser::is_in_view( CullTraverserData &data )
{
        BSPLoader *loader = _loader;

        // First test view frustum.	
        if ( !data.is_in_view( get_camera_mask() ) )
        {
                return false;
        }

        CPT( BSPFaceAttrib ) bfa;
        data._state->get_attrib_def( bfa );
        if ( bfa->get_ignore_pvs() )
        {
                // Don't test this node against PVS.
                return true;
        }

        // View frustum test passed.
        // Now test against PVS (AABBs of all potentially visible leafs).

        pvs_node_xform_collector.start();
        CPT( GeometricBoundingVolume ) bbox = loader->make_net_bounds(
                data.get_net_transform( this ),
                data.node()->get_bounds()->as_geometric_bounding_volume() );
        pvs_node_xform_collector.stop();

        pvs_test_node_collector.start();
        bool ret = loader->pvs_bounds_test( bbox, get_required_leaf_flags() );
        pvs_test_node_collector.stop();
        return ret;
}

INLINE bool geom_cull_test( CPT( Geom ) geom, CPT( RenderState ) state, CullTraverserData &data,
	const GeometricBoundingVolume *&geom_gbv, bool needs_culling )
{
        CPT( BoundingVolume ) geom_volume = geom->get_bounds();
        geom_gbv = DCAST( GeometricBoundingVolume, geom_volume );

	if ( needs_culling )
	{
		if ( data._view_frustum != nullptr )
		{
			int result = data._view_frustum->contains( geom_gbv );
			if ( result == BoundingVolume::IF_no_intersection )
			{
				// Cull this Geom.
				return false;
			}
		}
		if ( !data._cull_planes->is_empty() )
		{
			// Also cull the Geom against the cull planes.
			int result;
			data._cull_planes->do_cull( result, state, geom_gbv );
			if ( result == BoundingVolume::IF_no_intersection )
			{
				// Cull.
				return false;
			}
		}
	}

	return true;
}

INLINE void BSPCullTraverser::add_geomnode_for_draw( GeomNode *node, CullTraverserData &data )
{
        BSPLoader *loader = _loader;

        _geom_nodes_pcollector.add_level( 1 );

        // Get all the Geoms, with no decalling.
        GeomNode::Geoms geoms = node->get_geoms( get_current_thread() );
        int num_geoms = geoms.get_num_geoms();
        CPT( TransformState ) internal_transform = data.get_internal_transform( this );
        CPT( TransformState ) net_transform = data.get_net_transform( this );

        for ( int i = 0; i < num_geoms; i++ )
        {
                CPT( Geom ) geom = geoms.get_geom( i );
                if ( geom->is_empty() )
                {
                        continue;
                }

                CPT( RenderState ) state = data._state->compose( geoms.get_geom_state( i ) );
                if ( needs_culling() && state->has_cull_callback() && !state->cull_callback( this, data ) )
                {
                        // Cull.
                        continue;
                }

		if ( has_camera_bits( CAMERA_SHADOW ) )
		{
			if ( geom->get_primitive_type() != Geom::PT_polygons )
			{
				// We can only render triangles to the shadow maps.
				continue;
			}
		}

                // Cull the Geom bounding volume against the view frustum andor the cull
                // planes.  Don't bother unless we've got more than one Geom, since
                // otherwise the bounding volume of the GeomNode is (probably) the same as
                // that of the one Geom, and we've already culled against that.
                if ( num_geoms > 1 )
                {
                        // Cull the individual Geom against the view frustum/leaf AABBs/cull planes.
                        const GeometricBoundingVolume *geom_gbv;
			if ( !geom_cull_test( geom, state, data, geom_gbv, needs_culling() ) )
                        {
                                continue;
                        }

                        CPT( BSPFaceAttrib ) bfa;
                        data._state->get_attrib_def( bfa );
                        if ( !bfa->get_ignore_pvs() )
                        {
                                pvs_xform_collector.start();
                                CPT( GeometricBoundingVolume ) net_geom_volume =
                                        loader->make_net_bounds( net_transform, geom_gbv );
                                pvs_xform_collector.stop();

                                pvs_test_geom_collector.start();
                                // Test geom bounds against visible leaf bounding boxes.
				// Always test against PVS even if camera's bit isn't set in CAMERA_MASK_CULLING.
                                if ( !loader->pvs_bounds_test( net_geom_volume, get_required_leaf_flags() ) )
                                {
                                        // Didn't intersect any, cull.
                                        pvs_test_geom_collector.stop();
                                        continue;
                                }
                                pvs_test_geom_collector.stop();
                        }                        
                }

		if ( _loader->get_wireframe() && has_camera_bits( CAMERA_MAIN | CAMERA_VIEWMODEL ) )
                {
                        CPT( RenderAttrib ) wfattr = RenderModeAttrib::make( RenderModeAttrib::M_wireframe,
                                                                             0.5, true, dynamic_wf_color );
                        CPT( RenderAttrib ) tattr = TextureAttrib::make_all_off();
                        CPT( RenderAttrib ) cattr = ColorAttrib::make_flat( dynamic_wf_color );
                        CPT( RenderState ) wfstate = RenderState::make( wfattr, tattr, cattr, 10 );
                        state = state->compose( wfstate );
                }

                makecullable_geomnode_collector.start();
                CullableObject *object =
                        new CullableObject( std::move( geom ), std::move( state ), internal_transform );
                get_cull_handler()->record_object( object, this );
                makecullable_geomnode_collector.stop();
                _geoms_pcollector.add_level( 1 );
        }
}

static PStatCollector wsp_ctest_collector( "Cull:BSP:WorldSpawn:CullTest" );
static PStatCollector wsp_tuple_collector( "Cull:BSP:WorldSpawn:Tuple" );
static PStatCollector wsp_bitset_collector( "Cull:BSP:WorldSpawn:BitSetTest" );
static PStatCollector wsp_record_collector( "Cull:BSP:WorldSpawn:RecordGeom" );
static PStatCollector wsp_trav_collector( "Cull:BSP:WorldSpawn:TraverseLeafs" );
static PStatCollector wsp_geom_traverse_collector( "Cull:BSP:WorldSpawn:TraverseLeafGeoms" );
static PStatCollector wsp_make_cullableobject_collector( "Cull:BSP:WorldSpawn:MakeCullableObject" );

// For worldspawn rendering
static byte *visited = new byte[MAX_MAP_FACES];

void BSPCullTraverser::traverse_below( CullTraverserData &data )
{
        _nodes_pcollector.add_level( 1 );
        PandaNodePipelineReader *node_reader = data.node_reader();
        PandaNode *node = data.node();

        bool keep_going = true;

        // Add the current node to be drawn during the Draw stage.
        if ( !data.is_this_node_hidden( get_camera_mask() ) )
        {
                // Check for a decal effect.
                const RenderEffects *node_effects = node_reader->get_effects();
                if ( node_effects->has_decal() )
                {
                        // If we *are* implementing decals with DepthOffsetAttribs, apply it
                        // now, so that each child of this node gets offset by a tiny amount.
                        data._state = data._state->compose( get_depth_offset_state() );
                }

		if ( node->is_of_type( BSPModel::get_class_type() ) &&
			node->get_name() == "model-0" ) // UNDONE: think of a better way to identify world geometry?
		{
			wsp_trav_collector.start();

			CPT( TransformState ) internal_transform = data.get_internal_transform( this );

			keep_going = false;

			_loader->_leaf_aabb_lock.acquire();
			bool should_render = _loader->_curr_leaf_idx != 0;

			if ( should_render )
			{
				const GeomNode::Geoms &world_geoms = _loader->_leaf_world_geoms[_loader->_curr_leaf_idx];

				int num_world_geoms = world_geoms.get_num_geoms();
				for ( int i = 0; i < num_world_geoms; i++ )
				{
					const RenderState *world_state = data._state->compose( world_geoms.get_geom_state( i ) );

					if ( has_camera_bits( CAMERA_SHADOW ) )
					{
						const BSPMaterialAttrib *bma;
						world_state->get_attrib( bma );
						if ( bma )
						{
							const BSPMaterial *mat = bma->get_material();
							if ( mat && mat->get_shader() == "SkyBox" )
							{
								// This is a terrible hack to make skybox
								// faces not render to shadow maps.
								continue;
							}
						}
					}

					const Geom *world_geom = world_geoms.get_geom( i );

					wsp_ctest_collector.start();
					const GeometricBoundingVolume *geom_gbv;
					if ( !geom_cull_test( world_geom, world_state, data, geom_gbv, needs_culling() ) )
					{
						// Geom culled away by view frustum or clip planes.
						wsp_ctest_collector.stop();
						continue;
					}
					wsp_ctest_collector.stop();

					if ( _loader->get_wireframe() && has_camera_bits( CAMERA_MAIN ) )
					{
						CPT( RenderAttrib ) wfattr = RenderModeAttrib::make( RenderModeAttrib::M_filled_wireframe,
							0.5, true, brush_wf_color );
						CPT( RenderState ) wfstate = RenderState::make( wfattr, 1 );
						world_state = world_state->compose( wfstate );
					}

					wsp_make_cullableobject_collector.start();
					// Go ahead and render this worldspawn Geom.
					CullableObject *object = new CullableObject(
						std::move( world_geom ), std::move( world_state ),
						internal_transform );
					wsp_make_cullableobject_collector.stop();
					wsp_record_collector.start();
					get_cull_handler()->record_object( object, this );
					_geoms_pcollector.add_level( 1 );
					wsp_record_collector.stop();
				}
			}

			_loader->_leaf_aabb_lock.release();

			wsp_trav_collector.stop();
		}
		else if ( has_camera_bits( CAMERA_MAIN | CAMERA_VIEWMODEL ) &&
			node->is_of_type( ModelRoot::get_class_type() ) )
                {
                        // Only run this logic on the main Camera and viewmodel.

                        // This signifies the root of a model...
                        // we should follow this convention.

                        // Check if this node was given set_shader_off()
                        bool disabled = true;
                        if ( data._state->has_attrib( ShaderAttrib::get_class_slot() ) )
                        {
                                const ShaderAttrib *shattr = DCAST( ShaderAttrib, data._state->get_attrib( ShaderAttrib::get_class_slot() ) );
                                disabled = !shattr->auto_shader();
                        }


                        if ( !disabled )
                        {
                                // Update the node's ambient probe stuff:
                                const RenderState *input_state = _loader->_amb_probe_mgr.update_node(
                                        node, data.get_net_transform( this ) );
                                if ( input_state )
                                {
                                        data._state = data._state->compose( input_state );
                                }
                                ambientprobe_nodes_collector.add_level( 1 );
                        }
                }
                else if ( node->is_of_type( GeomNode::get_class_type() ) )
                {
                        // HACKHACK:
                        // Needed to test the individual Geoms against the PVS.
                        addfordraw_collector.start();
                        add_geomnode_for_draw( DCAST( GeomNode, node ), data );
                        addfordraw_collector.stop();
                }
                else
                {
                        node->add_for_draw( this, data );
                }
        }

        if ( !keep_going )
        {
                // Don't continue to the children of this node.
                return;
        }

        // Now visit all the node's children.
        PandaNode::Children children = node_reader->get_children();
        node_reader->release();
        int num_children = children.get_num_children();
        if ( !node->has_selective_visibility() )
        {
                for ( int i = 0; i < num_children; ++i )
                {
                        CullTraverserData next_data( data, children.get_child( i ) );
                        do_traverse( next_data );
                }
        }
        else
        {
                int i = node->get_first_visible_child();
                while ( i < num_children )
                {
                        CullTraverserData next_data( data, children.get_child( i ) );
                        do_traverse( next_data );
                        i = node->get_next_visible_child( i );
                }
        }
}

/**
* Returns a RenderState for increasing the DepthOffset by one.
*/
CPT( RenderState ) BSPCullTraverser::get_depth_offset_state()
{
        // Once someone asks for this pointer, we hold its reference count and never
        // free it.
        static CPT( RenderState ) state = nullptr;
        if ( state == nullptr )
        {
                state = RenderState::make( DepthOffsetAttrib::make( 1 ) );
        }
        return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////

TypeDef( BSPRender );

BSPRender::BSPRender( const std::string &name, BSPLoader *loader ) :
        PandaNode( name ),
        _loader( loader )
{
        set_cull_callback();
}

bool BSPRender::cull_callback( CullTraverser *trav, CullTraverserData &data )
{
        BSPLoader *loader = _loader;
        if ( loader->has_visibility() )
        {
                ambientprobe_nodes_collector.flush_level();
                ambientprobe_nodes_collector.set_level( 0 );

                // Transform all of the potentially visible lights into view space for this frame.
		// Not for shadow passes though (doesn't need lighting information)
		//
		// TODO: perform lighting calculations in world space so we don't even have to do this
		if ( ( trav->get_camera_mask() & CAMERA_MAIN ) != 0u )
		{
			loader->_amb_probe_mgr.xform_lights( trav->get_scene()->get_cs_world_transform() );
		}

                BSPCullTraverser bsp_trav( trav, _loader );
                bsp_trav.local_object();
                bsp_trav.traverse_below( data );
                bsp_trav.end_traverse();

                // No need for CullTraverser to go further down this node,
                // the BSPCullTraverser has already handled it.
                return false;
        }

        // If no BSP level, do regular culling.
        return true;
}

TypeDef( BSPRoot );
TypeDef( BSPProp );
TypeDef( BSPModel );

BSPRoot::BSPRoot( const std::string &name ) :
        PandaNode( name )
{
}

bool BSPRoot::safe_to_combine() const
{
        return true;
}

bool BSPRoot::safe_to_flatten() const
{
        return true;
}

BSPProp::BSPProp( const std::string &name ) :
	    ModelRoot( name )
{
}

bool BSPProp::safe_to_combine() const
{
        return true;
}

bool BSPProp::safe_to_flatten() const
{
        return true;
}

BSPModel::BSPModel( const std::string &name ) :
        ModelNode( name )
{
}

bool BSPModel::safe_to_combine() const
{
        return false;
}

bool BSPModel::safe_to_flatten() const
{
        return false;
}