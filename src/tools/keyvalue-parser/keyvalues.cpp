/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file keyvalues.cpp
 * @author Brian Lach
 * @date April 15, 2020
 *
 * @desc The new and much cleaner interface for dealing with Valve's KeyValue
 *	 format. KeyValues can either be created procedurally or loaded from
 *	 disk. 
 *
 * Parsing code is based on the implementation from https://github.com/JulioC/keyvalues-python.
 *
 */

#include "keyvalues.h"

#include "virtualFileSystem.h"

NotifyCategoryDeclNoExport( keyvalues )
NotifyCategoryDef( keyvalues, "" )

enum
{
	KVTOKEN_NONE,
	KVTOKEN_BLOCK_BEGIN,
	KVTOKEN_BLOCK_END,
	KVTOKEN_STRING,
	KVTOKEN_MACROS,
};

struct KeyValueToken_t
{
	int type;
	std::string data;

	bool invalid() const
	{
		return type == KVTOKEN_NONE;
	}
};

class CKeyValuesTokenizer
{
public:
	CKeyValuesTokenizer( const std::string &buffer );

	KeyValueToken_t next_token();

private:
	void ignore_whitespace();
	bool ignore_comment();
	std::string get_string();

	char current();
	bool forward();
	char next();
	std::string location();

private:
	std::string _buffer;
	size_t _buflen;
	size_t _position;
	int _last_line_break;
	int _line;
};

CKeyValuesTokenizer::CKeyValuesTokenizer( const std::string &buffer )
{
	_buffer = buffer;
	_buflen = buffer.size();
	_position = 0;
	_last_line_break = 0;
	_line = 1;
}

KeyValueToken_t CKeyValuesTokenizer::next_token()
{
	KeyValueToken_t token;

	while ( 1 )
	{
		ignore_whitespace();
		if ( !ignore_comment() )
		{
			break;
		}
	}

	// Get the next character and check if we got any character
	char c = current();
	if ( !c )
	{
		token.type = KVTOKEN_NONE;
		return token;
	}

	// Emit any valid tokens
	if ( c == '{' )
	{
		forward();
		token.type = KVTOKEN_BLOCK_BEGIN;
		return token;
	}
	else if ( c == '}' )
	{
		forward();
		token.type = KVTOKEN_BLOCK_END;
		return token;
	}
	else
	{
		token.data = get_string();
		token.type = KVTOKEN_STRING;
		return token;
	}
}

std::string CKeyValuesTokenizer::get_string()
{
	bool escape = false;
	std::string result = "";

	bool quoted = false;
	if ( current() == '"' )
	{
		quoted = true;
		forward();
	}

	while ( 1 )
	{
		char c = current();

		// Check if we have a character yet
		if ( !c )
		{
			break;
		}

		// These characters are not part of unquoted strings.
		if ( !quoted && ( c == '{' || c == '}' ) )
		{
			break;
		}

		// Check if it's the end of a quoted string.
		if ( !escape && quoted && c == '"' )
		{
			break;
		}

		// Add the character or escape sequence to the result
		if ( escape )
		{
			escape = false;

			if ( c == '"' )
			{
				result += '"';
			}
			else if ( c == '\\' )
			{
				result += '\\';
			}
		}
		else if ( c == '\\' )
		{
			escape = true;
		}
		else if ( c != '\n' && c != '\r' )
		{
			result += c;
		}

		forward();
	}

	if ( quoted )
	{
		forward();
	}

	return result;
}

void CKeyValuesTokenizer::ignore_whitespace()
{
	while ( 1 )
	{
		char c = current();

		if ( !c )
		{
			break;
		}

		if ( c == '\n' )
		{
			// Keep track of this data for debug
			_last_line_break = _position;
			_line++;
		}

		if ( c != ' ' && c != '\n' && c != '\t' && c != '\r' )
		{
			break;
		}

		forward();
	}
}

bool CKeyValuesTokenizer::ignore_comment()
{
	if ( current() == '/' && next() == '/' )
	{
		while ( current() != '\n' )
		{
			forward();
		}

		return true;
	}

	return false;
}

