#include "parentMgr.h"

void ParentMgr::
destroy()
{
        _token_2_nodepath.clear();
}

void ParentMgr::
request_reparent( NodePath *child, int parenttoken )
{
        bool exists = _token_2_nodepath.find( parenttoken ) != _token_2_nodepath.end();
        if ( exists )
        {
                child->reparent_to( *_token_2_nodepath[parenttoken] );
        }
}

void ParentMgr::
register_reparent( int token, NodePath *parent )
{
        if ( _token_2_nodepath.find( token ) != _token_2_nodepath.end() )
        {
                return;
        }
        _token_2_nodepath[token] = parent;
}

void ParentMgr::
unregister_reparent( int token )
{
        if ( _token_2_nodepath.find( token ) != _token_2_nodepath.end() )
        {
                _token_2_nodepath.erase( _token_2_nodepath.find( token ) );
        }
}