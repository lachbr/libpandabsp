#include "scenecomponent_shared.h"

#include "modelRoot.h"

bool SceneComponent::initialize()
{
	if ( !BaseClass::initialize() )
	{
		return false;
	}

	np = NodePath( new ModelRoot( "entity" ) );
	parent_entity = -1;
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

void SceneComponent::transition_xform( const NodePath &landmark_np, const LMatrix4 &transform )
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

void C_SceneComponent::update_parent_entity( int entnum )
{
	if ( entnum != 0 )
	{
		CBaseEntityShared *ent = clents->get_entity( entnum );
		C_SceneComponent *parent_scene;
		if ( !ent->get_component( parent_scene ) )
		{
			return;
		}

		np.reparent_to( parent_scene->np );
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
