#include "ambient_probes.h"
#include "bsploader.h"
#include "bspfile.h"
#include "mathlib.h"

#include <shader.h>
#include <loader.h>
#include <textNode.h>

using std::cos;
using std::sin;

#define CHANGED_EPSILON 0.1
//#define VISUALIZE_AMBPROBES

CPT( ShaderAttrib ) AmbientProbeManager::_identity_shattr = nullptr;
CPT( Shader ) AmbientProbeManager::_shader = nullptr;

AmbientProbeManager::AmbientProbeManager() :
        _loader( nullptr )
{
}

AmbientProbeManager::AmbientProbeManager( BSPLoader *loader ) :
        _loader( loader )
{
}

CPT( Shader ) AmbientProbeManager::get_shader()
{
        if ( _shader == nullptr )
        {
                _shader = Shader::load( Shader::SL_GLSL, Filename( "phase_14/models/shaders/bsp_dynamic_v.glsl" ),
                                        Filename( "phase_14/models/shaders/bsp_dynamic_f.glsl" ) );
        }

        return _shader;
}

CPT( ShaderAttrib ) AmbientProbeManager::get_identity_shattr()
{
        if ( _identity_shattr == nullptr )
        {
                CPT( ShaderAttrib ) shattr = DCAST( ShaderAttrib, ShaderAttrib::make( get_shader() ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "ambient_cube", PTA_LVecBase3::empty_array( 6 ) ) ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "num_locallights", PTA_int::empty_array( 1 ) ) ) );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "locallight_type", PTA_int::empty_array( 2 ) ) ) );
                for ( int i = 0; i < 2; i++ )
                {
                        std::stringstream ss;
                        ss << "locallight[" << i << "]";
                        std::string name = ss.str();
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".pos", LVector3( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".direction", LVector4( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".atten", LVector4( 0 ) ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( name + ".color", LVector3( 1 ) ) ) );
                }

                _identity_shattr = shattr;
        }

        return _identity_shattr;
}

INLINE LVector3 angles_to_vector( const vec3_t &angles )
{
        return LVector3( cos( angles[0] ) * cos( angles[1] ),
                         sin( angles[0] ) * cos( angles[1] ),
                         sin( angles[1] ) );
}

INLINE int lighttype_from_classname( const char *classname )
{
        if ( !strncmp( classname, "light_environment", 18 ) )
        {
                return LIGHTTYPE_SUN;
        }
        else if ( !strncmp( classname, "light_spot", 11 ) )
        {
                return LIGHTTYPE_SPOT;
        }
        else if ( !strncmp( classname, "light", 5 ) )
        {
                return LIGHTTYPE_POINT;
        }

        return LIGHTTYPE_POINT;
}

