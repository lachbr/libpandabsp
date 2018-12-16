#ifndef __PARSER_H__
#define __PARSER_H__

#include "viftokenizer.h"
#include <dtoolbase_cc.h>
#include <aa_luse.h>

struct Property
{
        string name;
        string value;

        INLINE Property( const std::string &k, const std::string &v )
        {
                name = k;
                value = v;
        }

        INLINE Property()
        {
        }
};

struct Object
{
        string name;
        pvector<Property> properties;
        pvector<Object> objects;

        INLINE void add_property( const std::string &k, const std::string &v )
        {
                properties.push_back( Property( k, v ) );
        }

        template <class T>
        INLINE void add_property( const std::string &k, const T &v )
        {
                properties.push_back( Property( k, Parser::parse( v ) ) );
        }
 
        INLINE void add_child( const Object &child )
        {
                objects.push_back( child );
        }
        INLINE Object &make_child()
        {
                Object obj;
                objects.push_back( obj );
                return objects[objects.size() - 1];
        }

        INLINE Object( const std::string &n )
        {
                name = n;
        }
        INLINE Object()
        {
        }
};

class EXPCL_VIF Parser
{
public:
        pvector<Object> _base_objects;
        pvector<Object> _all_objects;

        static pvector<Object> _global_base_objects;
        static pvector<Object> _global_all_objects;

        Parser();
        Parser( TokenVec &tokens );
        static bool has_property( Object &obj, const string &key );
        static string get_property_value( Object &obj, const string &key );
        static pvector<Property> get_properties( Object &obj );
        static pvector<int> parse_num_list_str( const string &str );
        static pvector<float> parse_float_list_str( const string &str );
        static pvector<pvector<int>> parse_int_tuple_list_str( const string &str );
        static pvector<pvector<float>> parse_num_array_str( const string &str );
        Object get_object_with_id( const string &id );
        static Object get_object_with_name( Object &base_obj, const string &name );
        pvector<Object> get_base_objects_with_name( const string &name );
        static pvector<Object> get_objects_with_name( Object &base_obj, const string &name );
        static bool has_object_named( Object &base_obj, const string &name );
        void make_objects_global();

        template<class T>
        static string parse( T v )
        {
                return std::to_string( v );
        }

        template<class T>
        static string parse( const pvector<T> &v )
        {
                string res = "[ ";
                for ( size_t i = 0; i < v.size(); i++ )
                {
                        res += parse( v[i] );
                }

                res += " ]";

                return res;
        }

        static string parse( const LVecBase3f &v );
        static string parse( const LVecBase4f &v );
        static string parse( const LVecBase2f &v );

        static LVecBase3f to_3f( const std::string &str );
        static LVecBase2f to_2f( const std::string &str );
        static LVecBase4f to_4f( const std::string &str );
private:

        Property walk_prop();
        Object walk_obj();

        int _current_index;
        TokenVec _tokens;
};

#endif // __PARSER_H__