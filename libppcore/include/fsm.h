/**
* PANDA PLUS LIBRARY
* Copyright (c) 2017 Brian Lach <lachb@cat.pcsb.org>
*
* @file fsm.h
* @author Brian Lach
* @date 2017-04-05
*
* @desc Simple implementation of a Finite-State Machine.
*/

#ifndef FSM_H
#define FSM_H

#include "config_pandaplus.h"

typedef void StateFunc( void * );
typedef pvector<string> TransitionVec;

struct StateDef
{
        string name;
        StateFunc *enter_func;
        StateFunc *exit_func;
        void *func_data;
        TransitionVec transitions;

        StateDef( const string &n, StateFunc &enf, StateFunc *exf, void *fd )
        {
                name = n;
                enter_func = enf;
                exit_func = exf;
                func_data = fd;
        }

        StateDef( const string &n, StateFunc *enf, StateFunc *exf, void *fd, TransitionVec &trans )
        {
                name = n;
                enter_func = enf;
                exit_func = exf;
                func_data = fd;
                transitions = trans;
        }
};

typedef pvector<StateDef> StateVec;

NotifyCategoryDeclNoExport( fsm );

class EXPCL_PANDAPLUS FSM
{
public:
        FSM( const string &name );

        void request( const string &state );
        //bool has_transition(const string &state, const string &transition) const;

        void add_state( StateDef def );

        const StateDef *get_current_state() const;
private:
        StateVec _states;
        StateDef *_curr_state;
        string _name;
};

#endif // FSM_H
