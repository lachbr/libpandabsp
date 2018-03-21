#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include "config_vifparser.h"

#include <iostream>
#include <vector>

using namespace std;

struct Token
{
        string type;
        string value;
        Token( string ty, string val )
        {
                type = ty;
                value = val;
        }
};

typedef EXPCL_VIF vector<Token> TokenVec;

extern EXPCL_VIF TokenVec tokenizer( string &input );

#endif // __TOKENIZER_H__