void AmbientProbeManager::process_ambient_probes()
{

#ifdef VISUALIZE_AMBPROBES
        if ( !_vis_root.is_empty() )
        {
                _vis_root.remove_node();
        }
        _vis_root = _loader->_result.attach_new_node( "visAmbProbesRoot" );
#endif

        _light_pvs.clear();
        _light_pvs.resize( g_dmodels[0].visleafs + 1 );
        _all_lights.clear();

        // Build light data structures
        for ( int entnum = 0; entnum < g_numentities; entnum++ )
        {
                entity_t *ent = g_entities + entnum;
                const char *classname = ValueForKey( ent, "classname" );
                if ( !strncmp( classname, "light", 5 ) )
                {
                        PT( light_t ) light = new light_t;

                        vec3_t pos;
                        GetVectorForKey( ent, "origin", pos );
                        VectorCopy( pos, light->pos );
                        VectorScale( light->pos, 1 / 16.0, light->pos );

                        light->leaf = _loader->find_leaf( light->pos );
                        light->color = color_from_value( ValueForKey( ent, "_light" ) ).get_xyz();
                        light->type = lighttype_from_classname( classname );

                        if ( light->type == LIGHTTYPE_SUN ||
                             light->type == LIGHTTYPE_SPOT)
                        {
                                vec3_t angles;
                                GetVectorForKey( ent, "angles", angles );
                                float temp = angles[0];
                                angles[0] = angles[1] - 90;
                                angles[1] = temp;
                                VectorCopy( angles_to_vector( angles ), light->direction );
                                light->direction[3] = 1.0;
                        }

                        if ( light->type == LIGHTTYPE_SPOT ||
                             light->type == LIGHTTYPE_POINT )
                        {
                                float fade = FloatForKey( ent, "_fade" );
                                light->atten = LVector4( 1.0, 0.0, fade * 0.03, 1.0 );
                                if ( light->type == LIGHTTYPE_SPOT )
                                {
                                        float cone = FloatForKey( ent, "_cone2" );
                                        light->atten[3] = cone;
                                }
                        }

                        _all_lights.push_back( light );
                        if ( light->type == LIGHTTYPE_SUN )
                        {
                                _sunlight = light;
                        }
                }
        }

        // Build light PVS
        for ( size_t lightnum = 0; lightnum < _all_lights.size(); lightnum++ )
        {
                light_t *light = _all_lights[lightnum];
                for ( int leafnum = 0; leafnum < g_dmodels[0].visleafs + 1; leafnum++ )
                {
                        if ( light->type != LIGHTTYPE_SUN &&
                             _loader->is_cluster_visible(light->leaf, leafnum) )
                        {
                                _light_pvs[leafnum].push_back( light );
                        }
                }
        }

        for ( size_t i = 0; i < g_leafambientindex.size(); i++ )
        {
                dleafambientindex_t *ambidx = &g_leafambientindex[i];
                dleaf_t *leaf = g_dleafs + i;
                _probes[i] = pvector<ambientprobe_t>();
                for ( int j = 0; j < ambidx->num_ambient_samples; j++ )
                {
                        dleafambientlighting_t *light = &g_leafambientlighting[ambidx->first_ambient_sample + j];
                        ambientprobe_t probe;
                        probe.leaf = i;
                        probe.pos = LPoint3( RemapValClamped( light->x, 0, 255, leaf->mins[0], leaf->maxs[0] ),
                                             RemapValClamped( light->y, 0, 255, leaf->mins[1], leaf->maxs[1] ),
                                             RemapValClamped( light->z, 0, 255, leaf->mins[2], leaf->maxs[2] ) );
                        probe.pos /= 16.0;
                        probe.cube = PTA_LVecBase3::empty_array( 6 );
                        for ( int k = 0; k < 6; k++ )
                        {
                                probe.cube.set_element( k, color_shift_pixel( &light->cube.color[k], _loader->_gamma ) );
                        }
#ifdef VISUALIZE_AMBPROBES
                        PT( TextNode ) tn = new TextNode( "visText" );
                        char text[40];
                        sprintf( text, "Ambient Sample %i\nLeaf %i", j, i );
                        tn->set_align( TextNode::A_center );
                        tn->set_text( text );
                        tn->set_text_color( LColor( 1, 1, 1, 1 ) );
                        NodePath smiley = _vis_root.attach_new_node( tn->generate() );
                        smiley.set_pos( probe.pos );
                        smiley.set_billboard_axis();
                        smiley.clear_model_nodes();
                        smiley.flatten_strong();
                        probe.visnode = smiley;
#endif
                        _probes[i].push_back( probe );
                }
        }
}

