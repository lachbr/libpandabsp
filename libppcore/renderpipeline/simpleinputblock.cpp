#include "simpleInputBlock.h"

SimpleInputBlock::
SimpleInputBlock( const string &name )
{
}

void SimpleInputBlock::
add_input( const ShaderInput *inp )
{
        _inputs.push_back( inp );
}

void SimpleInputBlock::
bind_to( RenderTarget *target )
{
        for ( size_t i = 0; i < _inputs.size(); i++ )
        {
                target->set_shader_input( _inputs[i] );
        }
}