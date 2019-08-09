/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file aux_data_attrib.cpp
 * @author Brian Lach
 * @date January 02, 2019
 */

#include "aux_data_attrib.h"

TypeHandle AuxDataAttrib::_type_handle;
int AuxDataAttrib::_attrib_slot;

CPT( RenderAttrib ) AuxDataAttrib::make( TypedReferenceCount *data )
{
        AuxDataAttrib *ada = new AuxDataAttrib;
        ada->_data = data;
        return return_new( ada );
}

CPT( RenderAttrib ) AuxDataAttrib::make_default()
{
        AuxDataAttrib *ada = new AuxDataAttrib;
        return return_new( ada );
}

int AuxDataAttrib::compare_to_impl( const RenderAttrib *other ) const
{
        const AuxDataAttrib *ada = (const AuxDataAttrib *)other;

        if ( _data != ada->_data )
        {
                return _data < ada->_data ? -1 : 1;
        }

        return 0; 
}

size_t AuxDataAttrib::get_hash_impl() const
{
        size_t hash = 0;
        hash = pointer_hash::add_hash( hash, _data );

        return hash;
}
