#pragma once

#include "config_bsp.h"
#include "referenceCount.h"
#include "nodePath.h"
#include "partBundleHandle.h"
#include "pandaNode.h"
#include "partBundle.h"
#include "simpleHashMap.h"

// Maps animation names to animation files.
typedef std::map<std::string, std::string> DiskAnimTable_t;

struct AnimDef_t
{
        Filename filename;
        AnimBundle *anim_bundle;
        AnimControl *anim_control;
};

struct SubpartDef_t
{
        std::string part_name;
        PartSubset subset;
};

struct PartDef_t
{
        NodePath part_bundle_np;
        PartBundleHandle *part_bundle_handle;
        PandaNode *part_model;

        SimpleHashMap<std::string, SubpartDef_t, string_hash> subparts;

        SimpleHashMap<std::string, AnimDef_t, string_hash> anims;
        AnimDef_t *current_anim;

        PartBundle *bundle() const
        {
                return part_bundle_handle->get_bundle();
        }
};



/*
 * An useful interface for dealing with animated characters.
 */
class EXPCL_PANDABSP CActor : public ReferenceCount
{
public:
        CActor();
        ~CActor();

        void load_model( const Filename &model_file, int part = 0 );
};