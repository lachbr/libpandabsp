#ifndef PPDCBASE_H
#define PPDCBASE_H

#include <stdint.h>

#include <dcFile.h>
#include <dcKeyword.h>
#include <pmap.h>

#include "config_ppdistributed.h"
#include "dcclass_pp.h"

class DistributedObjectBase;

typedef       uint64_t   CHANNEL_TYPE;
typedef       uint32_t   DOID_TYPE;
typedef       uint32_t   ZONEID_TYPE;

typedef void DCFieldFunc( DCPacker &, void * );
typedef vector<DCFieldFunc *> DCFieldFuncVec;
typedef pmap<string, DCFieldFuncVec> FieldName2SetGetFunc;
typedef pmap<string, FieldName2SetGetFunc> DClass2Fields;
typedef vector<DCFieldPP *> DCPPFieldVec;
typedef pmap<string, DistributedObjectBase *> ClsName2Singleton;

typedef pmap<int, DCClassPP *> DClassMapNum;

extern EXPCL_PPDISTRIBUTED ClsName2Singleton dc_singleton_by_name;

extern EXPCL_PPDISTRIBUTED DClass2Fields dc_field_data;

extern EXPCL_PPDISTRIBUTED int game_globals_id;
extern EXPCL_PPDISTRIBUTED bool dc_has_owner_view;

extern EXPCL_PPDISTRIBUTED DCFile dc_file;

#endif // PPDCBASE_H