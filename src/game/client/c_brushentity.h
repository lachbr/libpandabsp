#pragma once

#include "c_baseentity.h"

class EXPORT_CLIENT_DLL C_BrushEntity : public C_BaseEntity
{
	DECLARE_CLASS( C_BrushEntity, C_BaseEntity )
	DECLARE_CLIENTCLASS()

public:
	virtual void spawn();
};