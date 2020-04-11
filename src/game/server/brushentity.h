#pragma once

#include "baseentity.h"

class EXPORT_SERVER_DLL CBrushEntity : public CBaseEntity
{
	DECLARE_CLASS( CBrushEntity, CBaseEntity )

public:
	virtual void init( entid_t entnum );
	virtual void spawn();

	virtual bool can_transition() const;
};