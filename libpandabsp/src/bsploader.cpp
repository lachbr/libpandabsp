#include "bsploader.h"
#include "bspfile.h"

#include <array>
#include <bitset>
#include <math.h>

#include <asyncTaskManager.h>
#include <eggData.h>
#include <eggPolygon.h>
#include <eggVertexUV.h>
#include <geomNode.h>
#include <load_egg_file.h>
#include <loader.h>
#include <nodePathCollection.h>
#include <pnmFileTypeJPG.h>
#include <pointLight.h>
#include <randomizer.h>
#include <rigidBodyCombiner.h>
#include <texture.h>
#include <texturePool.h>
#include <textureStage.h>
#include <virtualFileSystem.h>
#include <directionalLight.h>
#include <ambientLight.h>
#include <spotlight.h>
#include <fog.h>

//#define VISUALIZE_PLANE_COLORS
//#define EXTRACT_LIGHTMAPS

#define DEFAULT_GAMMA 1.0
#define QRAD_GAMMA 0.55
#define DEFAULT_OVERBRIGHT 1
#define MAP_SCALE .075

void GetParamsFromEnt( entity_t *mapent )
{
	int num_epairs = sizeof( mapent->epairs ) / sizeof( epair_t );
	cout << "Number of epairs: " << num_epairs << endl;
}

double round( double num )
{
	return floor( num + 0.5 );
}

NotifyCategoryDef(bspfile, "");

static float lineartovertex[4096];

static void build_gamma_table( float gamma, int overbright )
{
        float f;
        float overbright_factor = 1.0;
        if ( overbright == 2 )
        {
                overbright_factor = 0.5;
        }
        else if ( overbright == 4 )
        {
                overbright_factor = 0.25;
        }

        for ( int i = 0; i < 4096; i++ )
        {
                f = pow( i / 1024.0, 1.0 / gamma );
                lineartovertex[i] = f * overbright_factor;
                if ( lineartovertex[i] > 1 )
                {
                        lineartovertex[i] = 1.0;
                }
                        
        }
}

double linear_to_vert_light( double c )
{
        int idx = (int)( c * 1024.0 + 0.5 );
        if ( idx > 4095 )
        {
                idx = 4095;
        }
        else if ( idx < 0 )
        {
                idx = 0;
        }

        return lineartovertex[idx];
}

LRGBColor color_shift_pixel( unsigned char *sample )
{
	return LRGBColor( linear_to_vert_light( sample[0] / 255.0 ),
			  linear_to_vert_light( sample[1] / 255.0 ),
			  linear_to_vert_light( sample[2] / 255.0 ) );
}


PNMImage BSPLoader::lightmap_image_from_face( dface_t *face, FaceLightmapData *ld )
{
	int width = ld->texsize[0];
	int height = ld->texsize[1];
        int num_luxels = width * height;

	if ( num_luxels <= 0 )
	{
		return PNMImage( 16, 16 );
	}

        PNMImage img( width, height );

	int luxel = 0;

	for ( int y = 0; y < height; y++ )
	{
		for ( int x = 0; x < width; x++ )
		{
			LRGBColor luxel_col( 1, 1, 1 );

			// To get the final pixel color, multiply all of the individual lightstyle samples together.
			for ( int lightstyle = 0; lightstyle < 4; lightstyle++ )
			{
				if ( face->styles[lightstyle] == 0xFF )
				{
					// Doesn't have this lightstyle.
					continue;
				}

				// From p3rad
				int sample_idx = face->lightofs + lightstyle * num_luxels * 3 + luxel * 3;
				unsigned char *sample = &g_dlightdata[sample_idx];

				luxel_col.componentwise_mult( color_shift_pixel( sample ) );

			}

			img.set_xel( x, y, luxel_col );
			luxel++;
		}
	}

        return img;
}

