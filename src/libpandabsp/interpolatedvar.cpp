// Copyright (c) 1996-2018, Valve Corporation, All rights reserved.
//
// Purpose:
//

#include "interpolatedvar.h"

CInterpolationContext *CInterpolationContext::s_pHead = NULL;
bool CInterpolationContext::s_bAllowExtrapolation = false;
float CInterpolationContext::s_flLastTimeStamp = 0;

float g_flLastPacketTimestamp = 0;

ConfigVariableDouble cl_extrapolate_amount(
	"cl_extrapolate_amount", 0.25,
	"Set how many seconds the client will extrapolate entities for." );