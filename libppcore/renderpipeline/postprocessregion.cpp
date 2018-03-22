#include "postProcessRegion.h"
#include <geomVertexFormat.h>
#include <geomVertexData.h>
#include <geomVertexWriter.h>
#include <geom.h>
#include <geomTriangles.h>
#include <geomNode.h>

#include <omniBoundingVolume.h>
#include <orthographicLens.h>

PostProcessRegion PostProcessRegion::make( PT( GraphicsOutput ) internal_buffer )
{
        return PostProcessRegion( internal_buffer );
}

PostProcessRegion::PostProcessRegion()
{
}

PostProcessRegion::PostProcessRegion( PT( GraphicsOutput ) internal_buffer )
{
        _buffer = internal_buffer;
        _region = _buffer->make_display_region();
        _node = NodePath( "RTRoot" );
        make_fullscreen_tri();
        make_fullscreen_cam();
}

void PostProcessRegion::set_attrib( CPT( RenderAttrib ) attrib, int priority )
{
        _tri.set_attrib( attrib, priority );
}

void PostProcessRegion::set_sort( int sort )
{
        _region->set_sort( sort );
}

void PostProcessRegion::set_instance_count( int count )
{
        _tri.set_instance_count( count );
}

void PostProcessRegion::disable_clears()
{
        _region->disable_clears();
}

void PostProcessRegion::set_active( bool active )
{
        _region->set_active( active );
}

void PostProcessRegion::set_clear_depth_active( bool active )
{
        _region->set_clear_depth_active( active );
}

void PostProcessRegion::set_clear_depth( PN_stdfloat depth )
{
        _region->set_clear_depth( depth );
}

void PostProcessRegion::set_shader( CPT( Shader ) shader, int priority )
{
        _tri.set_shader( shader, priority );
}

void PostProcessRegion::set_camera( const NodePath &cam )
{
        _region->set_camera( cam );
}

void PostProcessRegion::set_clear_color_active( bool active )
{
        _region->set_clear_color_active( active );
}

void PostProcessRegion::set_clear_color( const LColor &color )
{
        _region->set_clear_color( color );
}

const NodePath &PostProcessRegion::get_node() const
{
        return _node;
}

const NodePath &PostProcessRegion::get_tri() const
{
        return _tri;
}

void PostProcessRegion::set_shader_input( const ShaderInput *inp, bool override )
{
        if ( override )
        {
                _node.set_shader_input( inp );
        }
        else
        {
                _tri.set_shader_input( inp );
        }
}

void PostProcessRegion::make_fullscreen_tri()
{
        CPT( GeomVertexFormat ) form = GeomVertexFormat::get_v3();
        PT( GeomVertexData ) vdata = new GeomVertexData( "vertices", form, Geom::UH_static );
        vdata->set_num_rows( 3 );
        GeomVertexWriter vwriter( vdata, "vertex" );
        vwriter.add_data3f( -1, 0, -1 );
        vwriter.add_data3f( 3, 0, -1 );
        vwriter.add_data3f( -1, 0, 3 );
        PT( GeomTriangles ) tris = new GeomTriangles( Geom::UH_static );
        tris->add_next_vertices( 3 );
        PT( Geom ) geom = new Geom( vdata );
        geom->add_primitive( tris );
        PT( GeomNode ) gn = new GeomNode( "gn" );
        gn->add_geom( geom );
        gn->set_final( true );
        gn->set_bounds( new OmniBoundingVolume() );
        NodePath tri = NodePath( gn );
        tri.set_depth_test( false );
        tri.set_depth_write( false );
        tri.set_attrib( TransparencyAttrib::make( TransparencyAttrib::M_none ), 10000 );
        tri.set_color( LVecBase4f( 1 ) );
        tri.set_bin( "unsorted", 10 );
        tri.reparent_to( _node );
        _tri = tri;
}

void PostProcessRegion::make_fullscreen_cam()
{
        PT( Camera ) cam = new Camera( "BufferCamera" );
        PT( OrthographicLens ) lens = new OrthographicLens();
        lens->set_film_size( 2, 2 );
        lens->set_film_offset( 0, 0 );
        lens->set_near_far( -100, 100 );
        cam->set_lens( lens );
        cam->set_cull_bounds( new OmniBoundingVolume() );
        _cam = _node.attach_new_node( cam );
        _region->set_camera( _cam );
}