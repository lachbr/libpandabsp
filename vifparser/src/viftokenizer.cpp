#include "vifTokenizer.h"

#include <string>

#include <algorithm>

TokenVec tokenizer( string &input )
{

        char chars[63] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                           'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                           'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
                           'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F',
                           'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                           'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                           'W', 'X', 'Y', 'Z', '0', '1', '2', '3',
                           '4', '5', '6', '7', '8', '9', '_'
                         };
        vector<char> valid_chars;
        for ( int i = 0; i < 63; i++ )
        {
                valid_chars.push_back( chars[i] );
        }

        TokenVec tokens;
        int current = 0;

        bool comment = false;

        char lastlet = ' ';

        string commentcontents = "";

        //cout << input.length() << endl;

        while ( current < input.length() )
        {
                char let = input[current];
                string val;
                if ( let == '/' && lastlet == '/' && !comment )
                {
                        // ooo a comment!
                        //cout << "Entering a comment at position " << current << endl;
                        comment = true;
                        current++;
                }
                else if ( let == '}' || let == '{' && !comment )
                {
                        val = "";
                        val += let;
                        tokens.push_back( Token( "bracket", val ) );
                        current++;
                }
                else if ( let == '"' && !comment )
                {
                        val = "";
                        let = input[++current];
                        while ( let != '"' )
                        {
                                val += let;
                                let = input[++current];
                        }
                        tokens.push_back( Token( "string", val ) );
                        current++;
                }
                else if ( let == ' ' || let == '\n' || let == '\t' || let == '\v' ||
                          let == '\r' || let == '\f' || let == '\a' || let == '\b' )
                {
                        if ( let == '\n' && comment )
                        {
                                //cout << "Exiting comment at position " << current << endl;
                                comment = false;
                        }
                        current++;
                }
                else if ( find( valid_chars.begin(), valid_chars.end(), let ) != valid_chars.end() && !comment )
                {
                        val = "";
                        while ( find( valid_chars.begin(), valid_chars.end(), let ) != valid_chars.end() )
                        {
                                val += let;
                                let = input[++current];
                        }
                        tokens.push_back( Token( "name", val ) );
                }
                else
                {
                        if ( !comment )
                        {
                                cout << "Unknown character " << let << " at position " << current << endl;
                                cout << let << endl;
                        }
                        else
                        {
                                current++;
                        }
                }
                if ( comment )
                {
                        commentcontents += let;
                }
                else if ( !comment && commentcontents.length() != 0 )
                {
                        //cout << "Contents of comment was: " << commentcontents << endl;
                        commentcontents = "";
                }
                lastlet = let;
        }

        return tokens;
}