int BSPLoader::find_leaf( const LPoint3 &pos )
{
        int i = 0;

        // Walk the BSP tree to find the index of the leaf which contains the specified
        // position.
        while ( i >= 0 )
        {
                const dnode_t *node = &g_dnodes[i];
                const dplane_t *plane = &g_dplanes[node->planenum];
                float distance = plane->normal[0] * pos.get_x() +
                        plane->normal[1] * pos.get_y() +
                        plane->normal[2] * pos.get_z() - plane->dist;

                if ( distance >= 0.0 )
                {
                        i = node->children[0];
                }     
                else
                {
                        i = node->children[1];
                }
                        
        }

        return ~i;
}

LTexCoord BSPLoader::get_vertex_uv( texinfo_t *texinfo, dvertex_t *vert ) const
{
        float *vpos = vert->point;
	LVector3 vert_pos( vpos[0], vpos[1], vpos[2] );

	LVector3 s_vec( texinfo->vecs[0][0], texinfo->vecs[0][1], texinfo->vecs[0][2] );
	float s_dist = texinfo->vecs[0][3];

	LVector3 t_vec( texinfo->vecs[1][0], texinfo->vecs[1][1], texinfo->vecs[1][2] );
	float t_dist = texinfo->vecs[1][3];

	return LTexCoord( s_vec.dot( vert_pos ) + s_dist, t_vec.dot( vert_pos ) + t_dist );
}

PT( EggVertex ) BSPLoader::make_vertex( EggVertexPool *vpool, EggPolygon *poly,
                                        dedge_t *edge, texinfo_t *texinfo,
                                        dface_t *face, int k, FaceLightmapData *ld,
					Texture *tex )
{
        dvertex_t *vert = &g_dvertexes[edge->v[k]];
        float *vpos = vert->point;
        PT( EggVertex ) v = new EggVertex;
        v->set_pos( LPoint3d( vpos[0], vpos[1], vpos[2] ) );

	// The widths and heights are retrieved from the actual loaded textures that were referenced.
	double df_width = 1.0;
	double df_height = 1.0;
	if ( tex != nullptr )
	{
		df_width = tex->get_orig_file_x_size();
		df_height = tex->get_orig_file_y_size();
	}

        // Texture and lightmap coordinates
        LTexCoord uv = get_vertex_uv( texinfo, vert );
	LTexCoordd df_uv( uv.get_x() / df_width, -uv.get_y() / df_height );
	LTexCoordd lm_uv( ( ld->midtexs[0] + ( uv.get_x() - ld->midpolys[0] ) / TEXTURE_STEP ) / ld->texsize[0],
		         -( ld->midtexs[1] + ( uv.get_y() - ld->midpolys[1] ) / TEXTURE_STEP ) / ld->texsize[1] );

	v->set_uv( "diffuse",  df_uv);
	v->set_uv( "lightmap", lm_uv );

        return v;
}

PT( Texture ) BSPLoader::try_load_texref( texref_t *tref )
{
        if ( _texref_textures.find( tref ) != _texref_textures.end() )
        {
                return _texref_textures[tref];
        }

        vector_string extensions;
        extensions.push_back( ".jpg" );
        extensions.push_back( ".png" );
        extensions.push_back( ".bmp" );
        extensions.push_back( ".tif" );

        string name = tref->name;

        for ( size_t i = 0; i < extensions.size(); i++ )
        {
                string ext = extensions[i];
                PT( Texture ) tex = TexturePool::load_texture( name + ext );
                if ( tex != nullptr )
                {
                        bspfile_cat.info()
				<< "Loaded texref " << tref->name << "\n";
                        _texref_textures[tref] = tex;
                        return tex;
                }
        }

        return nullptr;
}

