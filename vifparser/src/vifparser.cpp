#include "vifParser.h"

#include <string>

pvector<Object> Parser::_global_all_objects;
pvector<Object> Parser::_global_base_objects;

pvector<Property> Parser::get_properties( Object &obj )
{
        return obj.properties;
}

pvector<Object> Parser::get_base_objects_with_name( const string &name )
{
        pvector<Object> result;
        for ( size_t i = 0; i < _base_objects.size(); i++ )
        {
                Object obj = _base_objects[i];
                if ( obj.name.compare( name ) == 0 )
                {
                        result.push_back( obj );
                }
        }
        return result;
}

bool Parser::has_property( Object &obj, const string &key )
{
        bool found = false;
        for ( size_t i = 0; i < obj.properties.size(); i++ )
        {
                Property prop = obj.properties[i];
                if ( prop.name.compare( key ) == 0 )
                {
                        // Found this property.
                        found = true;
                        break;
                }
        }
        return found;
}

bool Parser::has_object_named( Object &baseObj, const string &name )
{
        bool found = false;
        for ( size_t i = 0; i < baseObj.objects.size(); i++ )
        {
                Object obj = baseObj.objects[i];
                if ( obj.name.compare( name ) == 0 )
                {
                        // Found this object.
                        found = true;
                        break;
                }
        }
        return found;
}

string Parser::get_property_value( Object &obj, const string &key )
{
        string val = "";
        for ( size_t i = 0; i < obj.properties.size(); i++ )
        {
                Property prop = obj.properties[i];
                if ( prop.name.compare( key ) == 0 )
                {
                        // Found this property.
                        val = prop.value;
                        break;
                }
        }
        return val;
}

pvector<float> Parser::parse_float_list_str( const string &str )
{
        pvector<float> result;
        string curr_num_string;
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

pvector<int> Parser::parse_num_list_str( const string &str )
{
        pvector<int> result;
        string curr_num_string;
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

pvector<pvector<int>> Parser::parse_int_tuple_list_str( const string &str )
{
        pvector<pvector<int>> result;
        int current = 0;
        string curr_num_string;
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

pvector<pvector<float>> Parser::parse_num_array_str( const string &str )
{
        pvector<pvector<float>> result;
        int current = 0;
        string curr_num_string;
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

Object Parser::get_object_with_id( const string &id )
{
        Object result;
        for ( size_t i = 0; i < _all_objects.size(); i++ )
        {
                result = _all_objects[i];
                if ( Parser::get_property_value( result, "id" ).compare( id ) == 0 )
                {
                        return result;
                }
        }
        return result;
}

pvector<Object> Parser::get_objects_with_name( Object &base_obj, const string &name )
{
        pvector<Object> result;
        for ( size_t i = 0; i < base_obj.objects.size(); i++ )
        {
                Object obj = base_obj.objects[i];
                if ( obj.name.compare( name ) == 0 )
                {
                        result.push_back( obj );
                }
        }
        return result;
}

// Warning: If there are multiple objects with the same name, only the first occurance will be returned.
// Use Parser::get_objects_with_name to get a pvector of objects with the name.
Object Parser::get_object_with_name( Object &base_obj, const string &name )
{
        Object result;
        for ( size_t i = 0; i < base_obj.objects.size(); i++ )
        {
                result = base_obj.objects[i];
                if ( result.name.compare( name ) == 0 )
                {
                        return result;
                }
        }
        return result;
}

Property Parser::walk_prop()
{
        Token token = _tokens[_current_index];
        Property node;
        node.name = token.value;

        token = _tokens[++_current_index];
        node.value = token.value;

        _current_index++;
        return node;
}

bool strings_equal( string &str1, const char *str2 )
{
        return ( str1.compare( str2 ) == 0 );
}

void Parser::make_objects_global()
{
        for ( size_t i = 0; i < _base_objects.size(); i++ )
        {
                _global_base_objects.push_back( _base_objects[i] );
        }

        for ( size_t i = 0; i < _all_objects.size(); i++ )
        {
                _global_all_objects.push_back( _all_objects[i] );
        }
}

Object Parser::walk_obj()
{
        Token token = _tokens[_current_index];
        Object node;
        node.name = token.value;

        token = _tokens[_current_index += 2];

        while ( ( !strings_equal( token.type, "bracket" ) ) || ( strings_equal( token.type, "bracket" ) && !strings_equal( token.value, "}" ) ) )
        {
                if ( strings_equal( token.type, "string" ) )
                {
                        Property prop = walk_prop();
                        node.properties.push_back( prop );
                }
                else if ( strings_equal( token.type, "name" ) )
                {
                        if ( strings_equal( _tokens[_current_index + 1].type, "parenthesis" ) )
                        {
                                if ( token.value == "inherit" )
                                {
                                        string inherit_from = _tokens[_current_index + 2].value;
                                        for ( size_t i = 0; i < _global_base_objects.size(); i++ )
                                        {
                                                Object base_obj = _global_base_objects[i];
                                                if ( base_obj.name == inherit_from )
                                                {
                                                        size_t j;
                                                        for ( j = 0; j < base_obj.properties.size(); j++ )
                                                        {
                                                                Property prop = base_obj.properties[j];
                                                                node.properties.push_back( prop );
                                                        }
                                                        for ( j = 0; j < base_obj.objects.size(); i++ )
                                                        {
                                                                Object obj = base_obj.objects[i];
                                                                node.objects.push_back( obj );
                                                        }
                                                }
                                        }
                                        _current_index += 4;
                                }
                        }
                        else
                        {
                                Object obj = walk_obj();
                                node.objects.push_back( obj );
                                _all_objects.push_back( obj );
                        }
                }
                token = _tokens[_current_index];
        }
        _current_index++;
        return node;
}

Parser::Parser()
{
        _current_index = 0;
}

Parser::Parser( TokenVec &tokens )
{
        _tokens = tokens;
        _current_index = 0;
        while ( _current_index < _tokens.size() )
        {

                _base_objects.push_back( walk_obj() );
        }
}

string Parser::parse( const LVecBase2f &v )
{
        stringstream ss;
        ss << "[ " << v[0] << " " << v[1] << " ]";
        return ss.str();
}

string Parser::parse( const LVecBase3f &v )
{
        stringstream ss;
        ss << "[ " << v[0] << " " << v[1] << " " << v[2] << " ]";
        return ss.str();
}

string Parser::parse( const LVecBase4f &v )
{
        stringstream ss;
        ss << "[ " << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << " ]";
        return ss.str();
}

LVecBase2f Parser::to_2f( const std::string &str )
{
        pvector<float> vec = Parser::parse_float_list_str( str );
        nassertr( vec.size() >= 2, LVecBase2f( 0 ) );
        return LVecBase2f( vec[0], vec[1] );
}

LVecBase3f Parser::to_3f( const std::string &str )
{
        pvector<float> vec = Parser::parse_float_list_str( str );
        nassertr( vec.size() >= 3, LVecBase3f( 0 ) );
        return LVecBase3f( vec[0], vec[1], vec[2] );
}

LVecBase4f Parser::to_4f( const std::string &str )
{
        pvector<float> vec = Parser::parse_float_list_str( str );
        nassertr( vec.size() >= 4, LVecBase4f( 0 ) );
        return LVecBase4f( vec[0], vec[1], vec[2], vec[3] );
}