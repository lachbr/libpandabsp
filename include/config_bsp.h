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

//

ConfigureDecl( config_bsp, EXPCL_PANDABSP, EXPTP_PANDABSP );

#define TypeDecl2(classname, parentname1, parentname2)\
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

#define TypeDecl(classname, parentname)\
private:\
    static TypeHandle _type_handle;\
public:\
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

#define TypeDef(classname)\
TypeHandle classname::_type_handle;

#define AttribDecl(classname, parentname)\
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

#define AttribDef(classname)\
TypeHandle classname::_type_handle;\
int classname::_attrib_slot;

#define StubAttrib(classname)\
class classname : public RenderAttrib {\
  AttribDecl(classname, RenderAttrib);\
  PUBLISHED:\
    static CPT(RenderAttrib) make();\
};\
AttribDef(classname);\
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
