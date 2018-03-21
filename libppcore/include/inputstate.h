#ifndef __INPUT_STATE_H__
#define __INPUT_STATE_H__

#include <string>

#include <eventHandler.h>
#include <pmap.h>

#include "config_pandaplus.h"

class InputState;

class InputDef : public ReferenceCount
{

public:
        bool _set;
        bool _forcing;
        string _on;
        string _off;
        string _name;
        InputDef( InputState *inp_state );

        InputDef &operator =( const InputDef &other );

        bool operator ==( const InputDef &other ) const;
        void accept();
        void ignore();

        InputState *_input_state;

private:

        static void event_on( const Event *e, void *data );

        static void event_off( const Event *e, void *data );

};


/**
* When InputState::input_exists is called, it will return this.
* It holds a bool saying if the input definition exists, and if it
* does, it will hold the index in _input_defs that the definition
* is located at.
*/
struct ExistInfo
{
        bool exists;
        int index;
};

class EXPCL_PANDAPLUS InputState
{
public:
        InputState();

        ExistInfo input_exists( const string &name ) const;
        bool is_set( const string &name ) const;

        string define_input( const string &name, const string &event_on, const string &event_off );
        void undefine_input( const string &name );
        void force_input( const string &name );
        void unforce_input( const string &name );

        void set( const string &name, bool val );

        friend class InputDef;

private:
        typedef pvector<PT( InputDef )> InputDefs;

        InputDefs _input_defs;

        EventHandler *_event_handler;
};

#endif // __INPUT_STATE_H__