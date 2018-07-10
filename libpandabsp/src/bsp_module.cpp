
#include "dtoolbase.h"
#include "interrogate_request.h"

#include "py_panda.h"

extern LibraryDef bsp_moddef;
extern void Dtool_bsp_RegisterTypes();
extern void Dtool_bsp_ResolveExternals();
extern void Dtool_bsp_BuildInstants(PyObject *module);

#if PY_MAJOR_VERSION >= 3 || !defined(NDEBUG)
#ifdef _WIN32
extern "C" __declspec(dllexport) PyObject *PyInit_bsp();
#elif __GNUC__ >= 4
extern "C" __attribute__((visibility("default"))) PyObject *PyInit_bsp();
#else
extern "C" PyObject *PyInit_bsp();
#endif
#endif
#if PY_MAJOR_VERSION < 3 || !defined(NDEBUG)
#ifdef _WIN32
extern "C" __declspec(dllexport) void initbsp();
#elif __GNUC__ >= 4
extern "C" __attribute__((visibility("default"))) void initbsp();
#else
extern "C" void initbsp();
#endif
#endif

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef py_bsp_module = {
  PyModuleDef_HEAD_INIT,
  "bsp",
  nullptr,
  -1,
  nullptr,
  nullptr, nullptr, nullptr, nullptr
};

PyObject *PyInit_bsp() {
  PyImport_Import(PyUnicode_FromString("panda3d.core"));
  Dtool_bsp_RegisterTypes();
  Dtool_bsp_ResolveExternals();

  LibraryDef *defs[] = {&bsp_moddef, nullptr};

  PyObject *module = Dtool_PyModuleInitHelper(defs, &py_bsp_module);
  if (module != nullptr) {
    Dtool_bsp_BuildInstants(module);
  }
  return module;
}

#ifndef NDEBUG
void initbsp() {
  PyErr_SetString(PyExc_ImportError, "bsp was compiled for Python " PY_VERSION ", which is incompatible with Python 2");
}
#endif
#else  // Python 2 case

void initbsp() {
  PyImport_Import(PyUnicode_FromString("panda3d.core"));
  Dtool_bsp_RegisterTypes();
  Dtool_bsp_ResolveExternals();

  LibraryDef *defs[] = {&bsp_moddef, nullptr};

  PyObject *module = Dtool_PyModuleInitHelper(defs, "bsp");
  if (module != nullptr) {
    Dtool_bsp_BuildInstants(module);
  }
}

#ifndef NDEBUG
PyObject *PyInit_bsp() {
  PyErr_SetString(PyExc_ImportError, "bsp was compiled for Python " PY_VERSION ", which is incompatible with Python 3");
  return nullptr;
}
#endif
#endif

