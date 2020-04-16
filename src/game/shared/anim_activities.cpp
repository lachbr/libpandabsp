#include "anim_activities.h"

#include "keyvalues.h"

#include "virtualFileSystem.h"

void ACT_parse_activity( CActivity *act, CKeyValues *act_def )
{
	act->name = ( *act_def )["name"];

	size_t count = act_def->get_num_children();
	for ( size_t i = 0; i < count; i++ )
	{
		CKeyValues *child = act_def->get_child( i );
		if ( child->get_name() == "actor" )
		{
			PT( CActActor ) actor = new CActActor;
			ACT_parse_actor( actor, child );
			act->actors[actor->name] = actor;
		}
	}
}

bool ACT_load_from_script( const Filename &filename, CActor *actor, ActivityMap_t &output )
{
	output.clear();

	PT( CKeyValues ) kv = CKeyValues::load( filename );
	if ( !kv )
	{
		return false;
	}

	size_t children = kv->get_num_children();
	for ( size_t i = 0; i < children; i++ )
	{
		CKeyValues *child = kv->get_child( i );
		if ( child->get_name() == "activity" )
		{
			PT( CActivity ) act = new CActivity;
			ACT_parse_activity( act, child );
			output[act->name] = act;
		}
	}
}