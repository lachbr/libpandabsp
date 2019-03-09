/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file shader_generator.h
 * @author Brian Lach
 * @date October 22, 2018
 */

#ifndef BSP_SHADERGENERATOR_H
#define BSP_SHADERGENERATOR_H

#include <shaderGenerator.h>
#include <genericAsyncTask.h>
#include <nodePath.h>
#include <weakNodePath.h>
#include <configVariableColor.h>
#include <camera.h>

#include "shader_spec.h"

class PSSMCameraRig;
class GraphicsStateGuardian;

extern ConfigVariableInt pssm_splits;
extern ConfigVariableInt pssm_size;
extern ConfigVariableBool want_pssm;
extern ConfigVariableDouble depth_bias;
extern ConfigVariableDouble normal_offset_scale;
extern ConfigVariableDouble softness_factor;
extern ConfigVariableBool normal_offset_uv_space;
extern ConfigVariableColor ambient_light_identifier;
extern ConfigVariableColor ambient_light_min;
extern ConfigVariableDouble ambient_light_scale;

#define DEFAULT_SHADER "UnlitNoMat"

static const BitMask32 shadow_camera_mask = BitMask32::bit( 5 );

NotifyCategoryDeclNoExport(bspShaderGenerator);

class nodeshaderinput_t;

class PSSMShaderGenerator : public ShaderGenerator
{
PUBLISHED:
        PSSMShaderGenerator( GraphicsStateGuardian *gsg, const NodePath &camera, const NodePath &render );

        virtual CPT( ShaderAttrib ) synthesize_shader( const RenderState *rs,
                                                       const GeomVertexAnimationSpec &anim );

        void set_sun_light( const NodePath &np );

        void start_update();

        void add_shader( PT( ShaderSpec ) spec );

        INLINE bool has_shadow_sunlight() const
        {
                return _has_shadow_sunlight;
        }
        INLINE Texture *get_pssm_array_texture() const
        {
                return _pssm_split_texture_array;
        }
        INLINE PSSMCameraRig *get_pssm_rig() const
        {
                return _pssm_rig;
        }

        INLINE NodePath get_skybox_root() const
        {
                return _skybox_root;
        }
        INLINE Texture *get_skybox_rtt() const
        {
                return _skybox_rtt;
        }

        static Texture *get_identity_cubemap();

private:
        struct SplitShadowMap
        {
                PT( GraphicsOutput ) buffer;
                int index;
        };

        pmap<std::string, PT( ShaderSpec )> _shaders;

        PT( Texture ) _pssm_split_texture_array;
        PT( GraphicsOutput ) _pssm_layered_buffer;

        pvector<SplitShadowMap> _split_maps;

        PSSMCameraRig *_pssm_rig;

        static AsyncTask::DoneStatus update_pssm( GenericAsyncTask *task, void *data );

        bool _has_shadow_sunlight;
        WeakNodePath _sunlight;
        PT( GenericAsyncTask ) _update_task;
        GraphicsStateGuardian *_gsg;
        LVector3 _sun_vector;
        NodePath _camera;
        NodePath _render;

        PT( Texture ) _skybox_rtt;
        PT( Camera ) _skybox_camera;
        NodePath _skybox_root;

        static PT( Texture ) _identity_cubemap;

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                ShaderGenerator::init_type();
                register_type( _type_handle, "PSSMShaderGenerator",
                               ShaderGenerator::get_class_type() );
        }
        virtual TypeHandle get_type() const
        {
                return get_class_type();
        }
        virtual TypeHandle force_init_type() { init_type(); return get_class_type(); }

private:
        static TypeHandle _type_handle;

        friend class PSSMCameraRig;
};

#endif // BSP_SHADERGENERATOR_H