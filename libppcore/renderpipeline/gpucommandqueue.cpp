#include "gpuCommandQueue.h"
#include <texture.h>

GPUCommandQueue::GPUCommandQueue()
{
        _commands_per_frame = 1024;
        _pta_num_commands = PTA_int::empty_array( 1 );
}

void GPUCommandQueue::init()
{
        create_data_storage();
        create_command_target();
        //register_defines(
}

void GPUCommandQueue::create_data_storage()
{
        int combsize = _commands_per_frame * 32;
        _data_texture = Image::create_buffer( "CommandQueue", combsize, Texture::F_r32 );
}

void GPUCommandQueue::register_input( const ShaderInput *inp )
{
        _command_target->set_shader_input( inp );
}

void GPUCommandQueue::clear_queue()
{
}

void GPUCommandQueue::process_queue()
{
        PTA_uchar ptr = _data_texture->modify_ram_image();
        int num_cmds_exec = _list.write_commands_to( ptr, _commands_per_frame );
        _pta_num_commands[0] = num_cmds_exec;
}

void GPUCommandQueue::create_command_target()
{
        _command_target = new RenderTarget( "ExecCommandTarget" );
        _command_target->set_size( LVecBase2i( 1, 1 ) );
        _command_target->prepare_buffer();
        _command_target->set_shader_input( new ShaderInput( "CommandQueue", _data_texture ) );
        _command_target->set_shader_input( new ShaderInput( "commandCount", _pta_num_commands ) );
}

int GPUCommandQueue::get_num_processed_commands() const
{
        return _pta_num_commands[0];
}

size_t GPUCommandQueue::get_num_queued_commands()
{
        return _list.get_num_commands();
}

GPUCommandList *GPUCommandQueue::get_command_list()
{
        return &_list;
}

void GPUCommandQueue::reload_shaders()
{
        PT( Shader ) shader = Shader::load( Shader::SL_GLSL, "shader/default_post_process.vert.glsl",
                                            "shader/process_command_queue.frag.glsl" );
        _command_target->set_shader( shader );
}