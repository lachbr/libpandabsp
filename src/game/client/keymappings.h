#pragma once

#include "config_clientdll.h"
#include <referenceCount.h>
#include <simpleHashMap.h>
#include <buttonHandle.h>

class EXPORT_CLIENT_DLL CKeyMapping
{
public:
	CKeyMapping();
	CKeyMapping( int button_type, const ButtonHandle &btn );

	int button_type;
	ButtonHandle button;

	bool is_down() const;
};

class EXPORT_CLIENT_DLL CKeyMappings : public ReferenceCount
{
public:
	void add_mapping( int button_type, const ButtonHandle &btn );
	
	SimpleHashMap<int, CKeyMapping, int_hash> mappings;
};