void BSPLoader::make_faces()
{
        bspfile_cat.info()
		<< "Making faces...\n";
	for ( int facenum = 0; facenum < g_numfaces; facenum++ )
	{
		dface_t *face = &g_dfaces[facenum];

		PT( EggData ) data = new EggData;
		data->set_bsp_mode( _want_visibility );
		PT( EggVertexPool ) vpool = new EggVertexPool( "facevpool" );
		data->add_child( vpool );

		// This face lies on this plane, create it.
		PT( EggPolygon ) poly = new EggPolygon;
		data->add_child( poly );

		texinfo_t *texinfo = &g_texinfo[face->texinfo];

		texref_t *texref = &g_dtexrefs[texinfo->texref];
		contents_t contents = GetTextureContents( texref->name );
		if ( contents == CONTENTS_SKY )
		{
			// We don't render sky brushes.
			continue;
		}

		PT( Texture ) tex = try_load_texref( texref );
		string texture_name = texref->name;
		transform( texture_name.begin(), texture_name.end(), texture_name.begin(), tolower );

		if ( texture_name.find( "trigger" ) != string::npos )
		{
			continue;
		}

		bool has_lighting = face->lightofs != -1;

		PT( Texture ) lm_tex = new Texture( "lightmap_texture" );

		/* ************* FROM P3RAD ************* */

		FaceLightmapData ld;

		ld.mins[0] = ld.mins[1] = 999999.0;
		ld.maxs[0] = ld.maxs[1] = -99999.0;

		for ( int i = 0; i < face->numedges; i++ )
		{
			int edge_idx = g_dsurfedges[face->firstedge + i];
			dedge_t *edge;
			dvertex_t *vert;
			if ( edge_idx >= 0 )
			{
				edge = &g_dedges[edge_idx];
				vert = &g_dvertexes[edge->v[0]];
			}
			else
			{
				edge = &g_dedges[-edge_idx];
				vert = &g_dvertexes[edge->v[1]];
			}

			LTexCoord uv = get_vertex_uv( texinfo, vert );

			if ( uv.get_x() < ld.mins[0] )
				ld.mins[0] = uv.get_x();
			else if ( uv.get_x() > ld.maxs[0] )
				ld.maxs[0] = uv.get_x();

			if ( uv.get_y() < ld.mins[1] )
				ld.mins[1] = uv.get_y();
			else if ( uv.get_y() > ld.maxs[1] )
				ld.maxs[1] = uv.get_y();
		}

		ld.texmins[0] = floor( ld.mins[0] / TEXTURE_STEP );
		ld.texmins[1] = floor( ld.mins[1] / TEXTURE_STEP );
		ld.texmaxs[0] = ceil( ld.maxs[0] / TEXTURE_STEP );
		ld.texmaxs[1] = ceil( ld.maxs[1] / TEXTURE_STEP );

		ld.texsize[0] = floor( (double)( ld.texmaxs[0] - ld.texmins[0] ) + 1 );
		ld.texsize[1] = floor( (double)( ld.texmaxs[1] - ld.texmins[1] ) + 1 );

		ld.midpolys[0] = ( ld.mins[0] + ld.maxs[0] ) / 2.0;
		ld.midpolys[1] = ( ld.mins[1] + ld.maxs[1] ) / 2.0;
		ld.midtexs[0] = ld.texsize[0] / 2.0;
		ld.midtexs[1] = ld.texsize[1] / 2.0;

		if ( has_lighting )
		{
			// Generate a lightmap texture from the samples
			PNMImage lm_img = lightmap_image_from_face( face, &ld );
#ifdef EXTRACT_LIGHTMAPS
			stringstream ss;
			ss << "extractedLightmaps/lightmap_" << face << ".jpg";
			PNMFileTypeJPG jpg;
			lm_img.write( Filename( ss.str() ), &jpg );
#endif
			lm_tex->load( lm_img );
			lm_tex->set_magfilter( SamplerState::FT_linear );
			lm_tex->set_minfilter( SamplerState::FT_linear_mipmap_linear );
		}


		bspfile_cat.info()
			<< "Face has " << face->numedges << " edges\n";

		int last_edge = face->firstedge + face->numedges;
		int first_edge = face->firstedge;
		for ( int j = last_edge - 1; j >= first_edge; j-- )
		{
			int surf_edge = g_dsurfedges[j];

			dedge_t *edge;
			if ( surf_edge >= 0 )
				edge = &g_dedges[surf_edge];
			else
				edge = &g_dedges[-surf_edge];

			if ( surf_edge < 0 )
			{
				for ( int k = 0; k < 2; k++ )
				{
					PT( EggVertex ) v = make_vertex( vpool, poly, edge, texinfo,
									        face, k, &ld, tex );
					vpool->add_vertex( v );
					poly->add_vertex( v );
				}
			}
			else
			{
				for ( int k = 1; k >= 0; k-- )
				{
					PT( EggVertex ) v = make_vertex( vpool, poly, edge, texinfo,
										face, k, &ld, tex );
					vpool->add_vertex( v );
					poly->add_vertex( v );
				}
			}
		}

		data->recompute_polygon_normals();
		data->recompute_vertex_normals( 90.0 );
		data->remove_unused_vertices( true );
		data->remove_invalid_primitives( true );

		PT( EggTexture ) d_tex = new EggTexture( "diffuse_tex", tex != nullptr ? tex->get_filename() : Filename() );
		d_tex->set_uv_name( "diffuse" );
		poly->add_texture( d_tex );

		PT( PandaNode ) face_node = load_egg_data( data );

		NodePath face_np = _brushroot.attach_new_node( face_node );

		if ( has_lighting )
		{
			PT( TextureStage ) lm_stage = new TextureStage( "lightmap_stage" );
			lm_stage->set_texcoord_name( "lightmap" );
			lm_stage->set_mode( TextureStage::M_modulate );
			face_np.set_texture( lm_stage, lm_tex, 1 );
		}

                face_np.clear_model_nodes();
                face_np.flatten_strong();
		NodePath gnnp = face_np.find( "**/+GeomNode" );
		if ( !gnnp.is_empty() )
		{
			GeomNode *gn = DCAST( GeomNode, gnnp.node() );
			_face_geomnodes[facenum] = gn;
		}
		else
		{
			face_np.remove_node();
		}
			
	}

        bspfile_cat.info()
		<< "Finished making faces.\n";
}

