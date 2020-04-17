/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file ambient_boost_effect.cpp
 * @author Brian Lach
 * @date March 16, 2019
 */

#include "ambient_boost_effect.h"

TypeHandle AmbientBoostEffect::m_sTypeHandle;

CPT( RenderEffect ) AmbientBoostEffect::make()
{
        AmbientBoostEffect *effect = new AmbientBoostEffect;
        return return_new( effect );
}

bool AmbientBoostEffect::safe_to_combine() const
{
        return true;
}

int AmbientBoostEffect::compare_to_impl( const RenderEffect *other ) const
{
        return 0;
}

void AmbientBoostEffect::register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), MakeFromBam );
}

void AmbientBoostEffect::write_datagram( BamWriter *manager, Datagram &dg )
{
        RenderEffect::write_datagram( manager, dg );
}

TypedWritable *AmbientBoostEffect::MakeFromBam( const FactoryParams &params )
{
        AmbientBoostEffect *effect = new AmbientBoostEffect;
        DatagramIterator scan;
        BamReader *manager;

        parse_params( params, scan, manager );
        effect->fillin( scan, manager );

        return effect;
}

void AmbientBoostEffect::fillin( DatagramIterator &scan, BamReader *manager )
{
        RenderEffect::fillin( scan, manager );
}
