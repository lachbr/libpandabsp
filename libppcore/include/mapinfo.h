#ifndef MAP_INFO_H
#define MAP_INFO_H

#include <pandaNode.h>

#include "pp_utils.h"

class EXPCL_PANDAPLUS MapInfo : public PandaNode
{
        TypeDecl( MapInfo, PandaNode );

public:
        MapInfo( const string &name );

        void set_map_version( const string &ver );
        void set_map_title( const string &title );
        void set_sky_name( const string &sky );

        string get_map_version() const;
        string get_map_title() const;
        string get_sky_name() const;

private:
        string _map_version;
        string _map_title;
        string _sky_name;

public:
        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &dg );

protected:
        static TypedWritable *make_from_bam( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );
};

#endif // MAP_INFO_H