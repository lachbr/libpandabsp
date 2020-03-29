/**
 * PANDA3D BSP TOOLS
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file clhelper.h
 * @author Brian Lach
 * @date November 25, 2018
 */

#ifndef CLHELPER_H
#define CLHELPER_H

#include "common_config.h"

#include <CL/cl.h>

#include <pvector.h>

class _BSPEXPORT CLHelper
{
public:
        class CLProgram;

        class CLKernel
        {
        public:
                CLKernel( CLProgram *prog, const char *name );
                bool SetupKernel();

                void Execute( size_t dimensions, const size_t global_work_size );

        private:
                CLProgram *_program;
                const char *_name;
                cl_kernel _kernel;
        };

        class CLProgram
        {
        public:
                void SetupProgram( const char *filename );
                CLKernel *MakeKernel( const char *kernel_name );

                cl_program _program;

        private:
                pvector<CLKernel> _kernels;
        };

        static void SetupCL( cl_uint preferred_device = CL_DEVICE_TYPE_GPU );
        static CLProgram *MakeProgram( const char *filename );

        static cl_mem MakeAndWriteBuffer( bool read, bool write, size_t size, bool blocking, void *host_mem );

        static cl_mem MakeBuffer( bool read, bool write, size_t size );
        static void WriteBuffer( cl_mem buf, bool blocking, size_t size, void *host_mem );

public:
        static cl_context context;
        static cl_command_queue command_queue;
        static pvector<CLProgram> programs;

private:
        static bool is_setup;
}

#endif // CLHELPER_H