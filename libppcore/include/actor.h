#ifndef ACTOR_H
#define ACTOR_H

#include "config_pandaplus.h"

#include <nodePath.h>
#include <modelNode.h>
#include <partBundleHandle.h>
#include <genericAsyncTask.h>
#include <partSubset.h>
#include <animControlCollection.h>

NotifyCategoryDeclNoExport( actor );

struct SubpartDef
{
        string part_name;
        PartSubset subset;

        AnimControlCollection control;
};

/**
 * This class is used to represent an "animated node".
 * It provides a high level interface for playing animations on animated characters.
 */
class EXPCL_PANDAPLUS Actor : public NodePath
{
public:
        typedef vector<string> NameVec;
        typedef map<string, NameVec> AnimMap;
        typedef vector<AnimControlCollection *> ControlMap;

public:
        Actor();
        virtual ~Actor();

        void load_actor( const string &model_file,
                         const AnimMap *anim_map,
                         int hierarchy_match_flags = 0 );

        void set_geom_node( const NodePath &geom_np );
        NodePath get_geom_node() const;

        NodePath control_joint( const string &joint_name );
        NodePath expose_joint( const string &joint_name );

        void make_subpart( const string &part, NameVec &include_joints, NameVec &exclude_joints = NameVec(),
                           bool overlapping = false );

        void set_subparts_complete( bool flag );
        bool get_subparts_complete() const;

        vector<PartGroup *> get_joints( const string &joint = "*" );
        vector<PartGroup *> get_joints( const string &subpart_name, const string &joint = "*" );

        void play( const string &anim );
        void play( const string &anim, const string &subpart_name );
        void play( const string &anim, const string &subpart_name, int from_frame, int to_frame );
        void play( const string &anim, int from_frame, int to_frame );

        void loop( const string &anim );
        void loop( const string &anim, const string &subpart_name );
        void loop( const string &anim, const string &subpart_name, int from_frame, int to_frame );
        void loop( const string &anim, int from_frame, int to_frame );

        void stop( const string &subpart_name );
        void stop();

        string get_current_anim() const;
        string get_current_anim( const string &subpart = "" );
        PN_stdfloat get_duration( const string &anim_name, int from_frame, int to_frame ) const;
        PN_stdfloat get_duration( const string &anim_name ) const;
        int get_num_frames( const string &anim_name ) const;
        PN_stdfloat get_frame_time( const string &anim_name, int frame ) const;

        void set_play_rate( PN_stdfloat rate, const string &anim, const char *part = nullptr );

private:
        void get_part_joints( vector<PartGroup *> &joints, GlobPattern &pattern, PartGroup *pPartNode, PartSubset *sub, bool bIncluded );

        void setup_geom_node();

        PartBundleHandle *prepare_bundle( const NodePath &bundle_np );

        void load_anims( const AnimMap *anim_map,
                         const string &filename,
                         const NodePath &bundle_np,
                         int hierarchy_match_flags = 0 );

        bool is_stored( const AnimControl *anim, const AnimControlCollection *control );

private:
        typedef vector<NodePath> NodePathVec;
        typedef map<string, SubpartDef> SubpartDict;

        NodePath _geom_np;
        bool _first_time;
        bool _got_name;
        bool _subparts_complete;
        SubpartDict _subpart_dict;
        NodePath _character_np;
        PartBundleHandle *_part_bundle_handle;
        PandaNode *_part_model;
        AnimControlCollection _control;
};

struct ActorDef
{
        string model;
        Actor::AnimMap animations;
};

#endif // ACTOR_H
