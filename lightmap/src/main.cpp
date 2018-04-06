#include <pandaFramework.h>
#include <ambientLight.h>
#include <directionalLight.h>
#include <pointLight.h>
#include <sphereLight.h>
#include <spotlight.h>
#include <nodePath.h>
#include <nodePathCollection.h>
#include <geomNode.h>
#include <geom.h>
#include <geomPrimitive.h>
#include <geomVertexData.h>
#include <geomVertexReader.h>
#include <internalName.h>
#include <loader.h>
#include <virtualFileSystem.h>
#include <pnmImage.h>
#include <pnmFileTypeJPG.h>

#define TOONTOWN

static int lightmap_idx = 0;

struct Lightmap;

struct LuxelInfo
{
        Lightmap *lightmap;

        // the 0-63 x and y pixel position
        LVector2i pixel_pos;

        // 0-1 UV coordinate
        LVector2f uv;

        // 3D space world position of this luxel
        LVector3f world_pos;

        bool legal;
};

struct Lightmap
{
        PNMImage *texture;
        pvector<LuxelInfo> luxels;
        const GeomPrimitive *primitive;
        const GeomVertexData *vert_data;
        GeomVertexReader vreader;
        GeomVertexReader nreader;
        GeomVertexReader treader;
};

vector<Lightmap *> lightmaps;

void luxel_get_world_pos( LuxelInfo &luxel )
{

        bool inside = false;
        for ( int p = 0; p < luxel.lightmap->primitive->get_num_primitives(); p++ )
        {
                int start = luxel.lightmap->primitive->get_primitive_start( p );
                int end = luxel.lightmap->primitive->get_primitive_end( p );

                for ( int i = start, j = end - 1; i < end; i = j++ )
                {
                        int ivert_idx = luxel.lightmap->primitive->get_vertex( i );
                        luxel.lightmap->treader.set_row( ivert_idx );
                        LVecBase3f icoord = luxel.lightmap->treader.get_data3f();

                        int jvert_idx = luxel.lightmap->primitive->get_vertex( j );
                        luxel.lightmap->treader.set_row( jvert_idx );
                        LVecBase3f jcoord = luxel.lightmap->treader.get_data3f();

                        bool intersect = ( ( icoord.get_y() > luxel.uv.get_y() ) != ( jcoord.get_y() > luxel.uv.get_y() ) ) &&
                                (luxel.uv.get_x() < (jcoord.get_x() - icoord.get_x()) * (luxel.uv.get_y() - icoord.get_y()) / (jcoord.get_y() - icoord.get_y()) + icoord.get_x());

                        if ( intersect )
                                inside = !inside;
                }

        }

        luxel.legal = inside;
        if ( luxel.legal )
                luxel.world_pos = get_world_pos();
        
}

void process_primitive( const GeomPrimitive *prim, const GeomVertexData *vert_data, GeomVertexReader &vreader,
                        GeomVertexReader &nreader, GeomVertexReader &treader )
{
        nassertv( prim != nullptr );

        for ( int i = 0; i < prim->get_num_primitives(); i++ )
        {
                int lms = 64;

                PNMImage *lightmap_img = new PNMImage( lms, lms );
                // Lightmap is completely black until lit by a light
                lightmap_img->fill( 0.0 );

                Lightmap *lm = new Lightmap;
                lm->texture = lightmap_img;
                lm->primitive = prim;
                lm->vreader = vreader;
                lm->nreader = nreader;
                lm->treader = treader;
                lm->vert_data = vert_data;

                lightmaps.push_back( lm );

                for ( int row = 0; row < lms; row++ )
                {
                        for ( int col = 0; col < lms; col++ )
                        {
                                LuxelInfo luxel;
                                luxel.lightmap = lm;
                                luxel.pixel_pos = LVector2i( col, row );
                                luxel.uv = LVector2f( ( col + 0.5 ) / lms, ( row + 0.5 ) / lms );
                                luxel_get_world_pos( luxel );
                                lm->luxels.push_back( luxel );
                        }
                }

                PNMFileTypeJPG jpg;
                stringstream ss;
                ss << "generated_lightmaps/lightmap-" << lightmap_idx << ".jpg";
                lightmap_img->write( Filename( ss.str() ), &jpg );
                lightmap_idx++;

        }
}

void process_geom( const Geom *geom )
{
        nassertv( geom != nullptr );

        const GeomVertexData *vert_data = geom->get_vertex_data();

        nassertv( vert_data != nullptr );

        if ( !vert_data->has_column( InternalName::get_vertex() ) ||
             !vert_data->has_column( InternalName::get_normal() ) ||
             !vert_data->has_column( InternalName::get_texcoord() ) )
        {
                cout << "Skipping geom that is missing required columns." << endl;
                return;
        }

        GeomVertexReader vreader( vert_data, InternalName::get_vertex() );
        GeomVertexReader nreader( vert_data, InternalName::get_normal() );
        GeomVertexReader treader( vert_data, InternalName::get_texcoord() );

        for ( int i = 0; i < geom->get_num_primitives(); i++ )
        {
                process_primitive( geom->get_primitive( i ), vert_data, vreader, nreader, treader );
        }
}

void process_geomnode( GeomNode *gn )
{
        int num_geoms = gn->get_num_geoms();
        for ( int j = 0; j < num_geoms; j++ )
        {
                const Geom *geom = gn->get_geom( j );
                process_geom( geom );
        }
}

int main( int argc, char *argv[] )
{
        Loader *loader = Loader::get_global_ptr();
        VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

#ifdef TOONTOWN
        vfs->mount( Filename( "phase_0.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_3.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_3.5.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_4.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_5.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_5.5.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_6.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_7.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_8.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_9.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_10.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_11.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_12.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
        vfs->mount( Filename( "phase_13.mf" ), Filename( "." ), VirtualFileSystem::MF_read_only );
#endif

        PandaFramework framework;
        framework.open_framework( argc, argv );

        WindowFramework *window = framework.open_window();
        window->setup_trackball();
        window->enable_keyboard();

        system( "mkdir generated_lightmaps" );

        NodePath render = window->get_render();

        if ( argc < 2 )
        {
                cout << "Error: No scene file specified! Cannot generate lightmaps." << endl;
                return 1;
        }
        char *scene_file = argv[1];
        NodePath scene = render.attach_new_node( loader->load_sync( scene_file ) );

        NodePathCollection all_geomnodes = render.find_all_matches( "**/+GeomNode" );
        for ( int i = 0; i < all_geomnodes.get_num_paths(); i++ )
        {
                GeomNode *gn = DCAST( GeomNode, all_geomnodes.get_path( i ).node() );
                process_geomnode( gn );
        }

        
        framework.main_loop();
        return 0;
}