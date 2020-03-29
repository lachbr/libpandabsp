#include "radstaticprop.h"
#include <loader.h>
#include "bspfile.h"
#include <collisionNode.h>
#include <geomNode.h>
#include <geom.h>
#include <geomVertexReader.h>
#include <geomVertexData.h>
#include <geomPrimitive.h>
#include <nodePathCollection.h>
#include <lightMutexHolder.h>
#include <look_at.h>
#include <virtualFileSystem.h>

#include "threads.h"
#include "qrad.h"
#include "lightingutils.h"
#include "lightmap.h"
#include "trace.h"

pvector<RADStaticProp *> g_static_props;

static NodePath g_proproot( "proproot" );

void LoadStaticProps()
{
        Loader *loader = Loader::get_global_ptr();

        // Use one mesh for all of the props
        PT( RayTraceTriangleMesh ) mesh = new RayTraceTriangleMesh;
        mesh->set_mask( CONTENTS_PROP );
        mesh->set_build_quality( RayTraceScene::BUILD_QUALITY_HIGH );

        for ( size_t i = 0; i < g_bspdata->dstaticprops.size(); i++ )
        {
                dstaticprop_t *prop = &g_bspdata->dstaticprops[i];
                string mdl_path = prop->name;

                NodePath propnp( loader->load_sync( Filename::from_os_specific( mdl_path ) ) );
                if ( !propnp.is_empty() )
                {
                        propnp.set_scale( prop->scale[0] * PANDA_TO_HAMMER, prop->scale[1] * PANDA_TO_HAMMER, prop->scale[2] * PANDA_TO_HAMMER );
                        propnp.set_pos( prop->pos[0], prop->pos[1], prop->pos[2] );
                        propnp.set_hpr( prop->hpr[1] - 90, prop->hpr[0], prop->hpr[2] );

                        // Which leaf does the prop reside in?
                        dleaf_t *leaf = PointInLeaf( prop->pos );

                        bool shadow_caster = ( prop->flags & STATICPROPFLAGS_LIGHTMAPSHADOWS ) != 0;

                        RADStaticProp *sprop = new RADStaticProp;
                        sprop->shadows = shadow_caster;
                        sprop->leafnum = leaf - g_bspdata->dleafs;
                        sprop->mdl = propnp;
                        sprop->propnum = (int)i;

                        // Apply all transforms and attribs to the vertices so they are now in world space,
                        // but do not remove any nodes or rearrange the vertices.
                        propnp.clear_model_nodes();
                        propnp.flatten_light();

                        if ( shadow_caster )
                        {
                                NodePathCollection npc = propnp.find_all_matches( "**/+GeomNode" );
                                for ( int i = 0; i < npc.get_num_paths(); i++ )
                                {
                                        NodePath geomnp = npc.get_path( i );
                                        GeomNode *gn = DCAST( GeomNode, geomnp.node() );
                                        for ( int j = 0; j < gn->get_num_geoms(); j++ )
                                        {
                                                mesh->add_triangles_from_geom( gn->get_geom( j ) );
                                        }
                                }
                        }


                        g_static_props.push_back( sprop );

                        cout << "Successfully loaded static prop " << mdl_path << endl;
                }
                else
                {
                        cout << "Warning! Could not load static prop " << mdl_path << ", no shadows" << endl;
                }
        }

        mesh->build();
        RADTrace::scene->add_geometry( mesh );
        g_proproot.attach_new_node( mesh );
        RADTrace::scene->update();
}

void BuildGeomNodes_r( PandaNode *node, pvector<PT( GeomNode )> &list )
{
        if ( node->is_of_type( GeomNode::get_class_type() ) )
        {
                list.push_back( DCAST( GeomNode, node ) );
        }

        for ( int i = 0; i < node->get_num_children(); i++ )
        {
                BuildGeomNodes_r( node->get_child( i ), list );
        }
}

pvector<PT( GeomNode )> BuildGeomNodes( const NodePath &root )
{
        pvector<PT( GeomNode )> list;

        BuildGeomNodes_r( root.node(), list );

        return list;
}

struct VDataDef
{
        CPT( GeomVertexData ) vdata;
        LMatrix4 mat_to_world;
};

