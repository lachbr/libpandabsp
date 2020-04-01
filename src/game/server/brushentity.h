#pragma once

#include "baseentity.h"

class EXPORT_SERVER_DLL CBrushEntity : public CBaseEntity
{
	DECLARE_CLASS( CBrushEntity, CBaseEntity );
	DECLARE_SERVERCLASS();

public:
	virtual void spawn();

	virtual bool can_transition() const;
};