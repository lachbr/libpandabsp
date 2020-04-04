#include "config_clientdll.h"

ConfigVariableDouble cl_normspeed( "cl_normspeed", 320 / 16.0 );
ConfigVariableDouble cl_walkspeed( "cl_walkspeed", 190 / 16.0 );
ConfigVariableDouble cl_runspeed( "cl_runspeed", 416 / 16.0 );
ConfigVariableDouble cl_mousesensitivity( "cl_mousesensitivity", 0.07 );
ConfigVariableDouble cl_duckspeed( "cl_duckspeed", 75.0 / 16.0 );

ConfigVariableInt cl_cmdrate
( "cl_cmdrate", 64,
  "Maximum command packets per second, sending to the server." );

ConfigVariableInt cl_updaterate
( "cl_updaterate", 20,
  "Client update rate." );

ConfigVariableDouble cl_interp_ratio
( "cl_interp_ratio", 2.0,
  "Client interpolation ratio." );

ConfigVariableDouble cl_interp
( "cl_interp", 0.1,
  "Seconds of interpolation" );

float get_client_interp_amount()
{
        return std::max( cl_interp.get_value(), cl_interp_ratio / cl_updaterate );
}