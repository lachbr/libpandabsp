#ifndef __BUILDER_H__
#define __BUIDLER_H__

#include <pandaFramework.h>

class Parser;

class Builder
{
public:
        Builder( const char *output, WindowFramework *window, Parser *p );
};

#endif // __BUILDER_H__