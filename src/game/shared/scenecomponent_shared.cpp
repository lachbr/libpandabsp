#include "scenecomponent_shared.h"

#include "modelRoot.h"

SceneComponent::SceneComponent() :
	CBaseComponent()

#ifdef CLIENT_DLL
	,
	iv_origin( "C_SceneComponent_iv_origin" ),
	iv_angles( "C_SceneComponent_iv_angles" ),
	iv_scale( "C_SceneComponent_iv_scale" )
#endif
{
#ifdef CLIENT_DLL
	add_var( &origin, &iv_origin, LATCH_SIMULATION_VAR );
	add_var( &angles, &iv_angles, LATCH_SIMULATION_VAR );
	add_var( &scale, &iv_scale, LATCH_SIMULATION_VAR );
#endif
}

void SceneComponent::spawn()
{
	BaseClass::spawn();
	update_parent_entity( parent_entity );
}

bool SceneComponent::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	np = NodePath( new ModelRoot( "entity" ) );
	parent_entity = 0;
	origin = LPoint3f( 0 );
	angles = LVector3f( 0 );
	scale = LVector3f( 1 );

	return true;
}

void SceneComponent::shutdown()
{
	BaseClass::shutdown();

	if ( !np.is_empty() )
		np.remove_node();
}

#if !defined( CLIENT_DLL )

#include "baseentity.h"
#include "sv_entitymanager.h"
#include "serverbase.h"

void CSceneComponent::update_parent_entity( int entnum )
{
	if ( entnum != 0 )
	{
		CBaseEntityShared *ent = svents->get_entity( entnum );
		if ( ent )
		{
			CSceneComponent *parent_scene;
			if ( !ent->get_component( parent_scene ) )
			{
				return;
			}

			np.reparent_to( parent_scene->np );
		}

	}
	else
	{
		np.reparent_to( sv->get_root() );
	}
}

void CSceneComponent::init_mapent()
{
	if ( _centity->has_entity_value( "origin" ) )
	{
		LVector3f org = _centity->get_entity_value_vector( "origin" );
		set_origin( org / 16.0f );
	}

	if ( _centity->has_entity_value( "angles" ) )
	{
		LVector3f ang = _centity->get_entity_value_vector( "angles" );
		set_angles( LVector3f( ang[1] - 90, ang[0], ang[2] ) );
	}

	if ( _centity->has_entity_value( "scale" ) )
	{
		LVector3f sc = _centity->get_entity_value_vector( "scale" );
		set_scale( sc );
	}
}

void CSceneComponent::transition_xform( const NodePath &landmark_np, const LMatrix4 &transform )
{
	np.set_mat( landmark_np, transform );
	origin = np.get_pos();
	angles = np.get_hpr();
	scale = np.get_scale();
}

IMPLEMENT_SERVERCLASS_ST_NOBASE( CSceneComponent )
	SendPropInt( PROPINFO( parent_entity ) ),
	SendPropVec3( PROPINFO( origin ) ),
	SendPropVec3( PROPINFO( angles ) ),
	SendPropVec3( PROPINFO( scale ) )
END_SEND_TABLE()

#else

#include "cl_rendersystem.h"
#include "cl_entitymanager.h"

void C_SceneComponent::post_interpolate()
{
	// Apply interpolated transform to node
	if ( !np.is_empty() )
	{
		np.set_pos( origin );
		np.set_hpr( angles );
		np.set_scale( scale );
	}
}

void C_SceneComponent::update_parent_entity( int entnum )
{
	if ( entnum != 0 )
	{
		CBaseEntityShared *ent = clents->get_entity( entnum );
		if ( ent )
		{
			C_SceneComponent *parent_scene;
			if ( !ent->get_component( parent_scene ) )
			{
				return;
			}

			np.reparent_to( parent_scene->np );
		}
		
	}
	else
	{
		np.reparent_to( clrender->get_render() );
	}
}

void RecvProxy_ParentEntity( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Int32( prop, object, out, dgi );

	C_SceneComponent *ent = (C_SceneComponent *)object;
	ent->update_parent_entity( ent->parent_entity );
}

void RecvProxy_Origin( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_SceneComponent *ent = (C_SceneComponent *)object;
	ent->np.set_pos( ent->origin );
}

void RecvProxy_Angles( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_SceneComponent *ent = (C_SceneComponent *)object;
	ent->np.set_hpr( ent->angles );
}

void RecvProxy_Scale( RecvProp *prop, void *object, void *out, DatagramIterator &dgi )
{
	RecvProxy_Vec<LVector3>( prop, object, out, dgi );

	C_SceneComponent *ent = (C_SceneComponent *)object;
	ent->np.set_scale( ent->scale );
}

IMPLEMENT_CLIENTCLASS_RT_NOBASE( C_SceneComponent, CSceneComponent )
	RecvPropInt( PROPINFO( parent_entity ), RecvProxy_ParentEntity ),
	RecvPropVec3( PROPINFO( origin ), RecvProxy_Origin ),
	RecvPropVec3( PROPINFO( angles ), RecvProxy_Angles ),
	RecvPropVec3( PROPINFO( scale ), RecvProxy_Scale )
END_RECV_TABLE()

#endif
