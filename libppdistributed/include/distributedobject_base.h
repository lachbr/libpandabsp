#ifndef DISTRIBUTED_OBJECT_BASE_H
#define DISTRIBUTED_OBJECT_BASE_H

#include <notifyCategoryProxy.h>
#include <pointerTo.h>
#include <typedObject.h>
#include <pvector.h>
#include <dcPacker.h>

#include "config_pandaplus.h"
#include "dcfield_pp.h"
#include "dcclass_pp.h"
#include "pp_dcbase.h"

#define DClassDecl_omitInitType(classname, parentname)\
  private:\
    static TypeHandle _type_handle;\
    static bool _initialized;\
    static bool _never_disable;\
    static bool _global_do;\
  public:\
    static TypeHandle get_class_type() {\
      return _type_handle;\
    }\
    virtual TypeHandle get_type() const {\
      return classname::get_class_type();\
    }\
    static classname *make_new_static(bool add_to_objects = true) {\
      classname *obj = new classname();\
      if (!_initialized) { obj->force_init_type(); }\
      return obj;\
    }\
    static void register_do() {\
      classname::make_new_static(false);\
    }\
    virtual classname *make_new(bool add_to_objects = true) {\
      return classname::make_new_static(add_to_objects);\
    }\
    static bool get_never_disable_static() {\
      return _never_disable;\
    }\
    virtual bool get_never_disable() {\
      return classname::get_never_disable_static();\
    }\
    static bool is_global_do_static() {\
      return _global_do;\
    }\
    virtual bool is_global_do() {\
      return classname::is_global_do_static();\
    }

#define DClassDecl(classname, parentname)\
  DClassDecl_omitInitType(classname, parentname);\
  public:\
    static void init_type() {\
      parentname::init_type();\
      register_type(_type_handle, #classname,\
                    parentname::get_class_type());\
      _initialized = true;\
    }\

#define DClassDef(classname)\
TypeHandle classname::_type_handle;\
bool classname::_initialized = false;\
bool classname::_never_disable = false;\
bool classname::_global_do = false;

#define DCFuncArgs DCPacker &packer, void *data

#define DCFuncDecl(fieldname)\
static void fieldname(DCFuncArgs);

// Same as DClassDef, but omits the _global_do initialization.
// You will have to manually initialize the _global_do static.
#define DClassDef_omitGD(classname)\
TypeHandle classname::_type_handle;\
bool classname::_initialized = false;\
bool classname::_never_disable = false;\

// Same as DClassDef, but omits the _never_disable initialization.
// You will have to manually initialize the _never_disable static.
#define DClassDef_omitND(classname)\
TypeHandle classname::_type_handle;\
bool classname::_initialized = false;\
bool classname::_global_do = false;

#define DCFieldDefStart()\
  dc_singleton_by_name.insert(ClsName2Singleton::value_type(_type_handle.get_name(), this));\
  FieldName2SetGetFunc fields_map;\

