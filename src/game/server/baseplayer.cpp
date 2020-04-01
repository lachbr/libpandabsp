#include "baseplayer.h"
#include "basegame.h"
#include "clockdelta.h"

NotifyCategoryDef( baseplayer, "" )

CBasePlayer::CBasePlayer() :
	CBaseCombatCharacter(),
	CBasePlayerShared(),
	_paused( false )
{
}

void CBasePlayer::setup_controller()
{
	CBasePlayerShared::setup_controller();
	_np.reparent_to( _controller->get_movement_parent() );
	_np = _controller->get_movement_parent();
}

void CBasePlayer::spawn()
{
	BaseClass::spawn();
	setup_controller();
}

int CBasePlayer::get_command_context_count() const
{
	return (int)_command_contexts.size();
}

CCommandContext *CBasePlayer::get_command_context( int i )
{
	nassertr( i >= 0 && i < _command_contexts.size(), nullptr );

	return &_command_contexts[i];
}

CCommandContext *CBasePlayer::alloc_command_context()
{
	size_t i = _command_contexts.size();
	_command_contexts.push_back( CCommandContext() );
	if ( _command_contexts.size() > 1000 )
	{
		nassert_raise( "Too many command contexts" );
	}
	return &_command_contexts[i];
}

void CBasePlayer::remove_command_context( int i )
{
	_command_contexts.erase( _command_contexts.begin() + i );
}

void CBasePlayer::remove_all_command_contexts()
{
	_command_contexts.clear();
}

CCommandContext *CBasePlayer::remove_all_command_contexts_except_newest()
{
	size_t count = _command_contexts.size();
	size_t to_remove = count - 1;
	if ( to_remove > 0 )
	{
		_command_contexts.erase( _command_contexts.begin(), _command_contexts.begin() + to_remove );
	}

	if ( !_command_contexts.size() )
	{
		assert( 0 );
		CCommandContext *ctx = alloc_command_context();
		memset( ctx, 0, sizeof( *ctx ) );
	}

	return &_command_contexts[0];
}

void CBasePlayer::replace_context_commands( CCommandContext *ctx,
					    CUserCmd *commands, int num_commands )
{
	ctx->cmds.clear();

	ctx->numcmds = num_commands;
	ctx->totalcmds = num_commands;
	ctx->dropped_packets = 0;

	// Add them in so the most recent is at slot 0
	for ( int i = num_commands - 1; i >= 0; i-- )
	{
		ctx->cmds.push_back( commands[i] );
	}
}

int CBasePlayer::determine_simulation_ticks()
{
	int ctx_count = get_command_context_count();

	int ctx_number;
	
	int simulation_ticks = 0;

	// Determine how much time we will be running this frame and fixup player
	// clock as needed
	for ( ctx_number = 0; ctx_number < ctx_count; ctx_number++ )
	{
		CCommandContext const *ctx = get_command_context( ctx_number );
		assert( ctx );
		assert( ctx->numcmds > 0 );
		assert( ctx->dropped_packets >= 0 );
		
		// Determine how long it will take to run those packets
		simulation_ticks += ctx->numcmds + ctx->dropped_packets;
	}

	return simulation_ticks;
}

void CBasePlayer::adjust_player_time_base( int simulation_ticks )
{
	assert( simulation_ticks >= 0 );
	if ( simulation_ticks < 0 )
		return;

	ServerBase *bg = ServerBase::ptr();
	CHostState *hs = bg->get_host_state();
	if ( hs->maxclients == 1 )
	{
		// Set tickbase so that player simulation tick matches hs->tickcount
		// after all commands have been executed.
		_tickbase = hs->tickcount - simulation_ticks + hs->sim_ticks_this_frame;
	}
	else
	{
		float correction_seconds = clamp( sv_clockcorrection_msecs / 1000.0f, 0.0f, 1.0f );
		int correction_ticks = bg->time_to_ticks( correction_seconds );

		// Set the target tick correction_seconds (rounded to ticks) ahead in the
		// future. This way the client can
		//  alternate around this target tick without getting smaller than
		//  hs->tickcount.
		// After running the commands simulation time should be equal or after
		// current hs->tickcount,
		//  otherwise the simulation time drops out of the client side interpolated
		//  var history window.

		int ideal_final_tick = hs->tickcount + correction_ticks;
		int estimated_final_tick = _tickbase + simulation_ticks;

		// If client gets ahead of this, we'll need to correct
		int too_fast_limit = ideal_final_tick + correction_ticks;
		// If client falls behind this, we'll also need to correct
		int too_slow_limit = ideal_final_tick - correction_ticks;

		// See if we are too fast
		if ( estimated_final_tick > too_fast_limit ||
		     estimated_final_tick < too_slow_limit )
		{
			int corrected_tick = ideal_final_tick - simulation_ticks + hs->sim_ticks_this_frame;

			_tickbase = corrected_tick;
		}
	}
}

