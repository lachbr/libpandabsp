/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file aux_data_attrib.h
 * @author Brian Lach
 * @date January 02, 2019
 */

#ifndef AUXDATAATTRIB_H
#define AUXDATAATTRIB_H

#include "config_bsp.h"

#include <renderAttrib.h>

class EXPCL_PANDABSP AuxDataAttrib : public RenderAttrib
{
private:
        INLINE AuxDataAttrib() :
                _data( nullptr )
        {
        }

PUBLISHED:
        static CPT( RenderAttrib ) make( TypedReferenceCount *data );
        static CPT( RenderAttrib ) make_default();

        INLINE bool has_data() const
        {
                return _data != nullptr;
        }

        INLINE TypedReferenceCount *get_data() const
        {
                return _data;
        }

private:
        PT( TypedReferenceCount ) _data;

protected:
        virtual int compare_to_impl( const RenderAttrib *other ) const;
        virtual size_t get_hash_impl() const;

PUBLISHED:
        static int get_class_slot()
        {
                return _attrib_slot;
        }
        virtual int get_slot() const
        {
                return get_class_slot();
        }
        MAKE_PROPERTY( class_slot, get_class_slot );

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                RenderAttrib::init_type();
                register_type( _type_handle, "AuxDataAttrib",
                               RenderAttrib::get_class_type() );
                _attrib_slot = register_slot( _type_handle, 100, new AuxDataAttrib );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
        static TypeHandle _type_handle;
        static int _attrib_slot;
};

#endif // AUXDATAATTRIB_H
