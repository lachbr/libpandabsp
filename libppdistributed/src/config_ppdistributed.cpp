#include "config_ppdistributed.h"
#include "distributedobject_base.h"

ConfigureDef( config_ppdistributed );

ConfigureFn( config_ppdistributed )
{
        init_libppdistributed();
}

// TIME MANAGER:
ConfigVariableDouble timemanager_update_freq
( "time-manager-freq", 1800 );

ConfigVariableDouble timemanager_min_wait
( "time-manager-min-wait", 10 );

ConfigVariableDouble timemanager_max_uncertainty
( "time-manager-max-uncertainty", 1 );

ConfigVariableInt timemanager_max_attemps
( "time-manager-max-attemps", 5 );

ConfigVariableInt timemanager_extra_skew
( "time-manager-extra-skew", 0 );

ConfigVariableDouble timemanager_report_fr_ival
( "report-frame-rate-interval", 300.0 );

void init_libppdistributed()
{
        static bool initialized = false;

        if ( initialized )
        {
                return;
        }

        initialized = true;
}