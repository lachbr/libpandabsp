#include "actor.h"

#include <auto_bind.h>
#include <character.h>
#include <loader.h>

NotifyCategoryDef( actor, "" );

Actor::Actor() :
        NodePath( "actor" )
{

        _first_time = true;
        _subparts_complete = false;
        _got_name = false;

        setup_geom_node();
}

Actor::~Actor()
{
}

void Actor::setup_geom_node()
{
        if ( _first_time )
        {
                // Setup geom node stuff
                PT( ModelNode ) root = new ModelNode( "actor" );
                root->set_preserve_transform( ModelNode::PT_local );
                NodePath::operator=( NodePath( root ) );
                PT( ModelNode ) actor_geom = new ModelNode( "actorGeom" );
                set_geom_node( attach_new_node( actor_geom ) );
                _first_time = false;
        }
}


// This function loads an actor model and its animations, letting the user specify the name of each animation.
//
// Return value:
// The actor, parented to the framework's models node (to render the actor, reparent it to the render node)
//
// Parameters:
//    windowFrameworkPtr : the WindowFramework for this actor
//    actorFilename      : the actor's egg file
//    animMap            : a map of the desired name (first) of each animation and its associated file (second).
//                         Set this parameter to nullptr if there are no animations to bind to the actor.
//    hierarchyMatchFlags: idem as the same parameter from auto_bind().
void Actor::load_actor( const string &model_file,
                        const AnimMap *anim_map,
                        int hierarchy_match_flags )
{
        Loader *loader = Loader::get_global_ptr();

        // load the actor model and parent it to the geom node
        NodePath model_np = NodePath( loader->load_sync( model_file ) );
        if ( model_np.is_empty() )
        {
                actor_cat.error()
                        << "Could not load actor model!\n";
                return;
        }
        model_np.reparent_to( get_geom_node() );

        // Path to the Character node on this model.
        NodePath char_np;

        if ( model_np.node()->is_of_type( Character::get_class_type() ) )
        {
                char_np = model_np;
        }
        else
        {
                char_np = model_np.find( "**/+Character" );
        }

        PartBundleHandle *bhandle;

        if ( char_np.is_empty() )
        {
                actor_cat.error()
                        << model_file << " is not a Character!\n";
                return;
        }
        else
        {
                char_np.reparent_to( _geom_np );
                bhandle = prepare_bundle( char_np );
        }

        // If there are no animations to bind to the actor, we are done here.
        if ( anim_map == nullptr )
        {
                return;
        }

        // load anims from the actor model first
        load_anims( anim_map, model_file, char_np, hierarchy_match_flags );

        NodePathVec anim_np_vec;
        anim_np_vec.reserve( anim_map->size() );

        // then for each animations specified by the user
        for ( AnimMap::const_iterator anim_map_itr = anim_map->begin(); anim_map_itr != anim_map->end(); ++anim_map_itr )
        {
                const string &filename = ( *anim_map_itr ).first;
                if ( filename != model_file )
                {
                        // load the animation as a child of the actor
                        NodePath anim_np = NodePath( loader->load_sync( filename ) );
                        if ( anim_np.is_empty() )
                        {
                                actor_cat.error()
                                        << "Could not load actor animation '" << filename << "'\n";
                                continue;
                        }
                        anim_np.reparent_to( char_np );

                        // load anims from that file
                        load_anims( anim_map, filename, char_np, hierarchy_match_flags );

                        // detach the node so that it will not appear in a new call to auto_bind()
                        anim_np.detach_node();
                        // keep it on the side
                        anim_np_vec.push_back( anim_np );
                }
        }

        // re-attach the animation nodes to the actor
        for ( NodePathVec::iterator npItr = anim_np_vec.begin(); npItr < anim_np_vec.end(); ++npItr )
        {
                ( *npItr ).reparent_to( char_np );
        }

        _character_np = char_np;
        _part_bundle_handle = bhandle;
        _part_model = model_np.node();
}

PartBundleHandle *Actor::prepare_bundle( const NodePath &bundle_np )
{
        if ( !_got_name )
        {
                node()->set_name( bundle_np.node()->get_name() );
                _got_name = true;
        }

        Character *char_node = DCAST(Character, bundle_np.node());
        nassertr( char_node->get_num_bundles() == 1, nullptr );
        PartBundleHandle *bhandle = char_node->get_bundle_handle( 0 );

        return bhandle;
}

void Actor::set_geom_node( const NodePath &node )
{
        _geom_np = node;
}

NodePath Actor::get_geom_node() const
{
        return _geom_np;
}

