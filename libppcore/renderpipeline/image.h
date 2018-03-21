#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <texture.h>
#include "..\config_pandaplus.h"

class EXPCL_PANDAPLUS Image : public Texture
{
public:

        static pvector<PT( Image )> registered_images;

        Image( const string &name );

        static PT( Image ) create_counter( const string &name );
        static PT( Image ) create_buffer( const string &name, int size, Texture::Format format );
        static PT( Image ) create_2d( const string &name, int size_x, int size_y, Texture::Format format );
        static PT( Image ) create_2d_array( const string &name, int size_x, int size_y, int size_z, Texture::Format format );
        static PT( Image ) create_3d( const string &name, int size_x, int size_y, int size_z, Texture::Format format );
        static PT( Image ) create_cube( const string &name, int size, Texture::Format format );
        static PT( Image ) create_cube_array( const string &name, int size, int num_cubemaps, Texture::Format format );

        static Texture::ComponentType tex_type_from_format( Texture::Format format );

};

#endif // __IMAGE_H__