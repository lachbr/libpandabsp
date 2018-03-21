#include "astronDatabaseInterface.h"
#include "datagram_extensions.h"
#include "pp_globals.h"

#include "dcClass_pp.h"

#include "astronInternalRepository.h"

NotifyCategoryDef( astronDatabaseInterface, "" );

AstronDatabaseInterface::AstronDatabaseInterface()
        : m_pAIR( ( AIRepositoryBase * )gpAIR )
{
}

void AstronDatabaseInterface::CreateObject( int dbid, DCClassPP *dclass, AstronDatabaseInterface::DBFieldMap &fields,
                                            AstronDatabaseInterface::CallbackCreate *callback, void *data )
{
        int ctx = m_pAIR->GetContext();
        m_mCallbacksCreate[ctx] = callback;
        m_mCallbacksData[ctx] = data;

        DCPacker fieldpacker;
        int fieldcount = 0;
        for ( DBFieldMap::iterator itr = fields.begin(); itr != fields.end(); ++itr )
        {
                DCFieldPP *field = dclass->GetFieldWrapper( dclass->GetDClass()->get_field_by_name( itr->first ) );
                if ( field == NULL )
                {
                        // boi
                        return;
                }
                fieldpacker.raw_pack_uint16( field->GetField()->get_number() );
                fieldpacker.begin_pack( field->GetField() );

                // Add the field data from the packer that was sent.
                // It should have the value for the field packed in.
                fieldpacker.append_data( itr->second.get_data(), itr->second.get_length() );

                fieldpacker.end_pack();
                fieldcount++;
        }

        Datagram dg;
        DgAddServerHeader( dg, dbid, m_pAIR->GetOurChannel(), DBSERVER_CREATE_OBJECT );
        dg.add_uint32( ctx );
        dg.add_uint16( dclass->GetDClass()->get_number() );
        dg.add_uint16( fieldcount );
        dg.append_data( fieldpacker.get_string() );
        m_pAIR->Send( dg );
}

void AstronDatabaseInterface::HandleCreateObjectResp( DatagramIterator &di )
{
        int ctx = di.get_uint32();
        DOID_TYPE doid = di.get_uint32();

        if ( m_mCallbacksCreate.find( ctx ) == m_mCallbacksCreate.end() )
        {
                return;
        }

        if ( m_mCallbacksCreate[ctx] != NULL )
        {
                ( *m_mCallbacksCreate[ctx] )( m_mCallbacksData[ctx] );
        }

        m_mCallbacksCreate.erase( m_mCallbacksCreate.find( ctx ) );
        m_mCallbacksData.erase( m_mCallbacksData.find( ctx ) );
}

void AstronDatabaseInterface::QueryObject( int dbid, DOID_TYPE doid, AstronDatabaseInterface::CallbackQuery *clbk,
                                           void *data, DCClassPP *dclass, vector<string> fieldnames )
{
        int ctx = m_pAIR->GetContext();
        m_mCallbacksQuery[ctx] = clbk;
        m_mCallbacksData[ctx] = data;
        m_mDclasses[ctx] = dclass;

        Datagram dg;

        if ( fieldnames.size() == 0 )
        {
                DgAddServerHeader( dg, dbid, m_pAIR->GetOurChannel(), DBSERVER_OBJECT_GET_ALL );
        }
        else
        {
                nassertv( dclass != NULL );
                if ( fieldnames.size() > 1 )
                {
                        DgAddServerHeader( dg, dbid, m_pAIR->GetOurChannel(), DBSERVER_OBJECT_GET_FIELDS );
                }
                else
                {
                        DgAddServerHeader( dg, dbid, m_pAIR->GetOurChannel(), DBSERVER_OBJECT_GET_FIELD );
                }
        }

        dg.add_uint32( ctx );
        dg.add_uint32( doid );

        if ( fieldnames.size() > 1 )
        {
                dg.add_uint16( fieldnames.size() );
        }

        for ( size_t i = 0; i < fieldnames.size(); i++ )
        {
                DCFieldPP *field = dclass->GetFieldWrapper( dclass->GetDClass()->get_field_by_name( fieldnames[i] ) );
                if ( field == NULL )
                {
                        // boi
                        return;
                }
                dg.add_uint16( field->GetField()->get_number() );
        }

        m_pAIR->Send( dg );
}

