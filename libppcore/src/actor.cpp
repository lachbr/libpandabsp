#include <auto_bind.h>
#include <character.h>

#include "actor.h"

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
                        const string &part,
                        int hierarchy_match_flags )
{

        const string lod_name = "lodRoot";

        // Setup the geom node
        //PT(PandaNode) root = new PandaNode("actor");
        //NodePath::operator=(NodePath(root));
        //set_geom_node(NodePath(*this));

        // load the actor model and parent it to the geom node
        NodePath model_np = g_base->load_model( model_file );
        model_np.reparent_to( get_geom_node() );

        NodePath bundle_np;

        if ( model_np.node()->is_of_type( Character::get_class_type() ) )
        {
                bundle_np = model_np;
        }
        else
        {
                bundle_np = model_np.find( "**/+Character" );
        }

        PartBundleHandle *bhandle;
        BundleDict bundle_dict;
        AnimControlCollection control;

        if ( bundle_np.is_empty() )
        {
                cout << model_file << " is not a Character!" << endl;
                return;
        }
        else
        {
                bundle_np.reparent_to( _geom_np );

                bhandle = prepare_bundle( &bundle_np, part, lod_name );

                if ( _part_bundle_dict.find( lod_name ) != _part_bundle_dict.end() )
                {
                        bundle_dict = _part_bundle_dict[lod_name];
                }

                bundle_np.node()->set_name( "__Actor_" + part );
        }

        // If there are no animations to bind to the actor, we are done here.
        if ( anim_map == nullptr )
        {
                return;
        }

        // load anims from the actor model first
        load_anims( anim_map, model_file, part, &bundle_np, &control, hierarchy_match_flags );

        NodePathVec anim_np_vec;
        anim_np_vec.reserve( anim_map->size() );

        // then for each animations specified by the user
        for ( AnimMap::const_iterator anim_map_itr = anim_map->begin(); anim_map_itr != anim_map->end(); ++anim_map_itr )
        {
                const string &filename = ( *anim_map_itr ).first;
                if ( filename != model_file )
                {
                        // load the animation as a child of the actor
                        NodePath anim_np = g_base->load_model( filename );
                        anim_np.reparent_to( bundle_np );

                        // load anims from that file
                        load_anims( anim_map, filename, part, &bundle_np, &control, hierarchy_match_flags );

                        // detach the node so that it will not appear in a new call to auto_bind()
                        anim_np.detach_node();
                        // keep it on the side
                        anim_np_vec.push_back( anim_np );
                }
        }

        // re-attach the animation nodes to the actor
        for ( NodePathVec::iterator npItr = anim_np_vec.begin(); npItr < anim_np_vec.end(); ++npItr )
        {
                ( *npItr ).reparent_to( bundle_np );
        }

        PartDef partdef = { bundle_np, bhandle, model_np.node(), control };
        bundle_dict[part] = partdef;
        _part_bundle_dict[lod_name] = bundle_dict;

}

PartBundleHandle *Actor::prepare_bundle( NodePath *bundle_np,
                                         string part_name, const string &lod_name )
{

        if ( !_got_name )
        {
                node()->set_name( bundle_np->node()->get_name() );
                _got_name = true;
        }

        Character *node = (Character *)bundle_np->node();
        nassertr( node->get_num_bundles() == 1, (PartBundleHandle *)nullptr );
        PartBundleHandle *bundleHandle = node->get_bundle_handle( 0 );

        if ( true )
        {
                if ( _common_bundle_handles.find( part_name ) == _common_bundle_handles.end() )
                {
                        // We haven't already got a bundle for this part; store it.
                        _common_bundle_handles[part_name] = bundleHandle;
                }
                else
                {
                        // We've already got a bundle for this part; merge it.
                        PartBundleHandle *loaded_bundle_handle = _common_bundle_handles[part_name];

                        node->merge_bundles( bundleHandle, loaded_bundle_handle );
                        bundleHandle = loaded_bundle_handle;
                }
        }


        _part_names.push_back( part_name );

        return bundleHandle;
}

void Actor::set_geom_node( NodePath &node )
{
        _geom_np = node;
}

NodePath &Actor::get_geom_node()
{
        return _geom_np;
}

