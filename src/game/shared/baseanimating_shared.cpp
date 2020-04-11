#include "baseanimating_shared.h"
#include <virtualFileSystem.h>
#include <loader.h>

#include "vifparser.h"

#include <filename.h>
#include <auto_bind.h>
#include <character.h>
#include <nodePathCollection.h>
#include <animControlCollection.h>
#include <configVariableBool.h>

#include "scenecomponent_shared.h"

BaseAnimating::BaseAnimating() :
	_playing_anim( nullptr ),
	_hbundle( nullptr ),
	_last_anim( nullptr ),
	_doing_blend( false ),
	_scene( nullptr )
{
}

bool BaseAnimating::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	_entity->get_component( _scene );

	animation = "";
	model_path = "";
	model_origin = LVector3f( 0 );
	model_angles = LVector3f( 0 );
	model_scale = LVector3f( 1 );

	return true;
}

void BaseAnimating::reset()
{
	_anims.clear();
	_last_anim = nullptr;
	_playing_anim = nullptr;
	_doing_blend = false;
	_hbundle = nullptr;
	if ( !_model_np.is_empty() )
	{
		_model_np.remove_node();
	}
}

void BaseAnimating::update( double frametime )
{
	if ( _playing_anim && _hbundle )
	{
		update_blending();

#if !defined( CLIENT_DLL )
		// Advance the animation frame and joint positions if we have to.
		_hbundle->get_bundle()->update();

		int curr_frame = _playing_anim->anim_control->get_frame();
		if ( curr_frame != _playing_anim->last_frame )
		{
			// Check if there is an anim event associated with this frame.
			for ( AnimEvent_t ae : _playing_anim->anim_events )
			{
				if ( ae.event_frame == curr_frame )
				{
					handle_anim_event( &ae );
				}
			}
			_playing_anim->last_frame = curr_frame;
		}
#endif
	}
}

void BaseAnimating::set_model( const std::string &path )
{
	reset();

	model_path = path;

	VirtualFileSystem *vfs = VirtualFileSystem::get_global_ptr();
	Loader *loader = Loader::get_global_ptr();

	Filename modelfile( path );
	if ( modelfile.get_extension() == "act" )
	{
		// This is an actor definition file, which maps a model to a list of
		// animation names and their correspoding animation files.
		std::string adef_data = vfs->read_file( modelfile, true );
		TokenVec toks = tokenizer( adef_data );
		Parser p( toks );
		Object actor = p.get_base_objects_with_name( "actor" )[0];
		pvector<Object> animlist = p.get_objects_with_name( actor, "anim" );
		std::string model_path = p.get_property_value( actor, "model" );

		// Load the mesh.
		_model_np = NodePath( loader->load_sync( model_path ) );
		nassertv( !_model_np.is_empty() );

		// Make sure this model is set up to be animated.
		NodePath char_np = _model_np.find( "**/+Character" );
		if ( char_np.is_empty() )
		{
			nassert_raise( modelfile + " does not define a valid actor!" );
			return;
		}

		Character *char_node = DCAST( Character, char_np.node() );
		if ( char_node->get_num_bundles() != 1 )
		{
			nassert_raise( modelfile + " has an invalid number of bundles!" );
			return;
		}

		_hbundle = char_node->get_bundle_handle( 0 );
		_hbundle->get_bundle()->set_frame_blend_flag( ConfigVariableBool( "interpolate-frames", false ) );

		//
		// Walk through each animation, load it up, and bind it to the character.
		//

		NodePathCollection anim_nodes;

		for ( Object animdef : animlist )
		{
			std::string aname = p.get_property_value( animdef, "name" );
			std::string apath = p.get_property_value( animdef, "file" );

			pvector<Object> events = p.get_objects_with_name( animdef, "event" );

			NodePath anim_np( loader->load_sync( apath ) );
			if ( anim_np.is_empty() )
			{
				nassert_raise( "Could not load animation " + apath + " from actor def " + path );
				return;
			}
			anim_np.reparent_to( char_np );

			AnimControlCollection temp_acc;
			auto_bind( char_np.node(), temp_acc );

			AnimControl *ac = temp_acc.get_anim( 0 );
			ac->set_name( aname );

			bool loop = false;
			if ( p.has_property( animdef, "loop" ) )
			{
				loop = (bool)atoi( p.get_property_value( animdef, "loop" ).c_str() );
			}
			float play_rate = 1.0f;
			if ( p.has_property( animdef, "playrate" ) )
			{
				play_rate = atof( p.get_property_value( animdef, "playrate" ).c_str() );
			}
			int start_frame = 0;
			if ( p.has_property( animdef, "startframe" ) )
			{
				start_frame = atoi( p.get_property_value( animdef, "startframe" ).c_str() );
			}
			int end_frame = ac->get_num_frames();
			if ( p.has_property( animdef, "endframe" ) )
			{
				end_frame = atoi( p.get_property_value( animdef, "endframe" ).c_str() );
			}
			bool snap = false;
			if ( p.has_property( animdef, "snap" ) )
			{
				snap = (bool)atoi( p.get_property_value( animdef, "snap" ).c_str() );
			}
			float fade_in = 0.2f;
			if ( p.has_property( animdef, "fadein" ) )
			{
				fade_in = atof( p.get_property_value( animdef, "fadein" ).c_str() );
			}
			float fade_out = 0.2f;
			if ( p.has_property( animdef, "fadeout" ) )
			{
				fade_out = atof( p.get_property_value( animdef, "fadeout" ).c_str() );
			}

			AnimInfo_t ainfo;
			ainfo.name = aname;
			ainfo.anim_control = ac;
			ainfo.loop = loop;
			ainfo.play_rate = play_rate;
			ainfo.start_frame = start_frame;
			ainfo.end_frame = end_frame;
			ainfo.last_frame = -INT32_MAX;
			ainfo.snap = snap;
			ainfo.fade_in = fade_in;
			ainfo.fade_out = fade_out;

			for ( Object animevent : events )
			{
				AnimEvent_t ae;
				ae.event_name = p.get_property_value( animevent, "name" );
				ae.event_frame = atoi( p.get_property_value( animevent, "frame" ).c_str() );
				ainfo.anim_events.push_back( ae );
			}

			_anims[aname] = ainfo;

			temp_acc.clear_anims();

			anim_np.detach_node();
			anim_nodes.add_path( anim_np );
		}

		for ( int i = 0; i < anim_nodes.get_num_paths(); i++ )
		{
			anim_nodes.get_path( i ).reparent_to( char_np );
		}

	}
	else if ( !modelfile.empty() )
	{
		// Non-animated model
		_model_np = NodePath( loader->load_sync( path ) );
		nassertv( !_model_np.is_empty() );
	}
	else
	{
		_model_np = NodePath( "emptyModel" );
	}

	if ( _scene )
		_model_np.reparent_to( _scene->np );
}

