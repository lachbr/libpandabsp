#include "pp_dcbase.h"

ClsName2Singleton dc_singleton_by_name;
DClass2Fields dc_field_data;

int game_globals_id = 0;
bool dc_has_owner_view = false;

DCFile dc_file;