#include "shadowplacer.h"
#include "pp_globals.h"

#include <collisionTraverser.h>
#include <collisionRay.h>
#include <collisionNode.h>

NotifyCategoryDef( shadowPlacer, "" );

static const PN_stdfloat floor_offset = 0.025;

ShadowPlacer::ShadowPlacer() :
        _trav( nullptr ),
        _is_active( false ),
        _update_task( nullptr )
{
}

ShadowPlacer::ShadowPlacer( CollisionTraverser *trav, const NodePath &shadow_np,
                            const BitMask32 &floor_mask ) :
        _trav( trav ),
        _lifter( nullptr ),
        _shadow_np( shadow_np ),
        _floor_mask( floor_mask ),
        _is_active( false ),
        _update_task( nullptr )
{
}

void ShadowPlacer::setup()
{
        PT( CollisionRay ) cray = new CollisionRay( 0.0, 0.0, 4000.0, 0.0, 0.0, -1.0 );

        PT( CollisionNode ) cnode = new CollisionNode( "shadowPlacer" );
        cnode->add_solid( cray );
        cnode->set_from_collide_mask( _floor_mask );
        cnode->set_into_collide_mask( BitMask32::all_off() );

        _ray_np = NodePath( cnode );

        _lifter = new CollisionHandlerQueue;

        _update_task = new GenericAsyncTask( "update_task", update_task, this );
}

AsyncTask::DoneStatus ShadowPlacer::update_task( GenericAsyncTask *task, void *data )
{
        ShadowPlacer *self = (ShadowPlacer *)data;
        
        if ( self->_lifter->get_num_entries() != 0 )
        {
                self->_lifter->sort_entries();

                CollisionEntry *entry = self->_lifter->get_entry( 0 );
                self->_shadow_np.set_z( g_render, entry->get_surface_point( g_render ).get_z() );
        }

        return AsyncTask::DS_cont;
}

void ShadowPlacer::cleanup()
{
        off();
        _ray_np.remove_node();
}

/**
* Turn on the shadow placement. The shadow z position will
* start being updated until a call to off() is made.
*/
void ShadowPlacer::on()
{
        if ( _is_active )
        {
                return;
        }

        nassertv( _trav != nullptr );
        nassertv( !_trav->has_collider( _ray_np ) );

        _ray_np.reparent_to( _shadow_np.get_parent() );
        _trav->add_collider( _ray_np, _lifter );

        AsyncTaskManager::get_global_ptr()->add( _update_task );

        _is_active = true;
}

/**
* Turn off the shadow placement. The shadow will still be
* there, but the z position will not be updated until a call
* to on() is made.
*/
void ShadowPlacer::off()
{
        if ( !_is_active )
        {
                return;
        }

        _update_task->remove();

        nassertv( _trav != nullptr );
        nassertv( _trav->has_collider( _ray_np ) );
        nassertv( _trav->remove_collider( _ray_np ) );

        // Now that we have disabled collisions, make one more pass
        // right now to ensure we aren't standing in a wall.
        one_time_collide();
        _ray_np.detach_node();

        _is_active = false;
}

void ShadowPlacer::one_time_collide()
{
        CollisionTraverser temp_trav( "oneTimeCollide" );
        temp_trav.add_collider( _ray_np, _lifter );
        temp_trav.traverse( g_render );
}

void ShadowPlacer::reset_to_origin()
{
        if ( !_shadow_np.is_empty() )
        {
                _shadow_np.set_pos( 0, 0, 0 );
        }
}