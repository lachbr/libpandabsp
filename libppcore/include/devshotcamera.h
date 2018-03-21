#ifndef DEVSHOTCAMERA_H
#define DEVSHOTCAMERA_H

#include "config_pandaplus.h"
#include "pp_utils.h"

#include <pandaNode.h>

/**
 * This class defines properties for a static camera setup in a map.
 * It holds info like the position and orientation of the camera,
 * as well as the FOV.
 * Useful for spectator views and other things.
 */
class EXPCL_PANDAPLUS DevshotCamera : public PandaNode
{
        TypeDecl( DevshotCamera, PandaNode );

public:
        DevshotCamera( const string &name );

        void set_fov( PN_stdfloat fov );
        PN_stdfloat get_fov() const;

private:
        PN_stdfloat _fov;

public:
        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &dg );

protected:
        static TypedWritable *make_from_bam( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );
};

#endif // DEVSHOTCAMERA_H