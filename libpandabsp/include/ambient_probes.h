#ifndef AMBIENT_PROBES_H
#define AMBIENT_PROBES_H

#include <pvector.h>
#include <weakNodePath.h>
#include <aa_luse.h>
#include <pta_LVecBase3.h>
#include <colorAttrib.h>
#include <texGenAttrib.h>
#include <lightRampAttrib.h>
#include <cullableObject.h>
#include <shaderAttrib.h>

class BSPLoader;
struct dleafambientindex_t;
struct dleafambientlighting_t;

enum
{
        LIGHTTYPE_SUN   = 0,
        LIGHTTYPE_POINT = 1,
        LIGHTTYPE_SPOT  = 2,
};

struct ambientprobe_t
{
        int leaf;
        LPoint3 pos;
        PTA_LVecBase3 cube;
        NodePath visnode;
};

struct light_t : public ReferenceCount
{
        int leaf;
        int type;
        LVector4 direction;
        LPoint3 pos;
        LVector3 color;
        LVector4 atten;
};

#define MAXLIGHTS 2

struct nodeshaderinput_t : public ReferenceCount
{
        PTA_LVecBase3 ambient_cube;

        PTA_int light_count;
        PTA_int light_type;
        PTA_LVecBase3 light_pos;
        PTA_LVecBase3 light_color;
        PTA_LVecBase4 light_direction;
        PTA_LVecBase4 light_atten;

        nodeshaderinput_t() :
                ReferenceCount()
        {
                ambient_cube = PTA_LVecBase3::empty_array( 6 );

                light_count = PTA_int::empty_array( 1 );
                light_type = PTA_int::empty_array( MAXLIGHTS );
                light_pos = PTA_LVecBase3::empty_array( MAXLIGHTS );
                light_color = PTA_LVecBase3::empty_array( MAXLIGHTS );
                light_direction = PTA_LVecBase4::empty_array( MAXLIGHTS );
                light_atten = PTA_LVecBase4::empty_array( MAXLIGHTS );
        }
};

class AmbientProbeManager;

INLINE CPT( ShaderAttrib ) set_shader_inputs( CPT( ShaderAttrib ) shattr, const nodeshaderinput_t *input )
{
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "ambient_cube", input->ambient_cube ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_count", input->light_count ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_type", input->light_type ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_pos", input->light_pos ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_color", input->light_color ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_direction", input->light_direction ) ) );
        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "light_atten", input->light_atten ) ) );
        return shattr;
}

/**
 * Generates shaders for dynamic objects in a BSP level.
 * GLSL
 */
class BSPShaderGenerator
{
public:
        BSPShaderGenerator();

        CPT( ShaderAttrib ) generate_shader( CPT( RenderState ) net_state );

        struct shaderinfo_t
        {
                shaderinfo_t();
                bool operator < ( const shaderinfo_t &other ) const;
                bool operator == ( const shaderinfo_t &other ) const;
                bool operator != ( const shaderinfo_t &other ) const { return !operator ==( other ); }

                int material_flags;
                int texture_flags;
                int shade_model;

                ColorAttrib::Type color_type;

                GeomVertexAnimationSpec anim_spec;

                int fog_mode;

                int outputs;
                bool calc_primary_alpha;
                bool disable_alpha_write;
                RenderAttrib::PandaCompareFunc alpha_test_mode;
                PN_stdfloat alpha_test_ref;

                int num_clip_planes;

                CPT( LightRampAttrib ) light_ramp;

                enum textureflags
                {
                        TEXTUREFLAGS_HAS_RGB = 0x001,
                        TEXTUREFLAGS_HAS_ALPHA = 0x002,
                        TEXTUREFLAGS_HAS_SCALE = 0x004,
                        TEXTUREFLAGS_HAS_MAT = 0x008,
                        TEXTUREFLAGS_SAVED_RESULT = 0x010,
                        TEXTUREFLAGS_NORMALMAP = 0x020,
                        TEXTUREFLAGS_HEIGHTMAP = 0x040,
                        TEXTUREFLAGS_GLOWMAP = 0x080,
                        TEXTUREFLAGS_GLOSSMAP = 0x100,
                        TEXTUREFLAGS_USES_COLOR = 0x200,
                        TEXTUREFLAGS_USES_PRIMARY_COLOR = 0x400,
                        TEXTUREFLAGS_USES_LAST_SAVED_RESULT = 0x800,

                        TEXTUREFLAGS_RGB_SCALE_2 = 0x1000,
                        TEXTUREFLAGS_RGB_SCALE_4 = 0x2000,
                        TEXTUREFLAGS_ALPHA_SCALE_2 = 0x4000,
                        TEXTUREFLAGS_ALPHA_SCALE_4 = 0x8000,
                        TEXTUREFLAGS_SPHEREMAP = 0x10000,

                        TEXTUREFLAGS_COMBINE_RGB_MODE_SHIFT = 16,
                        TEXTUREFLAGS_COMBINE_RGB_MODE_MASK = 0x0000f0000,
                        TEXTUREFLAGS_COMBINE_ALPHA_MODE_SHIFT = 20,
                        TEXTUREFLAGS_COMBINE_ALPHA_MODE_MASK = 0x000f00000,
                };

                struct textureinfo_t
                {
                        CPT_InternalName texcoord_name;
                        Texture::TextureType type;
                        TextureStage::Mode mode;
                        TexGenAttrib::Mode gen_mode;
                        int flags;
                        uint16_t combine_rgb;
                        uint16_t combine_alpha;
                };
                pvector<textureinfo_t> textures;
        };

private:
        void analyze_renderstate( shaderinfo_t &key, CPT( RenderState ) state );

        pmap<shaderinfo_t, CPT( ShaderAttrib )> _shader_cache;
};

class AmbientProbeManager
{
public:
        AmbientProbeManager();
        AmbientProbeManager( BSPLoader *loader );

        void process_ambient_probes();

        void update_node( PandaNode *node, CPT( TransformState ) net_ts, CPT( RenderState ) net_state );

        void cleanup();

        static CPT( ShaderAttrib ) get_identity_shattr();
        static CPT( Shader ) get_shader();

        BSPShaderGenerator *get_shader_generator();

public:
        void consider_garbage_collect();

private:
        INLINE ambientprobe_t *find_closest_sample( int leaf_id, const LPoint3 &pos );
        INLINE bool is_sky_visible( const LPoint3 &point );
        INLINE bool is_light_visible( const LPoint3 &point, const light_t *light );

        INLINE void garbage_collect_cache();


        void generate_shaders( PandaNode *node, CPT( RenderState ) net_state, bool first,
                               const nodeshaderinput_t *input );

private:
        BSPLoader *_loader;
        BSPShaderGenerator _shader_generator;

        // NodePaths to be influenced by the ambient probes.
        pmap<WPT( PandaNode ), CPT( TransformState )> _pos_cache;
        pmap<WPT( PandaNode ), PT( nodeshaderinput_t )> _node_data;
        pmap<int, pvector<ambientprobe_t>> _probes;
        pvector<ambientprobe_t *> _all_probes;
        pvector<PT( light_t )> _all_lights;
        pvector<pvector<light_t *>> _light_pvs;
        light_t *_sunlight;

        NodePath _vis_root;

        double _last_garbage_collect_time;

        static CPT( Shader ) _shader;
        static CPT( ShaderAttrib ) _identity_shattr;
};

#endif // AMBIENT_PROBES_H