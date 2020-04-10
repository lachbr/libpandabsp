#pragma once

#include "basecombatcharacter.h"
#include "baseplayer_shared.h"
#include "usercmd.h"

#include <pvector.h>

NotifyCategoryDeclNoExport( baseplayer )

class Client;

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

class CPlayerSimInfo
{
public:
	// realtime of sampler
	double time;
	// # of CUserCmds in this update
	int numcmds;
	// If clock needs correction, # of ticks added/removed
	int tickedcorrected;
	// player's simulationtime at end of frame
	float finalsimulationtime;
	float gamesimulationtime;
	// estimate of server perf
	float serverframetime;
	LVector3 origin;
};

class CBasePlayer;
class CPlayerInfo
{
public:
	CPlayerInfo()
	{

	}
	~CPlayerInfo()
	{
	}

	void set_parent( CBasePlayer *p )
	{
		parent = p;
	}

	std::string get_name() const;
	bool is_connected() const;

	LVector3 get_origin() const;
	LVector3 get_angles() const;
	LVector3 get_player_mins() const;
	LVector3 get_player_maxs() const;
	int get_health() const;
	int get_max_health() const;

	CBasePlayer *parent;
};

class EXPORT_SERVER_DLL CBasePlayer : public CBaseCombatCharacter, public CBasePlayerShared
{
	DECLARE_SERVERCLASS( CBasePlayer, CBaseCombatCharacter )

public:
	CBasePlayer();

	virtual void init( entid_t entnum );

	virtual void setup_controller();
	virtual void spawn();

	INLINE void set_view_angles( const LVector3 &angles )
	{
		_view_angles = angles;
	}

	INLINE LVector3 get_view_angles() const
	{
		return _view_angles;
	}

	INLINE CPlayerInfo *get_player_info()
	{
		return &_player_info;
	}

	INLINE Client *get_client() const
	{
		return _client;
	}

	void set_last_user_command( CUserCmd *cmd );
	CUserCmd *get_last_user_command();

	int get_command_context_count() const;
	CCommandContext *get_command_context( int i );
	CCommandContext *alloc_command_context();
	void remove_command_context( int i );
	void remove_all_command_contexts();
	CCommandContext *remove_all_command_contexts_except_newest();
	void replace_context_commands( CCommandContext *ctx, CUserCmd *commands,
				       int numcommands );

	int determine_simulation_ticks();
	void adjust_player_time_base( int simulation_ticks );

	//void add_to_player_simulation_list( CBaseEntity *other );
	//void remove_from_player_simulation_list( CBaseEntity *other );
	//void simluate_player_simulated_entities( void );
	//void clear_player_simulation_list();

	virtual void player_run_command( CUserCmd *cmd, float frametime );

	virtual void simulate();
	void force_simulation();

	virtual void process_usercmds( CUserCmd *cmds, int numcmds, int totalcmds, bool paused );

private:
	NetworkVec3( _view_offset );

	LVector3 _view_angles;
	CPlayerInfo _player_info;
	Client *_client;
	pvector<CCommandContext> _command_contexts;
	CUserCmd _last_cmd;
	pvector<WPT( CBaseEntity )> _simulated_by_this_player;

	bool _paused;

	// Clients try to run their own realtime clock, this is the client's clock
	NetworkVar( int, _tickbase );
};

INLINE void CBasePlayer::set_last_user_command( CUserCmd *cmd )
{
	_last_cmd = *cmd;
}

INLINE CUserCmd *CBasePlayer::get_last_user_command()
{
	return &_last_cmd;
}