#include "config_serverdll.h"

// Ticks per sec
ConfigVariableInt sv_tickrate
( "sv_tickrate", 66, "How many ticks the server runs per second." );

ConfigVariableInt sv_maxclients
( "sv_maxclients", 1,
  "The maximum number of clients that can connect to the server." );

// 2 ticks ahead or behind current clock means we need to fix clock on client
ConfigVariableInt sv_clockcorrection_msecs
( "sv_clockcorrection_msecs", 60,
  "The server tries to keep each player's tickbase within this many "
  "msecs of the server absolute tickcount." );