void BSPLoader::group_models()
{
	// In BSP files, models are brushes that have been grouped together to be used as an entity.
	// We can group all of the face GeomNodes of the model to a root node.

	for ( int modelnum = 0; modelnum < g_nummodels; modelnum++ )
	{
		dmodel_t *model = &g_dmodels[modelnum];
		int firstface = model->firstface;
		int numfaces = model->numfaces;
		float *florigin = model->origin;
		float *flmins = model->mins;
		float *flmaxs = model->maxs;
		LPoint3 origin( florigin[0], florigin[1], florigin[2] );
		LPoint3 mins( flmins[0], flmins[1], flmins[2] );
		LPoint3 maxs( flmaxs[0], flmaxs[1], flmaxs[1] );
		cout << "model mins: " << mins << endl;
		cout << "model maxs: " << maxs << endl;
		cout << "model origin: " << origin << endl;
		
		
		stringstream name;
		name << "model-" << modelnum;
		NodePath modelroot = _brushroot.attach_new_node( name.str() );
		modelroot.set_pos( origin );
		for ( int facenum = firstface; facenum < firstface + numfaces; facenum++ )
		{
			GeomNode *gn = _face_geomnodes[facenum];
			if ( gn != nullptr )
			{
				NodePath( gn ).wrt_reparent_to( modelroot );
			}
		}
	}
}

LColor color_from_rgb_scalar( vec_t *color )
{
	double scalar = color[3];
	return LColor( color[0] * scalar / 255.0,
		       color[1] * scalar / 255.0,
		       color[2] * scalar / 255.0,
		       1.0 );
}

