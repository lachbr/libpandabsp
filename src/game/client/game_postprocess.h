#pragma once

#include "config_clientdll.h"

#include "postprocess/postprocess.h"
#include "postprocess/hdr.h"
#include "postprocess/bloom.h"
#include "postprocess/fxaa.h"

class EXPORT_CLIENT_DLL GamePostProcess : public PostProcess
{
public:
	GamePostProcess();

	virtual void setup();
	virtual void cleanup();

public:
	bool _hdr_enabled;
	bool _bloom_enabled;
	bool _fxaa_enabled;

	PT( HDREffect ) _hdr;
	PT( BloomEffect ) _bloom;
	PT( FXAA_Effect ) _fxaa;
};