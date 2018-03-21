#ifndef DCCLASS_PP_H
#define DCCLASS_PP_H

#include "config_ppdistributed.h"
#include "dcfield_pp.h"

#include <datagramIterator.h>
#include <datagram.h>
#include <pvector.h>
#include <dcClass.h>

class DistributedObjectBase;

/**
* This is a wrapper around DCClass that provides functionality this implementation needs.
*/
class EXPCL_PPDISTRIBUTED DCClassPP
{
public:
        DCClassPP( DCClass *dclass );
        //virtual ~DCClassPP();

        void receive_update( DistributedObjectBase *obj, DatagramIterator &di ) const;
        void receive_update_broadcast_required( DistributedObjectBase *obj, DatagramIterator &di ) const;
        void receive_update_broadcast_required_owner( DistributedObjectBase *obj, DatagramIterator &di ) const;
        void receive_update_all_required( DistributedObjectBase *obj, DatagramIterator &di ) const;
        void receive_update_other( DistributedObjectBase *obj, DatagramIterator &di ) const;

        void direct_update( DistributedObjectBase *obj, const string &field_name, const string &value_blob );
        void direct_update( DistributedObjectBase *obj, const string &field_name, const Datagram &datagram );

        bool pack_required_field( Datagram &datagram, DistributedObjectBase *obj, const DCFieldPP *field ) const;
        bool pack_required_field( DCPacker &packer, DistributedObjectBase *obj, const DCFieldPP *field ) const;

        // Pass in the singleton instance of the object.
        void set_obj_singleton( DistributedObjectBase *singleton );
        DistributedObjectBase *get_obj_singleton();

        DCPacker start_client_format_update( const string &field_name, uint32_t do_id ) const;
        Datagram end_client_format_update( const string &field_name, DCPacker &pack ) const;

        DCPacker start_ai_format_update( const string &field_name, uint32_t do_id,
                                         uint32_t to_id, uint32_t from_id ) const;
        Datagram end_ai_format_update( const string &field_name, DCPacker &pack ) const;

        DCPacker start_ai_format_update_msg_type( const string &field_name, uint32_t do_id,
                                                  uint32_t to_id, uint32_t from_id, int msg_type ) const;
        Datagram end_ai_format_update_msg_type( const string &field_name, DCPacker &pack ) const;

        Datagram ai_format_generate( DistributedObjectBase *obj, DOID_TYPE do_id, DOID_TYPE parent_id,
                                     ZONEID_TYPE zone_id, CHANNEL_TYPE dist_chan_id, CHANNEL_TYPE from_chan_id ) const;

        DCFieldPP *get_field_wrapper( DCField *field ) const;

        DCClass *get_dclass() const;

        void add_ppfield( DCFieldPP *ppfield );

        Datagram client_format_generate_cmu( DistributedObjectBase *obj, DOID_TYPE do_id, ZONEID_TYPE zone_id,
                                             pvector<string> &optional_fields = pvector<string>() );

private:
        DCClass *_dclass;
        DistributedObjectBase *_obj_singleton;
        string _curr_update_f;

        vector<DCFieldPP *> _pp_fields;
};

#endif // DCCLASS_PP_H