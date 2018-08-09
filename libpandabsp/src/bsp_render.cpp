#include "bsp_render.h"
#include "bsploader.h"

#include <depthOffsetAttrib.h>
#include <cullableObject.h>
#include <cullHandler.h>
#include <characterJointEffect.h>

//#define PVSTEST_ACCURATE

TypeDef( BSPCullTraverser );

BSPCullTraverser::BSPCullTraverser( CullTraverser *trav ) :
        CullTraverser( *trav )
{
}

bool BSPCullTraverser::is_in_view( CullTraverserData &data )
{
        BSPLoader *loader = BSPLoader::get_global_ptr();
        // First test view frustum.
        if ( !data.is_in_view( get_camera_mask() ) )
        {
                return false;
        }

        // View frustum test passed.
        // Now test against PVS (AABBs of all potentially visible leafs).

#ifdef PVSTEST_ACCURATE
                LPoint3 mins, maxs;
                data.get_node_path().calc_tight_bounds( mins, maxs );
                PT( BoundingBox ) bbox = new BoundingBox( mins, maxs );
#else
        CPT( GeometricBoundingVolume ) bbox = loader->make_net_bounds(
                data.get_net_transform( this ),
                data.node()->get_bounds()->as_geometric_bounding_volume() );
#endif
        bool ret = loader->pvs_bounds_test( bbox );
        return ret;
}

INLINE void BSPCullTraverser::add_geomnode_for_draw( GeomNode *node, CullTraverserData &data )
{
        BSPLoader *loader = BSPLoader::get_global_ptr();

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
                        CPT( BoundingVolume ) geom_volume = geom->get_bounds();
                        const GeometricBoundingVolume *geom_gbv =
                                DCAST( GeometricBoundingVolume, geom_volume );

                        if ( data._view_frustum != nullptr )
                        {
                                int result = data._view_frustum->contains( geom_gbv );
                                if ( result == BoundingVolume::IF_no_intersection )
                                {
                                        // Cull this Geom.
                                        continue;
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
                                        continue;
                                }
                        }

                        CPT( GeometricBoundingVolume ) net_geom_volume =
                                loader->make_net_bounds( net_transform, geom_gbv );
                                
                        // Test geom bounds against visible leaf bounding boxes.
                        if ( !loader->pvs_bounds_test( net_geom_volume ) )
                        {
                                // Didn't intersect any, cull.
                                continue;
                        }
                }

                CullableObject *object =
                        new CullableObject( std::move( geom ), std::move( state ), internal_transform );
                get_cull_handler()->record_object( object, this );
                _geoms_pcollector.add_level( 1 );
        }
}

void BSPCullTraverser::traverse_below( CullTraverserData &data )
{
        _nodes_pcollector.add_level( 1 );
        PandaNodePipelineReader *node_reader = data.node_reader();
        PandaNode *node = data.node();

        // Add the current node to be drawn during the Draw stage.
        if ( !data.is_this_node_hidden( get_camera_mask() ) )
        {
                if ( node->is_of_type( GeomNode::get_class_type() ) )
                {
                        // HACKHACK:
                        // Needed to test the individual Geoms against the PVS.
                        add_geomnode_for_draw( DCAST( GeomNode, node ), data );
                }
                else
                {
                        node->add_for_draw( this, data );
                }

                // Check for a decal effect.
                const RenderEffects *node_effects = node_reader->get_effects();
                if ( node_effects->has_decal() )
                {
                        // If we *are* implementing decals with DepthOffsetAttribs, apply it
                        // now, so that each child of this node gets offset by a tiny amount.
                        data._state = data._state->compose( get_depth_offset_state() );
#ifndef NDEBUG
                        // This is just a sanity check message.
                        if ( !node->is_geom_node() )
                        {
                                pgraph_cat.error()
                                        << "DecalEffect applied to " << *node << ", not a GeomNode.\n";
                        }
#endif
                }

                // Update the node's ambient probe stuff:
                // For a node to be influenced by dynamic lights and ambient cubes
                // it must be a ModelNode with non-identity local transform and no
                // CharacterJointEffect (meaning it's not an exposed joint).
                if ( node->is_of_type( ModelNode::get_class_type() ) &&
                     !node->get_transform()->is_identity() &&
                     !node->has_effect( CharacterJointEffect::get_class_type() ) )
                {
                        BSPLoader::get_global_ptr()->_amb_probe_mgr.update_node( node );
                }
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

BSPRender::BSPRender( const std::string &name ) :
        PandaNode( name )
{
        set_cull_callback();

        set_attrib( AmbientProbeManager::get_identity_shattr() );
}

bool BSPRender::cull_callback( CullTraverser *trav, CullTraverserData &data )
{
        BSPLoader *loader = BSPLoader::get_global_ptr();
        if ( loader->has_visibility() )
        {
                BSPCullTraverser bsp_trav( trav );
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