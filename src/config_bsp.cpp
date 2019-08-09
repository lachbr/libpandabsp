/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file config_bsp.cpp
 * @author Brian Lach
 * @date March 27, 2018
 */

#include "config_bsp.h"
#include "entity.h"
#include "bsploader.h"
#include "bsp_render.h"
#include "shader_generator.h"
#include "bsp_material.h"
#include "shader_spec.h"
#include "aux_data_attrib.h"
#include "bounding_kdop.h"
#include "raytrace.h"
#include "ambient_boost_effect.h"
#include "glow_node.h"
#include "postprocess/postprocess.h"
#include "postprocess/hdr.h"
#include "postprocess/bloom.h"

#include <texturePool.h>
#include "texture_filter.h"

ConfigureDef( config_bsp );
ConfigureFn( config_bsp )
{
        init_libpandabsp();
}

void init_libpandabsp()
{
        static bool initialized = false;
        if ( initialized )
                return;
        initialized = true;

	BSPFaceAttrib::init_type();
	CBaseEntity::init_type();
        CPointEntity::init_type();
	CBrushEntity::init_type();
        CBoundsEntity::init_type();
        BSPRender::init_type();
        BSPCullTraverser::init_type();
        BSPRoot::init_type();
        BSPProp::init_type();
        BSPModel::init_type();
        BSPShaderGenerator::init_type();
	GlowNode::init_type();

        AmbientBoostEffect::init_type();
        AmbientBoostEffect::register_with_read_factory();

        BSPMaterial::init_type();
        BSPMaterialAttrib::init_type();
        BSPMaterialAttrib::register_with_read_factory();

        AuxDataAttrib::init_type();

        ShaderSpec::init_type();

        BoundingKDOP::init_type();

        RayTrace::initialize();
        RayTraceGeometry::init_type();
        RayTraceTriangleMesh::init_type();

	PostProcessPass::init_type();
	PostProcessEffect::init_type();

	HDRPass::init_type();
	BloomEffect::init_type();

	BSPTextureFilter::init_type();
	TexturePool::get_global_ptr()->register_filter( new BSPTextureFilter );
}