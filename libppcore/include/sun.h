#ifndef SUN_H
#define SUN_H

#include <pandaNode.h>

#include "config_pandaplus.h"
#include "pp_utils.h"

class EXPCL_PANDAPLUS Sun : public PandaNode
{
        TypeDecl( Sun, PandaNode );

public:
        Sun( const string &name );

        void set_sun_color( LColor &rgb );
        LColor get_sun_color() const;

private:
        LColor _sun_color;

public:
        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &dg );

protected:
        static TypedWritable *make_from_bam( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );
};

#endif // SUN_H