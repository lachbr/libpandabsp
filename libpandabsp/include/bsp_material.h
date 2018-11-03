#ifndef BSP_MATERIAL_H
#define BSP_MATERIAL_H

#include <material.h>
#include <shaderInput.h>

class BSPMaterial : public Material
{
PUBLISHED:
        explicit BSPMaterial(const std::string &name = "");

        void set_shader( const std::string &shader_name );
        INLINE std::string get_shader() const
        {
                return _shader_name;
        }

        void set_shader_input( const ShaderInput &input );
        INLINE const pvector<ShaderInput> &get_shader_inputs() const
        {
                return _shader_inputs;
        }

private:
        std::string _shader_name;
        pvector<ShaderInput> _shader_inputs;

public:
        static void register_with_read_factory();
        virtual void write_datagram( BamWriter *manager, Datagram &me );

protected:
        static TypedWritable *make_BSPMaterial( const FactoryParams &params );
        void fillin( DatagramIterator &scan, BamReader *manager );

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                Material::init_type();
                register_type( _type_handle, "BSPMaterial",
                               Material::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
        static TypeHandle _type_handle;
};

#endif // BSP_MATERIAL_H