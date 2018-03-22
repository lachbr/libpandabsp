#include "devshotcamera.h"

#include "pp_globals.h"

TypeDef( DevshotCamera );

DevshotCamera::DevshotCamera( const string &name ) :
        PandaNode( name ),
        _fov( 0.0 )
{
}

void DevshotCamera::set_fov( PN_stdfloat fov )
{
        _fov = fov;
}

PN_stdfloat DevshotCamera::get_fov() const
{
        return _fov;
}

///////////////////////////// BAM STUFF ////////////////////////////////

void DevshotCamera::register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), make_from_bam );
}

void DevshotCamera::write_datagram( BamWriter *manager, Datagram &dg )
{
        PandaNode::write_datagram( manager, dg );

        dg.add_stdfloat( _fov );
}

TypedWritable *DevshotCamera::make_from_bam( const FactoryParams &params )
{
        DevshotCamera *node = new DevshotCamera( "" );
        DatagramIterator scan;
        BamReader *manager;

        parse_params( params, scan, manager );
        node->fillin( scan, manager );

        return node;
}

void DevshotCamera::fillin( DatagramIterator &scan, BamReader *manager )
{
        PandaNode::fillin( scan, manager );

        set_fov( scan.get_stdfloat() );
}