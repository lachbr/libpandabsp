#include "distributedobject.h"
#include "pp_globals.h"
#include "clientrepository.h"

#include <throw_event.h>

DClassDef( DistributedObject );

NotifyCategoryDef( distributedObject, "" );

DistributedObject::
DistributedObject() :
        DistributedObjectBase()
{
        _active_state = ES_new;
        _next_context = 0;
        _computed_auto_interests = false;
        _auto_interest_handle = 0;
}

bool DistributedObject::
is_local() const
{
        return g_cr->is_local_id( _do_id );
}

void DistributedObject::
b_set_location( DOID_TYPE parent, ZONEID_TYPE zone )
{
        d_set_location( parent, zone );
        set_location( parent, zone );
}

void DistributedObject::
d_set_location( DOID_TYPE parent, ZONEID_TYPE zone ) {}

void DistributedObject::
set_location( DOID_TYPE parent, ZONEID_TYPE zone )
{
        g_cr->store_object_location( this, parent, zone );
}

//vector<ZONEID_TYPE>
//_get_auto_interests(DistributedObjectBase *cls) {
//  vector<ZONEID_TYPE> result;
//  if (cls->_computed_auto_interests)
//}

vector<ZONEID_TYPE> DistributedObject::
get_auto_interests()
{
        vector<ZONEID_TYPE> autos;
        _auto_interests = autos;
        _computed_auto_interests = true;
        return autos;
}

void DistributedObject::
delete_or_delay()
{
        if ( 0 )
        {

        }
        else
        {
                disable_announce_and_delete();
        }
}

void DistributedObject::
disable_announce_and_delete()
{
        disable_and_announce();
        delete_do();
        destroy_do();
}

string DistributedObject::
get_disable_event() const
{
        return unique_name( "disable" );
}

void DistributedObject::
disable_and_announce()
{
        if ( _active_state != ES_disabled )
        {
                _active_state = ES_disabled;
                throw_event( get_disable_event() );
                disable();
                _active_state = ES_disabled;
                deactivate_do();
        }
}

void DistributedObject::
announce_generate()
{
}

void DistributedObject::
deactivate_do()
{
        if ( g_cr == NULL )
        {
                distributedObject_cat.warning()
                                << "cr is NULL in deactivate_do " << _do_id << "\n";
                return;
        }

        g_cr->close_auto_interests( this );
        set_location( 0, 0 );
        g_cr->delete_object_location( this, _parent_id, _zone_id );
}

void DistributedObject::
destroy_do()
{
        _dclass = NULL;
        // This is the end of the object. Do not try to access ANYTHING
        // from this object after here. All of your tasks and functions
        // should be cleaned up and nothing should still be happening
        // to the object. If you try to do anything with this object
        // after here, you'll be sad :(.
        delete this;
}

void DistributedObject::
disable()
{
}

bool DistributedObject::
is_disabled() const
{
        return _active_state == ES_disabled;
}

bool DistributedObject::
is_generated() const
{
        return _active_state == ES_generated;
}

void DistributedObject::
delete_do()
{
}

void DistributedObject::
generate()
{
        _active_state = ES_generating;
        g_cr->open_auto_interests( this );
}

void DistributedObject::
generate_init()
{
        _active_state = ES_generating;
}

void DistributedObject::
post_generate_message()
{
        if ( _active_state == ES_generated )
        {
                _active_state = ES_generated;
                throw_event( unique_name( "generate" ), EventParameter( ( double )get_do_id() ) );
        }
}

void DistributedObject::
update_required_fields( DCClassPP *dclass, DatagramIterator &di )
{
        dclass->receive_update_broadcast_required( this, di );
        announce_generate();
        post_generate_message();
}

void DistributedObject::
update_all_required_fields( DCClassPP *dclass, DatagramIterator &di )
{
        dclass->receive_update_all_required( this, di );
        announce_generate();
        post_generate_message();
}

void DistributedObject::
update_required_other_fields( DCClassPP *dclass, DatagramIterator &di )
{
        dclass->receive_update_broadcast_required( this, di );
        announce_generate();
        post_generate_message();
        dclass->receive_update_other( this, di );
}

void DistributedObject::
send_disable_msg()
{
        // unused apparently
}

void DistributedObject::
send_delete_msg()
{
        // Unused apparently
}

void DistributedObject::
add_interest( ZONEID_TYPE zone, const string &note, const string &event )
{
        vector<ZONEID_TYPE> zones;
        zones.push_back( zone );
        g_cr->add_interest( get_do_id(), zones, note, event );
}

void DistributedObject::
remove_interest( int handle, const string &event )
{
        g_cr->remove_interest( handle, event );
}

bool DistributedObject::
is_grid_parent() const
{
        return false;
}