LColor color_from_value( const string &value, bool scale = true )
{
	double r, g, b, s;
	sscanf( value.c_str(), "%lf %lf %lf %lf", &r, &g, &b, &s );
	
	if ( scale )
	{
		r *= s / 255.0;
		g *= s / 255.0;
		b *= s / 255.0;
	}

	r /= 255.0;
	g /= 255.0;
	b /= 255.0;

	LColor col( r, g, b, 1.0 );
	cout << col << endl;

	return col;

	//return LColor( r, g, b, 1.0 );
}

void BSPLoader::load_entities()
{
	Loader *loader = Loader::get_global_ptr();

	for ( int entnum = 0; entnum < g_numentities; entnum++ )
	{
		entity_t *ent = &g_entities[entnum];

		string classname = ValueForKey( ent, "classname" );
		string id = ValueForKey( ent, "id" );

		vec_t origin[3];
		GetVectorForKey( ent, "origin", origin );

		vec_t angles[3];
		GetVectorForKey( ent, "angles", angles );

		string targetname = ValueForKey( ent, "targetname" );

		if ( classname == "light" )
		{
			// It's a pointlight!!!

			LColor color = color_from_value( ValueForKey( ent, "_light" ) );

			PT( PointLight ) light = new PointLight( "pointLight-" + targetname );
			light->set_color( color );
			NodePath light_np = _result.attach_new_node( light );
			light_np.set_pos( origin[0], origin[1], origin[2] );

			_render.set_light( light_np );

			_entities.push_back( light_np );
		}
		else if ( classname == "light_environment" )
		{
			// Directional light + ambient light
			LColor sun_color = color_from_value( ValueForKey( ent, "_light" ), false );
			LColor amb_color = color_from_value( ValueForKey( ent, "_diffuse_light" ) );

			NodePath group = _result.attach_new_node( "light_environment-group" );

			PT( DirectionalLight ) sun = new DirectionalLight( "directionalLight-" + targetname );
			sun->set_color( sun_color );
			NodePath sun_np = group.attach_new_node( sun );
			sun_np.set_pos( origin[0], origin[1], origin[2] );
			sun_np.set_hpr( angles[1], angles[0], angles[2] );

			PT( AmbientLight ) amb = new AmbientLight( "ambientLight-" + targetname );
			amb->set_color( amb_color );
			NodePath amb_np = group.attach_new_node( amb );

			_render.set_light( amb_np );
			_render.set_light( sun_np );
			
			_entities.push_back( group );
		}
		else if ( classname == "prop_static" )
		{
			// A static prop
			string mdl_path = ValueForKey( ent, "modelpath" );
			vec_t scale[3];
			GetVectorForKey( ent, "scale", scale );
			int phys_type = IntForKey( ent, "physics" );

			NodePath prop_np = _result.attach_new_node( loader->load_sync( Filename( mdl_path ) ) );
			if ( prop_np.is_empty() )
			{
				bspfile_cat.warning()
					<< "Could not load static prop " << mdl_path << "!\n";
			}
			else
			{
				bspfile_cat.info()
					<< "Loaded static prop " << mdl_path << "...\n";
				prop_np.set_pos( origin[0], origin[1], origin[2] );
				prop_np.set_hpr( angles[1], angles[0], angles[2] );
				prop_np.set_scale( scale[0] / MAP_SCALE, scale[1] / MAP_SCALE, scale[2] / MAP_SCALE );
				_entities.push_back( prop_np );
			}
		}
		else if ( classname == "env_fog" )
		{
			// Fog
			PN_stdfloat density = FloatForKey( ent, "fogdensity" );
			LColor fog_color = color_from_value( ValueForKey( ent, "fogcolor" ) );
			PT( Fog ) fog = new Fog( "env_fog" );
			fog->set_exp_density( density );
			fog->set_color( fog_color );
			NodePath fognp = _render.attach_new_node( fog );
			_render.set_fog( fog );
			_entities.push_back( fognp );
		}
		else if ( classname == "func_door" )
		{
			// A sliding door
			string model = ValueForKey( ent, "model" );
			if ( model[0] == '*' )
			{
				// Brush model
				int modelnum = atoi( model.substr( 1 ).c_str() );
				NodePath modelroot = get_model( modelnum );
				cout << "For func_door: " << endl;
				modelroot.ls();
			}
		}
	}
}

