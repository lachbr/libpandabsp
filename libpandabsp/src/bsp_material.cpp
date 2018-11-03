#include "bsp_material.h"
#include "shader_generator.h"

TypeHandle BSPMaterial::_type_handle;

BSPMaterial::BSPMaterial( const std::string &name ) :
        Material( name ),
        _shader_name( DEFAULT_SHADER )
{
}

/**
 * Sets the name of the shader that this material will invoke.
 * Ex: "VertexLitGeneric", "LightmappedGeneric", "Water"
 */
void BSPMaterial::set_shader( const std::string &shader_name )
{
        _shader_name = shader_name;
}

/**
 * Adds a new input to the shader that runs for this material.
 */
void BSPMaterial::set_shader_input( const ShaderInput &input )
{
        _shader_inputs.push_back( input );
}

//=====================================================================
//======================== BAM STUFF ==================================

void BSPMaterial::register_with_read_factory()
{
        BamReader::get_factory()->register_factory( get_class_type(), make_BSPMaterial );
}

void BSPMaterial::write_datagram( BamWriter *manager, Datagram &me )
{
        Material::write_datagram( manager, me );
        me.add_string( _shader_name );
}

TypedWritable *BSPMaterial::make_BSPMaterial( const FactoryParams &params )
{
        BSPMaterial *mat = new BSPMaterial;
        DatagramIterator scan;
        BamReader *manager;
        parse_params( params, scan, manager );
        mat->fillin( scan, manager );
        return mat;
}

void BSPMaterial::fillin( DatagramIterator &scan, BamReader *manager )
{
        Material::fillin( scan, manager );
        set_shader( scan.get_string() );
}