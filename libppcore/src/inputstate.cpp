#include "inputstate.h"

#include <string>

InputDef::
InputDef( InputState *inp_state ) :
        _input_state( inp_state )
{
}

InputDef &InputDef::
operator =( const InputDef &other )
{
        _set = other._set;
        _forcing = other._forcing;
        _on = other._on;
        _off = other._off;
        _name = other._name;

        return *this;
}

bool InputDef::
operator ==( const InputDef &other ) const
{
        return _name.compare( other._name ) == 0;
}

void InputDef::
accept()
{
        _input_state->_event_handler->add_hook( _on, event_on, this );
        _input_state->_event_handler->add_hook( _off, event_off, this );
}

void InputDef::
ignore()
{
        _input_state->_event_handler->remove_hooks( _on );
        _input_state->_event_handler->remove_hooks( _off );
}

void InputDef::
event_on( const Event *e, void *data )
{
        PT( InputDef ) inp = ( InputDef * )data;
        inp->_input_state->set( inp->_name, true );
}

void InputDef::
event_off( const Event *e, void *data )
{
        PT( InputDef ) inp = ( InputDef * )data;
        inp->_input_state->set( inp->_name, false );
}

/////////////////////////////////////////////////////////////////////////////////

InputState::
InputState() :
        _event_handler( EventHandler::get_global_event_handler() )
{
}

ExistInfo InputState::
input_exists( const string &name ) const
{
        ExistInfo info;
        info.exists = false;
        info.index = -1;

        for ( size_t i = 0; i < _input_defs.size(); i++ )
        {
                InputDef *inp = _input_defs[i];
                if ( inp->_name.compare( name ) == 0 )
                {
                        info.exists = true;
                        info.index = i;
                        break;
                }
        }

        return info;
}

void InputState::
force_input( const string &name )
{
        ExistInfo info = input_exists( name );
        if ( info.exists )
        {
                PT( InputDef ) inp = _input_defs[info.index];
                inp->_forcing = true;
        }
        else
        {
                cout << "InputState: cannot force an undefined input" << endl;
        }
}

void InputState::
unforce_input( const string &name )
{
        ExistInfo info = input_exists( name );
        if ( info.exists )
        {
                PT( InputDef ) inp = _input_defs[info.index];
                inp->_forcing = false;
        }
        else
        {
                cout << "InputState: cannot unforce an undefined input" << endl;
        }
}

void InputState::
set( const string &name, bool val )
{
        ExistInfo info = input_exists( name );
        if ( info.exists )
        {
                PT( InputDef ) inp = _input_defs[info.index];
                inp->_set = val;
        }
        else
        {
                cout << "InputState: cannot set an undefined input" << endl;
        }
}

string InputState::
define_input( const string &name, const string &event_on, const string &event_off )
{

        ExistInfo info = input_exists( name );
        if ( info.exists )
        {
                cout << "InputState: modifying an existing input definition" << endl;
                _input_defs[info.index]->_on = event_on;
                _input_defs[info.index]->_off = event_off;
                _input_defs[info.index]->_forcing = false;

                return name;
        }

        PT( InputDef ) inp = new InputDef( this );
        inp->_name = name;
        inp->_on = event_on;
        inp->_off = event_off;
        inp->_set = false;
        inp->_forcing = false;

        _input_defs.push_back( inp );

        size_t index = _input_defs.size() - 1;
        _input_defs[index]->accept();

        return name;
}

void InputState::
undefine_input( const string &name )
{
        ExistInfo info = input_exists( name );
        if ( info.exists )
        {
                // Stop listening to the events this definition wanted.
                PT( InputDef ) inp = _input_defs[info.index];
                inp->ignore();

                _input_defs.erase( find( _input_defs.begin(), _input_defs.end(), inp ) );
        }
        else
        {
                cout << "InputState: cannot undefine an undefined input" << endl;
        }
}

bool InputState::
is_set( const string &name ) const
{
        ExistInfo info = input_exists( name );
        if ( info.exists )
        {
                PT( InputDef ) inp = _input_defs.at( info.index );
                return inp->_set || inp->_forcing;
        }
        return false;
}