#ifndef ASTRONDATABASEINTERFACE_H
#define ASTRONDATABASEINTERFACE_H

//#include "distributedObjectBase.h"

#include <dcPacker.h>
#include "dcClass_pp.h"
#include <datagramIterator.h>
#include <pmap.h>
#include "pp_dcbase.h"

#include "config_ppdistributed.h"

NotifyCategoryDeclNoExport( astronDatabaseInterface );

class EXPCL_PPDISTRIBUTED AstronDatabaseInterface
{
public:

        AstronDatabaseInterface();

        typedef pmap<string, DCPacker> DBFieldMap;

        typedef void CallbackCreate( void * );
        typedef void CallbackQuery( void *, DCClassPP *, DCPacker & );
        typedef void CallbackUpdate( void *, DCPacker & );

        void CreateObject( int dbid, DCClassPP *dclass,
                           DBFieldMap &fields = DBFieldMap(),
                           CallbackCreate *callback = NULL,
                           void *data = NULL );

        void HandleCreateObjectResp( DatagramIterator &di );

        void QueryObject( int dbid, DOID_TYPE doid, CallbackQuery *clbk, void *data, DCClassPP *dclass = NULL,
                          vector<string> fieldnames = vector<string>() );

        void HandleQueryObjectResp( int msgtype, DatagramIterator &di );

        void UpdateObject( int dbid, DOID_TYPE doid, DCClassPP *dclass, DBFieldMap &newfields, CallbackUpdate *clbk = NULL );

        void HandleUpdateObjectResp( DatagramIterator &di, bool multi );

        void HandleDatagram( int msgtype, DatagramIterator &di );

private:

        typedef pmap<int, void *> DataMap;
        DataMap m_mCallbacksData;

        typedef pmap<int, CallbackUpdate *> CallbackUpdMap;
        CallbackUpdMap m_mCallbacksUpdate;

        typedef pmap<int, CallbackCreate *> CallbackCrMap;
        CallbackCrMap m_mCallbacksCreate;

        typedef pmap<int, CallbackQuery *> CallbackQuMap;
        CallbackQuMap m_mCallbacksQuery;

        DClassMapNum m_mDclasses;

        AIRepositoryBase *m_pAIR;
};

#endif // ASTRONDATABASEINTERFACE_H