// Copyright (c) 1996-2018, Valve Corporation, All rights reserved.

#include "rangecheckedvar.h"
#include <thread.h>

bool g_bDoRangeChecks = true;

static int g_nDisables = 0;

CDisableRangeChecks::CDisableRangeChecks()
{
	if ( !( Thread::get_current_thread() == Thread::get_main_thread() ) )
		return;
	g_nDisables++;
	g_bDoRangeChecks = false;
}

CDisableRangeChecks::~CDisableRangeChecks()
{
	if ( !( Thread::get_current_thread() == Thread::get_main_thread() ) )
		return;
	nassertv( g_nDisables > 0 );
	--g_nDisables;
	if ( g_nDisables == 0 )
	{
		g_bDoRangeChecks = true;
	}
}