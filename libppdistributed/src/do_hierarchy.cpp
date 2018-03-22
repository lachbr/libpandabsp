#include "do_hierarchy.h"
#include "distributedobject_base.h"

DoHierarchy::DoHierarchy()
{
}

bool DoHierarchy::is_empty() const
{
        return ( _table.size() == 0 && _all_do_ids.size() == 0 );
}

size_t DoHierarchy::get_length() const
{
        return _all_do_ids.size();
}

void DoHierarchy::clear()
{
        _table.clear();
        _all_do_ids.clear();
}

void DoHierarchy::store_object_location( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zone )
{
        DOID_TYPE doid = dobj->get_do_id();
        if ( find( _all_do_ids.begin(), _all_do_ids.end(), doid ) != _all_do_ids.end() )
        {
                cerr << "storeObjectLocation(" << dobj->get_type().get_name() << " " << doid
                        << ") already in _all_doids; duplicate generate()? or didn't cleanup previous instance of DO?"
                        << endl;
                return;
        }

        DoIdVec zonedoset;
        zonedoset.push_back( doid );
        Zone2ChildId parentzonedict;
        parentzonedict[zone] = zonedoset;
        _all_do_ids.push_back( doid );
        _table[parentid] = parentzonedict;
}

void DoHierarchy::delete_object_location( DistributedObjectBase *dobj, DOID_TYPE parentid, ZONEID_TYPE zone )
{
        DOID_TYPE doid = dobj->get_do_id();
        if ( find( _all_do_ids.begin(), _all_do_ids.end(), doid ) != _all_do_ids.end() )
        {
                cerr << "storeObjectLocation(" << dobj->get_type().get_name() << " " << doid
                        << ") already in _all_doids; duplicate generate()? or didn't cleanup previous instance of DO?"
                        << endl;
                return;
        }

        // This is a mess:
        if ( _table.find( parentid ) != _table.end() )
        {
                Zone2ChildId &parentzonedict = _table[parentid];
                if ( parentzonedict.find( zone ) != parentzonedict.end() )
                {
                        DoIdVec &zonedoset = parentzonedict[zone];
                        if ( find( zonedoset.begin(), zonedoset.end(), doid ) != zonedoset.end() )
                        {
                                zonedoset.erase( find( zonedoset.begin(), zonedoset.end(), doid ) );
                                _all_do_ids.erase( find( _all_do_ids.begin(), _all_do_ids.end(), doid ) );
                                if ( zonedoset.size() == 0 )
                                {
                                        parentzonedict.erase( parentzonedict.find( zone ) );
                                        if ( parentzonedict.size() == 0 )
                                        {
                                                _table.erase( _table.find( parentid ) );
                                        }
                                }
                        }
                        else
                        {
                                cerr << "deleteObjectLocation: objId: " << doid << " not found" << endl;
                                return;
                        }
                }
                else
                {
                        cerr << "deleteObjectLocation: zone_id: " << zone << " not found" << endl;
                        return;
                }
        }
        else
        {
                cerr << "deleteObjectLocation: parentId: " << parentid << " not found" << endl;
                return;
        }
}