#define DCFieldDef(fieldname, setterfunc, getterfunc)\
  DCFieldFuncVec fieldname##_funcs;\
  fieldname##_funcs.push_back(setterfunc);\
  fieldname##_funcs.push_back(getterfunc);\
  fields_map.insert(FieldName2SetGetFunc::value_type(#fieldname, fieldname##_funcs));

#define DCFieldDefEnd()\
  dc_field_data.insert(DClass2Fields::value_type(_type_handle.get_name(), fields_map));

#define InitTypeStart() public: virtual TypeHandle force_init_type() { if(_initialized) { return get_class_type(); } init_type();
#define InitTypeEnd() return get_class_type(); }

#ifdef ASTRON_IMPL

#define BeginAIUpdate_CH(fieldname, channel)\
  DCPacker packer = get_dclass()->start_ai_format_update(fieldname, get_do_id(), channel, g_air->get_our_channel());

#define BeginAIUpdate(fieldname)\
  BeginAIUpdate_CH(fieldname, get_do_id());

#define BeginAIUpdate_AVID(fieldname, avid)\
  BeginAIUpdate_CH(fieldname, get_puppet_connection_channel(avid));

#define BeginAIUpdate_ACC(fieldname, accid)\
  BeginAIUpdate_CH(fieldname, get_account_connection_channel(accid));

#define SendAIUpdate(fieldname)\
  Datagram dg = get_dclass()->end_ai_format_update(fieldname, packer);\
  g_air->send(dg);

#else // ASTRON_IMPL

#define SendAIUpdate_CH(fieldname, channel)\
  Datagram dg = get_dclass()->end_client_format_update(fieldname, packer);\
  DatagramIterator dgi(dg);\
  dgi.get_uint16();\
  Datagram dg2;\
  dg2.add_uint16(CLIENT_OBJECT_UPDATE_FIELD_TARGETED_CMU);\
  dg2.add_uint32(channel & 0xffffffff);\
  dg2.append_data(dgi.get_remaining_bytes());\
  g_air->send(dg2);

#define BeginAIUpdate(fieldname) BeginCLUpdate(fieldname)
#define SendAIUpdate(fieldname)\
  Datagram dg = get_dclass()->end_client_format_update(fieldname, packer);\
  g_air->send(dg);

#endif // ASTRON_IMPL

#define BeginCLUpdate(fieldname)\
  DCPacker packer = get_dclass()->start_client_format_update(fieldname, get_do_id());

#define SendCLUpdate(fieldname)\
  Datagram dg = get_dclass()->end_client_format_update(fieldname, packer);\
  g_cr->send(dg);

// Send a "blank" update that contains no extra arguments.
#define SendBlankCLUpdate(fieldname) BeginCLUpdate(fieldname) SendCLUpdate(fieldname)

#define SendBlankAIUpdate(fieldname) BeginAIUpdate(fieldname) SendAIUpdate(fieldname)

#ifdef ASTRON_IMPL
#define SendBlankAIUpdate_CH(fieldname, channel) BeginAIUpdate_CH(fieldname, channel) SendAIUpdate(fieldname)
#define SendBlankAIUpdate_AVID(fieldname, avid) BeginAIUpdate_AVID(fieldname, avid) SendAIUpdate(fieldname)
#define SendBlankAIUpdate_ACC(fieldname, accid) BeginAIUpdate_ACC(fieldname, accid) SendAIUpdate(fieldname)
#else // ASTRON_IMPL
#define SendBlankAIUpdate_CH(fieldname, channel) BeginAIUpdate(fieldname, channel) SendAIUpdate_CH(fieldname, channel)
#endif // ASTRON_IMPL

#define AddArg(type, arg)\
  packer.pack_##type(arg);

NotifyCategoryDeclNoExport( distributedObjectBase );

class EXPCL_PPDISTRIBUTED DistributedObjectBase : public TypedObject
{
        DClassDecl( DistributedObjectBase, TypedObject );

protected:
        DOID_TYPE _do_id;
        DOID_TYPE _parent_id;
        ZONEID_TYPE _zone_id;
        DCClassPP *_dclass;
        DistributedObjectBase *_parent_obj;

public:
        DistributedObjectBase();
        virtual ~DistributedObjectBase();

        virtual void generate_init();
        virtual void generate();
        virtual void announce_generate();
        virtual void disable();
        virtual void delete_do();
        virtual void handle_child_arrive( DistributedObjectBase *child, ZONEID_TYPE zone );
        virtual void handle_child_arrive_zone( DistributedObjectBase *child, ZONEID_TYPE zone );
        virtual void handle_child_leave( DistributedObjectBase *child, ZONEID_TYPE zone );
        virtual void handle_child_leave_zone( DistributedObjectBase *child, ZONEID_TYPE zone );
        virtual void set_location( DOID_TYPE parent_id, ZONEID_TYPE zone_id );
        DCFuncDecl( set_location_field );
        virtual void update_required_fields( DCClassPP *dclass, DatagramIterator &di );
        virtual void update_required_other_fields( DCClassPP *dclass, DatagramIterator &di );
        virtual void update_all_required_fields( DCClassPP *dclass, DatagramIterator &di );

        virtual void delete_or_delay();

        string task_name( const string &task_string ) const;
        string unique_name( const string &id_string ) const;

        uint32_t get_location_pid() const;
        uint32_t get_location_zid() const;

        DistributedObjectBase *get_parent_obj();

        bool has_parenting_rules() const;

        void set_parent_id( DOID_TYPE id );
        DOID_TYPE get_parent_id() const;

        void set_zone_id( ZONEID_TYPE id );
        ZONEID_TYPE get_zone_id() const;

        void set_do_id( DOID_TYPE do_id );
        DOID_TYPE get_do_id() const;

        void set_dclass( DCClassPP *dclass );
        DCClassPP *get_dclass() const;

        InitTypeStart();

        DCFieldDefStart();

        DCFieldDefEnd();

        InitTypeEnd();
};

#endif // DISTRIBUTED_OBJECT_BASE_H