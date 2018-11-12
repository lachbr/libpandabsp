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

#include <bitset>

#include "shader_generator.h"

static PStatCollector pvs_test_geom_collector( "Cull:BSP:Geom_LeafBoundsIntersect" );
static PStatCollector pvs_test_node_collector( "Cull:BSP:Node_LeafBoundsIntersect" );
static PStatCollector pvs_xform_collector( "Cull:BSP:Geom_LeafBoundsXForm" );
static PStatCollector pvs_node_xform_collector( "Cull:BSP:Node_LeafBoundsXForm" );
static PStatCollector addfordraw_collector( "Cull:BSP:AddForDraw" );
static PStatCollector findgeomshader_collector( "Cull:BSP:FindGeomShader" );

CullableObject *BSPCullableObject::make_copy()
{
        return new BSPCullableObject( *this );
}

void BSPCullableObject::ensure_generated_shader( GraphicsStateGuardianBase *gsgbase )
{
#if 1
        GraphicsStateGuardian *gsg = DCAST( GraphicsStateGuardian, gsgbase );
        ShaderGenerator *shgen = gsg->get_shader_generator();
        if ( shgen == nullptr )
        {
                return;
        }
        if ( shgen->is_of_type( PSSMShaderGenerator::get_class_type() ) )
        {
                PSSMShaderGenerator *pshgen = DCAST( PSSMShaderGenerator, shgen );

                const ShaderAttrib *shader_attrib;
                _state->get_attrib_def( shader_attrib );

                if ( shader_attrib->auto_shader() )
                {
                        BSPLoader *loader = BSPLoader::get_global_ptr();

                        CPT( RenderAttrib ) generated_shader = nullptr;
                        findgeomshader_collector.start();
                        int itr = loader->_geom_shader_cache.find( _geom );
                        findgeomshader_collector.stop();
                        if ( itr == -1 || _state->_generated_shader == nullptr )
                        {
                                GeomVertexAnimationSpec spec;

                                // Currently we overload this flag to request vertex animation for the
                                // shader generator.
                                if ( shader_attrib->get_flag( ShaderAttrib::F_hardware_skinning ) )
                                {
                                        spec.set_hardware( 4, true );
                                }

                                // Cache the generated ShaderAttrib on the shader state.
                                generated_shader = pshgen->synthesize_shader( _state, spec, _bsp_node_input );
                                generated_shader = DCAST( ShaderAttrib, generated_shader )->set_flag( BSPSHADERFLAG_AUTO, true );
                                loader->_geom_shader_cache[_geom] = generated_shader;
                        }
                        else
                        {
                                generated_shader = loader->_geom_shader_cache.get_data( itr );
                        }

                        nassertv( generated_shader != nullptr );

                        //if ( _bsp_node_input != nullptr )
                        //{
                        //        
                        //        std::cout << _state << " => " << _bsp_node_input->node_sequence << std::endl;
                        //}

                        //generated_shader = DCAST( ShaderAttrib, generated_shader )->set_shader_auto();

                        _state = _state->set_attrib( generated_shader );
                        _state->_generated_shader = generated_shader;
                        
                }

        }
#endif
}

TypeHandle IgnorePVSAttrib::_type_handle;
int IgnorePVSAttrib::_attrib_slot;

INLINE IgnorePVSAttrib::IgnorePVSAttrib() :
        RenderAttrib()
{
}

CPT( RenderAttrib ) IgnorePVSAttrib::make()
{
        IgnorePVSAttrib *attr = new IgnorePVSAttrib;
        return return_new( attr );
}

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

        if ( data._state->has_attrib( IgnorePVSAttrib::get_class_slot() ) )
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
        bool ret = loader->pvs_bounds_test( bbox );
        pvs_test_node_collector.stop();
        return ret;
}

INLINE bool geom_cull_test( CPT( Geom ) geom, CPT( RenderState ) state, CullTraverserData &data, const GeometricBoundingVolume *&geom_gbv )
{
        CPT( BoundingVolume ) geom_volume = geom->get_bounds();
        geom_gbv = DCAST( GeometricBoundingVolume, geom_volume );

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

        return true;
}

