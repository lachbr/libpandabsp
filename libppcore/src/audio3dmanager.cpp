#include "audio3dmanager.h"
#include "pp_globals.h"

NotifyCategoryDef( audio3DManager, "" );

Audio3DManager::Audio3DManager() :
        _audio_manager( nullptr ),
        _listener_target( NodePath() ),
        _root( NodePath() ),
        _update_task( nullptr ),
        _task_priority( 0 ),
        _listener_vel( VelocityData( LVector3f( 0 ), true ) )
{
}

Audio3DManager::Audio3DManager( PT( AudioManager ) audio_mgr, const NodePath &listener_target,
                                const NodePath &root, int task_priority ) :
        _audio_manager( audio_mgr ),
        _listener_target( listener_target ),
        _root( root ),
        _update_task( nullptr ),
        _task_priority( task_priority ),
        _listener_vel( VelocityData( LVector3f( 0 ), true ) )
{
        if ( _root.is_empty() )
        {
                _root = g_render;
        }
}

/**
* Begins the task which updates the 3D positions of each sound.
*/
void Audio3DManager::start_update()
{
        // Stop the previous update task, if it exists.
        stop_update();

        _update_task = new GenericAsyncTask( "Audio3DManager-update_task", update_task, this );
        _update_task->set_priority( _task_priority );
        g_task_mgr->add( _update_task );
}

/**
* Ends the task which updates the 3D positions of each sound.
*/
void Audio3DManager::stop_update()
{
        if ( _update_task != (GenericAsyncTask *)nullptr )
        {
                if ( g_task_mgr->has_task( _update_task ) )
                {
                        g_task_mgr->remove( _update_task );
                }
                _update_task = nullptr;
        }
}

/**
* Use Audio3DManager::load_sfx to load a sound with 3D positioning enabled.
*/
PT( AudioSound ) Audio3DManager::load_sfx( const Filename &name )
{
        PT( AudioSound ) s = _audio_manager->get_sound( name, 1 );
        set_sound_velocity_auto( s );
        return s;
}

/**
* Control the scale that sets the distance units for 3D spacialized audio.
* Default is 1.0 which is adjusted in panda to be feet.
* When you change this, don't forget that this effects the scale of set_sound_min_distance.
*/
void Audio3DManager::set_distance_factor( PN_stdfloat factor )
{
        _audio_manager->audio_3d_set_distance_factor( factor );
}

PN_stdfloat Audio3DManager::get_distance_factor() const
{
        return _audio_manager->audio_3d_get_distance_factor();
}

/**
* Control the presence of the Doppler effect. Default is 1.0.
* For exaggerated Doppler, use > 1.0.
* For diminished Doppler, use < 1.0.
*/
void Audio3DManager::set_doppler_factor( PN_stdfloat factor )
{
        _audio_manager->audio_3d_set_doppler_factor( factor );
}

PN_stdfloat Audio3DManager::get_doppler_factor() const
{
        return _audio_manager->audio_3d_get_doppler_factor();
}

/**
* Exaggerate or diminish the effect of distance on sound. Default is 1.0.
* Valid range is 0.0 to 10.0.
* For faster drop off, use > 1.0.
* For slower drop off, use < 1.0.
*/
void Audio3DManager::set_drop_off_factor( PN_stdfloat factor )
{
        _audio_manager->audio_3d_set_drop_off_factor( factor );
}

PN_stdfloat Audio3DManager::get_drop_off_factor() const
{
        return _audio_manager->audio_3d_get_drop_off_factor();
}

/**
* Controls the distance (in units) that this sound begins to fall off.
* Also affects the rate it falls off.
* Default is 3.28 (in feet, this is 1 meter).
* Don't forget to change this when you change the distance factor.
*/
void Audio3DManager::set_sound_min_distance( PT( AudioSound ) sound, PN_stdfloat dist )
{
        sound->set_3d_min_distance( dist );
}

PN_stdfloat Audio3DManager::get_sound_min_distance( PT( AudioSound ) sound ) const
{
        return sound->get_3d_min_distance();
}

