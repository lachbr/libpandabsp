
#include "config_module.h"
#include "dconfig.h"
#include "rnCrowdAgent.h"
#include "rnNavMesh.h"
#include "rnNavMeshManager.h"


Configure( config_recastnavigation );
NotifyCategoryDef( recastnavigation , "");

ConfigureFn( config_recastnavigation ) {
  init_librecastnavigation();
}

void
init_librecastnavigation() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  // Init your dynamic types here, e.g.:
  // MyDynamicClass::init_type();
  RNNavMesh::init_type();
  RNCrowdAgent::init_type();
  RNNavMeshManager::init_type();
  RNNavMesh::register_with_read_factory();
  RNCrowdAgent::register_with_read_factory();

  return;
}

