#pragma once

#include "basecomponent.h"
#include "usercmd.h"

// For queuing and processing usercmds
class CCommandContext
{
public:
	pvector<CUserCmd> cmds;

	int numcmds;
	int totalcmds;
	int dropped_packets;
	bool paused;
};

// Info about the last 20 or so updates
class CPlayerCmdInfo
{
public:
	CPlayerCmdInfo() :
		time( 0.0 ),
		numcmds( 0 ),
		droppedpackets( 0 )
	{
	}

	// realtime of sample
	double time;
	// # of CUserCmds in this update
	int numcmds;
	// # of dropped packets on the link
	int droppedpackets;
};

/**
 * Component that is attached to the main player entity.
 * It manages the client's command contexts and dispatches commands to
 * CPlayerControlled components.
 */
class CPlayerComponent : public CBaseComponent
{
	DECLARE_SERVERCLASS( CPlayerComponent, CBaseComponent )

public:
	INLINE void set_view_angles( const LVector3 &angles )
	{
		_view_angles = angles;
	}

	INLINE LVector3 get_view_angles() const
	{
		return _view_angles;
	}

	virtual bool initialize();
	virtual void update( double frametime );

	void set_last_user_command( CUserCmd *cmd );
	CUserCmd *get_last_user_command();

	int determine_simulation_ticks();
	void adjust_player_time_base( int simulation_ticks );

	virtual void player_run_command( CUserCmd *cmd, float frametime );
	virtual void process_usercmds( CUserCmd *cmds, int numcmds, int totalcmds, bool paused );

	int get_command_context_count() const;
	CCommandContext *get_command_context( int i );
	CCommandContext *alloc_command_context();
	void remove_command_context( int i );
	void remove_all_command_contexts();
	CCommandContext *remove_all_command_contexts_except_newest();
	void replace_context_commands( CCommandContext *ctx, CUserCmd *commands,
				       int numcommands );

	void force_simulation();

	// Clients try to run their own realtime clock, this is the client's clock
	NetworkVar( int, tickbase );

	NetworkInt( simulation_tick );

	pvector<CCommandContext> _command_contexts;
	CUserCmd _last_cmd;

	LVector3f _view_angles;

	bool _paused;
};

inline void CPlayerComponent::set_last_user_command( CUserCmd *cmd )
{
	_last_cmd = *cmd;
}

inline CUserCmd *CPlayerComponent::get_last_user_command()
{
	return &_last_cmd;
}
