#ifndef DATAGRAMEXTENSIONS_H
#define DATAGRAMEXTENSIONS_H

#include "config_ppdistributed.h"
#include "pp_dcbase.h"
#include <datagram.h>

extern EXPCL_PPDISTRIBUTED void dg_add_server_control_header( Datagram &dg, uint16_t code );
extern EXPCL_PPDISTRIBUTED void dg_add_server_header( Datagram &dg, CHANNEL_TYPE chan, CHANNEL_TYPE sender, uint16_t code );
extern EXPCL_PPDISTRIBUTED void dg_add_old_server_header( Datagram &dg, CHANNEL_TYPE chan, CHANNEL_TYPE sender, uint16_t code );

#endif // DATAGRAMEXTENSIONS_H