/**
* Controls the distance (in units) that this sound stops falling off.
* The sound does not stop at this point, it just doesn't get any quieter.
* You should rarely need to adjust this.
* Default is 1000000000.0.
*/
void Audio3DManager::set_sound_max_distance( PT( AudioSound ) sound, PN_stdfloat dist )
{
        sound->set_3d_max_distance( dist );
}

PN_stdfloat Audio3DManager::get_sound_max_distance( PT( AudioSound ) sound ) const
{
        return sound->get_3d_max_distance();
}

/**
* Sets the velocity vector (in units/sec) of the sound, for calculating doppler shift.
* This is relative to the sound root (probably render).
* Default: LVector3f(0)
*/
void Audio3DManager::set_sound_velocity( PT( AudioSound ) sound, const LVector3f &velocity )
{
        _velocities[sound] = VelocityData( velocity, false );
}

LVector3f Audio3DManager::get_sound_velocity( PT( AudioSound ) sound )
{
        if ( _velocities.find( sound ) != _velocities.end() )
        {
                VelocityData v_dat = _velocities[sound];

                if ( !v_dat.auto_vel )
                {
                        // Velocity has been explicity set.
                        return v_dat.velocity;

                }
                else
                {
                        // Auto-velocity case.
                        for ( SoundMap::iterator itr = _sounds.begin(); itr != _sounds.end(); ++itr )
                        {
                                NodePath np = itr->first;
                                // Make sure the sound is attached to this nodepath.
                                if ( std::find( itr->second.begin(), itr->second.end(), sound ) != itr->second.end() )
                                {
                                        // Return the velocity.
                                        return np.get_pos_delta( _root ) / g_global_clock->get_dt();
                                }
                        }

                }
        }

        return LVector3f( 0 );
}

/**
* If the velocity is set to auto, the velocity will be determined by the
* previous position of the object the sound is attached to and the frame dt.
* Make sure if you use this function that you remember to clear the previous
* transformation between frames.
*/
void Audio3DManager::set_sound_velocity_auto( PT( AudioSound ) sound )
{
        _velocities[sound] = VelocityData( LVector3f( 0 ), true );
}

/**
* Set the velocity vector (in units/sec) of the listener, for calculating doppler shift.
* This is relative to the sound root (probably render).
* Default: LVector3f(0)
*/
void Audio3DManager::set_listener_velocity( const LVector3f &vel )
{
        _listener_vel = VelocityData( vel, false );
}

/**
* If velocity is set to auto, the velocity will be determined by the
* previous position of the object the listener is attached to and the frame dt.
* Make sure if you use this method that you remember to clear the previous
* transformation between frames.
*/
void Audio3DManager::set_listener_velocity_auto()
{
        _listener_vel = VelocityData( LVector3f( 0 ), true );
}

LVector3f Audio3DManager::get_listener_velocity() const
{

        if ( !_listener_vel.auto_vel )
        {
                return _listener_vel.velocity;

        }
        else if ( !_listener_target.is_empty() )
        {
                return _listener_target.get_pos_delta( _root ) / g_global_clock->get_dt();

        }

        return LVector3f( 0 );
}

void Audio3DManager::remove_existing_sound( AudioSound *sound )
{
        for ( SoundMap::iterator sm_itr = _sounds.begin(); sm_itr != _sounds.end(); ++sm_itr )
        {
                SoundVec *sounds = &sm_itr->second;
                SoundVec::iterator sound_itr = std::find( sounds->begin(), sounds->end(), sound );
                if ( sound_itr != sounds->end() )
                {
                        // This sound is already attached to something.
                        audio3DManager_cat.debug()
                                << "Sound " << sound << " is already attached to something. Removing it.\n";
                        sounds->erase( sound_itr );

                        if ( sounds->size() == 0 )
                        {
                                audio3DManager_cat.debug()
                                        << "NodePath " << sm_itr->first << " has no sounds attached. Removing it.\n";
                                _sounds.erase( sm_itr );

                                audio3DManager_cat.debug()
                                        << "Restarting remove_existing_sound\n";
                                // We have to restart.
                                remove_existing_sound( sound );
                                return;
                        }
                }
        }
}

