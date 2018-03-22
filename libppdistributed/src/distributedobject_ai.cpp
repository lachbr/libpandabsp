#include "distributedobject_ai.h"
#include "pp_globals.h"
#include "clientrepository.h"

#include <throw_event.h>

DClassDef( DistributedObjectAI );

NotifyCategoryDef( distributedObjectAI, "" );

ZONEID_TYPE DistributedObjectAI::_quiet_zone = 1;

DistributedObjectAI::DistributedObjectAI() :
        DistributedObjectBase()
{
        _account_name = "";
        _next_barrier_context = 0;
        _generates = 0;
        _generated = false;
        _requested_delete = false;
        _prealloc_do_id = false;
        _last_non_quiet_zone = 0;
}

string DistributedObjectAI::get_delete_event() const
{
        stringstream ss;
        ss << "distObjDelete-" << _do_id;
        return ss.str();
}

void DistributedObjectAI::send_delete_event()
{
        string del_evt = get_delete_event();
        throw_event( del_evt );
}

/**
* This method exists only to mirror the similar method on
* DistributedObject. AI objects aren't cacheable.
*/
bool DistributedObjectAI::get_cacheable() const
{
        return false;
}

/**
* This method exists only to mirror the similar method on
* DistributedObject. AI objects dont have delayDelete, they
* just get deleted immediately.
*/
void DistributedObjectAI::delete_or_delay()
{
        delete_do();
}

int DistributedObjectAI::get_delay_delete_count() const
{
        return 0;
}

void DistributedObjectAI::delete_do()
{
        _generates--;

        if ( _generates < 0 )
        {
                distributedObjectAI_cat.debug()
                        << "DistributedObjectAI::delete_do() called more times then generate()\n";
        }

        if ( _generates == 0 )
        {
                _requested_delete = true;

                //release_zone_data();

                // clean up barriers.

                _parent_id = 0;
                _zone_id = 0;
                _generated = false;

                // This is the end of the object. Do not try to access ANYTHING
                // from this object after here. All of your tasks and functions
                // should be cleaned up and nothing should still be happening
                // to the object. If you try to do anything with this object
                // after here, you'll be sad :(.
                delete this;
        }
}

bool DistributedObjectAI::is_deleted() const
{
        return _generated;
}

bool DistributedObjectAI::is_generated() const
{
        return _generated;
}


void DistributedObjectAI::announce_generate()
{
        _generated = true;
}

void DistributedObjectAI::add_interest( ZONEID_TYPE zone, string note, string event )
{
        vector<ZONEID_TYPE> zones;
        zones.push_back( zone );
        g_air->add_interest( _do_id, zones, note, event );
}

void DistributedObjectAI::b_set_location( DOID_TYPE parent, ZONEID_TYPE zone )
{
        d_set_location( parent, zone );
        set_location( parent, zone );
}

void DistributedObjectAI::d_set_location( DOID_TYPE parent, ZONEID_TYPE zone )
{
        g_air->send_set_location( _do_id, parent, zone );
}

void DistributedObjectAI::set_location( DOID_TYPE parent, ZONEID_TYPE zone )
{
        // prevent duplicate set_locations being called
        if ( ( _parent_id == parent ) && ( _zone_id = zone ) )
        {
                return;
        }

        DOID_TYPE oldparent = _parent_id;
        ZONEID_TYPE oldzone = _zone_id;
        g_air->store_object_location( this, parent, zone );
        if ( ( oldparent != parent ) ||
                ( oldzone != zone ) )
        {

                //release_zone_data();
                // this may cause problems-------------------------
                throw_event( get_zone_change_event(), EventParameter( (int)zone ), EventParameter( (int)oldzone ) );

                // if we are not going into the quiet zone, send a 'logical' zone
                // change message
                if ( zone != _quiet_zone )
                {
                        ZONEID_TYPE last_logical_zone = oldzone;
                        if ( oldzone == _quiet_zone )
                        {
                                last_logical_zone = _last_non_quiet_zone;
                        }
                        handle_logical_zone_change( zone, last_logical_zone );
                        _last_non_quiet_zone = zone;
                }
        }
}

void DistributedObjectAI::get_location_field( DCFuncArgs )
{
        DistributedObjectAI *obj = (DistributedObjectAI *)data;
        // pack the parent id
        packer.pack_uint( obj->get_location_pid() );
        // pack the zone id
        packer.pack_uint( obj->get_location_zid() );
}

void DistributedObjectAI::post_generate_message()
{
        _generated = true;
        throw_event( unique_name( "generate" ), EventParameter( (double)get_do_id() ) );
}

