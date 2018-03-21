#ifndef __SIMPLE_INPUT_BLOCK_H__
#define __SIMPLE_INPUT_BLOCK_H__

#include "..\config_pandaplus.h"
#include "renderTarget.h"

#include <pmap.h>

class EXPCL_PANDAPLUS SimpleInputBlock
{
public:
        SimpleInputBlock( const string &name = "" );

        void add_input( const ShaderInput *inp );
        void bind_to( RenderTarget *target );

private:
        typedef pvector<const ShaderInput *> InputVector;
        InputVector _inputs;
};

#endif // __SIMPLE_INPUT_BLOCK_H__