void ComputeStaticPropLighting( const int prop_idx )
{
        if ( prop_idx < 0 || prop_idx > (int)g_static_props.size() - 1 )
        {
                Warning( "ThreadComputeStaticPropLighting: prop %i is invalid\n", prop_idx );
                return;
        }
        RADStaticProp *prop = g_static_props[prop_idx];
        if ( ( g_bspdata->dstaticprops[prop->propnum].flags & STATICPROPFLAGS_STATICLIGHTING ) == 0 )
        {
                // baked lighting not wanted
                return;
        }

        pvector<VDataDef> vdatas;
        vdatas.reserve( 1000 );

        // transform all vertices to be in world space
        prop->mdl.clear_model_nodes();
        prop->mdl.flatten_light();

        // we're not using NodePath::find_all_matches() because we need a consistent order in the list
        pvector<PT( GeomNode )> geomnodes = BuildGeomNodes( prop->mdl );
        for ( size_t i = 0; i < geomnodes.size(); i++ )
        {
                PT( GeomNode ) gn = geomnodes[i];
                for ( int j = 0; j < gn->get_num_geoms(); j++ )
                {
                        CPT( Geom ) geom = gn->get_geom( j );
                        CPT( GeomVertexData ) vdata = geom->get_vertex_data();

                        VDataDef def;
                        def.vdata = vdata;
                        def.mat_to_world = NodePath( gn ).get_net_transform()->get_mat();
                        vdatas.push_back( def );
                }
        }

        pvector<dstaticpropvertexdata_t> newvdatas;
        newvdatas.reserve( vdatas.size() );
        pvector<colorrgbexp32_t> newsamples;
        newsamples.reserve( 10000 );

        for ( size_t i = 0; i < vdatas.size(); i++ )
        {
                CPT( GeomVertexData ) vdata = vdatas[i].vdata;
                GeomVertexReader vtx_reader( vdata, InternalName::get_vertex() );
                GeomVertexReader norm_reader( vdata, InternalName::get_normal() );

                dstaticpropvertexdata_t dvdata;
                dvdata.first_lighting_sample = newsamples.size();

                for ( int row = 0; row < vdata->get_num_rows(); row++ )
                {
                        vtx_reader.set_row( row );
                        norm_reader.set_row( row );

                        LVector3 world_pos( vtx_reader.get_data3f() );
                        LNormalf world_normal( norm_reader.get_data3f() );

                        LVector3 direct_col( 0 );
                        ComputeDirectLightingAtPoint( world_pos, world_normal, direct_col );

                        LVector3 indirect_col( 0 );
                        ComputeIndirectLightingAtPoint( world_pos, world_normal, indirect_col, true );

                        colorrgbexp32_t sample;
                        VectorToColorRGBExp32( direct_col + indirect_col, sample );
                        newsamples.push_back( sample );
                }

                dvdata.num_lighting_samples = newsamples.size() - dvdata.first_lighting_sample;

                newvdatas.push_back( dvdata );
        }

        ThreadLock();
        dstaticprop_t *dprop = &g_bspdata->dstaticprops[prop->propnum];
        dprop->first_vertex_data = g_bspdata->dstaticpropvertexdatas.size();
        for ( size_t i_vdata = 0; i_vdata < newvdatas.size(); i_vdata++ )
        {
                dstaticpropvertexdata_t &dvdata = newvdatas[i_vdata];
                int first = dvdata.first_lighting_sample;
                dvdata.first_lighting_sample = g_bspdata->staticproplighting.size();
                for ( int i = 0; i < dvdata.num_lighting_samples; i++ )
                {
                        g_bspdata->staticproplighting.push_back( newsamples[first + i] );
                }
                g_bspdata->dstaticpropvertexdatas.push_back( dvdata );
        }
        dprop->num_vertex_datas = g_bspdata->dstaticpropvertexdatas.size() - dprop->first_vertex_data;
        ThreadUnlock();
}

void DoComputeStaticPropLighting()
{
        //Log( "Computing static prop lighting...\n" );
        NamedRunThreadsOnIndividual( (int)g_static_props.size(), g_estimate, ComputeStaticPropLighting );
        //for ( size_t i = 0; i < g_static_props.size(); i++ )
        //{
        //        Log( "%i ", (int)i );
        //        ComputeStaticPropLighting( i );
        //}
}