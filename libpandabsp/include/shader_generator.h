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
#include <fog.h>

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

NotifyCategoryDeclNoExport(bspShaderGenerator);

class nodeshaderinput_t;

BEGIN_PUBLISH
enum ShaderQuality
{
        SHADERQUALITY_LOW,
        SHADERQUALITY_MEDIUM,
        SHADERQUALITY_HIGH,
};

enum CameraBits
{
	CAMERA_MAIN		= 1 << 0,
	CAMERA_SHADOW		= 1 << 1,
	CAMERA_REFLECTION	= 1 << 2,
	CAMERA_REFRACTION	= 1 << 3,
	CAMERA_VIEWMODEL	= 1 << 4,
	CAMERA_COMPUTE		= 1 << 5,
};

enum AuxBits
{
	AUXBITS_NORMAL = 1 << 0,
	AUXBITS_ARME = 1 << 1,
};
END_PUBLISH

// Which cameras need lighting information?
#define CAMERA_MASK_LIGHTING ( CAMERA_MAIN | CAMERA_REFLECTION | CAMERA_REFRACTION | CAMERA_VIEWMODEL )
// Which cameras should use view frustum culling?
#define CAMERA_MASK_CULLING ( CAMERA_MAIN | CAMERA_REFLECTION | CAMERA_REFRACTION )

class BSPShaderGenerator : public ShaderGenerator
{
PUBLISHED:
        BSPShaderGenerator( GraphicsStateGuardian *gsg, const NodePath &camera, const NodePath &render );

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

        void set_shader_quality( int quality );
        INLINE int get_shader_quality() const
        {
                return _shader_quality;
        }

	INLINE void set_fog( Fog *fog )
	{
		_fog = fog;
		_render.set_fog( _fog );
	}
	INLINE void clear_fog()
	{
		_fog = nullptr;
		_render.clear_fog();
	}
	INLINE Fog *get_fog() const
	{
		return _fog;
	}
	INLINE PTA_LVecBase4f get_fog_data() const
	{
		return _pta_fogdata;
	}

	INLINE void set_exposure_texture( Texture *tex )
	{
		_exposure_texture = tex;
	}
	INLINE Texture *get_exposure_texture() const
	{
		return _exposure_texture;
	}

        static void set_identity_cubemap( Texture *tex );
        static Texture *get_identity_cubemap();

	static CPT( Shader ) make_shader( const ShaderSpec *spec, const ShaderPermutations *perms );

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

	PT( Fog ) _fog;
	PTA_LVecBase4f _pta_fogdata;

	PT( Texture ) _exposure_texture;

        static AsyncTask::DoneStatus update_pssm( GenericAsyncTask *task, void *data );

        int _shader_quality;
        bool _has_shadow_sunlight;
        WeakNodePath _sunlight;
        PT( GenericAsyncTask ) _update_task;
        GraphicsStateGuardian *_gsg;
        LVector3 _sun_vector;
        NodePath _camera;
        NodePath _render;

        static PT( Texture ) _identity_cubemap;

public:
        static TypeHandle get_class_type()
        {
                return _type_handle;
        }
        static void init_type()
        {
                ShaderGenerator::init_type();
                register_type( _type_handle, "BSPShaderGenerator",
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