void CBasePlayer::simulate()
{
	ServerBase *bg = ServerBase::ptr();
	CHostState *hs = bg->get_host_state();

	// Make sure not to simulate this guy twice per frame
	if ( _simulation_tick == hs->tickcount )
		return;

	_simulation_tick = hs->tickcount;

	baseplayer_cat.debug()
		<< "Simulate tick " << _simulation_tick << std::endl;

	// See how many CUserCmds are queued up for running
	int simulation_ticks = determine_simulation_ticks();

	// If some time will elapse, make sure our clock (_tickbase) starts at the
	// correct time
	if ( simulation_ticks > 0 )
		adjust_player_time_base( simulation_ticks );

	// Store off true server timestamps
	double savetime = hs->curtime;
	double saveframetime = hs->frametime;

	int command_context_count = get_command_context_count();

	// Build a list of all available commands
	pvector<CUserCmd> availcommands;

	// Contexts go from oldest to newest
	for ( int context_number = 0; context_number < command_context_count; context_number++ )
	{
		// Get oldest ( newer are added to tail )
		CCommandContext *ctx = get_command_context( context_number );

		//if ( !should_run_commands_in_context( ctx ) )
		//	continue;

		if ( !ctx->cmds.size() )
			continue;

		int numbackup = ctx->totalcmds - ctx->numcmds;

		// If we haven't dropped too many packets, then run some commands
		if ( ctx->dropped_packets < 24 )
		{
			int droppedcmds = ctx->dropped_packets;

			// run the last known cmd for each dropped cmd we don't have a backup for
			while ( droppedcmds > numbackup )
			{
				_last_cmd.tickcount++;
				availcommands.push_back( _last_cmd );
				droppedcmds--;
			}

			// Now run the "history" commands if we still have dropped packets
			while ( droppedcmds > 0 )
			{
				int cmdnum = ctx->numcmds + droppedcmds - 1;
				availcommands.push_back( ctx->cmds[cmdnum] );
				droppedcmds--;
			}
		}

		// Now run any new commands. Go backward because the most recent command
		// is at index 0.
		for ( int i = ctx->numcmds - 1; i >= 0; i-- )
		{
			availcommands.push_back( ctx->cmds[i] );
		}

		// Save off the last good command in case we drop > numbackup packets and
		// need to rerun them
		//  we'll use this to "guess" at what was in the missing packets
		_last_cmd = ctx->cmds[0];
	}

	// hs->sim_ticks_this_from == number of ticks remaining to be run, so we
	// should take the last N CUserCmds and postpone them until the next frame

	// If we're running multiple ticks this frame, don't peel off all of the
	// commands, spread them out over the server ticks. Use blocks of two in
	// alternate ticks
	int cmdlimit = sv_alternateticks ? 2 : 1;
	int cmdstorun = (int)availcommands.size();
	if ( hs->sim_ticks_this_frame >= cmdlimit &&
	     (int)availcommands.size() > cmdlimit )
	{
		int cmds_to_roll_over =
			std::min( (int)availcommands.size(), hs->sim_ticks_this_frame - 1 );
		cmdstorun = availcommands.size() - cmds_to_roll_over;
		assert( cmdstorun >= 0 );

		// Clear all contexts except the last one
		if ( cmds_to_roll_over > 0 )
		{
			CCommandContext *ctx = remove_all_command_contexts_except_newest();
			replace_context_commands( ctx, &availcommands[cmdstorun], cmds_to_roll_over );
		}
		else
		{
			// Clear all contexts
			remove_all_command_contexts();
		}
	}
	else
	{
		// Clear all contexts
		remove_all_command_contexts();
	}

	// Now run the commands
	if ( cmdstorun > 0 )
	{
		for ( int i = 0; i < cmdstorun; i++ )
		{
			player_run_command( &availcommands[i], hs->frametime );
		}
		
	}

	// Restore the true server clock
	hs->curtime = savetime;
	hs->frameticks = saveframetime;
}

void CBasePlayer::player_run_command( CUserCmd *cmd, float frametime )
{
	// Apply pitch view angle, player_move() will update yaw view angle
	_view_angles[PITCH] = cmd->viewangles[PITCH];
	player_move( cmd, frametime );
}

void CBasePlayer::force_simulation()
{
	_simulation_tick = -1;
}

void CBasePlayer::process_usercmds( CUserCmd *cmds, int numcmds, int totalcmds, bool paused )
{
	baseplayer_cat.debug()
		<< "Processing " << numcmds << " cmds\n";

	CCommandContext *ctx = alloc_command_context();
	assert( ctx );

	int i;
	for ( i = totalcmds - 1; i >= 0; i-- )
	{
		ctx->cmds.push_back( cmds[totalcmds - 1 - i] );
	}
	ctx->numcmds = numcmds;
	ctx->totalcmds = totalcmds;
	ctx->paused = paused;

	if ( ctx->paused )
	{
		baseplayer_cat.debug()
			<< "Game is paused\n";

		for ( i = 0; i < ctx->numcmds; i++ )
		{
			ctx->cmds[i].buttons = 0;
			ctx->cmds[i].forwardmove = 0;
			ctx->cmds[i].sidemove = 0;
			ctx->cmds[i].upmove = 0;
			ctx->cmds[i].viewangles = _view_angles;
		}

		ctx->dropped_packets = 0;
	}

	_paused = paused;

	if ( paused )
	{
		force_simulation();
		simulate();
	}
}

IMPLEMENT_SERVERCLASS_ST( CBasePlayer, DT_BasePlayer, baseplayer )
	SendPropInt( SENDINFO( _tickbase ) )
END_SEND_TABLE()