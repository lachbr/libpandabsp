#include "do_interestmanager.h"
#include "distributedobject.h"

#include <throw_event.h>

InterestState::InterestState( const string &desc, InterestState::State state, int context,
                              const string &event, DOID_TYPE parentid, vector<ZONEID_TYPE> zones,
                              int &event_counter, bool isauto ) :
        _event_counter( event_counter )
{

        _desc = desc;
        _state = state;
        _context = context;
        if ( event.length() > 0 )
        {
                add_event( event );
        }
        _parent_id = parentid;
        _zones = zones;
        _auto = isauto;
}

void InterestState::set_state( State state )
{
        _state = state;
}

InterestState::State InterestState::get_state() const
{
        return _state;
}

void InterestState::set_context_id( int context )
{
        _context = context;
}

int InterestState::get_context_id() const
{
        return _context;
}

void InterestState::set_desc( const string &desc )
{
        _desc = desc;
}

string InterestState::get_desc() const
{
        return _desc;
}

void InterestState::set_parent_id( DOID_TYPE parentid )
{
        _parent_id = parentid;
}

DOID_TYPE InterestState::get_parent_id() const
{
        return _parent_id;
}

void InterestState::set_zones( vector<ZONEID_TYPE> zones )
{
        _zones = zones;
}

void InterestState::add_event( const string &event )
{
        _events.push_back( event );
        _event_counter++;
}

vector<string> InterestState::get_events() const
{
        return _events;
}

void InterestState::clear_events()
{
        _event_counter -= _events.size();
        nassertv( _event_counter >= 0 );
        _events.clear();
}

void InterestState::send_events()
{
        for ( size_t i = 0; i < _events.size(); i++ )
        {
                throw_event( _events[i] );
        }
        clear_events();
}

bool InterestState::is_pending_delete() const
{
        return _state == IS_pending_del;
}

////////////////////////////////////////////////////////////////////

int DoInterestManager::_context_id_mask = 0x3FFFFFFF;
int DoInterestManager::_context_id_serial_num = 100;
int DoInterestManager::_handle_mask = 0x7FFF;
int DoInterestManager::_handle_serial_num = 0;
pmap<int, PT( InterestState )> DoInterestManager::_interests;

SerialNumGen DoInterestManager::_serial_gen;
int DoInterestManager::_serial_num = PPUtils::serial_num();

NotifyCategoryDef( doInterestManager, "" );

DoInterestManager::DoInterestManager()
{
        _add_interest_event = PPUtils::unique_name( "DoInterestManager-Add" );
        _remove_interest_event = PPUtils::unique_name( "DoInterestManager-Remove" );
        _no_new_interests = false;
        _event_counter = 0;
}

string DoInterestManager::get_anonymous_event( const string &desc ) const
{
        stringstream ss;
        ss << "anonymous-" << desc << "-" << _serial_gen.next();
        return ss.str();
}

void DoInterestManager::set_no_new_interests( bool flag )
{
        _no_new_interests = flag;
}

bool DoInterestManager::no_new_interests() const
{
        return _no_new_interests;
}

////setallinterestscompletecallback

string DoInterestManager::get_all_interests_complete_callback() const
{
        stringstream ss;
        ss << "allInterestsComplete-" << _serial_num;
        return ss.str();
}

void DoInterestManager::reset_interest_state_for_connection_loss()
{
        _interests.clear();
        _event_counter = 0;
}

bool DoInterestManager::is_valid_interest_handle( int handle )
{
        return _interests.find( handle ) != _interests.end();
}

void DoInterestManager::update_interest_description( int handle, const string &desc )
{
        PT( InterestState ) istate = _interests[handle];
        istate->set_desc( desc );
}

int DoInterestManager::add_interest( DOID_TYPE parentid, vector<ZONEID_TYPE> zones,
                                     const string &desc, const string &event )
{
        int handle = get_next_handle();
        if ( _no_new_interests )
        {
                doInterestManager_cat.warning()
                        << "addInterest: addingInterests on delete: " << handle << "\n";
                return -1;
        }

        if ( parentid != game_globals_id )
        {
                DistributedObjectBase *par = get_do( parentid );
                if ( par == nullptr )
                {
                        return -1;
                }
                else
                {
                        if ( !par->has_parenting_rules() )
                        {
                                return -1;
                        }
                }
        }

        int contextid = 0;
        if ( event.length() > 0 )
        {
                contextid = get_next_context_id();
        }

        _interests[handle] = new InterestState( desc, InterestState::IS_active, contextid,
                                                event, parentid, zones, _event_counter );
        send_add_interest( handle, contextid, parentid, zones, desc );
        if ( event.length() > 0 )
        {
                throw_event( get_add_interest_event(), EventParameter( event ) );
        }

        return handle;
}

