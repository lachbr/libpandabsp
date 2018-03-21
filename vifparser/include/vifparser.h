#ifndef __PARSER_H__
#define __PARSER_H__

#include "vifTokenizer.h"

struct Property
{
        string name;
        string value;
};

struct Object
{
        string name;
        vector<Property> properties;
        vector<Object> objects;
};

//static vector<Solid> solids;

//template <class T>
//void walk(int &current, TokenVec &tokens);

//void parser(TokenVec &tokens);



class EXPCL_VIF Parser
{
public:
        vector<Object> _base_objects;
        vector<Object> _all_objects;
        Parser( TokenVec &tokens );
        static bool has_property( Object &obj, const string &key );
        static string get_property_value( Object &obj, const string &key );
        static vector<int> parse_num_list_str( string &str );
        static vector<float> parse_float_list_str( string &str );
        static vector<vector<int>> parse_int_tuple_list_str( string &str );
        static vector<vector<float>> parse_num_array_str( string &str );
        Object get_object_with_id( string &id );
        Object get_object_with_name( Object &base_obj, const string &name );
        vector<Object> get_base_objects_with_name( const string &name );
        vector<Object> get_objects_with_name( Object &base_obj, const string &name );
        bool has_object_named( Object &base_obj, const string &name );
private:

        Property walk_prop();
        Object walk_obj();

        int _current_index;
        TokenVec _tokens;
};

#endif // __PARSER_H__