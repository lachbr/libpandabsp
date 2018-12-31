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
static PStatCollector applyenvmap_collector( "Cull:BSP:ApplyCubeMaps" );
static PStatCollector applyshaderattrib_collector( "Cull:BSP:ApplyShaderAttrib" );
static PStatCollector makecullable_geomnode_collector( "Cull:BSP:AddForDraw:MakeCullableObject" );

class BSPCullTraverserData : public CullTraverserData
{
PUBLISHED:
        INLINE BSPCullTraverserData( const CullTraverserData &parent,
                                     nodeshaderinput_t *inp,
                                     PandaNode *child ) :
                CullTraverserData( parent, child ),
                _inp( inp )
        {
        }

        INLINE BSPCullTraverserData( const CullTraverserData &copy ) :
                CullTraverserData( copy ),
                _inp( nullptr )
        {
        }

        INLINE nodeshaderinput_t *get_input() const
        {
                return _inp;
        }

private:
        nodeshaderinput_t *_inp;
};

static ConfigVariableColor dynamic_wf_color( "bsp-dynamic-wireframe-color", LColor( 0, 1.0, 1.0, 1.0 ) );
static ConfigVariableColor brush_wf_color( "bsp-brush-wireframe-color", LColor( 231 / 255.0, 129 / 255.0, 129 / 255.0, 1.0 ) );

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

                        //applyenvmap_collector.start();
                        // this is extremely hacky, we have to update the cubemap_tex sampler
                        //if ( _bsp_node_input && _bsp_node_input->cubemap_tex )
                        //{
                        //        const BSPMaterialAttrib *bma;
                        //        _state->get_attrib_def( bma );

                        //        const BSPMaterial *mat = bma->get_material();

                        //        if ( mat &&
                        //             mat->has_env_cubemap() )
                        //        {

                                        //std::cout << "update cubemap_tex sampler " << _bsp_node_input->cubemap_tex->cubemap_tex << std::endl;
                                        // update cubemap_tex sampler 
                        //                generated_shader = DCAST( ShaderAttrib, generated_shader )->set_shader_input(
                        //                        ShaderInput( "envmapSampler", _bsp_node_input->cubemap_tex->cubemap_tex ) );
                                        //loader->_geom_shader_cache[_geom] = generated_shader;
                        //        }

                                
                        //}
                        //applyenvmap_collector.stop();

                        // Cache the shader on the old state
                        _state->_generated_shader = generated_shader;

                        // Now make a unique attrib, avoids composition cache.
                        // The composition cache will unify states and result in shared bsp node inputs,
                        // which is bad.
                        applyshaderattrib_collector.start();
                        _state = _state->set_attrib( generated_shader );
                        applyshaderattrib_collector.stop();
                        _state->_generated_shader = generated_shader;
                        
                }

        }
#endif
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

                CPT( RenderState ) state = data._state->compose( geoms.get_geom_state( i ) );
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
                                if ( !loader->pvs_bounds_test( net_geom_volume ) )
                                {
                                        // Didn't intersect any, cull.
                                        pvs_test_geom_collector.stop();
                                        continue;
                                }
                                pvs_test_geom_collector.stop();
                        }                        
                }

                if ( _loader->get_wireframe() )
                {
                        CPT( RenderAttrib ) wfattr = RenderModeAttrib::make( RenderModeAttrib::M_wireframe,
                                                                             0.5, true, dynamic_wf_color );
                        CPT( RenderAttrib ) tattr = TextureAttrib::make_all_off();
                        CPT( RenderAttrib ) cattr = ColorAttrib::make_flat( dynamic_wf_color );
                        CPT( RenderState ) wfstate = RenderState::make( wfattr, tattr, cattr, 10 );
                        state = state->compose( wfstate );
                }

                makecullable_geomnode_collector.start();
                BSPCullableObject *object =
                        new BSPCullableObject( std::move( geom ), std::move( state ), internal_transform, shinput );
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

void BSPCullTraverser::traverse_below( CullTraverserData &cdata )
{
        BSPCullTraverserData &data = (BSPCullTraverserData &)cdata;

        _nodes_pcollector.add_level( 1 );
        PandaNodePipelineReader *node_reader = data.node_reader();
        PandaNode *node = data.node();

        PT( nodeshaderinput_t ) shinput = data.get_input();

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
                        byte visited[MAX_MAP_FACES] = { 0 };

                        _loader->_leaf_aabb_lock.acquire();

                        size_t num_visible_leafs = _loader->_visible_leafs.size();
                        for ( size_t i = 0; i < num_visible_leafs; i++ )
                        {
                                const int &leaf = _loader->_visible_leafs[i];
                                const pvector<int> &geoms = _loader->_leaf_geom_list[leaf];
                                size_t num_geoms = geoms.size();
                                
                                for ( size_t j = 0; j < num_geoms; j++ )
                                {
                                        wsp_geom_traverse_collector.start();
                                        const int &geom_id = geoms[j];
                                        wsp_geom_traverse_collector.stop();

                                        wsp_bitset_collector.start();
                                        if ( visited[geom_id] )
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
                                                visited[geom_id] = 1;
                                                wsp_ctest_collector.stop();
                                                continue;
                                        }
                                        wsp_ctest_collector.stop();

                                        CPT( RenderState ) state = gs.state;

                                        if ( _loader->get_wireframe() )
                                        {
                                                CPT( RenderAttrib ) wfattr = RenderModeAttrib::make( RenderModeAttrib::M_filled_wireframe,
                                                                                                     0.5, true, brush_wf_color );
                                                CPT( RenderState ) wfstate = RenderState::make( wfattr, 1 );
                                                state = state->compose( wfstate );
                                        }                                        

                                        wsp_make_cullableobject_collector.start();
                                        // Go ahead and render this worldspawn Geom.
                                        CullableObject *object = new CullableObject(
                                                std::move( gs.geom ), std::move( state ),
                                                internal_transform );
                                        wsp_make_cullableobject_collector.stop();
                                        wsp_record_collector.start();
                                        get_cull_handler()->record_object( object, this );
                                        _geoms_pcollector.add_level( 1 );
                                        wsp_record_collector.stop();

                                        // Make sure we don't render this geom again.
                                        visited[geom_id] = 1;
                                }
                                
                        }

                        _loader->_leaf_aabb_lock.release();

                        wsp_trav_collector.stop();
                }
                else if ( node->is_of_type( ModelRoot::get_class_type() ) )
                {
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
                                shinput = _loader->_amb_probe_mgr.update_node( node, data.get_net_transform( this ) );
                                ambientprobe_nodes_collector.add_level( 1 );
                        }
                }
                else if ( node->is_of_type( GeomNode::get_class_type() ) )
                {
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
                        BSPCullTraverserData next_data( data, shinput, children.get_child( i ) );
                        do_traverse( next_data );
                }
        }
        else
        {
                int i = node->get_first_visible_child();
                while ( i < num_children )
                {
                        BSPCullTraverserData next_data( data, shinput, children.get_child( i ) );
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
                loader->_amb_probe_mgr.xform_lights( trav->get_scene()->get_cs_world_transform() );

                BSPCullTraverser bsp_trav( trav, _loader );
                bsp_trav.local_object();
                bsp_trav.traverse_below( BSPCullTraverserData( data ) );
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