int DoInterestManager::add_auto_interest( DOID_TYPE parent, vector<ZONEID_TYPE> zones, const string &desc )
{
        int handle = get_next_handle();
        if ( _no_new_interests )
        {
                return -1;
        }

        if ( parent != game_globals_id )
        {
                DistributedObjectBase *par = get_do( parent );
                if ( par == nullptr )
                {
                        return -1;
                }
                else
                {
                        if ( !par->has_parenting_rules() )
                        {
                                return -1;
                        }
                }
        }

        _interests[handle] = new InterestState( desc, InterestState::IS_active, 0,
                                                "", parent, zones, _event_counter );

        return handle;
}

void DoInterestManager::remove_interest( int handle, const string &event )
{
        string event_final;
        if ( event.length() == 0 )
        {
                event_final = get_anonymous_event( "removeInterest" );
        }
        else
        {
                event_final = event;
        }

        if ( _interests.find( handle ) != _interests.end() )
        {
                InterestState *state = _interests[handle];
                if ( event_final.length() > 0 )
                {
                        throw_event( get_remove_interest_event(), EventParameter( event_final ) );
                }
                if ( state->is_pending_delete() )
                {
                        cerr << "removeInterest: interest " << handle << " already pending removal" << endl;
                        if ( event_final.length() > 0 )
                        {
                                state->add_event( event_final );
                        }
                }
                else
                {
                        if ( state->get_events().size() > 0 )
                        {
                                state->clear_events();
                        }

                        state->set_state( InterestState::IS_pending_del );
                        int contextid = get_next_context_id();
                        state->set_context_id( contextid );
                        if ( event_final.length() > 0 )
                        {
                                state->add_event( event_final );
                        }
                        send_remove_interest( handle, contextid );
                        if ( event_final.length() == 0 )
                        {
                                consider_remove_interest( handle );
                        }
                }
        }
}

void DoInterestManager::remove_auto_interest( int handle )
{
        if ( _interests.find( handle ) != _interests.end() )
        {
                InterestState *state = _interests[handle];
                if ( state->is_pending_delete() )
                {
                        cerr << "removeInterest: interest " << handle << " already pending removal" << endl;
                }
                else
                {
                        if ( state->get_events().size() > 0 )
                        {
                                state->clear_events();
                        }

                        state->set_state( InterestState::IS_pending_del );
                        consider_remove_interest( handle );
                }
        }
}

void DoInterestManager::remove_ai_interest( int handle )
{
        send_remove_ai_interest( handle );
}

void DoInterestManager::alter_interest( int handle, DOID_TYPE parent, vector<ZONEID_TYPE> zones,
                                        const string &desc, const string &event )
{
        if ( _no_new_interests )
        {
                return;
        }

        string event_final;
        if ( event.length() == 0 )
        {
                event_final = get_anonymous_event( "alterInterest" );
        }
        else
        {
                event_final = event;
        }

        if ( _interests.find( handle ) != _interests.end() )
        {
                string desc_final;
                if ( desc.length() > 0 )
                {
                        _interests[handle]->set_desc( desc );
                }
                else
                {
                        desc_final = _interests[handle]->get_desc();
                }

                if ( _interests[handle]->get_context_id() != NO_CONTEXT )
                {
                        _interests[handle]->clear_events();
                }

                int contextid = get_next_context_id();
                _interests[handle]->set_context_id( contextid );
                _interests[handle]->set_parent_id( parent );
                _interests[handle]->set_zones( zones );
                _interests[handle]->add_event( event_final );

                send_add_interest( handle, contextid, parent, zones, desc_final );
        }
}


// Client side only:

void DoInterestManager::open_auto_interests( DistributedObject *dobj )
{
        if ( dobj->_auto_interest_handle > -1 )
        {
                // Already has an auto interest handle.
                return;
        }

        vector<ZONEID_TYPE> autointerests = dobj->get_auto_interests();

        dobj->_auto_interest_handle = -1;

        if ( autointerests.size() == 0 )
        {
                return;
        }

        stringstream desc;
        desc << dobj->get_type().get_name() << "-autoInterest";

        dobj->_auto_interest_handle = add_auto_interest( dobj->get_do_id(), autointerests, desc.str() );
}

