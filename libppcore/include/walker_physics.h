#ifndef WALKER_PHYSICS_H
#define WALKER_PHYSICS_H

#include "walker.h"

#include <collisionHandlerQueue.h>
#include <actorNode.h>
#include <linearFrictionForce.h>
#include <linearVectorForce.h>

class EXPCL_PANDAPLUS PhysicsWalker : public BaseWalker
{
public:
        PhysicsWalker( PN_stdfloat gravity = -32.1674, PN_stdfloat standable_ground = 0.707,
                       PN_stdfloat hard_landing_force = 16.0 );

        virtual void setup_ray( const BitMask32 &bitmask );
        virtual void set_avatar( const NodePath &avatar );
        virtual PN_stdfloat get_airborne_height() const;

        virtual void initialize_collisions( CollisionTraverser *ctrav, const NodePath &avatar_np,
                                            PN_stdfloat avatar_radius, PN_stdfloat flr_offset,
                                            PN_stdfloat reach );

        void setup_physics();

        virtual void set_collisions_active( bool active );

        virtual void handle_avatar_controls();

        virtual void one_time_collide();

        virtual void reset();

        virtual LVector3 get_velocity() const;

        NodePath get_physics_actor() const;

private:
        PT( CollisionHandlerQueue ) _cray_queue;

        // Physics stuff
        PT( ActorNode ) _actor;
        NodePath _physics_actor_np;
};

#endif // WALKER_PHYSICS_H