void BaseAnimating::set_model_origin( const LPoint3 &origin )
{
	nassertv( !_model_np.is_empty() );
	_model_np.set_pos( origin );
	model_origin = origin;
}

void BaseAnimating::set_model_angles( const LVector3 &angles )
{
	nassertv( !_model_np.is_empty() );
	_model_np.set_hpr( angles );
	model_angles = angles;
}

void BaseAnimating::set_model_scale( const LVector3 &scale )
{
	nassertv( !_model_np.is_empty() );
	_model_np.set_scale( scale );
	model_scale = scale;
}

void BaseAnimating::reset_blends()
{
	for ( auto itr : _anims )
	{
		_hbundle->get_bundle()->set_control_effect( itr.second.anim_control, 0.0f );
	}
}

void BaseAnimating::update_blending()
{
	if ( _doing_blend && _hbundle )
	{
		PartBundle *bundle = _hbundle->get_bundle();

		ClockObject *global_clock = ClockObject::get_global_clock();
		double now = global_clock->get_frame_time();
		double delta = now - _playing_anim->play_time;
		if ( delta < _playing_anim->fade_in )
		{
			float ratio = delta / _playing_anim->fade_in;
			bundle->set_control_effect( _playing_anim->anim_control, ratio );
			bundle->set_control_effect( _last_anim->anim_control, 1 - ratio );
		}
		else
		{
			bundle->set_control_effect( _last_anim->anim_control, 0.0f );
			_last_anim->anim_control->stop();

			bundle->set_control_effect( _playing_anim->anim_control, 1.0f );
			bundle->set_anim_blend_flag( false );
			_doing_blend = false;
		}
	}
}

void BaseAnimating::set_animation( const std::string &anim )
{
	animation = anim;

	if ( _anims.size() == 0 )
		return;

	auto itr = _anims.find( animation );
	if ( itr == _anims.end() )
	{
		//nassert_raise( "Animation " + animation.get() + " not found!" );
		return;
	}

	AnimInfo_t &info = itr->second;
	info.anim_control->set_play_rate( info.play_rate );

	ClockObject *global_clock = ClockObject::get_global_clock();
	info.play_time = global_clock->get_frame_time();
	
	_last_anim = _playing_anim;
	_playing_anim = &info;

	if ( _playing_anim && !_playing_anim->snap && _playing_anim->fade_in > 0 && _last_anim && _playing_anim != _last_anim )
	{
		_hbundle->get_bundle()->set_anim_blend_flag( true );
		_doing_blend = true;
	}
	else
	{
		if ( _last_anim )
			_last_anim->anim_control->stop();
		_hbundle->get_bundle()->set_anim_blend_flag( false );
		_doing_blend = false;
	}

	if ( info.loop )
		info.anim_control->loop( true, info.start_frame, info.end_frame );
	else
		info.anim_control->play( info.start_frame, info.end_frame );
}

#if !defined( CLIENT_DLL )

void CBaseAnimating::handle_anim_event( AnimEvent_t *ae )
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseAnimating )
	SendPropString( PROPINFO( model_path ) ),
	SendPropString( PROPINFO( animation ) ),
	SendPropVec3( PROPINFO( model_origin ) ),
	SendPropVec3( PROPINFO( model_angles ) ),
	SendPropVec3( PROPINFO( model_scale ) ),
END_SEND_TABLE()

#else

void RecvProxy_ModelPath( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_String( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model( self->model_path );
	if ( self->_scene )
		self->_model_np.reparent_to( self->_scene->np );
}

void RecvProxy_ModelOrigin( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model_origin( self->model_origin );
}

void RecvProxy_ModelAngles( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model_angles( self->model_angles );
}

void RecvProxy_ModelScale( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_model_scale( self->model_scale );
}

void RecvProxy_Animation( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_String( prop, object, out, dgi );

	C_BaseAnimating *self = (C_BaseAnimating *)object;
	self->set_animation( self->animation );
}

IMPLEMENT_CLIENTCLASS_RT_NOBASE( C_BaseAnimating, CBaseAnimating )
	RecvPropString( PROPINFO( model_path ), RecvProxy_ModelPath ),
	RecvPropString( PROPINFO( animation ), RecvProxy_Animation ),
	RecvPropVec3( PROPINFO( model_origin ), RecvProxy_ModelOrigin ),
	RecvPropVec3( PROPINFO( model_angles ), RecvProxy_ModelAngles ),
	RecvPropVec3( PROPINFO( model_scale ), RecvProxy_ModelScale ),
END_RECV_TABLE()

#endif