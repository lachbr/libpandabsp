#ifndef ACTOR_H
#define ACTOR_H

#include "pp_globals.h"

#include <modelNode.h>
#include <partBundleHandle.h>
#include <genericAsyncTask.h>
#include <partSubset.h>

struct PartDef
{
        NodePath bundle_np;
        PartBundleHandle *part_bundle_handle;
        PandaNode *part_model;

        AnimControlCollection control;

};

struct SubpartDef
{
        string true_part_name;
        PartSubset subset;

        AnimControlCollection control;
};

class EXPCL_PANDAPLUS Actor : public NodePath
{
public:

        typedef vector<string> NameVec;
        typedef map<string, NameVec> AnimMap;

        Actor();
        virtual ~Actor();

        void load_actor( const string &model_file,
                         const AnimMap *anim_map,
                         const string &part_name,
                         int hierarchy_match_flags );

        void set_geom_node( NodePath &geom_np );
        NodePath &get_geom_node();
        NodePath control_joint( const string &joint_name );
        NodePath expose_joint( const string &joint_name );

        void make_subpart( const string &part, NameVec &include_joints, NameVec &exclude_joints = NameVec(),
                           const string &parent = "modelRoot", bool overlapping = false );

        void set_subparts_complete( bool flag );
        bool get_subparts_complete() const;

        void verify_subparts_complete( const string &part, const string &lod );

        vector<PartGroup *> get_joints( const string &part, const string &joint = "*", const string &lod = "lodRoot" );

        NodePath get_part( string part_name, string lod = "lodRoot" );

        void play( const string &anim );
        void play( const string &anim, const string &part );
        void play( const string &anim, const string &part, int from_frame, int to_frame );
        void play( const string &anim, int from_frame, int to_frame );

        void loop( const string &anim );
        void loop( const string &anim, const string &part );
        void loop( const string &anim, const string &part, int from_frame, int to_frame );
        void loop( const string &anim, int from_frame, int to_frame );

        void stop( const string &part );
        void stop();

        typedef vector<AnimControlCollection *> ControlMap;

        string get_current_anim( const string &part );

        void set_play_rate( float rate, const string &anim, const char *part = NULL );

        ControlMap get_anim_controls( const char *anim = NULL, const char *part = NULL,
                                      const char *lod = NULL, bool allow_async_bind = true );

private:
        void get_part_joints( vector<PartGroup *> &joints, GlobPattern &pattern, PartGroup *pPartNode, PartSubset *sub, bool bIncluded );

        void setup_geom_node();

        NodePath _geom_np;

        bool _first_time;

        typedef vector<NodePath> NodePathVec;

        PartBundleHandle *prepare_bundle( NodePath *, string, const string & );

        void load_anims( const AnimMap *anim_map,
                         const string &filename,
                         string part_name,
                         NodePath *bundle_np,
                         AnimControlCollection *control,
                         int hierarchy_match_flags );
        bool is_stored( const AnimControl *anim, const AnimControlCollection *control );

        bool _got_name;

        bool _subparts_complete;

        typedef map<string, PartDef> BundleDict;
        typedef map<string, BundleDict> PartBundleMap;



        PartBundleMap _part_bundle_dict;

        NameVec _part_names;

        map<string, PartBundleHandle *> _common_bundle_handles;

        typedef map<string, SubpartDef> SubpartDict;

        SubpartDict _subpart_dict;
};

#endif // ACTOR_H