INLINE void BSPCullTraverser::add_geomnode_for_draw( GeomNode *node, CullTraverserData &data, nodeshaderinput_t *shinput )
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

                CPT( RenderState ) geom_state = geoms.get_geom_state( i );
                CPT( RenderState ) state = data._state->compose( geom_state );
                if ( state->has_cull_callback() && !state->cull_callback( this, data ) )
                {
                        // Cull.
                        continue;
                }

                // Cull the Geom bounding volume against the view frustum andor the cull
                // planes.  Don't bother unless we've got more than one Geom, since
                // otherwise the bounding volume of the GeomNode is (probably) the same as
                // that of the one Geom, and we've already culled against that.
                if ( num_geoms > 1 )
                {
                        // Cull the individual Geom against the view frustum/leaf AABBs/cull planes.
                        const GeometricBoundingVolume *geom_gbv;
                        if ( !geom_cull_test( geom, state, data, geom_gbv ) )
                        {
                                continue;
                        }

                        if ( !state->has_attrib( IgnorePVSAttrib::get_class_slot() ) )
                        {
                                pvs_xform_collector.start();
                                CPT( GeometricBoundingVolume ) net_geom_volume =
                                        loader->make_net_bounds( net_transform, geom_gbv );
                                pvs_xform_collector.stop();

                                pvs_test_geom_collector.start();
                                // Test geom bounds against visible leaf bounding boxes.
                                if ( !loader->pvs_bounds_test( net_geom_volume ) )
                                {
                                        // Didn't intersect any, cull.
                                        pvs_test_geom_collector.stop();
                                        continue;
                                }
                                pvs_test_geom_collector.stop();
                        }                        
                }

#if 0

                bool updated_geom_state = false;

                CPT( ShaderAttrib ) gs_shattr = nullptr;
                geom_state->get_attrib( gs_shattr );
                const ShaderAttrib *shattr = nullptr;
                state->get_attrib_def( shattr );
                if ( shattr->auto_shader() )
                {
                        if ( ( gs_shattr == nullptr || ( gs_shattr != nullptr && gs_shattr->auto_shader() ) ) && geom_state->_generated_shader == nullptr )
                        {
                                GeomVertexAnimationSpec spec;

                                // Currently we overload this flag to request vertex animation for the
                                // shader generator.
                                if ( shattr->get_flag( ShaderAttrib::F_hardware_skinning ) )
                                {
                                        spec.set_hardware( 4, true );
                                }

                                BSPLoader *loader = BSPLoader::get_global_ptr();
                                ShaderGenerator *shgen = loader->_win->get_gsg()->get_shader_generator();
                                if ( shgen != nullptr && shgen->is_of_type( PSSMShaderGenerator::get_class_type() ) )
                                {
                                        std::cout << "Generating shader for geom, " << shinput << std::endl;

                                        PSSMShaderGenerator *pshgen = DCAST( PSSMShaderGenerator, shgen );
                                        gs_shattr = pshgen->synthesize_shader( state, spec, shinput );
                                        gs_shattr = DCAST( ShaderAttrib, gs_shattr->set_flag( BSPSHADERFLAG_AUTO, true ) );
                                        //gs_shattr = DCAST( ShaderAttrib, gs_shattr->set_shader_auto() );
                                        geom_state = geom_state->add_attrib( gs_shattr, 1 );
                                        geom_state->_generated_shader = gs_shattr;
                                        //geom_state->_generated_shader_seq = shinput->node_sequence;
                                        node->set_geom_state( i, geom_state );

                                        updated_geom_state = true;
                                }
                        }
                }

                if ( updated_geom_state )
                {
                        // Compose the updated state from the root of the graph to this Geom.
                        state = data._state->compose( geom_state );
                        state->_generated_shader = gs_shattr;
                        //state->_generated_shader_seq = shinput->node_sequence;
                }

                //if ( gs_shattr != nullptr && gs_shattr->auto_shader() && !gs_shattr->has_shader_input( "ambientCube" ) )
                //{
                //        std::cout << "Skipping geom " << geom << " with no ambientCube???" << std::endl;
                //        continue;
                //}
                //std::cout << gs_shattr->has_shader_input( "ambientCube" ) << std::endl;
