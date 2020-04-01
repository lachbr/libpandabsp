#include "config_clientdll.h"

ConfigVariableDouble cl_normspeed( "cl_normspeed", 320 / 16.0 );
ConfigVariableDouble cl_walkspeed( "cl_walkspeed", 190 / 16.0 );
ConfigVariableDouble cl_runspeed( "cl_runspeed", 416 / 16.0 );
ConfigVariableDouble cl_mousesensitivity( "cl_mousesensitivity", 0.07 );
ConfigVariableDouble cl_duckspeed( "cl_duckspeed", 75.0 / 16.0 );

ConfigVariableInt cl_cmdrate
( "cl_cmdrate", 64,
  "Maximum command packets per second, sending to the server." );