void AmbientProbeManager::update_node( PandaNode *node )
{
        CPT( ShaderAttrib ) shattr = nullptr;
        bool new_instance = false;
        if ( !node->has_attrib( ShaderAttrib::get_class_type() ) )
        {
                shattr = get_identity_shattr();
                new_instance = true;
        }
        else
        {
                shattr = DCAST( ShaderAttrib, node->get_attrib( ShaderAttrib::get_class_type() ) );
        }

        // Is it even necessary to update anything?
        CPT( TransformState ) curr_trans = node->get_transform();
        CPT( TransformState ) prev_trans = node->get_prev_transform();
        LVector3 pos_delta = curr_trans->get_pos() - prev_trans->get_pos();
        if ( pos_delta.length() >= CHANGED_EPSILON || new_instance || true )
        {
                LPoint3 curr_net = NodePath( node ).get_net_transform()->get_pos();
                int leaf_id = _loader->find_leaf( curr_net );

                // Update ambient cube
                if ( _probes[leaf_id].size() > 0 )
                {
                        ambientprobe_t *sample = find_closest_sample( leaf_id, curr_net );

#ifdef VISUALIZE_AMBPROBES
                        for ( size_t j = 0; j < _probes[leaf_id].size(); j++ )
                        {
                                _probes[leaf_id][j].visnode.set_color_scale( LColor( 0, 0, 1, 1 ), 1 );
                        }
                        if ( !sample->visnode.is_empty() )
                        {
                                sample->visnode.set_color_scale( LColor( 0, 1, 0, 1 ), 1 );
                        }
#endif
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "ambient_cube", sample->cube ) ) );
                }

                // Update local light sources
                pvector<light_t *> locallights = _light_pvs[leaf_id];
                // Sort local lights from closest to furthest distance from node, we will choose the two closest lights.
                std::sort( locallights.begin(), locallights.end(), [curr_net]( const light_t *a, const light_t *b )
                {
                        return ( a->pos - curr_net ).length() < ( b->pos - curr_net ).length();
                } );

#if 0
                if ( is_sky_visible( curr_net ) )
                {
                        // If we hit the sky from current position, sunlight takes
                        // precedence over all other local light sources.
                        locallights.insert( locallights.begin(), _sunlight );
                }
#endif

                size_t numlights = locallights.size();
                std::cout << "For node ";
                node->output( std::cout );
                std::cout << ":\n";
                std::cout << "\tnumlights: " << numlights << std::endl;
                PTA_int pta_numlights = PTA_int::empty_array( 1 );
                pta_numlights.set_element( 0, numlights < 2 ? numlights : 2 );
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "num_locallights", pta_numlights ) ) );
                PTA_int lighttypes = PTA_int::empty_array( 2 );
                for ( size_t i = 0; i < numlights && i < 2; i++ )
                {
                        light_t *light = locallights[i];
                        lighttypes.set_element( i, light->type );
                        stringstream ss;
                        ss << "locallight[" << i << "]";
                        std::string lightname = ss.str();
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".pos", light->pos ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".color", light->color ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".direction", light->direction ) ) );
                        shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( lightname + ".atten", light->atten ) ) );

                        std::cout << "\tlightcolor: " << light->color << "\n\tlighttype: " << light->type << "\n\tlightpos: " << light->pos << std::endl;
                }
                shattr = DCAST( ShaderAttrib, shattr->set_shader_input( ShaderInput( "locallight_type", lighttypes ) ) );

                // Apply the modified shader attrib.
                node->set_attrib( shattr );
        }
}

void AmbientProbeManager::add_node( const NodePath &node )
{
        WeakNodePath *wnp = new WeakNodePath( node );
        _objects.push_back( wnp );
        _object_pos_cache[wnp] = LPoint3( 0 );

        PT( Shader ) shader = Shader::load( Shader::SL_GLSL, Filename( "phase_14/models/shaders/bsp_dynamic_v.glsl" ),
                                            Filename( "phase_14/models/shaders/bsp_dynamic_f.glsl" ) );
        wnp->get_node_path().set_shader( shader );
        wnp->get_node_path().set_shader_input( "ambient_cube", PTA_LVecBase3::empty_array( 6 ) );
}

INLINE ambientprobe_t *AmbientProbeManager::find_closest_sample( int leaf_id, const LPoint3 &pos )
{
        pvector<ambientprobe_t> &probes = _probes[leaf_id];
        pvector<ambientprobe_t *> samples;
        samples.resize( probes.size() );
        for ( int i = 0; i < probes.size(); i++ )
        {
                samples[i] = &probes[i];
        }

        std::sort( samples.begin(), samples.end(), [pos]( const ambientprobe_t *a, const ambientprobe_t *b )
        {
                return ( a->pos - pos ).length() < ( b->pos - pos ).length();
        } );

        return samples[0];
}