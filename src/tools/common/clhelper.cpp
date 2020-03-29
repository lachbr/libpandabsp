#if 0

/**
 * PANDA3D BSP TOOLS
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file clhelper.cpp
 * @author Brian Lach
 * @date November 25, 2018
 */

#include "clhelper.h"

//#include <pnotify.h>
#include <virtualFileSystem.h>

#include <iostream>

#define MAX_PLATFORMS 32

cl_command_queue                CLHelper::command_queue;
cl_context                      CLHelper::context;
bool                            CLHelper::is_setup = false;
pvector<CLHelper::CLProgram>    CLHelper::programs;

void CLHelper::SetupCL( cl_uint preferred_device )
{
        nassertv( is_setup );

        cl_device_id    device_id;
        cl_uint         num_platforms;
        cl_int          err;

        // find number of platforms
        err = clGetPlatformIDs( 0, nullptr, &num_platforms );
        nassertv( num_platforms > 0 );

        // get all platforms
        cl_platform_id platforms[MAX_PLATFORMS];
        err = clGetPlatformIDs( num_platforms, platforms, nullptr );

        // secure whatever device they want
        for ( int i = 0; i < num_platforms; i++ )
        {
                if ( clGetDeviceIDs( platforms[i], preferred_device, 1, &device_id, nullptr ) == CL_SUCCESS )
                        break;
        }

        if ( device_id == nullptr )
        {
                // didn't find it, use whatever is available
                for ( int i = 0; i < num_platforms; i++ )
                {
                        if ( clGetDeviceIDs( platforms[i], CL_DEVICE_TYPE_ALL, 1, &device_id, nullptr ) == CL_SUCCESS )
                                break;
                }
        }

        nassertv( device_id != nullptr );

        context = clCreateContext( 0, 1, &device_id, nullptr, nullptr, &err );
        command_queue = clCreateCommandQueue( context, device_id, 0, &err );

        is_setup = true;
}

CLHelper::CLProgram *CLHelper::MakeProgram( const char *filename )
{
        nassertr( is_setup, nullptr );

        CLProgram prog;
        prog.SetupProgram( filename );

        programs.push_back( prog );

        return &programs[programs.size() - 1];
}

void CLHelper::CLProgram::SetupProgram( const char *filename )
{
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

        nassertv( vfs->exists( filename ) );
        std::string source = vfs->read_file( filename, true );
        const char *psource = source.c_str();

        cl_int err;

        _program = clCreateProgramWithSource( CLHelper::context, 1, &psource, nullptr, &err );
        nassertv( err == CL_SUCCESS );

        err = clBuildProgram( _program, 0, nullptr, nullptr, nullptr, nullptr );
        nassertv( err == CL_SUCCESS );
}

CLHelper::CLKernel *CLHelper::CLProgram::MakeKernel( const char *name )
{
        CLKernel kernel( this, name );
        nassertr( kernel.SetupKernel(), nullptr );

        _kernels.push_back( kernel );

        return &_kernels[_kernels.size() - 1];
}

CLHelper::CLKernel::CLKernel( CLProgram *prog, const char *name ) :
        _program( prog ),
        _name( name )
{
}

bool CLHelper::CLKernel::SetupKernel()
{
        cl_int err;
        _kernel = clCreateKernel( _program->_program, _name, &err );
        if ( err != CL_SUCCESS )
                return false;

        return true;
}

void CLHelper::CLKernel::Execute(size_t dimensions, const size_t global_work_size)
{
        cl_int err;
        err = clEnqueueNDRangeKernel( CLHelper::command_queue, _kernel, dimensions, nullptr,
                                      &global_work_size, nullptr, 0, nullptr, nullptr );
        nassertv( err == CL_SUCCESS );
}

/**
 * Creates a buffer of memory that can be accessed by the compute device.
 */
cl_mem CLHelper::MakeBuffer( bool read, bool write, size_t size )
{
        cl_int err;
        cl_mem buf;
        cl_mem_flags flags;

        if ( read && !write )
                flags = CL_MEM_READ_ONLY;
        else if ( read && write )
                flags = CL_MEM_READ_WRITE;
        else if ( !read && write )
                flags = CL_MEM_WRITE_ONLY;

        buf = clCreateBuffer( CLHelper::context, flags, size, nullptr, &err );
        nassertr( err == CL_SUCCESS, nullptr );

        return buf;
}

/**
 * Writes a buffer into device memory.
 */
void CLHelper::WriteBuffer( cl_mem buf, bool blocking, size_t size, void *host_mem )
{
        cl_int err;
        err = clEnqueueWriteBuffer( CLHelper::command_queue, buf, blocking, 0, size, host_mem, 0, nullptr, nullptr );
        nassertv( err == CL_SUCCESS );
}

/**
 * Creates a buffer and writes it into device memory.
 */
cl_mem CLHelper::MakeAndWriteBuffer( bool read, bool write, size_t size, bool blocking, void *host_mem )
{
        cl_mem buf;

        buf = MakeBuffer( read, write, size );
        WriteBuffer( buf, blocking, size, host_mem );

        return buf;
}

#endif