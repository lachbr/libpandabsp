#ifndef AUDIO3DMANAGER_H
#define AUDIO3DMANAGER_H

#include "config_pandaplus.h"

#include <audioManager.h>
#include <nodePath.h>
#include <lvecBase3.h>
#include <filename.h>
#include <genericAsyncTask.h>

NotifyCategoryDeclNoExport( audio3DManager );

class EXPCL_PANDAPLUS Audio3DManager
{
public:

        struct VelocityData
        {
                LVector3f velocity;
                bool auto_vel;

                VelocityData( const LVector3f &vel, bool av )
                {
                        velocity = vel;
                        auto_vel = av;
                }

                VelocityData()
                {
                        velocity = LVector3f( 0 );
                        auto_vel = false;
                }
        };

        typedef pvector<PT( AudioSound )> SoundVec;
        typedef pmap<NodePath, SoundVec> SoundMap;
        typedef pmap<PT( AudioSound ), VelocityData> VelMap;

        Audio3DManager();
        Audio3DManager( PT( AudioManager ) audio_mgr, const NodePath &listener_target = NodePath(),
                        const NodePath &root = NodePath(), int task_priority = 51 );

        void start_update();
        void stop_update();

        PT( AudioSound ) load_sfx( const Filename &name );

        void set_distance_factor( PN_stdfloat factor );
        PN_stdfloat get_distance_factor() const;

        void set_doppler_factor( PN_stdfloat factor );
        PN_stdfloat get_doppler_factor() const;

        void set_drop_off_factor( PN_stdfloat factor );
        PN_stdfloat get_drop_off_factor() const;

        void set_sound_min_distance( PT( AudioSound ) sound, PN_stdfloat dist );
        PN_stdfloat get_sound_min_distance( PT( AudioSound ) sound ) const;

        void set_sound_max_distance( PT( AudioSound ) sound, PN_stdfloat dist );
        PN_stdfloat get_sound_max_distance( PT( AudioSound ) sound ) const;

        void set_sound_velocity( PT( AudioSound ) sound, const LVector3f &velocity );
        LVector3f get_sound_velocity( PT( AudioSound ) sound );

        void set_sound_velocity_auto( PT( AudioSound ) sound );

        void set_listener_velocity( const LVector3f &velocity );
        LVector3f get_listener_velocity() const;

        void set_listener_velocity_auto();

        void attach_sound_to_object( PT( AudioSound ) sound, const NodePath &object );
        void detach_sound( PT( AudioSound ) sound );

        SoundVec get_sounds_on_object( const NodePath &object );

        void attach_listener( const NodePath &object );
        void detach_listener();

        void disable();

private:
        static AsyncTask::DoneStatus update_task( GenericAsyncTask *task, void *data );

        void remove_existing_sound( AudioSound *sound );

private:
        PT( GenericAsyncTask ) _update_task;
        PT( AudioManager ) _audio_manager;
        NodePath _listener_target;
        NodePath _root;
        int _task_priority;

        VelocityData _listener_vel;

        SoundMap _sounds;
        VelMap _velocities;
};

#endif // AUDIO3DMANAGER_H