char CKeyValuesTokenizer::current()
{
	if ( _position >= _buflen )
	{
		return 0;
	}

	return _buffer[_position];
}

bool CKeyValuesTokenizer::forward()
{
	_position++;
	return _position < _buflen;
}

char CKeyValuesTokenizer::next()
{
	if ( ( _position + 1 ) >= _buflen )
	{
		return 0;
	}

	return _buffer[_position + 1];
}

std::string CKeyValuesTokenizer::location()
{
	std::ostringstream ss;
	ss << "line " << _line << ", column " << ( _position - _last_line_break );
	return ss.str();
}

//------------------------------------------------------------------------------------------------

int CKeyValues::find_child( const std::string &name ) const
{
	size_t count = _children.size();
	for ( size_t i = 0; i < count; i++ )
	{
		if ( _children[i]->get_name() == name )
		{
			return (int)i;
		}
	}

	return -1;
}

pvector<CKeyValues *> CKeyValues::get_children_with_name( const std::string &name ) const
{
	pvector<CKeyValues *> result;

	size_t count = _children.size();
	for ( size_t i = 0; i < count; i++ )
	{
		CKeyValues *child = _children[i];
		if ( child->get_name() == name )
		{
			result.push_back( child );
		}
	}

	return result;
}

void CKeyValues::parse( CKeyValuesTokenizer *tokenizer )
{
	bool has_key = false;
	std::string key;

	while ( 1 )
	{
		KeyValueToken_t token = tokenizer->next_token();
		if ( token.invalid() )
		{
			//keyvalues_cat.error()
			//	<< "Unexpected end of file\n";
			break;
		}

		if ( has_key )
		{
			if ( token.type == KVTOKEN_BLOCK_BEGIN )
			{
				PT( CKeyValues ) child = new CKeyValues( key, this );
				child->_filename = _filename;
				child->parse( tokenizer );
				_children.push_back( child );
			}
			else if ( token.type == KVTOKEN_STRING )
			{
				_keyvalues[key] = token.data;
			}
			else
			{
				keyvalues_cat.error()
					<< "Invalid token " << token.type << "\n";
				break;
			}
			has_key = false;
		}
		else
		{
			if ( token.type == KVTOKEN_BLOCK_END )
			{
				break;
			}
			if ( token.type != KVTOKEN_STRING )
			{
				keyvalues_cat.error()
					<< "Invalid token " << token.type << "\n";
				break;
			}
			has_key = true;
			key = token.data;
		}
	}
}

PT( CKeyValues ) CKeyValues::load( const Filename &filename )
{
	VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
	if ( !vfs->exists( filename ) )
	{
		keyvalues_cat.error()
			<< "Unable to find `" << filename.get_fullpath() << "`\n";
		return nullptr;
	}

	std::string buffer = vfs->read_file( filename, true );

	CKeyValuesTokenizer tokenizer( buffer );

	PT( CKeyValues ) kv = new CKeyValues( "__root" );
	kv->_filename = filename;
	kv->parse( &tokenizer );

	// We should have nothing left.
	if ( !tokenizer.next_token().invalid() )
	{
		keyvalues_cat.error()
			<< "Unexpected end-of-file for `" << filename << "`\n";
		return nullptr;
	}

	return kv;
}

//------------------------------------------------------------------------------------------------
// Helper functions for parsing string values that represent numbers.
//------------------------------------------------------------------------------------------------

pvector<float> CKeyValues::parse_float_list_str( const std::string &str )
{
	pvector<float> result;
	std::string curr_num_string;
	int current = 0;
	while ( current < str.length() )
	{
		char let = str[current];
		if ( let == ' ' )
		{
			result.push_back( stof( curr_num_string ) );
			curr_num_string = "";
		}
		else
		{
			curr_num_string += let;
		}
		current++;

		if ( current >= str.length() )
		{
			result.push_back( stof( curr_num_string ) );
			curr_num_string = "";
		}
	}

	return result;
}