void AstronDatabaseInterface::HandleQueryObjectResp( int msgtype, DatagramIterator &di )
{
        int ctx = di.get_uint32();
        bool success = di.get_uint8() != 0;

        if ( m_mCallbacksQuery.find( ctx ) == m_mCallbacksQuery.end() )
        {
                return;
        }

        try
        {
                if ( !success )
                {
                        if ( m_mCallbacksQuery[ctx] != NULL )
                        {
                                ( *m_mCallbacksQuery[ctx] )( m_mCallbacksData[ctx], NULL, DCPacker() );
                        }
                        m_mCallbacksQuery.erase( m_mCallbacksQuery.find( ctx ) );
                        m_mCallbacksData.erase( m_mCallbacksData.find( ctx ) );
                        m_mDclasses.erase( m_mDclasses.find( ctx ) );
                        return;
                }

                DCClassPP *dclass;

                uint16_t dclassid;

                if ( msgtype == DBSERVER_OBJECT_GET_ALL_RESP )
                {
                        dclassid = di.get_uint16();
                        dclass = ( ( AstronInternalRepository * )m_pAIR )->m_mDClassByNumber[dclassid];
                }
                else
                {
                        dclass = m_mDclasses[ctx];
                }

                if ( dclass == NULL )
                {
                        astronDatabaseInterface_cat.error()
                                        << "Received bad dclass " << dclassid << " in DBSERVER_OBJECT_GET_ALL_RESP\n";
                        m_mCallbacksQuery.erase( m_mCallbacksQuery.find( ctx ) );
                        m_mCallbacksData.erase( m_mCallbacksData.find( ctx ) );
                        m_mDclasses.erase( m_mDclasses.find( ctx ) );
                        return;
                }

                uint16_t fieldcount;
                if ( msgtype == DBSERVER_OBJECT_GET_FIELD_RESP )
                {
                        fieldcount = 1;
                }
                else
                {
                        fieldcount = di.get_uint16();
                }

                DCPacker unpacker;
                unpacker.set_unpack_data( di.get_remaining_bytes() );

                if ( m_mCallbacksQuery[ctx] != NULL )
                {
                        ( *m_mCallbacksQuery[ctx] )( m_mCallbacksData[ctx], dclass, unpacker );
                }

        }
        catch ( const exception & )
        {
        }

        m_mCallbacksQuery.erase( m_mCallbacksQuery.find( ctx ) );
        m_mCallbacksData.erase( m_mCallbacksData.find( ctx ) );
        m_mDclasses.erase( m_mDclasses.find( ctx ) );
}

void AstronDatabaseInterface::UpdateObject( int dbid, DOID_TYPE doid, DCClassPP *dclass,
                                            AstronDatabaseInterface::DBFieldMap &newfields,
                                            AstronDatabaseInterface::CallbackUpdate *clbk )
{
        DCPacker packer;
        int fieldcount = 0;
        for ( DBFieldMap::iterator itr = newfields.begin();
              itr != newfields.end(); ++itr )
        {
                DCFieldPP *field = dclass->GetFieldWrapper( dclass->GetDClass()->get_field_by_name( itr->first ) );
                if ( field == NULL )
                {
                        return;
                }
                packer.raw_pack_uint16( field->GetField()->get_number() );
                packer.begin_pack( field->GetField() );
                packer.append_data( itr->second.get_data(), itr->second.get_length() );
                packer.end_pack();

                fieldcount++;
        }

        Datagram dg;
        if ( fieldcount == 1 )
        {
                DgAddServerHeader( dg, dbid, m_pAIR->GetOurChannel(), DBSERVER_OBJECT_SET_FIELD );
        }
        else
        {
                DgAddServerHeader( dg, dbid, m_pAIR->GetOurChannel(), DBSERVER_OBJECT_SET_FIELDS );
        }

        dg.add_uint32( doid );
        if ( fieldcount != 1 )
        {
                dg.add_uint16( fieldcount );
        }
        dg.append_data( packer.get_string() );
        m_pAIR->Send( dg );

}

void AstronDatabaseInterface::HandleUpdateObjectResp( DatagramIterator &di, bool multi )
{

}

void AstronDatabaseInterface::HandleDatagram( int msgtype, DatagramIterator &di )
{
        switch ( msgtype )
        {
        case DBSERVER_CREATE_OBJECT_RESP:
                HandleCreateObjectResp( di );
                break;
        case DBSERVER_OBJECT_GET_ALL_RESP:
        case DBSERVER_OBJECT_GET_FIELDS_RESP:
        case DBSERVER_OBJECT_GET_FIELD_RESP:
                HandleQueryObjectResp( msgtype, di );
                break;
        case DBSERVER_OBJECT_SET_FIELD_IF_EQUALS_RESP:
                HandleUpdateObjectResp( di, false );
                break;
        case DBSERVER_OBJECT_SET_FIELDS_IF_EQUALS_RESP:
                HandleUpdateObjectResp( di, false );
                break;
        }
}