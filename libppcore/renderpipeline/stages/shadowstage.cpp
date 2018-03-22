#include "shadowStage.h"
#include "..\..\pp_globals.h"

vector_string ShadowStage::get_required_inputs()
{
        vector_string vec;
        return vec;
}

vector_string ShadowStage::get_required_pipes()
{
        vector_string vec;
        return vec;
}

ShadowStage::ShadowStage() :
        RenderStage()
{
        _size = 4096;
}

void ShadowStage::make_pcf_state()
{
        _pcf.set_minfilter( SamplerState::FT_shadow );
        _pcf.set_magfilter( SamplerState::FT_shadow );
}

PT( GraphicsOutput ) ShadowStage::get_atlas_buffer() const
{
        return _target->get_internal_buffer();
}

void ShadowStage::create()
{
        make_pcf_state();

        _target = create_target( "ShadowAtlas" );
        _target->set_size( LVecBase2i( _size ) );
        _target->add_depth_attachment( 16 );
        _target->prepare_render( NodePath() );

        _target->get_internal_buffer()->remove_all_display_regions();
        _target->get_internal_buffer()->get_display_region( 0 )->set_active( false );

        _target->set_active( false );
        _target->get_internal_buffer()->set_clear_depth_active( false );
        _target->_source_region_def.set_clear_depth_active( false );
}

void ShadowStage::bind()
{
}

void ShadowStage::set_size( int size )
{
        _size = size;
}

int ShadowStage::get_size() const
{
        return _size;
}

void ShadowStage::set_shader_input( const ShaderInput *inp )
{
        render.set_shader_input( inp );
}