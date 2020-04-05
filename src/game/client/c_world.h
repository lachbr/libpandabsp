#pragma once

#include "c_brushentity.h"

class EXPORT_CLIENT_DLL C_World : public C_BrushEntity
{
	DECLARE_CLIENTCLASS( C_World, C_BrushEntity )

public:
	virtual void update_parent_entity( int entnum );
};