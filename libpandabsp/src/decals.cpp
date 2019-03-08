/**
 * PANDA3D BSP LIBRARY
 * Copyright (c) CIO Team. All rights reserved.
 *
 * @file decals.cpp
 * @author Brian Lach
 * @date March 08, 2019
 *
 * @desc Decals!
 *
 */

#include "decals.h"
#include "bsp_trace.h"
#include "bsploader.h"
#include "bsp_material.h"

#include <cardMaker.h>
#include <configVariableInt.h>

/**
 * Trace a decal onto the world.
 */
NodePath DecalManager::decal_trace( const std::string &decal_material, const LPoint2 &decal_scale,
        float rotate, const LPoint3 &start, const LPoint3 &end )
{
        // Find the surface to decal
        Trace tr;
        Ray ray( start * 16, end * 16, LPoint3::zero(), LPoint3::zero() );
        CM_BoxTrace( ray, 0, CONTENTS_SOLID, true, _loader->get_colldata(), tr );

        // Do we have a surface to decal?
        if ( !tr.has_hit() )
                return NodePath();
        
        LPoint3 decal_origin = tr.end_pos / 16.0;
        LPoint3 decal_dir( tr.plane.normal[0], tr.plane.normal[1], tr.plane.normal[2] );

        CardMaker cm( "decal" );
        cm.set_frame( -decal_scale[0] * 0.5, decal_scale[0] * 0.5, -decal_scale[1] * 0.5, decal_scale[1] * 0.5 );
        cm.set_has_uvs( true );
        cm.set_has_normals( true );
        cm.set_uv_range( LTexCoord( 0, 0 ), LTexCoord( 1, 1 ) );
        NodePath decalnp = _loader->get_result().attach_new_node( cm.generate() );
        decalnp.set_depth_offset( 1, 1 );
        decalnp.look_at( -decal_dir );
        decalnp.set_r( rotate );
        decalnp.set_shader_auto( 1 );
        const BSPMaterial *mat = BSPMaterial::get_from_file( decal_material );
        decalnp.set_attrib( BSPMaterialAttrib::make( mat ), 1 );
        if ( mat->has_transparency() )
                decalnp.set_transparency( TransparencyAttrib::M_dual, 1 );
        decalnp.flatten_strong();
        decalnp.set_pos( decal_origin );

        LPoint3 mins, maxs;
        decalnp.calc_tight_bounds( mins, maxs );
        PT( BoundingBox ) bounds = new BoundingBox( mins, maxs );

        for ( int i = (int)_decals.size() - 1; i >= 0; i-- )
        {
                Decal *other = _decals[i];

                // Check if we overlap with this decal.
                //
                // Only remove this decal if it is smaller than the decal
                // we are wanting to create over it, and it is not a static
                // decal (placed by the level designer, etc).
                if ( other->bounds->contains( bounds ) != BoundingVolume::IF_no_intersection &&
                        other->bounds->get_volume() <= bounds->get_volume() &&
                        ( other->flags & DECALFLAGS_STATIC ) == 0 )
                {
                        if ( !other->decalnp.is_empty() )
                                other->decalnp.remove_node();
                        _decals.erase( std::find( _decals.begin(), _decals.end(), other ) );
                }
        }

        if ( _decals.size() == ConfigVariableInt( "decals_max", 20 ).get_value() )
        {
                // Remove the oldest decal to make space for the new one.
                Decal *d = _decals.back();
                if ( !d->decalnp.is_empty() )
                        d->decalnp.remove_node();
                _decals.pop_back();
        }

        PT( Decal ) decal = new Decal;
        decal->decalnp = decalnp;
        decal->bounds = bounds;
        decal->flags = DECALFLAGS_NONE;
        _decals.push_front( decal );

        return decalnp;
}

void DecalManager::cleanup()
{
        for ( size_t i = 0; i < _decals.size(); i++ )
        {
                Decal *d = _decals[i];
                if ( !d->decalnp.is_empty() )
                        d->decalnp.remove_node();
        }
        _decals.clear();
}