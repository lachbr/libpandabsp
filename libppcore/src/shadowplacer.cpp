#include "shadowplacer.h"
#include "pp_globals.h"

#include <collisionTraverser.h>
#include <collisionRay.h>
#include <collisionNode.h>

NotifyCategoryDef( shadowPlacer, "" );

static const PN_stdfloat floor_offset = 0.025;

ShadowPlacer::
ShadowPlacer() :
        _trav( NULL ),
        _is_active( false )
{
}

ShadowPlacer::
ShadowPlacer( CollisionTraverser *trav, const NodePath &shadow_np,
              const BitMask32 &floor_mask ) :
        _trav( trav ),
        _shadow_np( shadow_np ),
        _floor_mask( floor_mask ),
        _is_active( false )
{
}

void ShadowPlacer::
setup()
{
        PT( CollisionRay ) cray = new CollisionRay( 0.0, 0.0, 4000.0, 0.0, 0.0, -1.0 );

        shadowPlacer_cat.info()
                        << "setup()\n";

        PT( CollisionNode ) cnode = new CollisionNode( "shadowPlacer" );
        cnode->add_solid( cray );
        cnode->set_from_collide_mask( _floor_mask );
        cnode->set_into_collide_mask( BitMask32::all_off() );

        _ray_np = _shadow_np.attach_new_node( cnode );
        _ray_np.show();

        _lifter = new CollisionHandlerFloor;
        _lifter->set_offset( floor_offset );
        _lifter->set_reach( 4.0 );
        _lifter->add_collider( _ray_np, _shadow_np );
}

void ShadowPlacer::
cleanup()
{
        off();
        _ray_np.remove_node();
}

/**
 * Turn on the shadow placement. The shadow z position will
 * start being updated until a call to off() is made.
 */
void ShadowPlacer::
on()
{
        if ( _is_active )
        {
                return;
        }

        shadowPlacer_cat.info()
                        << "on()\n";

        nassertv( _trav != NULL );
        nassertv( !_trav->has_collider( _ray_np ) );

        _trav->add_collider( _ray_np, _lifter );

        _is_active = true;
}

/**
 * Turn off the shadow placement. The shadow will still be
 * there, but the z position will not be updated until a call
 * to on() is made.
 */
void ShadowPlacer::
off()
{
        if ( !_is_active )
        {
                return;
        }

        nassertv( _trav != NULL );
        nassertv( _trav->has_collider( _ray_np ) );
        nassertv( _trav->remove_collider( _ray_np ) );

        // Now that we have disabled collisions, make one more pass
        // right now to ensure we aren't standing in a wall.
        one_time_collide();

        _is_active = false;
}

void ShadowPlacer::
one_time_collide()
{
        CollisionTraverser temp_trav( "oneTimeCollide" );
        temp_trav.add_collider( _ray_np, _lifter );
        temp_trav.traverse( g_render );
}

void ShadowPlacer::
reset_to_origin()
{
        if ( !_shadow_np.is_empty() )
        {
                _shadow_np.set_pos( 0, 0, 0 );
        }
}