void DistributedObjectAI::update_required_fields( DCClassPP *dclass, DatagramIterator &di )
{
        dclass->receive_update_broadcast_required( this, di );
        announce_generate();
        post_generate_message();
}

void DistributedObjectAI::update_all_required_fields( DCClassPP *dclass, DatagramIterator &di )
{
        dclass->receive_update_all_required( this, di );
        announce_generate();
        post_generate_message();
}

void DistributedObjectAI::update_required_other_fields( DCClassPP *dclass, DatagramIterator &di )
{
        dclass->receive_update_broadcast_required( this, di );
        announce_generate();
        post_generate_message();

        dclass->receive_update_other( this, di );
}

void DistributedObjectAI::update_all_required_other_fields( DCClassPP *dclass, DatagramIterator &di )
{
        dclass->receive_update_all_required( this, di );
        announce_generate();
        post_generate_message();

        dclass->receive_update_other( this, di );
}

string DistributedObjectAI::get_zone_change_event() const
{
        return get_zone_change_event_static( _do_id );
}

string DistributedObjectAI::get_logical_zone_change_event() const
{
        return get_logical_zone_change_event_static( _do_id );
}

string DistributedObjectAI::get_zone_change_event_static( DOID_TYPE doid )
{
        stringstream ss;
        ss << "DOChangeZone-" << doid;
        return ss.str();
}

string DistributedObjectAI::get_logical_zone_change_event_static( DOID_TYPE doid )
{
        stringstream ss;
        ss << "DOLogicalChangeZone-" << doid;
        return ss.str();
}

void DistributedObjectAI::handle_logical_zone_change( ZONEID_TYPE new_zone, ZONEID_TYPE old_zone )
{
        throw_event( get_logical_zone_change_event(), EventParameter( (int)new_zone ),
                     EventParameter( (int)old_zone ) );
}

CHANNEL_TYPE DistributedObjectAI::get_puppet_connection_channel( DOID_TYPE do_id )
{
        return do_id + ( (CHANNEL_TYPE)1 << 32 );
}

CHANNEL_TYPE DistributedObjectAI::get_account_connection_channel( DOID_TYPE do_id )
{
        return do_id + ( (CHANNEL_TYPE)3 << 32 );
}

DOID_TYPE DistributedObjectAI::get_account_id_from_channel_code( CHANNEL_TYPE chan )
{
        return chan >> 32;
}

DOID_TYPE DistributedObjectAI::get_avatar_id_from_channel_code( CHANNEL_TYPE chan )
{
        return chan & 0xffffffff;
}

#ifdef ASTRON_IMPL

void DistributedObjectAI::pre_allocate_do_id()
{
        _do_id = ( (AIRepositoryBase *)g_air )->allocate_channel();
        _prealloc_do_id = true;
}

void DistributedObjectAI::generate_with_required( ZONEID_TYPE zone )
{
        if ( _prealloc_do_id )
        {
                _prealloc_do_id = false;
                generate_with_required_and_id(
                        _do_id, _parent_id, zone );
        }

        if ( _dclass == (DCClassPP *)nullptr )
        {
                _dclass = g_air->get_dclass_by_name( get_type().get_name() );
        }

        AIRepositoryBase *air = (AIRepositoryBase *)g_air;

        CHANNEL_TYPE par = air->get_district_id();
        air->generate_with_required( this, par, zone );
        generate();
        announce_generate();
        post_generate_message();
}

void DistributedObjectAI::generate_with_required_and_id( DOID_TYPE doid, DOID_TYPE parent, ZONEID_TYPE zone )
{
        if ( _prealloc_do_id )
        {
                nassertv( doid == _do_id );
                _prealloc_do_id = false;
        }

        if ( _dclass == (DCClassPP *)nullptr )
        {
                _dclass = g_air->get_dclass_by_name( get_type().get_name() );
                int ps = _dclass->get_dclass()->get_num_parents();
                for ( int i = 0; i < ps; i++ )
                {
                        DCClass *p = _dclass->get_dclass()->get_parent( i );
                }
        }

        ( (AIRepositoryBase *)g_air )->generate_with_required_and_id( this, doid, parent, zone );
        generate();
        announce_generate();
        post_generate_message();
}


CHANNEL_TYPE DistributedObjectAI::generate_target_channel() const
{
        return ( (AIRepositoryBase *)g_air )->get_server_id();
}

void DistributedObjectAI::send_generate_with_required( DOID_TYPE parent, ZONEID_TYPE zone )
{
        Datagram dg = _dclass->ai_format_generate(
                this, _do_id, parent, zone,
                generate_target_channel(),
                ( (AIRepositoryBase *)g_air )->get_our_channel() );
        g_air->send( dg );
}

#endif // ASTRON_IMPL