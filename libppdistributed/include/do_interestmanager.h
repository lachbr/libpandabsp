/**
* PANDA PLUS LIBRARY
* Copyright (c) 2017 Brian Lach <lachb@cat.pcsb.org>
*
* @file doInterestManager.h
* @author Brian lach
* @date 2017-02-11
*/

#ifndef DOINTERESTMANAGER_H
#define DOINTERESTMANAGER_H

#include <pointerTo.h>
#include <referenceCount.h>
#include <pmap.h>

#include "pp_dcbase.h"
#include "config_pandaplus.h"
#include "do_collectionmanager.h"
#include "pp_utils.h"

class DistributedObject;

class EXPCL_PPDISTRIBUTED InterestState : public ReferenceCount
{
public:
        enum State
        {
                IS_active,
                IS_pending_del
        };

        InterestState( const string &desc, State state, int contextid, const string &event,
                       DOID_TYPE parentid, vector<ZONEID_TYPE> zones, int &event_counter,
                       bool isauto = false );

        void add_event( const string &event );
        vector<string> get_events() const;
        void clear_events();
        void send_events();
        bool is_pending_delete() const;

        void set_state( State state );
        State get_state() const;

        void set_context_id( int context );
        int get_context_id() const;

        void set_desc( const string &desc );
        string get_desc() const;

        void set_zones( vector<ZONEID_TYPE> zones );

        void set_parent_id( DOID_TYPE parent );
        DOID_TYPE get_parent_id() const;

private:
        State _state;
        int &_event_counter;
        string _desc;
        int _context;
        DOID_TYPE _parent_id;
        vector<ZONEID_TYPE> _zones;
        bool _auto;

        vector<string> _events;
};

NotifyCategoryDeclNoExport( doInterestManager );

#define NO_CONTEXT 0

class EXPCL_PPDISTRIBUTED DoInterestManager : public DoCollectionManager
{
public:

        typedef void InterestCompleteCallback( void * );

        DoInterestManager();

        string get_anonymous_event( const string &desc ) const;

        void set_no_new_interests( bool flag );
        bool no_new_interests() const;

        void set_all_interests_complete_callback( InterestCompleteCallback *callback, void *data );

        string get_all_interests_complete_callback() const;

        void reset_interest_state_for_connection_loss();

        bool is_valid_interest_handle( int handle );

        void update_interest_description( int handle, const string &desc );

        int add_interest( DOID_TYPE parentid, vector<ZONEID_TYPE> zoneidlist, const string &desc,
                          const string &event = "" );

        int add_auto_interest( DOID_TYPE parentid, vector<ZONEID_TYPE> zoneidlist, const string &desc );

        void remove_interest( int handle, const string &event = "" );

        void remove_auto_interest( int handle );

        void remove_ai_interest( int handle );

        // Client side only!!
        void open_auto_interests( DistributedObject *dobj );
        void close_auto_interests( DistributedObject *dobj );
        ////////////////////////////////////////////////////////

        void alter_interest( int handle, DOID_TYPE parentid, vector<ZONEID_TYPE> zoneidlist,
                             const string &desc = "", const string &event = "" );

        void cleanup_wait_all_interests_complete();

        void queue_all_interests_complete( int frames = 5 );

        void handle_interest_done_message( DatagramIterator &di );

protected:
        string _add_interest_event;
        string _remove_interest_event;
        bool _no_new_interests;
        int _event_counter;
        FrameDelayedCall _complete_delayed_callback;

        typedef pmap<InterestCompleteCallback *, void *> InterestClbkMap;
        InterestClbkMap _all_interest_complete_callbacks;

        string get_add_interest_event() const;
        string get_remove_interest_event() const;

        PT( InterestState ) get_interest_state( int handle );

        int get_next_handle();
        int get_next_context_id();

        void consider_remove_interest( int handle );

        // The actual implementation of these functions are in ConnectionRepository because we don't
        // have access to the send() function from DoInterestManager.
        virtual void send_add_interest( int handle, int contextid, DOID_TYPE parentid,
                                        vector<ZONEID_TYPE> zoneidlist, const string &desc = "" );
        virtual void send_remove_interest( int handle, int contextid );
        virtual void send_remove_ai_interest( int handle );

private:
        static void send_event_clbk( void *data );
        static bool check_more_interests( void *data );

protected:
        static int _handle_serial_num;
        static int _handle_mask;
        static pmap<int, PT( InterestState )> _interests;
        static int _context_id_serial_num;
        static int _context_id_mask;

        static SerialNumGen _serial_gen;
        static int _serial_num;
};

#endif // DOINTERESTMANAGER_H