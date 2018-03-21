#include "distributedobject_base.h"
#include "pp_globals.h"
#include "pp_dcbase.h"
#include "clientrepository.h"

NotifyCategoryDef( distributedObjectBase, "" );

DClassDef( DistributedObjectBase );

DistributedObjectBase::
DistributedObjectBase() :
        _dclass( NULL ),
        _parent_obj( NULL )
{
        _parent_id = 0;
        _zone_id = 0;
        _do_id = 0;
}

DistributedObjectBase::
~DistributedObjectBase()
{
}

void DistributedObjectBase::
set_do_id( DOID_TYPE doid )
{
        _do_id = doid;
}

DOID_TYPE DistributedObjectBase::
get_do_id() const
{
        return _do_id;
}

void DistributedObjectBase::
set_parent_id( DOID_TYPE parent )
{
        _parent_id = parent;
}

DOID_TYPE DistributedObjectBase::
get_parent_id() const
{
        return _parent_id;
}

void DistributedObjectBase::
set_zone_id( ZONEID_TYPE zone )
{
        _zone_id = zone;
}

ZONEID_TYPE DistributedObjectBase::
get_zone_id() const
{
        return _zone_id;
}

void DistributedObjectBase::
set_dclass( DCClassPP *dclass )
{
        _dclass = dclass;
}

DCClassPP *DistributedObjectBase::
get_dclass() const
{
        return _dclass;
}

void DistributedObjectBase::
set_location( DOID_TYPE parent, ZONEID_TYPE zone )
{
        // Must be overridden.
}

void DistributedObjectBase::
set_location_field( DCFuncArgs )
{
        DOID_TYPE parent_id = packer.unpack_uint();
        ZONEID_TYPE zone_id = packer.unpack_uint();
        ( ( DistributedObjectBase * )data )->set_location( parent_id, zone_id );
}

uint32_t DistributedObjectBase::
get_location_pid() const
{
        return _parent_id;
}

uint32_t DistributedObjectBase::
get_location_zid() const
{
        return _zone_id;
}

string DistributedObjectBase::
task_name( const string &task_name ) const
{
        stringstream ss;
        ss << task_name << "-" << _do_id;
        return ss.str();
}

string DistributedObjectBase::
unique_name( const string &id_string ) const
{
        stringstream ss;
        ss << id_string << "-" << _do_id;
        return ss.str();
}

bool DistributedObjectBase::
has_parenting_rules() const
{
        return _dclass->get_dclass()->get_field_by_name( "set_parenting_rules" ) != NULL;
}

void DistributedObjectBase::
handle_child_arrive( DistributedObjectBase *childobj, ZONEID_TYPE zoneid )
{
}

void DistributedObjectBase::
handle_child_arrive_zone( DistributedObjectBase *childobj, ZONEID_TYPE zoneid )
{
}

void DistributedObjectBase::
handle_child_leave( DistributedObjectBase *childobj, ZONEID_TYPE zoneid )
{
}

void DistributedObjectBase::
handle_child_leave_zone( DistributedObjectBase *childobj, ZONEID_TYPE zoneid )
{
}

DistributedObjectBase *DistributedObjectBase::
get_parent_obj()
{
        if ( _parent_id == 0 )
        {
                return NULL;
        }
        return g_cr->get_do( _parent_id );
}

void DistributedObjectBase::
generate_init()
{
}

void DistributedObjectBase::
generate()
{
        // Override if needed.
}

void DistributedObjectBase::
announce_generate()
{
}

void DistributedObjectBase::
disable()
{
}

void DistributedObjectBase::
delete_do()
{
        // Overwrite this to handle cleanup right before this object
        // gets deleted.
}

void DistributedObjectBase::
delete_or_delay()
{
}

void DistributedObjectBase::
update_required_fields( DCClassPP *dclass, DatagramIterator &di )
{
}

void DistributedObjectBase::
update_required_other_fields( DCClassPP *dclass, DatagramIterator &di )
{
}

void DistributedObjectBase::
update_all_required_fields( DCClassPP *dclass, DatagramIterator &di )
{
}