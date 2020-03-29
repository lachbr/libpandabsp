/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file ambient_boost_effect.h
 * @author Brian Lach
 * @date March 16, 2019
 */

#ifndef AMBIENT_BOOST_EFFECT_H
#define AMBIENT_BOOST_EFFECT_H

#include "config_bsp.h"

#include <renderEffect.h>

class AmbientBoostEffect : public RenderEffect
{
private:
        INLINE AmbientBoostEffect() :
                RenderEffect()
        {
        }

PUBLISHED:
        static CPT( RenderEffect ) make();

protected:
        virtual bool safe_to_combine() const;
        virtual int compare_to_impl( const RenderEffect *other ) const;

public:
        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &dg );

protected:
        static TypedWritable *make_from_bam( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                RenderEffect::init_type();
                register_type( _type_handle, "AmbientBoostEffect",
                        RenderEffect::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type()
        {
                init_type(); return get_class_type();
        }

private:
        static TypeHandle _type_handle;
};

#endif // AMBIENT_BOOST_EFFECT_H
