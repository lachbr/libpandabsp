/**
* @file dcField_pp.h
* @author Brian Lach
* @date 2017-02-07
*/

#ifndef DCFIELD_PP_H
#define DCFIELD_PP_H

#include <dcAtomicField.h>
#include <datagram.h>

#include "config_ppdistributed.h"

typedef void DCFunc( DCPacker &, void * );

class DistributedObjectBase;

/**
* This class is a wrapper around a DCAtomicField, providing the functionality that we need.
*/
class EXPCL_PPDISTRIBUTED DCFieldPP
{
public:
        DCFieldPP( DCAtomicField *field );
        //virtual ~DCFieldPP();

        void set_setter_func( DCFunc *func );
        void set_getter_func( DCFunc *func );

        DCFunc *get_setter_func();
        DCFunc *get_getter_func() const;

        //void receive_update(DCPacker &packer, DistributedObjectBase *obj);

        DCPacker start_client_format_update( uint32_t do_id );
        Datagram end_client_format_update( DCPacker &packer );

        DCPacker start_ai_format_update( uint32_t do_id, uint32_t to_id, uint32_t from_id );
        Datagram end_ai_format_update( DCPacker &packer );

        DCPacker start_ai_format_update_msg_type( uint32_t do_id, uint32_t to_id,
                                                  uint32_t from_id, int msgtype );
        Datagram end_ai_format_update_msg_type( DCPacker &packer );

        DCAtomicField *get_field() const;

private:
        DCFunc *_set_func;
        DCFunc *_get_func;

        DCAtomicField *_field;
};

#endif // __DCFIELD_PP_H__