void DoInterestManager::close_auto_interests( DistributedObject *dobj )
{
        if ( dobj->_auto_interest_handle <= -1 )
        {
                // Doesn't have an existing auto interest handle.
                return;
        }

        remove_auto_interest( dobj->_auto_interest_handle );
        dobj->_auto_interest_handle = -1;
}

////////////////////////////////////////////////////

string DoInterestManager::get_add_interest_event() const
{
        return _add_interest_event;
}

string DoInterestManager::get_remove_interest_event() const
{
        return _remove_interest_event;
}

PT( InterestState ) DoInterestManager::get_interest_state( int handle )
{
        return _interests[handle];
}

int DoInterestManager::get_next_handle()
{
        int handle = _handle_serial_num;
        while ( true )
        {
                handle = ( handle + 1 ) & _handle_mask;
                if ( _interests.find( handle ) == _interests.end() )
                {
                        break;
                }
                cerr << "interest " << handle << " already in use." << endl;
        }
        _handle_serial_num = handle;
        return _handle_serial_num;
}

int DoInterestManager::get_next_context_id()
{
        int contextid = _context_id_serial_num;
        while ( true )
        {
                contextid = ( contextid + 1 ) & _context_id_mask;
                if ( contextid != NO_CONTEXT )
                {
                        break;
                }
        }
        _context_id_serial_num = contextid;
        return _context_id_serial_num;
}

void DoInterestManager::consider_remove_interest( int handle )
{
        if ( _interests.find( handle ) != _interests.end() )
        {
                if ( _interests[handle]->is_pending_delete() )
                {
                        if ( _interests[handle]->get_context_id() == NO_CONTEXT )
                        {
                                _interests.erase( _interests.find( handle ) );
                        }
                }
        }
}

void DoInterestManager::send_add_interest( int handle, int context, DOID_TYPE parentid, vector<ZONEID_TYPE> zones,
                                           const string &desc )
{
        // Implementation is in ConnectionRepository.
}

void DoInterestManager::send_remove_interest( int handle, int context )
{
        // Implementation is in ConnectionRepository.
}

void DoInterestManager::send_remove_ai_interest( int handle )
{
        // Implementation is in ConnectionRepository.
}

void DoInterestManager::cleanup_wait_all_interests_complete()
{
        _complete_delayed_callback.destroy();
}

////////////////////////// Static callbacks ///////////////////////////////
void DoInterestManager::send_event_clbk( void *data )
{
        DoInterestManager *mgr = (DoInterestManager *)data;
        throw_event( mgr->get_all_interests_complete_callback() );
        for ( InterestClbkMap::iterator itr = mgr->_all_interest_complete_callbacks.begin();
              itr != mgr->_all_interest_complete_callbacks.end(); ++itr )
        {
                ( *itr->first )( itr->second );
        }
        mgr->_all_interest_complete_callbacks.clear();
}

bool DoInterestManager::check_more_interests( void *data )
{
        DoInterestManager *mgr = (DoInterestManager *)data;
        return mgr->_event_counter > 0;
}
///////////////////////////////////////////////////////////////////////////

void DoInterestManager::queue_all_interests_complete( int frames )
{
        cleanup_wait_all_interests_complete();
        _complete_delayed_callback = FrameDelayedCall(
                "waitForAllInterestCompletes",
                send_event_clbk,
                this,
                frames,
                check_more_interests
        );
}

void DoInterestManager::handle_interest_done_message( DatagramIterator &di )
{
        int context = di.get_uint32();
        int handle = di.get_uint16();
        if ( _interests.find( handle ) != _interests.end() )
        {
                vector<string> events;
                if ( context == _interests[handle]->get_context_id() )
                {
                        _interests[handle]->set_context_id( NO_CONTEXT );
                        events = _interests[handle]->get_events();
                }
                consider_remove_interest( handle );
                for ( size_t i = 0; i < events.size(); i++ )
                {
                        throw_event( events[i] );
                }
        }
        if ( _event_counter == 0 )
        {
                queue_all_interests_complete();
        }
}

void DoInterestManager::set_all_interests_complete_callback( DoInterestManager::InterestCompleteCallback *callback, void *data )
{
        if ( _event_counter == 0 && _complete_delayed_callback.is_finished() )
        {
                ( *callback )( data );
        }
        else
        {
                _all_interest_complete_callbacks[callback] = data;
        }
}
