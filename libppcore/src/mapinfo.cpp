#include "mapinfo.h"

TypeDef( MapInfo );

MapInfo::
MapInfo( const string &name ) :
        PandaNode( name ),
        _map_title( "" ),
        _map_version( "" ),
        _sky_name( "" )
{
}

void MapInfo::
set_map_version( const string &ver )
{
        _map_version = ver;
}

string MapInfo::
get_map_version() const
{
        return _map_version;
}

void MapInfo::
set_map_title( const string &title )
{
        _map_title = title;
}

string MapInfo::
get_map_title() const
{
        return _map_title;
}

void MapInfo::
set_sky_name( const string &sky )
{
        _sky_name = sky;
}

string MapInfo::
get_sky_name() const
{
        return _sky_name;
}

///////////////////////////// BAM STUFF ////////////////////////////////

void MapInfo::
register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), make_from_bam );
}

void MapInfo::
write_datagram( BamWriter *manager, Datagram &dg )
{
        PandaNode::write_datagram( manager, dg );

        dg.add_string( _map_version );
        dg.add_string( _map_title );
        dg.add_string( _sky_name );
}

TypedWritable *MapInfo::
make_from_bam( const FactoryParams &params )
{
        MapInfo *node = new MapInfo( "" );
        DatagramIterator scan;
        BamReader *manager;

        parse_params( params, scan, manager );
        node->fillin( scan, manager );

        return node;
}

void MapInfo::
fillin( DatagramIterator &scan, BamReader *manager )
{
        PandaNode::fillin( scan, manager );

        _map_version = scan.get_string();
        _map_title = scan.get_string();
        _sky_name = scan.get_string();
}