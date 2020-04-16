/**
 * PANDA3D BSP LIBRARY
 *
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file anim_activities.h
 * @author Brian Lach
 * @date April 15, 2020
 */

#pragma once

#include "config_game_shared.h"

#include "referenceCount.h"
#include "animControl.h"
#include "animControlCollection.h"
#include "simpleHashMap.h"

class CActor;

enum
{
        ACTANIM_PLAY,
        ACTANIM_LOOP,
        ACTANIM_PINGPONG,
        ACTANIM_POSE,
};

class CActAnim : public ReferenceCount
{
public:
        struct AnimDef_t
        {
                int type;

                std::string name;

                pvector<AnimControl *> controls;

                bool restart;

                float play_rate;

                int start_frame;
                int end_frame;

                bool snap;
                float fade_in;
                float fade_out;
        };

        bool subpart;
        std::string subpart_name;

        // Should we blend between the two anims based
        // on movement speed?
        bool blend_by_movement_speed;

        // Using a collection to support blended anims
        SimpleHashMap<std::string, AnimDef_t, string_hash> anims;
};

class CActActor : public ReferenceCount
{
public:
        std::string name;

        CActor *actor;
        pvector<PT( CActAnim )> anims;
};

class CActivity : public ReferenceCount
{
public:
        std::string name;

        SimpleHashMap<std::string, PT( CActActor ), string_hash> actors;
};

typedef SimpleHashMap<std::string, PT( CActivity ), string_hash> ActivityMap_t;

bool ACT_load_from_script( const Filename &filename, CActor *actor, ActivityMap_t &output );
