#include "renderStage.h"

#include <pta_int.h>
#include <pta_float.h>
#include <pta_LMatrix4.h>
#include <pta_LMatrix3.h>

#include "..\..\pp_globals.h"

bool RenderStage::disabled = false;

UniqueIdAllocator RenderStage::id_alloc( 0, 9999 );

vector_string RenderStage::
get_required_inputs()
{
        vector_string vec;
        return vec;
}

vector_string RenderStage::
get_required_pipes()
{
        vector_string vec;
        return vec;
}

RenderStage::
RenderStage()
{
        _id = id_alloc.allocate();
        _active = true;
}

string get_def_vert_path()
{
        return "shader/default_post_process.vert.glsl";
}

/**
 * Loads the fragment shader specified along with the default vert shader.
 */
PT( Shader ) RenderStage::
load_shader( const string &frag_path )
{
        return Shader::load( Shader::SL_GLSL, get_def_vert_path(), frag_path );
}

PT( Shader ) RenderStage::
load_shader( const string &vert_path, const string &frag_path )
{
        return Shader::load( Shader::SL_GLSL, vert_path, frag_path );
}

void RenderStage::
create()
{
}

void RenderStage::
set_active( bool active )
{
        if ( _active != active )
        {
                _active = active;
                for ( size_t i = 0; i < _targets.size(); i++ )
                {
                        _targets[i]->set_active( _active );
                }
        }
}

bool RenderStage::
get_active() const
{
        return _active;
}

PT( RenderTarget ) RenderStage::
create_target( const string &name )
{
        string nname = to_string( ( long long )_id ) + "." + name;
        PT( RenderTarget ) target = new RenderTarget( nname );
        _targets.push_back( target );
        return target;
}

void RenderStage::
bind_to_commons()
{
        // MainSceneData:
        set_shader_input( new ShaderInput( "view_mat_z_up", rpipeline->_view_mat_z_up ) );
        set_shader_input( new ShaderInput( "view_mat_billboard", rpipeline->_view_mat_billboard ) );
        set_shader_input( new ShaderInput( "camera_pos", rpipeline->_camera_pos ) );
        set_shader_input( new ShaderInput( "last_view_proj_mat_no_jitter", rpipeline->_last_view_proj_mat_no_jitter ) );
        set_shader_input( new ShaderInput( "last_inv_view_proj_mat_no_jitter", rpipeline->_last_inv_view_proj_mat_no_jitter ) );
        set_shader_input( new ShaderInput( "proj_mat", rpipeline->_proj_mat ) );
        set_shader_input( new ShaderInput( "inv_proj_mat", rpipeline->_inv_proj_mat ) );
        set_shader_input( new ShaderInput( "view_proj_mat_no_jitter", rpipeline->_view_proj_mat_no_jitter ) );
        set_shader_input( new ShaderInput( "frame_delta", rpipeline->_frame_delta ) );
        set_shader_input( new ShaderInput( "smooth_frame_delta", rpipeline->_smooth_frame_delta ) );
        set_shader_input( new ShaderInput( "frame_time", rpipeline->_frame_time ) );
        set_shader_input( new ShaderInput( "current_film_offset", rpipeline->_current_film_offset ) );
        set_shader_input( new ShaderInput( "frame_index", rpipeline->_frame_index ) );
        set_shader_input( new ShaderInput( "vs_frustum_directions", rpipeline->_vs_frustum_directions ) );
        set_shader_input( new ShaderInput( "ws_frustum_directions", rpipeline->_ws_frustum_directions ) );
        set_shader_input( new ShaderInput( "screen_size", rpipeline->_screen_size ) );
        set_shader_input( new ShaderInput( "native_screen_size", rpipeline->_native_screen_size ) );
        set_shader_input( new ShaderInput( "lc_tile_count", rpipeline->_lc_tile_count ) );
        ////////////////////////////////////////////////////////

        set_shader_input( new ShaderInput( "mainCam", base->_window->get_camera_group() ) );
        set_shader_input( new ShaderInput( "mainRender", render ) );
}

void RenderStage::
bind()
{
}

PT( RenderTarget ) RenderStage::
get_target() const
{
        return _target;
}

void RenderStage::
set_shader_input( const ShaderInput *inp )
{
        for ( size_t i = 0; i < _targets.size(); i++ )
        {
                _targets[i]->set_shader_input( inp );
        }
}

const pvector<PT( RenderTarget )> &RenderStage::
get_targets() const
{
        return _targets;
}

void RenderStage::
remove_target( PT( RenderTarget ) target )
{
        target->remove();

        pvector<PT( RenderTarget )>::const_iterator itr = find( _targets.begin(), _targets.end(), target );
        if ( itr != _targets.end() )
        {
                _targets.erase( itr );
        }
}

int RenderStage::
get_target_index( const string &name )
{
        int index = -1;
        for ( size_t i = 0; i < _targets.size(); i++ )
        {
                if ( _targets[i]->get_name().compare( name ) == 0 )
                {
                        index = i;
                        break;
                }
        }
        return index;
}

void RenderStage::
reload_shaders()
{
}

void RenderStage::
update()
{
}

void RenderStage::
set_dimensions()
{
}

void RenderStage::
handle_window_resize()
{
        set_dimensions();
        for ( size_t i = 0; i < _targets.size(); i++ )
        {
                _targets[i]->consider_resize();
        }
}