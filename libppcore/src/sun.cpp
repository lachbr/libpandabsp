#include "sun.h"

TypeDef( Sun );

Sun::Sun( const string &name ) :
        PandaNode( name ),
        _sun_color( 1, 1, 1, 1 )
{
}

void Sun::set_sun_color( LColor &color )
{
        _sun_color = color;
}

LColor Sun::get_sun_color() const
{
        return _sun_color;
}

///////////////////////////// BAM STUFF ////////////////////////////////

void Sun::register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), make_from_bam );
}

void Sun::write_datagram( BamWriter *manager, Datagram &dg )
{
        PandaNode::write_datagram( manager, dg );

        _sun_color.write_datagram( dg );
}

TypedWritable *Sun::make_from_bam( const FactoryParams &params )
{
        Sun *node = new Sun( "" );
        DatagramIterator scan;
        BamReader *manager;

        parse_params( params, scan, manager );
        node->fillin( scan, manager );

        return node;
}

void Sun::fillin( DatagramIterator &scan, BamReader *manager )
{
        PandaNode::fillin( scan, manager );

        _sun_color.read_datagram( scan );
}