#ifndef __GPU_COMMAND_QUEUE_H__
#define __GPU_COMMAND_QUEUE_H__

#include "..\config_pandaplus.h"
#include "gpuCommand.h"
#include "gpuCommandList.h"
#include "image.h"
#include "renderTarget.h"
#include <pta_int.h>

class EXPCL_PANDAPLUS GPUCommandQueue
{
public:
        GPUCommandQueue();
        void init();
        void register_input( const ShaderInput *inp );
        void reload_shaders();
        void process_queue();
        void clear_queue();

        int get_num_processed_commands() const;
        size_t get_num_queued_commands();
        GPUCommandList *get_command_list();

private:
        GPUCommandList _list;
        int _commands_per_frame;
        void create_data_storage();
        void create_command_target();
        //void register_defines();

        PT( Image ) _data_texture;
        PT( RenderTarget ) _command_target;

        PTA_int _pta_num_commands;

};

#endif // __GPU_COMMAND_QUEUE_H__