void Audio3DManager::attach_sound_to_object( PT( AudioSound ) sound, const NodePath &object )
{
        remove_existing_sound( sound );

        if ( _sounds.find( object ) == _sounds.end() )
        {
                audio3DManager_cat.debug()
                        << "Creating new NodePath entry for " << object << "\n";
                _sounds.insert( SoundMap::value_type( object, SoundVec() ) );
        }

        audio3DManager_cat.debug()
                << "attach_sound_to_object: " << sound << " to " << object << "\n";
        _sounds[object].push_back( sound );
}

void Audio3DManager::detach_sound( PT( AudioSound ) sound )
{
        remove_existing_sound( sound );
}

Audio3DManager::SoundVec Audio3DManager::get_sounds_on_object( const NodePath &object )
{
        SoundVec result;

        if ( _sounds.find( object ) != _sounds.end() )
        {
                result = _sounds[object];
        }

        return result;
}

void Audio3DManager::attach_listener( const NodePath &listener )
{
        _listener_target = listener;
}

void Audio3DManager::detach_listener()
{
        _listener_target = NodePath();
}

/**
* Updates position of sounds in the 3D audio system. Will be called automatically
* in a task.
*/
AsyncTask::DoneStatus Audio3DManager::update_task( GenericAsyncTask *task, void *data )
{
        Audio3DManager *a3dm = (Audio3DManager *)data;

        audio3DManager_cat.debug()
                << "update_task\n";

        if ( a3dm->_audio_manager == (AudioManager *)nullptr ||
             !a3dm->_audio_manager->get_active() )
        {
                audio3DManager_cat.debug()
                        << "audio manager is null\n";
                return AsyncTask::DS_cont;
        }

        for ( SoundMap::iterator sm_itr = a3dm->_sounds.begin();
              sm_itr != a3dm->_sounds.end(); ++sm_itr )
        {
                size_t tracked_sound = 0;
                while ( tracked_sound < sm_itr->second.size() )
                {
                        PT( AudioSound ) sound = sm_itr->second[tracked_sound];
                        audio3DManager_cat.debug()
                                << "_root is " << a3dm->_root << "\n";
                        LPoint3f pos = sm_itr->first.get_pos( a3dm->_root );
                        LVector3f vel = a3dm->get_sound_velocity( sound );
                        sound->set_3d_attributes( pos[0], pos[1], pos[2], vel[0], vel[1], vel[2] );
                        tracked_sound++;
                        audio3DManager_cat.debug()
                                << "Set 3d attributes of sound " << sound << ":\n"
                                << "\t" << pos << " " << vel << "\n";
                }
        }

        if ( !a3dm->_listener_target.is_empty() )
        {
                LPoint3f pos = a3dm->_listener_target.get_pos( a3dm->_root );
                LVector3f fwd = a3dm->_root.get_relative_vector( a3dm->_listener_target, LVector3f::forward() );
                LVector3f up = a3dm->_root.get_relative_vector( a3dm->_listener_target, LVector3::up() );
                LVector3f vel = a3dm->get_listener_velocity();
                a3dm->_audio_manager->audio_3d_set_listener_attributes( pos[0], pos[1], pos[2],
                                                                        vel[0], vel[1], vel[2],
                                                                        fwd[0], fwd[1], fwd[2],
                                                                        up[0], up[1], up[2] );
                audio3DManager_cat.debug()
                        << "Listener attrs: " << pos << " " << vel << " " << fwd << " " << up << "\n";
        }
        else
        {
                a3dm->_audio_manager->audio_3d_set_listener_attributes( 0, 0, 0, 0, 0, 0, 0,
                                                                        0, 0, 0, 0, 0 );
                audio3DManager_cat.debug()
                        << "Setting listener attrs to 0\n";
        }

        return AsyncTask::DS_cont;
}

void Audio3DManager::disable()
{
        stop_update();
        detach_listener();
        _sounds.clear();
        _velocities.clear();
}