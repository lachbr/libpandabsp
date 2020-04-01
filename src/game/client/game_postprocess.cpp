#include "game_postprocess.h"
#include "clientbase.h"

GamePostProcess::GamePostProcess() :
	PostProcess(),
	_hdr( nullptr ),
	_bloom( nullptr ),
	_fxaa( nullptr ),
	_hdr_enabled( false ),
	_bloom_enabled( false ),
	_fxaa_enabled( false )
{
}

void GamePostProcess::cleanup()
{
	if ( _hdr )
	{
		_hdr->shutdown();
		remove_effect( _hdr );
	}
	_hdr = nullptr;

	if ( _bloom )
	{
		_bloom->shutdown();
		remove_effect( _bloom );
	}
	_bloom = nullptr;

	if ( _fxaa )
	{
		_fxaa->shutdown();
		remove_effect( _fxaa );
	}
	_fxaa = nullptr;
}

void GamePostProcess::setup()
{
	cleanup();

	typedef pmap<std::string, Texture *> texmap_t;

	texmap_t texmap;
	texmap["sceneColorSampler"] = get_scene_color_texture();

	if ( _hdr_enabled )
	{
		_hdr = new HDREffect( this );
		_hdr->get_hdr_pass()->set_exposure_output(
			cl->_shader_generator->get_exposure_adjustment() );
		add_effect( _hdr );
	}

	if ( _bloom_enabled )
	{
		_bloom = new BloomEffect( this );
		add_effect( _bloom );
		texmap["bloomSampler"] = _bloom->get_final_texture();
	}

	if ( _fxaa_enabled )
	{
		_fxaa = new FXAA_Effect( this );
		add_effect( _fxaa );
		texmap["sceneColorSampler"] = _fxaa->get_final_texture();
	}

	NodePath final_quad = get_scene_pass()->get_quad();

	std::ostringstream vtext;
	vtext << "#version 330\n";
	vtext << "uniform mat4 p3d_ModelViewProjectionMatrix;\n";
	vtext << "in vec4 p3d_Vertex;\n";
	vtext << "in vec4 texcoord;\n";
	vtext << "out vec2 l_texcoord;\n";
	vtext << "void main()\n";
	vtext << "{\n";
	vtext << "  gl_Position = p3d_ModelViewProjectionMatrix * p3d_Vertex;\n";
	vtext << "  l_texcoord = texcoord.xy;\n";
	vtext << "}\n";

	std::ostringstream ptext;
	ptext << "#version 330\n";
	ptext << "out vec4 outputColor;\n";
	ptext << "in vec2 l_texcoord;\n";
	
	for ( auto itr : texmap )
	{
		ptext << "uniform sampler2D " << itr.first << ";\n";
	}

	ptext << "void main()\n";
	ptext << "{\n";
	ptext << "  outputColor = texture(sceneColorSampler, l_texcoord);\n";
	if ( _bloom_enabled )
		ptext << "  outputColor.rgb += texture(bloomSampler, l_texcoord).rgb;\n";
	ptext << "}\n";

	PT( Shader ) shader = Shader::make( Shader::SL_GLSL, vtext.str(), ptext.str() );
	if ( !shader )
		return;

	final_quad.set_shader( shader );

	for ( auto itr : texmap )
	{
		final_quad.set_shader_input( itr.first, itr.second );
	}
}