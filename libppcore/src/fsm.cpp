#include "fsm.h"

#include <algorithm>

NotifyCategoryDef( fsm, "" );

FSM::
FSM( const string &name ) :
        _name( name ),
        _curr_state( NULL )
{
}

void FSM::
request( const string &state_name )
{
        // Find the state with this name.
        StateDef *state = NULL;
        for ( size_t i = 0; i < _states.size(); i++ )
        {
                StateDef &s = _states[i];
                if ( s.name.compare( state_name ) == 0 )
                {
                        state = &s;
                        break;
                }
        }

        if ( state == NULL )
        {
                fsm_cat.warning()
                                << "[" << _name << "] " << "request: No state named " << state_name << "\n";
                return;
        }

        if ( _curr_state != NULL )
        {
                if ( find( _curr_state->transitions.begin(), _curr_state->transitions.end(), state_name ) == _curr_state->transitions.end() &&
                     _curr_state->transitions.size() > ( size_t )0 )
                {
                        fsm_cat.warning()
                                        << "[" << _name << "] " << "request: No transition exists from state " << _curr_state->name << " to " << state->name << "\n";
                        return;
                }

                // Exit the current state.
                ( *_curr_state->exit_func )( _curr_state->func_data );
        }

        // Enter the new state.
        _curr_state = state;
        ( *_curr_state->enter_func )( _curr_state->func_data );
}

const StateDef *FSM::
get_current_state() const
{
        return _curr_state;
}

void FSM::
add_state( StateDef def )
{
        for ( size_t i = 0; i < _states.size(); i++ )
        {
                StateDef &s = _states[i];
                if ( s.name.compare( def.name ) == 0 )
                {
                        fsm_cat.warning()
                                        << "[" << _name << "] add_state: State named " << def.name << " already exists. Aborting.\n";
                        return;
                }
        }

        // Add the state into our states vector.
        // Welcome aboard, new state.
        _states.push_back( def );
}