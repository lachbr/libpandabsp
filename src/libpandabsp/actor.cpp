#include "actor.h"

#include "loader.h"
#include "loaderOptions.h"
#include "character.h"
#include "modelRoot.h"
#include "modelNode.h"
#include "animBundleNode.h"
#include "animBundle.h"

NotifyCategoryDeclNoExport( actor )
NotifyCategoryDef( actor, "" )

static LoaderOptions model_loader_options
( LoaderOptions::LF_search | LoaderOptions::LF_report_errors | LoaderOptions::LF_convert_skeleton );

static LoaderOptions anim_loader_options
( LoaderOptions::LF_search | LoaderOptions::LF_report_errors | LoaderOptions::LF_convert_anim );

CActor::CActor( const std::string &name, bool is_root, bool is_final )
{
	_is_root = is_root;
	_is_final = is_final;
	_subparts_complete = false;
	if ( is_root )
	{
		_actor_root = NodePath( new ModelRoot( name ) );
		if ( is_final )
		{
			_actor_root.node()->set_final( true );
		}
	}
		
}

CActor::~CActor()
{
}

void CActor::load_model( const Filename &filename )
{
	Loader *loader = Loader::get_global_ptr();

	PT( PandaNode ) mdl_node = loader->load_sync( filename, model_loader_options );
	nassertv( mdl_node != nullptr );

	NodePath mdl( mdl_node );

	NodePath char_np = mdl.find( "**/+Character" );
	if ( char_np.is_empty() )
	{
		actor_cat.error()
			<< filename.get_fullpath() << " does not define a valid Character!\n";
		return;
	}

	if ( _is_root )
		char_np.reparent_to( _actor_root );
	else
		_actor_root = char_np;

	Character *char_node = DCAST( Character, char_np.node() );

	_part_bundle_np = char_np;
	_part_bundle_handle = char_node->get_bundle_handle( 0 );
}

void CActor::load_anims( const CKeyValues *anim_table )
{
	Loader *loader = Loader::get_global_ptr();

	size_t count = anim_table->get_num_keys();
	for ( size_t ianim = 0; ianim < count; ianim++ )
	{
		const std::string &anim_name = anim_table->get_key( ianim );
		const Filename &anim_file = anim_table->get_value( ianim );

		NodePath anim_np = NodePath( loader->load_sync( anim_file, anim_loader_options ) );
		if ( anim_np.is_empty() )
		{
			continue;
		}

		NodePath bundle_np = anim_np.find( "**/+AnimBundleNode" );
		if ( bundle_np.is_empty() )
		{
			actor_cat.error()
				<< anim_file.get_fullpath() << " does not define a valid AnimBundleNode!\n";
			continue;
		}

		AnimBundleNode *bundle_node = DCAST( AnimBundleNode, bundle_np.node() );
		AnimBundle *bundle = bundle_node->get_bundle();

		PT( AnimControl ) control = _part_bundle_handle->get_bundle()->bind_anim( bundle, -1 );
		if ( !control )
		{
			actor_cat.error()
				<< "Couldn't bind anim `" << anim_file.get_fullpath() << "!\n";
			continue;
		}

		control->set_name( anim_name );

		CAnimDef anim;
		anim.bundle = bundle;
		anim.control = control;

		_anims[anim_name] = anim;
	}
}

CSubpartDef *CActor::make_subpart( const std::string &subpart_name,
			   const vector_string &include_joints, const vector_string &exclude_joints )
{
	PT( CSubpartDef ) subpart = new CSubpartDef;

	size_t include_count = include_joints.size();
	for ( size_t i = 0; i < include_count; i++ )
	{
		subpart->subset.add_include_joint( GlobPattern( include_joints[i] ) );
	}

	size_t exclude_count = exclude_joints.size();
	for ( size_t i = 0; i < exclude_count; i++ )
	{
		subpart->subset.add_exclude_joint( GlobPattern( exclude_joints[i] ) );
	}

	// Bind all of our anims to the subpart.
	size_t anim_count = _anims.size();
	for ( size_t i = 0; i < anim_count; i++ )
	{
		const CAnimDef &anim = _anims.get_data( i );

		PT( AnimControl ) control = _part_bundle_handle->get_bundle()
			->bind_anim( anim.bundle, -1, subpart->subset );
		if ( !control )
		{
			actor_cat.error()
				<< "Could bind animation to subpart\n";
			continue;
		}

		CAnimDef sub_anim;
		sub_anim.control = control;
		sub_anim.bundle = anim.bundle;
		subpart->anims[_anims.get_key( i )] = sub_anim;
	}

	_subparts[subpart_name] = subpart;

	return subpart;
}

NodePath CActor::control_joint( const std::string &joint_name )
{
	Character *char_node = DCAST( Character, _part_bundle_np.node() );

	CharacterJoint *joint = char_node->find_joint( joint_name );
	if ( joint && joint->is_of_type( MovingPartMatrix::get_class_type() ) )
	{
		NodePath joint_np = _part_bundle_np.attach_new_node( new ModelNode( joint_name ) );
		joint_np.set_mat( joint->get_default_value() );
		_part_bundle_handle->get_bundle()->control_joint( joint_name, joint_np.node() );

		return joint_np;
	}

	return NodePath();
}

NodePath CActor::expose_joint( const std::string &joint_name )
{
	Character *char_node = DCAST( Character, _part_bundle_np.node() );

	CharacterJoint *joint = char_node->find_joint( joint_name );
	if ( joint && joint->is_of_type( MovingPartMatrix::get_class_type() ) )
	{
		NodePath joint_np = _part_bundle_np.attach_new_node( new ModelNode( joint_name ) );
		joint->add_net_transform( joint_np.node() );
		return joint_np;
	}

	return NodePath();
}

void CActor::release_joint( const std::string &joint_name )
{
	_part_bundle_handle->get_bundle()->release_joint( joint_name );
}
