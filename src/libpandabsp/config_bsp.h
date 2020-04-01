/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file config_bsp.h
 * @author Brian Lach
 * @date March 27, 2018
 */

#ifndef CONFIG_BSP_H
#define CONFIG_BSP_H

#include <dconfig.h>
#include <renderAttrib.h>

#ifdef BUILDING_LIBPANDABSP
#define EXPCL_PANDABSP EXPORT_CLASS
#define EXPTP_PANDABSP EXPORT_TEMPL
#else
#define EXPCL_PANDABSP IMPORT_CLASS
#define EXPTP_PANDABSP IMPORT_TEMPL
#endif

extern EXPCL_PANDABSP vector_string parse_cmd( const std::string &cmd );

ConfigureDecl( config_bsp, EXPCL_PANDABSP, EXPTP_PANDABSP );

#define DECLARE_CLASS2(classname, parentname1, parentname2)\
private:\
    static TypeHandle _type_handle;\
public:\
  static TypeHandle get_class_type() {\
    return _type_handle;\
  }\
  static void init_type() {\
    parentname1::init_type();\
    parentname2::init_type();\
    register_type(_type_handle, #classname,\
                  parentname1::get_class_type(),\
		  parentname2::get_class_type());\
  }\
  virtual TypeHandle get_type() const {\
    return classname::get_class_type();\
  }\
  virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

#define DECLARE_CLASS(classname, parentname)\
private:\
    static TypeHandle _type_handle;\
public:\
  typedef parentname BaseClass;\
  typedef classname MyClass;\
  static TypeHandle get_class_type() {\
    return _type_handle;\
  }\
  static void init_type() {\
    parentname::init_type();\
    register_type(_type_handle, #classname,\
                  parentname::get_class_type());\
  }\
  virtual TypeHandle get_type() const {\
    return classname::get_class_type();\
  }\
  virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

#define IMPLEMENT_CLASS(classname)\
TypeHandle classname::_type_handle; \
static int __init__##classname##_typehandle() \
{\
	classname::init_type(); \
	return 1; \
} \
int __##classname##_init_value = __init__##classname##_typehandle();

#define DECLARE_ATTRIB(classname, parentname)\
private:\
  static TypeHandle _type_handle;\
  static int _attrib_slot;\
protected:\
  virtual int compare_to_impl( const RenderAttrib *other ) const;\
  virtual size_t get_hash_impl() const;\
PUBLISHED:\
  static int get_class_slot() {\
    return _attrib_slot;\
  }\
  virtual int get_slot() const {\
    return get_class_slot();\
  }\
  MAKE_PROPERTY( class_slot, get_class_slot );\
public:\
  static TypeHandle get_class_type() {\
    return _type_handle;\
  }\
  static void init_type() {\
    parentname::init_type();\
    register_type(_type_handle, #classname,\
                  parentname::get_class_type());\
    _attrib_slot = register_slot( _type_handle, 100, new classname );\
  }\
  virtual TypeHandle get_type() const {\
    return classname::get_class_type();\
  }\
  virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

#define IMPLEMENT_ATTRIB(classname)\
TypeHandle classname::_type_handle;\
int classname::_attrib_slot;

#define STUB_ATTRIB(classname)\
class classname : public RenderAttrib {\
  DECLARE_ATTRIB(classname, RenderAttrib);\
  PUBLISHED:\
    static CPT(RenderAttrib) make();\
};\
IMPLEMENT_ATTRIB(classname);\
CPT(RenderAttrib) classname::make()\
{\
        classname *attr = new classname;\
        return return_new(attr);\
}\
int classname::compare_to_impl( const RenderAttrib *other ) const\
{\
	return 0;\
}\
size_t classname::get_hash_impl() const\
{\
	return 0;\
}

#ifndef CPPPARSER
extern EXPCL_PANDABSP void init_libpandabsp();
#endif

#endif // CONFIG_BSP_H
