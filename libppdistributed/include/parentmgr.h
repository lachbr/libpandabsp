/**
* PANDA PLUS LIBRARY
* Copyright (c) 2017 Brian Lach <lachb@cat.pcsb.org>
*
* @file parentMgr.h
* @author Brian Lach
* @date 2017-02-15
*/

#ifndef PARENTMGR_H
#define PARENTMGR_H

#include "config_ppdistributed.h"

#include <nodePath.h>

/**
* ParentMgr holds a table of nodes that avatars may be parented to
* in a distributed manner.All clients within a particular zone maintain
* identical tables of these nodes, and the nodes are referenced by 'tokens'
* which the clients can pass to each other to communicate distributed
* reparenting information.
*
* The functionality of ParentMgr used to be implemented with a simple
* token->node dictionary.As distributed 'parent' objects were manifested,
* they would add themselves to the dictionary.Problems occured when
* distributed avatars were manifested before the objects to which they
* were parented to.

* Since the order of object manifestation depends on the order of the
* classes in the DC file, we could maintain an ordering of DC definitions
* that ensures that the necessary objects are manifested before avatars.
* However, it's easy enough to keep a list of pending reparents and thus
* support the general case without requiring any strict ordering in the DC.
*/
class EXPCL_PPDISTRIBUTED ParentMgr
{
public:
        void destroy();
        void request_reparent( NodePath *child, int parenttoken );
        void register_reparent( int token, NodePath *parent );
        void unregister_reparent( int token );

private:
        typedef pmap<int, NodePath *> Token2NP;
        Token2NP _token_2_nodepath;
};

#endif // PARENTMGR_H