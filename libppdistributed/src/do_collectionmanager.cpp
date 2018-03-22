#include "do_collectionmanager.h"
#include "pp_globals.h"
#include "distributedobject_base.h"

#include <datagramIterator.h>

DoCollectionManager::DoCollectionManager()
{
}

DistributedObjectBase *DoCollectionManager::get_do( DOID_TYPE doid )
{
        if ( _do_id_2_do.find( doid ) != _do_id_2_do.end() )
        {
                return _do_id_2_do[doid];
        }
        return nullptr;
}

int DoCollectionManager::get_game_do_id() const
{
        return game_globals_id;
}

DistributedObjectBase *DoCollectionManager::get_owner_view( DOID_TYPE doid )
{
        nassertr( dc_has_owner_view, nullptr );
        return _do_id_2_owner_view[doid];
}

DoCollectionManager::DoMap &DoCollectionManager::get_do_table( bool ownerview )
{
        if ( ownerview )
        {
                return _do_id_2_owner_view;
        }
        return _do_id_2_do;
}

DistributedObjectBase *DoCollectionManager::do_find( const string &str )
{
        for ( DoMap::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                if ( itr->second->get_type().get_name().compare( str ) == 0 )
                {
                        return itr->second;
                }
        }

        return (DistributedObjectBase *)nullptr;
}

vector<DistributedObjectBase *> DoCollectionManager::do_find_all( const string &str )
{
        vector<DistributedObjectBase *> result;

        for ( DoMap::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                if ( itr->second->get_type().get_name().compare( str ) == 0 )
                {
                        result.push_back( itr->second );
                }
        }

        return result;
}

bool DoCollectionManager::has_owner_view_do_id( DOID_TYPE doid ) const
{
        return _do_id_2_owner_view.find( doid ) != _do_id_2_owner_view.end();
}

vector<DistributedObjectBase *> DoCollectionManager::get_all_of_type( TypeHandle &handle )
{
        vector<DistributedObjectBase *> result;

        for ( DoMap::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                if ( itr->second->get_type() == handle )
                {
                        result.push_back( itr->second );
                }
        }

        return result;
}

void DoCollectionManager::handle_object_location( DatagramIterator &di )
{
        DOID_TYPE doid = di.get_uint32();
        DOID_TYPE parentid = di.get_uint32();
        ZONEID_TYPE zoneid = di.get_uint32();

        DistributedObjectBase *dobj = get_do( doid );
        if ( dobj != nullptr )
        {
                dobj->set_location( parentid, zoneid );
        }
        else
        {
                cerr << "handleObjectLocation: asked to update non-existent obj: "
                        << doid << endl;
        }
}

void DoCollectionManager::store_object_location( DistributedObjectBase *dobj, DOID_TYPE parent, ZONEID_TYPE zone )
{
        DOID_TYPE oldpid = dobj->get_zone_id();
        ZONEID_TYPE oldzid = dobj->get_zone_id();
        if ( oldpid != parent )
        {
                DistributedObjectBase *oldpar = get_do( oldpid );
                if ( oldpar != nullptr )
                {
                        oldpar->handle_child_leave( dobj, oldzid );
                }
                delete_object_location( dobj, oldpid, oldzid );
        }
        else if ( oldzid != zone )
        {
                DistributedObjectBase *oldpar = get_do( oldpid );
                if ( oldpar != nullptr )
                {
                        oldpar->handle_child_leave_zone( dobj, oldzid );
                }
                delete_object_location( dobj, oldpid, oldzid );
        }
        else
        {
                return;
        }

        _do_hierarchy.store_object_location( dobj, parent, zone );
        dobj->set_parent_id( parent );
        dobj->set_zone_id( zone );

        if ( oldpid != parent )
        {
                DistributedObjectBase *par = get_do( parent );
                if ( par != nullptr )
                {
                        par->handle_child_arrive( dobj, zone );
                }
        }
        else if ( oldzid != zone )
        {
                DistributedObjectBase *par = get_do( parent );
                if ( par != nullptr )
                {
                        par->handle_child_arrive_zone( dobj, zone );
                }
        }
}

void DoCollectionManager::delete_object_location( DistributedObjectBase *dobj, DOID_TYPE parent, ZONEID_TYPE zone )
{

        _do_hierarchy.delete_object_location( dobj, parent, zone );
}

void DoCollectionManager::add_do_to_tables( DistributedObjectBase *dobj, DOID_TYPE parent, ZONEID_TYPE zone, bool ownerview )
{
        DoMap &dotable = get_do_table( ownerview );

        if ( dotable.find( dobj->get_do_id() ) != dotable.end() )
        {
                cerr << "do_id " << dobj->get_do_id() << " already in owner view or object table." << endl;
                return;
        }

        dotable[dobj->get_do_id()] = dobj;

        if ( !ownerview )
        {
                store_object_location( dobj, parent, zone );
        }
}

void DoCollectionManager::remove_do_from_tables( DistributedObjectBase *dobj )
{
        DOID_TYPE parent = dobj->get_location_pid();
        ZONEID_TYPE zone = dobj->get_location_zid();
        DOID_TYPE oldpid = parent;
        ZONEID_TYPE oldzid = zone;
        DistributedObjectBase *oldpar = get_do( oldpid );
        if ( oldpar != nullptr )
        {
                oldpar->handle_child_leave( dobj, oldzid );
        }
        delete_object_location( dobj, dobj->get_parent_id(), dobj->get_zone_id() );
        if ( _do_id_2_do.find( dobj->get_do_id() ) != _do_id_2_do.end() )
        {
                _do_id_2_do.erase( _do_id_2_do.find( dobj->get_do_id() ) );
        }
}

DoCollectionManager::DoMap DoCollectionManager::get_objects_in_zone( DOID_TYPE parentid, ZONEID_TYPE zoneid )
{
        DoMap result;
        for ( DoMap::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                DistributedObjectBase *dobj = itr->second;
                if ( dobj->get_parent_id() == parentid && dobj->get_zone_id() == zoneid )
                {
                        result[dobj->get_do_id()] = dobj;
                }
        }

        return result;
}

DoCollectionManager::DoMap DoCollectionManager::get_objects_of_class_in_zone( DOID_TYPE parentid, ZONEID_TYPE zoneid,
                                                                              TypeHandle &handle )
{
        DoMap result;
        for ( DoMap::iterator itr = _do_id_2_do.begin(); itr != _do_id_2_do.end(); ++itr )
        {
                DistributedObjectBase *dobj = itr->second;
                if ( dobj->get_parent_id() == parentid &&
                     dobj->get_zone_id() == zoneid &&
                     dobj->get_type() == handle )
                {
                        result[dobj->get_do_id()] = dobj;
                }
        }

        return result;
}