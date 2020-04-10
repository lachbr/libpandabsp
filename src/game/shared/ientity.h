#pragma once

#include "entityshared.h"
#include "typedReferenceCount.h"
#include "pointerTo.h"

class DataTable;

class IEntity : public TypedReferenceCount
{
public:
	virtual entid_t get_entnum() const = 0;
	virtual PT( IEntity ) make_new() = 0;
	virtual void despawn() = 0;
	virtual DataTable *get_network_class() = 0;
};