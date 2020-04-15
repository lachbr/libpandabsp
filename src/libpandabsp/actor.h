#pragma once

#include "config_bsp.h"
#include "referenceCount.h"
#include "nodePath.h"
#include "partBundleHandle.h"
#include "pandaNode.h"
#include "partBundle.h"
#include "simpleHashMap.h"
#include "animControl.h"
#include "keyvalues.h"

class CAnimDef
{
public:
        CAnimDef();

        PT( AnimControl ) control;
        PT( AnimBundle ) bundle;
};

inline CAnimDef::CAnimDef()
{
        control = nullptr;
        bundle = nullptr;
}

class CSubpartDef : public ReferenceCount
{
public:
        CSubpartDef();

        SimpleHashMap<std::string, CAnimDef, string_hash> anims;
        CAnimDef *current_anim;
        PartSubset subset;
};

inline CSubpartDef::CSubpartDef()
{
        current_anim = nullptr;
}

/*
 * An useful interface for dealing with animated characters.
 */
class EXPCL_PANDABSP CActor : public ReferenceCount
{
public:
        CActor( const std::string &name = "actor", bool is_root = true,
                bool is_final = false );
        ~CActor();

        void load_model( const Filename &model_file );
        void load_anims( const CKeyValues *anims );

        CSubpartDef *make_subpart( const std::string &subpart_name,
                           const vector_string &include_joints,
                           const vector_string &exclude_joints = vector_string() );

        NodePath get_part_np() const;
        const CSubpartDef *get_subpart( const std::string &part_name ) const;

        AnimControl *get_anim( const std::string &name ) const;
        AnimControl *get_anim( const std::string &name, const std::string &subpart_name ) const;

        NodePath expose_joint( const std::string &joint_name );
        NodePath control_joint( const std::string &joint_name );
        void release_joint( const std::string &joint_name );

        void set_subparts_complete( bool complete );

        NodePath root() const;
        
private:
        bool _is_root;
        bool _is_final;
        bool _subparts_complete;
        NodePath _actor_root;
        NodePath _part_bundle_np;
        PT( PartBundleHandle ) _part_bundle_handle;
        SimpleHashMap<std::string, CAnimDef, string_hash> _anims;
        SimpleHashMap<std::string, PT( CSubpartDef ), string_hash> _subparts;
};

inline AnimControl *CActor::get_anim( const std::string &name ) const
{
        int ianim = _anims.find( name );
        if ( ianim == -1 )
        {
                return nullptr;
        }

        return _anims.get_data( ianim ).control;
}

inline AnimControl *CActor::get_anim( const std::string &name, const std::string &subpart_name ) const
{
        int ipart = _subparts.find( subpart_name );
        if ( ipart == -1 )
        {
                return nullptr;
        }

        const CSubpartDef *part = _subparts.get_data( ipart );
        int ianim = part->anims.find( name );
        if ( ianim == -1 )
        {
                return nullptr;
        }

        return part->anims.get_data( ianim ).control;
}

inline void CActor::set_subparts_complete( bool complete )
{
        _subparts_complete = complete;
}

inline NodePath CActor::root() const
{
        return _actor_root;
}

inline NodePath CActor::get_part_np() const
{
        return _part_bundle_np;
}

inline const CSubpartDef *CActor::get_subpart( const std::string &part_name ) const
{
        int ipart = _subparts.find( part_name );
        if ( ipart == -1 )
        {
                return nullptr;
        }

        return _subparts.get_data( ipart );
}