void Actor::load_anims( const AnimMap *anim_map,
                        const string &filename,
                        string part_name,
                        NodePath *bundle_np,
                        AnimControlCollection *control,
                        int hierarchy_match_flags )
{
        // precondition
        if ( anim_map == nullptr )
        {
                nout << "ERROR: parameter anim_map cannot be nullptr." << endl;
                return;
        }

        // use auto_bind() to gather the anims
        AnimControlCollection temp_collection;
        auto_bind( bundle_np->node(), temp_collection, hierarchy_match_flags );

        // get the anim names for the current file
        AnimMap::const_iterator animMapItr = anim_map->find( filename );
        if ( animMapItr != anim_map->end() )
        {
                // first, test the anim names
                for ( NameVec::const_iterator nameItr = ( *animMapItr ).second.begin(); nameItr != ( *animMapItr ).second.end(); ++nameItr )
                {
                        // make sure this name is not currently stored by the actor
                        if ( control->find_anim( *nameItr ) == nullptr )
                        {
                                // check if auto_bind() found an animation with the right name
                                PT( AnimControl ) animPtr = temp_collection.find_anim( *nameItr );
                                if ( animPtr != nullptr )
                                {
                                        control->store_anim( animPtr, *nameItr );
                                        temp_collection.unbind_anim( *nameItr );
                                }
                        }
                }

                // deal the remaining anims
                int animIdx = 0;
                for ( NameVec::const_iterator nameItr = ( *animMapItr ).second.begin(); nameItr != ( *animMapItr ).second.end(); ++nameItr )
                {
                        // make sure this name is not currently stored by the actor
                        if ( control->find_anim( *nameItr ) == nullptr )
                        {
                                // make sure there is at least one anim left to store
                                PT( AnimControl ) animPtr = temp_collection.get_anim( animIdx );
                                if ( animPtr != nullptr )
                                {
                                        control->store_anim( animPtr, *nameItr );
                                        ++animIdx;
                                }
                        }
                }
        }
}

