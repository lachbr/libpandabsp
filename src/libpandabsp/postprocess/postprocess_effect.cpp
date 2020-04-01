/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file postprocess_effect.cpp
 * @author Brian Lach
 * @date July 24, 2019
 */

#include "postprocess/postprocess_effect.h"
#include "postprocess/postprocess_pass.h"

/////////////////////////////////////////////////////////////////////////////////////////
// PostProcessEffect

IMPLEMENT_CLASS( PostProcessEffect );

void PostProcessEffect::setup()
{
	for ( size_t i = 0; i < _passes.size(); i++ )
	{
		_passes.get_data( i )->setup();
	}
}

void PostProcessEffect::shutdown()
{
	for ( size_t i = 0; i < _passes.size(); i++ )
	{
		_passes.get_data( i )->shutdown();
	}
	_passes.clear();

	_pp = nullptr;
}

void PostProcessEffect::update()
{
	size_t passes = _passes.size();
	for ( size_t i = 0; i < passes; i++ )
	{
		_passes.get_data( i )->update();
	}
}

void PostProcessEffect::window_event( GraphicsOutput *output )
{
	size_t passes = _passes.size();
	for ( size_t i = 0; i < passes; i++ )
	{
		_passes.get_data( i )->window_event( output );
	}
}

void PostProcessEffect::add_pass( PostProcessPass *pass )
{
	_passes[pass->get_name()] = pass;
}

void PostProcessEffect::remove_pass( PostProcessPass *pass )
{
	int itr = _passes.find( pass->get_name() );
	if ( itr != -1 )
		_passes.remove_element( itr );
}

PostProcessPass *PostProcessEffect::get_pass( const std::string &name )
{
	return _passes[name];
}