bool BSPLoader::is_cluster_visible( int curr_cluster, int cluster ) const
{
	if ( ( curr_cluster < 0 || cluster < 0 ) || curr_cluster == cluster )
	{
		return true;
	}
	else if ( !_has_pvs_data )
	{
		return false;
	}

	// 1 means that the specified leaf is visible from the current leaf
	// 0 means it's not
	return ( ( _leaf_pvs[curr_cluster][cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) != 0 );
}

void BSPLoader::update()
{
        // Update visibility

	bitset<MAX_MAP_FACES> already_visible;

	int curr_leaf_idx = find_leaf( _camera.get_pos( _result ) );

        for ( int i = g_numleafs - 1; i >= 0; i-- )
        {
                const dleaf_t *leaf = &g_dleafs[i];
                if ( !is_cluster_visible( curr_leaf_idx, i ) )
                {
                        continue;
                }

                int face_count = leaf->nummarksurfaces;
		while( face_count-- )
		{
			int face = g_dmarksurfaces[leaf->firstmarksurface + face_count];
			if ( already_visible.test( face ) )
			{
				continue;
			}
			already_visible.set( face );
			GeomNode *gn = _face_geomnodes[face];
			if ( gn != nullptr )
			{
				bspfile_cat.info()
					<< "Rendering geomNode " << gn << "\n";
				gn->bsp_flag_for_render();
			}
				
		}
        }
}

AsyncTask::DoneStatus BSPLoader::update_task( GenericAsyncTask *task, void *data )
{
        BSPLoader *self = (BSPLoader *)data;
	if ( self->_want_visibility )
	{
		self->update();
	}

        return AsyncTask::DS_cont;
}

bool BSPLoader::read( const Filename &file )
{
	cleanup();

	if ( _gsg == nullptr )
	{
		bspfile_cat.error()
			<< "Cannot load BSP file: no GraphicsStateGuardian was specified\n";
		return false;
	}
	if ( _camera.is_empty() && _want_visibility )
	{
		bspfile_cat.error()
			<< "Cannot load BSP file: visibility requested but no Camera NodePath specified\n";
		return false;
	}
	if ( _render.is_empty() )
	{
		bspfile_cat.error()
			<< "Cannot load BSP file: no render NodePath specified\n";
		return false;
	}


	dtexdata_init();

	_result = NodePath( new RigidBodyCombiner( "maproot" ) );
	_brushroot = _result.attach_new_node( "brushroot" );
	_brushroot.set_light_off( 1 );
	_brushroot.set_material_off( 1 );
	_brushroot.set_shader_off( 1 );
	_has_pvs_data = false;

	//_entities.reserve( MAX_MAP_ENTITIES );
	//_face_geoms.resize( MAX_MAP_FACES );
	_face_geomnodes.resize( MAX_MAP_FACES );
	_leaf_pvs.resize( MAX_MAP_LEAFS );
	for ( size_t i = 0; i < _leaf_pvs.size(); i++ )
	{
		_leaf_pvs[i].resize( ( MAX_MAP_LEAFS + 7 ) / 8 );
	}

	bspfile_cat.info()
		<< "Reading " << file.get_fullpath() << "...\n";
	nassertr( file.exists(), false );

	VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();

	string data;
	nassertr( vfs->read_file( file, data, true ), false );
	int length = data.length();
	char *buffer = new char[length + 1];
	memcpy( buffer, data.c_str(), length );
	LoadBSPImage( (dheader_t *)buffer );

	ParseEntities();

	// Decompress the per leaf visibility data.
	for ( int i = 0; i < g_numleafs; i++ )
	{
		dleaf_t *leaf = &g_dleafs[i];
		if ( leaf->visofs != -1 )
		{
			DecompressVis( &g_dvisdata[leaf->visofs], _leaf_pvs[i].data(), _leaf_pvs[i].size() );
			_has_pvs_data = true;
		}
	}

	make_faces();
	group_models();
	load_entities();

	_update_task = new GenericAsyncTask( file.get_basename_wo_extension() + "-updateTask", update_task, this );
	AsyncTaskManager::get_global_ptr()->add( _update_task );

	_result.clear_model_nodes();
	_result.flatten_medium();
	_result.set_scale( MAP_SCALE );

	DCAST( RigidBodyCombiner, _result.node() )->collect();

	return true;
}

void BSPLoader::cleanup()
{
	if (!_result.is_empty())
		_result.remove_node();

	if ( g_dlightdata != nullptr )
		dtexdata_free();

	_leaf_pvs.clear();
	_face_geomnodes.clear();

	if ( _update_task != nullptr )
	{
		_update_task->remove();
		_update_task = nullptr;
	}

	_has_pvs_data = false;

	for ( size_t i = 0; i < _entities.size(); i++ )
	{
		_entities[i].remove_node();
	}
	_entities.clear();
}

BSPLoader::BSPLoader() :
	_update_task( nullptr ),
	_gsg( nullptr ),
	_has_pvs_data( false ),
	_want_visibility( true ),
	_physics_type( PT_panda )
{
	set_gamma( DEFAULT_GAMMA, DEFAULT_OVERBRIGHT );
}

void BSPLoader::set_camera( const NodePath &camera )
{
	_camera = camera;
}

void BSPLoader::set_render( const NodePath &render )
{
	_render = render;
}

void BSPLoader::set_want_visibility( bool flag )
{
	_want_visibility = flag;
}

void BSPLoader::set_gamma( PN_stdfloat gamma, int overbright )
{
	build_gamma_table( gamma, overbright );
}

void BSPLoader::set_gsg( GraphicsStateGuardian *gsg )
{
	_gsg = gsg;
}

NodePath BSPLoader::get_result() const
{
	return _result;
}

int BSPLoader::get_num_entities() const
{
	return g_numentities;
}

string BSPLoader::get_entity_value( int entnum, const char *key ) const
{
	entity_t *ent = &g_entities[entnum];
	return ValueForKey( ent, key );
}

int BSPLoader::get_entity_value_int( int entnum, const char *key ) const
{
	entity_t *ent = &g_entities[entnum];
	return IntForKey( ent, key );
}

float BSPLoader::get_entity_value_float( int entnum, const char *key ) const
{
	entity_t *ent = &g_entities[entnum];
	return FloatForKey( ent, key );
}

LVector3 BSPLoader::get_entity_value_vector( int entnum, const char *key ) const
{
	entity_t *ent = &g_entities[entnum];

	vec3_t vec;
	GetVectorForKey( ent, key, vec );

	return LVector3( vec[0], vec[1], vec[2] );
}

LColor BSPLoader::get_entity_value_color( int entnum, const char *key, bool scale ) const
{
	entity_t *ent = &g_entities[entnum];

	return color_from_value( ValueForKey( ent, key ), scale );
}

NodePath BSPLoader::get_entity( int entnum ) const
{
	return _entities[entnum];
}

NodePath BSPLoader::get_model( int modelnum ) const
{
	stringstream search;
	search << "**/model-" << modelnum;
	return _brushroot.find( search.str() );
}

void BSPLoader::set_physics_type( int type )
{
	_physics_type = type;
}