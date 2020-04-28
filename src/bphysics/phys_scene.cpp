#include "phys_scene.h"
#include "physx_globals.h"

CPhysScene::CPhysScene( const CPhysSceneDesc &desc )
{
	m_pScene = GetPxPhysics()->createScene( desc.get_desc() );
}

CPhysScene::~CPhysScene()
{
	if ( m_pScene )
	{
		m_pScene->release();
		m_pScene = nullptr;
	}
}