void Actor::load_anims( const AnimMap *anim_map,
                        const string &filename,
                        const NodePath &bundle_np,
                        int hierarchy_match_flags )
{
        if ( anim_map == nullptr )
        {
                actor_cat.error()
                        << "load_anims(): anim_map is nullptr" << endl;
                return;
        }

        // use auto_bind() to gather the anims
        AnimControlCollection temp_collection;
        auto_bind( bundle_np.node(), temp_collection, hierarchy_match_flags );

        // get the anim names for the current file
        AnimMap::const_iterator anim_map_itr = anim_map->find( filename );
        if ( anim_map_itr != anim_map->end() )
        {
                // first, test the anim names
                for ( NameVec::const_iterator name_itr = ( *anim_map_itr ).second.begin();
                      name_itr != ( *anim_map_itr ).second.end(); ++name_itr )
                {
                        // make sure this name is not currently stored by the actor
                        if ( _control.find_anim( *name_itr ) == nullptr )
                        {
                                // check if auto_bind() found an animation with the right name
                                PT( AnimControl ) anim_ptr = temp_collection.find_anim( *name_itr );
                                if ( anim_ptr != nullptr )
                                {
                                        _control.store_anim( anim_ptr, *name_itr );
                                        temp_collection.unbind_anim( *name_itr );
                                }
                        }
                }

                // deal the remaining anims
                int animIdx = 0;
                for ( NameVec::const_iterator name_itr = ( *anim_map_itr ).second.begin();
                      name_itr != ( *anim_map_itr ).second.end(); ++name_itr )
                {
                        // make sure this name is not currently stored by the actor
                        if ( _control.find_anim( *name_itr ) == nullptr )
                        {
                                // make sure there is at least one anim left to store
                                PT( AnimControl ) animPtr = temp_collection.get_anim( animIdx );
                                if ( animPtr != nullptr )
                                {
                                        _control.store_anim( animPtr, *name_itr );
                                        ++animIdx;
                                }
                        }
                }
        }
}

bool Actor::is_stored( const AnimControl *anim, const AnimControlCollection *control )
{
        for ( int i = 0; i < control->get_num_anims(); ++i )
        {
                PT( AnimControl ) stored_anim = control->get_anim( i );

                if ( anim->get_anim() == stored_anim->get_anim() &&
                     anim->get_part() == stored_anim->get_part() )
                {
                        return true;
                }
        }
        return false;
}

// This function gives access to a joint that can be controlled via its NodePath handle.
// Think of it as a write to that joint interface.
//
// Return value:
// The joint's NodePath, parented to the actor.
//
// Parameter:
//    jointName: the joint's name as found in the model file.
NodePath Actor::control_joint( const string &joint_name )
{
        Character *char_node;
        DCAST_INTO_R( char_node, _character_np.node(), NodePath() );

        CharacterJoint *joint = char_node->find_joint( joint_name );
        nassertr( joint != nullptr, NodePath() );

        NodePath joint_np = attach_new_node( joint_name );
        for ( int i = 0; i < char_node->get_num_bundles(); ++i )
        {
                if ( char_node->get_bundle( i )->control_joint( joint_name, joint_np.node() ) )
                        joint_np.set_mat( joint->get_default_value() );
        }

        return joint_np;
}

// This function exposes a joint via its NodePath handle.
// Think of it as a read from that joint interface.
//
// Return value:
// The joint's NodePath, parented to the actor.
//
// Parameter:
//    jointName: the joint's name as found in the model file.
NodePath Actor::expose_joint( const string &joint_name )
{
        bool found_joint = false;
        NodePath joint_np = attach_new_node( joint_name );

        Character *char_node;
        DCAST_INTO_R( char_node, _character_np.node(), NodePath() );

        CharacterJoint *joint = char_node->find_joint( joint_name );
        nassertr( joint != nullptr, NodePath() );
        joint->add_net_transform( joint_np.node() );

        return joint_np;
}

void Actor::play( const string &anim_name )
{
        // Play this anim from start to finish.
        _control.play( anim_name );
}

void Actor::play( const string &anim_name, int from_frame, int to_frame )
{
        _control.play( anim_name, from_frame, to_frame );
}

void Actor::play( const string &anim_name, const string &subpart_name )
{
        nassertv( _subpart_dict.find( subpart_name ) != _subpart_dict.end() );
        _subpart_dict[subpart_name].control.play( anim_name );
}

void Actor::play( const string &anim_name, const string &subpart_name, int from_frame, int to_frame )
{
        nassertv( _subpart_dict.find( subpart_name ) != _subpart_dict.end() );
        _subpart_dict[subpart_name].control.play( anim_name, from_frame, to_frame );
}

void Actor::loop( const string &anim_name )
{
        // Loop this anim on all parts from start to finish.
        _control.loop( anim_name, true );
}

void Actor::loop( const string &anim_name, int from_frame, int to_frame )
{
        _control.loop( anim_name, true, from_frame, to_frame );
}

void Actor::loop( const string &anim_name, const string &subpart_name )
{
        nassertv( _subpart_dict.find( subpart_name ) != _subpart_dict.end() );
        _subpart_dict[subpart_name].control.loop( anim_name, true );
}

void Actor::loop( const string &anim_name, const string &subpart_name, int from_frame, int to_frame )
{
        nassertv( _subpart_dict.find( subpart_name ) != _subpart_dict.end() );
        _subpart_dict[subpart_name].control.loop( anim_name, true, from_frame, to_frame );
}

void Actor::stop()
{
        _control.stop( _control.which_anim_playing() );
}