#endif
                BSPCullableObject *object =
                        new BSPCullableObject( std::move( geom ), std::move( state ), internal_transform, shinput );
                get_cull_handler()->record_object( object, this );
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
                     node->get_name() == "model-0" )
                {
                        wsp_trav_collector.start();

                        CPT( TransformState ) internal_transform = data.get_internal_transform( this );

                        keep_going = false;

                        // We have a specialized way of rendering worldspawn.
                        // Cuts down on Cull time.
                        bitset<MAX_MAP_FACES> rendered;
                        bitset<MAX_MAP_FACES> culled;
                        size_t num_visible_leafs = _loader->_visible_leafs.size();
                        for ( size_t i = 0; i < num_visible_leafs; i++ )
                        {
                                const int &leaf = _loader->_visible_leafs[i];
                                //std::cout << "Leaf " << leaf << " is visible." << std::endl;
                                const pvector<int> &geoms = _loader->_leaf_geom_list[leaf];
                                size_t num_geoms = geoms.size();
                                wsp_geom_traverse_collector.start();
                                for ( size_t j = 0; j < num_geoms; j++ )
                                {
                                        const int &geom_id = geoms[j];

                                        wsp_bitset_collector.start();
                                        if ( culled.test( geom_id ) || rendered.test( geom_id ) )
                                        {
                                                // Geom already culled or rendered.
                                                wsp_bitset_collector.stop();
                                                continue;
                                        }
                                        wsp_bitset_collector.stop();

                                        wsp_tuple_collector.start();
                                        const BSPLoader::WorldSpawnGeomState &gs =
                                                _loader->_leaf_geoms[geom_id];
                                        wsp_tuple_collector.stop();
                                        //CPT( TransformState ) transform = std::get<2>( _loader->_leaf_geoms[geom_id] );

                                        wsp_ctest_collector.start();
                                        const GeometricBoundingVolume *geom_gbv;
                                        if ( !geom_cull_test( gs.geom, gs.state, data, geom_gbv ) )
                                        {
                                                // Geom culled away by view frustum or clip planes.
                                                culled.set( geom_id );
                                                wsp_ctest_collector.stop();
                                                continue;
                                        }
                                        wsp_ctest_collector.stop();

                                        wsp_make_cullableobject_collector.start();
                                        // Go ahead and render this worldspawn Geom.
                                        CullableObject *object = new CullableObject(
                                                std::move( gs.geom ), std::move( gs.state ),
                                                internal_transform );
                                        wsp_make_cullableobject_collector.stop();
                                        wsp_record_collector.start();
                                        get_cull_handler()->record_object( object, this );
                                        _geoms_pcollector.add_level( 1 );
                                        wsp_record_collector.stop();

                                        // Make sure we don't render this geom again.
                                        rendered.set( geom_id );
                                }
                                wsp_geom_traverse_collector.stop();
                        }

                        wsp_trav_collector.stop();
                }
                else if ( node->is_of_type( GeomNode::get_class_type() ) )
                {
                        // Check if this node was given set_shader_off()
                        bool disabled = true;
                        if ( data._state->has_attrib( ShaderAttrib::get_class_slot() ) )
                        {
                                const ShaderAttrib *shattr = DCAST( ShaderAttrib, data._state->get_attrib( ShaderAttrib::get_class_slot() ) );
                                disabled = !shattr->auto_shader();
                        }

                        PT( nodeshaderinput_t ) shinput = nullptr;
                        if ( !disabled )
                        {
                                // Update the node's ambient probe stuff:
                                shinput = _loader->_amb_probe_mgr.update_node( node, data.get_net_transform( this ), data._state );
                        }

                        // HACKHACK:
                        // Needed to test the individual Geoms against the PVS.
                        addfordraw_collector.start();
                        add_geomnode_for_draw( DCAST( GeomNode, node ), data, shinput );
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
                BSPCullTraverser bsp_trav( trav, _loader );
                bsp_trav.local_object();
                bsp_trav.traverse_below( data );
                bsp_trav.end_traverse();

                loader->_amb_probe_mgr.consider_garbage_collect();

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
        ModelNode( name )
{
}

bool BSPProp::safe_to_combine() const
{
        return false;
}

bool BSPProp::safe_to_flatten() const
{
        return false;
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