bool Actor::is_stored( const AnimControl *animPtr, const AnimControlCollection *control )
{
        for ( int i = 0; i < control->get_num_anims(); ++i )
        {
                PT( AnimControl ) storedAnimPtr = control->get_anim( i );

                if ( animPtr->get_anim() == storedAnimPtr->get_anim() &&
                     animPtr->get_part() == storedAnimPtr->get_part() )
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
NodePath Actor::control_joint( const string &jointName )
{
        bool foundJoint = false;
        NodePath npJoint = attach_new_node( jointName );
        NodePath characterNP = find( "**/+Character" );
        PT( Character ) characterPtr = DCAST( Character, characterNP.node() );
        if ( characterPtr != nullptr )
        {
                PT( CharacterJoint ) characterJointPtr = characterPtr->find_joint( jointName );
                if ( characterJointPtr != nullptr )
                {
                        for ( int i = 0; !foundJoint && i < characterPtr->get_num_bundles(); ++i )
                        {
                                if ( characterPtr->get_bundle( i )->control_joint( jointName, npJoint.node() ) )
                                {
                                        foundJoint = true;
                                        npJoint.set_mat( characterJointPtr->get_default_value() );
                                }
                        }
                }
        }
        if ( !foundJoint )
        {
                nout << "ERROR: cannot control joint `" << jointName << "'." << endl;
                npJoint.remove_node();
        }
        return npJoint;
}

// This function exposes a joint via its NodePath handle.
// Think of it as a read from that joint interface.
//
// Return value:
// The joint's NodePath, parented to the actor.
//
// Parameter:
//    jointName: the joint's name as found in the model file.
NodePath Actor::expose_joint( const string &jointName )
{
        bool foundJoint = false;
        NodePath npJoint = attach_new_node( jointName );
        NodePath npCharacter = find( "**/+Character" );
        PT( Character ) characterPtr = DCAST( Character, npCharacter.node() );
        if ( characterPtr != nullptr )
        {
                PT( CharacterJoint ) characterJointPtr = characterPtr->find_joint( jointName );
                if ( characterJointPtr != nullptr )
                {
                        foundJoint = true;
                        characterJointPtr->add_net_transform( npJoint.node() );
                }
        }
        if ( !foundJoint )
        {
                nout << "ERROR: no joint named `" << jointName << "'." << endl;
                npJoint.remove_node();
        }
        return npJoint;
}

void Actor::play( const string &animName )
{
        // Play this anim on all parts from start to finish.

        for ( PartBundleMap::const_iterator lodItr = _part_bundle_dict.begin(); lodItr != _part_bundle_dict.end(); ++lodItr )
        {
                BundleDict bundleDict = _part_bundle_dict[lodItr->first];
                for ( BundleDict::iterator bundleItr = bundleDict.begin(); bundleItr != bundleDict.end(); ++bundleItr )
                {
                        ( (PartDef)bundleItr->second ).control.play( animName );
                }
        }
}

void Actor::play( const string &animName, int fromFrame, int toFrame )
{
        for ( PartBundleMap::const_iterator lodItr = _part_bundle_dict.begin(); lodItr != _part_bundle_dict.end(); ++lodItr )
        {
                BundleDict bundleDict = _part_bundle_dict[lodItr->first];
                for ( BundleDict::iterator bundleItr = bundleDict.begin(); bundleItr != bundleDict.end(); ++bundleItr )
                {
                        ( (PartDef)bundleItr->second ).control.play( animName, fromFrame, toFrame );
                }
        }
}

void Actor::play( const string &animName, const string &partName )
{
        if ( _part_bundle_dict["lodRoot"].find( partName ) != _part_bundle_dict["lodRoot"].end() )
        {
                _part_bundle_dict["lodRoot"][partName].control.play( animName );
        }
        else if ( _subpart_dict.find( partName ) != _subpart_dict.end() )
        {
                _subpart_dict[partName].control.play( animName );
        }
        else
        {
                cout << "Actor ERROR: Could not find part/subpart with _name " << partName << endl;
        }

}

void Actor::play( const string &animName, const string &partName, int fromFrame, int toFrame )
{
        if ( _part_bundle_dict["lodRoot"].find( partName ) != _part_bundle_dict["lodRoot"].end() )
        {
                _part_bundle_dict["lodRoot"][partName].control.play( animName, fromFrame, toFrame );
        }
        else if ( _subpart_dict.find( partName ) != _subpart_dict.end() )
        {
                _subpart_dict[partName].control.play( animName, fromFrame, toFrame );
        }
        else
        {
                cout << "Actor ERROR: Could not find part/subpart with _name " << partName << endl;
        }
}

void Actor::loop( const string &animName )
{
        // Loop this anim on all parts from start to finish.

        for ( PartBundleMap::const_iterator lodItr = _part_bundle_dict.begin(); lodItr != _part_bundle_dict.end(); ++lodItr )
        {
                BundleDict bundleDict = _part_bundle_dict[lodItr->first];
                for ( BundleDict::iterator bundleItr = bundleDict.begin(); bundleItr != bundleDict.end(); ++bundleItr )
                {
                        ( (PartDef)bundleItr->second ).control.loop( animName, true );
                }
        }
}

void Actor::loop( const string &animName, int fromFrame, int toFrame )
{
        for ( PartBundleMap::const_iterator lodItr = _part_bundle_dict.begin(); lodItr != _part_bundle_dict.end(); ++lodItr )
        {
                BundleDict bundleDict = _part_bundle_dict[lodItr->first];
                for ( BundleDict::iterator bundleItr = bundleDict.begin(); bundleItr != bundleDict.end(); ++bundleItr )
                {
                        ( (PartDef)bundleItr->second ).control.loop( animName, true, fromFrame, toFrame );
                }
        }
}

void Actor::loop( const string &animName, const string &partName )
{
        if ( _part_bundle_dict["lodRoot"].find( partName ) != _part_bundle_dict["lodRoot"].end() )
        {
                _part_bundle_dict["lodRoot"][partName].control.loop( animName, true );
        }
        else if ( _subpart_dict.find( partName ) != _subpart_dict.end() )
        {
                _subpart_dict[partName].control.loop( animName, true );
        }
        else
        {
                cout << "Actor ERROR: Could not find part/subpart with _name " << partName << endl;
        }
}

void Actor::loop( const string &animName, const string &partName, int fromFrame, int toFrame )
{
        if ( _part_bundle_dict["lodRoot"].find( partName ) != _part_bundle_dict["lodRoot"].end() )
        {
                _part_bundle_dict["lodRoot"][partName].control.loop( animName, true, fromFrame, toFrame );
        }
        else if ( _subpart_dict.find( partName ) != _subpart_dict.end() )
        {
                _subpart_dict[partName].control.loop( animName, true, fromFrame, toFrame );
        }
        else
        {
                cout << "Actor ERROR: Could not find part/subpart with _name " << partName << endl;
        }
}

void Actor::stop()
{
        for ( PartBundleMap::const_iterator lodItr = _part_bundle_dict.begin(); lodItr != _part_bundle_dict.end(); ++lodItr )
        {
                BundleDict bundleDict = _part_bundle_dict[lodItr->first];
                for ( BundleDict::iterator bundleItr = bundleDict.begin(); bundleItr != bundleDict.end(); ++bundleItr )
                {
                        AnimControlCollection control = ( (PartDef)bundleItr->second ).control;
                        control.stop( control.which_anim_playing() );
                }
        }
}

void Actor::stop( const string &partName )
{
        if ( _part_bundle_dict["lodRoot"].find( partName ) != _part_bundle_dict["lodRoot"].end() )
        {
                AnimControlCollection *pACC = &_part_bundle_dict["lodRoot"][partName].control;
                _part_bundle_dict["lodRoot"][partName].control.stop( pACC->which_anim_playing() );
        }
        else if ( _subpart_dict.find( partName ) != _subpart_dict.end() )
        {
                AnimControlCollection *pACC = &_subpart_dict[partName].control;
                _subpart_dict[partName].control.stop( pACC->which_anim_playing() );
        }
        else
        {
                cout << "Actor ERROR: Could not find part/subpart with _name " << partName << endl;
        }
}

NodePath Actor::get_part( string partName, string lod )
{
        if ( _part_bundle_dict.find( lod ) != _part_bundle_dict.end() )
        {

                // We have a bundle dictionary in this LOD.
                BundleDict bundledict = _part_bundle_dict[lod];

                if ( bundledict.find( partName ) != bundledict.end() )
                {

                        // We have a PartDef tied to this partName in bundledict.
                        PartDef part = bundledict[partName];
                        return part.bundle_np;
                }
        }

        return NodePath::not_found();
}

void Actor::make_subpart( const string &strPart, Actor::NameVec &includeJoints, Actor::NameVec &excludeJoints,
                          const string &strParent, bool bOverlapping )
{
        SubpartDef def;

        if ( _subpart_dict.find( strParent ) != _subpart_dict.end() )
        {
                def = _subpart_dict[strParent];
        }

        AnimControlCollection acc;

        PartSubset subset( def.subset );
        for ( size_t i = 0; i < includeJoints.size(); i++ )
        {
                subset.add_include_joint( GlobPattern( includeJoints[i] ) );
        }
        for ( size_t i = 0; i < excludeJoints.size(); i++ )
        {
                subset.add_exclude_joint( GlobPattern( excludeJoints[i] ) );
        }

        def.true_part_name = strParent;
        def.subset = subset;


        // Now we have to bind this subpart to all the animations of the parent part.
        if ( _part_bundle_dict["lodRoot"].find( strParent ) != _part_bundle_dict["lodRoot"].end() )
        {
                PartDef *pdef = &_part_bundle_dict["lodRoot"][strParent];
                AnimControlCollection *pACC = &pdef->control;
                PartBundle *pBundle = pdef->part_bundle_handle->get_bundle();

                int nAnims = pACC->get_num_anims();
                for ( int i = 0; i < nAnims; i++ )
                {
                        PT( AnimControl ) pAnimC = pACC->get_anim( i );
                        string anim_name = pACC->get_anim_name( i );
                        PT( AnimControl ) pNewAnim = pBundle->bind_anim( pAnimC->get_anim(), -1, subset );
                        acc.store_anim( pNewAnim, anim_name );
                }
        }

        def.control = acc;

        _subpart_dict[strPart] = def;
}

void Actor::set_subparts_complete( bool bFlag )
{
        _subparts_complete = bFlag;
}

bool Actor::get_subparts_complete() const
{
        return _subparts_complete;
}

void Actor::verify_subparts_complete( const string &strPart, const string &strLOD )
{
        NameVec subJoints;
        for ( SubpartDict::iterator itr = _subpart_dict.begin(); itr != _subpart_dict.end(); ++itr )
        {
                string strSubpart = itr->first;
                SubpartDef def = itr->second;
                if ( strSubpart != strPart && def.true_part_name == strPart )
                {
                        vector<PartGroup *> joints;
                }
        }
}

vector<PartGroup *> Actor::get_joints( const string &strPart, const string &strJoint, const string &strLOD )
{
        vector<PartGroup *> joints;

        GlobPattern pattern( strJoint );

        map<string, PartDef> partBundleDict = _part_bundle_dict[strLOD];

        SubpartDef *sub = nullptr;
        PartSubset *subset = nullptr;
        PartDef *part = nullptr;
        if ( _subpart_dict.find( strPart ) == _subpart_dict.end() )
        {
                part = &partBundleDict[strPart];
        }
        else
        {
                sub = &_subpart_dict[strPart];
                subset = &sub->subset;
                part = &partBundleDict[sub->true_part_name];
        }

        if ( part == nullptr )
        {
                cout << "No part named " << strPart << "!" << endl;
                return joints;
        }

        PartBundle *pBundle = part->part_bundle_handle->get_bundle();

        if ( pattern.has_glob_characters() && subset == nullptr )
        {
                PartGroup *pJoint = pBundle->find_child( strJoint );
                if ( pJoint != nullptr )
                {
                        joints.push_back( pJoint );
                }
        }
        else
        {
                bool bIncluded = true;
                if ( subset != nullptr )
                {
                        bIncluded = subset->is_include_empty();
                }
                get_part_joints( joints, pattern, pBundle, subset, bIncluded );
        }

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

Actor::ControlMap Actor::get_anim_controls( const char *szAnim, const char *szPart,
                                            const char *szLOD, bool bAllowAsyncBind )
{

        vector<AnimControlCollection *> controls;
        if ( szPart == nullptr && _subparts_complete )
        {

        }



        if ( szLOD == nullptr )
        {
                szLOD = "lodRoot";
        }

        //m_AnimCon

        return controls;
}

string Actor::get_current_anim( const string &strPart )
{
        if ( _part_bundle_dict["lodRoot"].find( strPart ) != _part_bundle_dict["lodRoot"].end() )
        {
                return _part_bundle_dict["lodRoot"][strPart].control.which_anim_playing();
        }
        else if ( _subpart_dict.find( strPart ) != _subpart_dict.end() )
        {
                return _subpart_dict[strPart].control.which_anim_playing();
        }

        return "PART_NOT_FOUND";
}

void Actor::set_play_rate( float flRate, const string &strAnim, const char *szPart )
{
        if ( szPart != nullptr )
        {
                if ( _part_bundle_dict["lodRoot"].find( szPart ) != _part_bundle_dict["lodRoot"].end() )
                {
                        AnimControlCollection *pACC = &_part_bundle_dict["lodRoot"][szPart].control;
                        int nAnims = pACC->get_num_anims();
                        for ( int i = 0; i < nAnims; i++ )
                        {
                                PT( AnimControl ) pAnim = pACC->get_anim( i );
                                if ( pAnim->get_name().compare( strAnim ) == 0 )
                                {
                                        pAnim->set_play_rate( flRate );
                                }
                        }
                }
                else if ( _subpart_dict.find( szPart ) != _subpart_dict.end() )
                {
                        AnimControlCollection *pACC = &_subpart_dict[szPart].control;
                        int nAnims = pACC->get_num_anims();
                        for ( int i = 0; i < nAnims; i++ )
                        {
                                PT( AnimControl ) pAnim = pACC->get_anim( i );
                                if ( pAnim->get_name().compare( strAnim ) == 0 )
                                {
                                        pAnim->set_play_rate( flRate );
                                }
                        }
                }
        }
        else
        {
                cout << "Actor ERROR: no partname specified, not changing playrate" << endl;
        }
}