void Actor::stop( const string &subpart_name )
{
        nassertv( _subpart_dict.find( subpart_name ) != _subpart_dict.end() );
        AnimControlCollection &control = _subpart_dict[subpart_name].control;
        control.stop( control.which_anim_playing() );
}

void Actor::make_subpart( const string &part_name, Actor::NameVec &include_joints,
                          Actor::NameVec &exclude_joints, bool overlapping )
{
        SubpartDef def;
        AnimControlCollection acc;

        PartSubset subset( def.subset );
        for ( size_t i = 0; i < include_joints.size(); i++ )
        {
                subset.add_include_joint( GlobPattern( include_joints[i] ) );
        }
        for ( size_t i = 0; i < exclude_joints.size(); i++ )
        {
                subset.add_exclude_joint( GlobPattern( exclude_joints[i] ) );
        }

        def.part_name = part_name;
        def.subset = subset;


        // Now we have to bind this subpart to all the animations of the parent part.
        PartBundle *bundle = _part_bundle_handle->get_bundle();

        int num_anims = _control.get_num_anims();
        for ( int i = 0; i < num_anims; i++ )
        {
                PT( AnimControl ) anim_c = _control.get_anim( i );
                string anim_name = _control.get_anim_name( i );
                PT( AnimControl ) new_anim = bundle->bind_anim( anim_c->get_anim(), -1, subset );
                acc.store_anim( new_anim, anim_name );
        }

        def.control = acc;

        _subpart_dict[part_name] = def;
}

void Actor::set_subparts_complete( bool bFlag )
{
        _subparts_complete = bFlag;
}

bool Actor::get_subparts_complete() const
{
        return _subparts_complete;
}

vector<PartGroup *> Actor::get_joints( const string &joint )
{
        vector<PartGroup *> joints;

        GlobPattern pattern( joint );
        PartBundle *bundle = _part_bundle_handle->get_bundle();

        if ( pattern.has_glob_characters() )
        {
                PartGroup *partgroup = bundle->find_child( joint );
                if ( partgroup != nullptr )
                {
                        joints.push_back( partgroup );
                }
        }

        return joints;
}

vector<PartGroup *> Actor::get_joints( const string &subpart_name, const string &joint )
{
        vector<PartGroup *> joints;

        GlobPattern pattern( joint );

        SubpartDef *sub = nullptr;
        PartSubset *subset = nullptr;

        sub = &_subpart_dict[subpart_name];
        subset = &sub->subset;
        /*
        subpart_name = &partBundleDict[sub->true_part_name];

        if ( subpart_name == nullptr )
        {
                cout << "No part named " << subpart_name << "!" << endl;
                return joints;
        }

        PartBundle *bundle = _part_bundle_handle->get_bundle();

        if ( pattern.has_glob_characters() && subset == nullptr )
        {
                PartGroup *joint = bundle->find_child( joint );
                if ( joint != nullptr )
                {
                        joints.push_back( joint );
                }
        }
        else
        {
                bool included = true;
                if ( subset != nullptr )
                {
                        included = subset->is_include_empty();
                }
                get_part_joints( joints, pattern, bundle, subset, included );
        }
        */

        return joints;
}

void Actor::get_part_joints( vector<PartGroup *> &joints, GlobPattern &pattern, PartGroup *pPartNode, PartSubset *sub,
                             bool bIncluded )
{
        string strName = pPartNode->get_name();
        if ( sub != nullptr )
        {
                if ( sub->matches_include( strName ) )
                {
                        bIncluded = true;
                }
                else if ( sub->matches_exclude( strName ) )
                {
                        bIncluded = false;
                }
        }

        if ( bIncluded && pattern.matches( strName ) && pPartNode->is_of_type( MovingPartBase::get_class_type() ) )
        {
                joints.push_back( pPartNode );
        }

        int nChildren = pPartNode->get_num_children();
        for ( int i = 0; i < nChildren; i++ )
        {
                PartGroup *pChild = pPartNode->get_child( i );
                get_part_joints( joints, pattern, pChild, sub, bIncluded );
        }
}

string Actor::get_current_anim() const
{
        return _control.which_anim_playing();
}

string Actor::get_current_anim( const string &subpart )
{
        nassertr( _subpart_dict.find( subpart ) != _subpart_dict.end(), "" );

        return _subpart_dict[subpart].control.which_anim_playing();
}

void Actor::set_play_rate( PN_stdfloat rate, const string &anim_name, const char *subpart )
{
        AnimControlCollection *acc;
        if ( subpart != nullptr )
        {
                // If a subpart was specified, use the AnimControlCollection associated with the subpart.
                nassertv( _subpart_dict.find( subpart ) != _subpart_dict.end() );
                acc = &_subpart_dict[subpart].control;
        }
        else
        {
                // No subpart was specified, use the actor wide AnimControlCollection
                acc = &_control;
        }

        int num_anims = acc->get_num_anims();
        for ( int i = 0; i < num_anims; i++ )
        {
                PT( AnimControl ) anim = acc->get_anim( i );
                if ( anim->get_name() == anim_name )
                {
                        anim->set_play_rate( rate );
                }
        }
}