pvector<int> CKeyValues::parse_num_list_str( const std::string &str )
{
	pvector<int> result;
	std::string curr_num_string;
	int current = 0;
	while ( current < str.length() )
	{
		char let = str[current];
		if ( let == ' ' )
		{
			result.push_back( stoi( curr_num_string ) );
			curr_num_string = "";
		}
		else
		{
			curr_num_string += let;
		}
		current++;

		if ( current >= str.length() )
		{
			result.push_back( stoi( curr_num_string ) );
			curr_num_string = "";
		}
	}

	return result;
}

pvector<pvector<int>> CKeyValues::parse_int_tuple_list_str( const std::string &str )
{
	pvector<pvector<int>> result;
	int current = 0;
	std::string curr_num_string;
	pvector<int> tuple_result;

	while ( current < str.length() )
	{
		char let = str[current];
		if ( let == '(' || ( let == ' ' && str[current - 1] == ')' ) )
		{
			tuple_result.clear();
		}
		else if ( let == ' ' || let == ')' )
		{
			tuple_result.push_back( stoi( curr_num_string ) );
			curr_num_string = "";
			if ( let == ')' )
			{
				result.push_back( tuple_result );
			}
		}
		else
		{
			curr_num_string += let;

		}
		current++;
	}

	return result;
}

pvector<pvector<float>> CKeyValues::parse_num_array_str( const std::string &str )
{
	pvector<pvector<float>> result;
	int current = 0;
	std::string curr_num_string;
	pvector<float> array_result;
	while ( current < str.length() )
	{
		char let = str[current];
		if ( let == '[' )
		{
		}
		else if ( let == ' ' && str[current - 1] != ']' )
		{
			array_result.push_back( stof( curr_num_string ) );
			curr_num_string = "";
		}
		else if ( let == ' ' )
		{
		}
		else if ( let == ']' )
		{
			if ( curr_num_string.length() > 0 )
			{
				array_result.push_back( stof( curr_num_string ) );
			}
			result.push_back( array_result );
			array_result.clear();
			curr_num_string = "";
		}
		else
		{
			curr_num_string += let;
			//cout << curr_num_string << endl;
		}
		current++;

		// Take care of the possible numbers at the end that are not enclosed in brackets.
		if ( current >= str.length() )
		{
			if ( curr_num_string.length() > 0 )
			{
				array_result.push_back( stof( curr_num_string ) );
				curr_num_string = "";
			}
			if ( array_result.size() > 0 )
			{
				result.push_back( array_result );
				array_result.clear();
			}
		}
	}

	return result;
}

LVecBase2f CKeyValues::to_2f( const std::string &str )
{
	pvector<float> vec = CKeyValues::parse_float_list_str( str );
	nassertr( vec.size() >= 2, LVecBase2f( 0 ) );
	return LVecBase2f( vec[0], vec[1] );
}

LVecBase3f CKeyValues::to_3f( const std::string &str )
{
	pvector<float> vec = CKeyValues::parse_float_list_str( str );
	nassertr( vec.size() >= 3, LVecBase3f( 0 ) );
	return LVecBase3f( vec[0], vec[1], vec[2] );
}

LVecBase4f CKeyValues::to_4f( const std::string &str )
{
	pvector<float> vec = CKeyValues::parse_float_list_str( str );
	nassertr( vec.size() >= 4, LVecBase4f( 0 ) );
	return LVecBase4f( vec[0], vec[1], vec[2], vec[3] );
}

template<class T>
static std::string CKeyValues::to_string( T v )
{
	return std::to_string( v );
}

template<class T>
static std::string CKeyValues::to_string( const pvector<T> &v )
{
	string res = "[ ";
	for ( size_t i = 0; i < v.size(); i++ )
	{
		res += to_string( v[i] );
	}

	res += " ]";

	return res;
}

std::string CKeyValues::to_string( const LVecBase2f &v )
{
	std::ostringstream ss;
	ss << "[ " << v[0] << " " << v[1] << " ]";
	return ss.str();
}

std::string CKeyValues::to_string( const LVecBase3f &v )
{
	std::ostringstream ss;
	ss << "[ " << v[0] << " " << v[1] << " " << v[2] << " ]";
	return ss.str();
}

std::string CKeyValues::to_string( const LVecBase4f &v )
{
	std::ostringstream ss;
	ss << "[ " << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << " ]";
	return ss.str();
}
