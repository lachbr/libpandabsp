#pragma once

#include <PxPhysicsAPI.h>
#include "luse.h"

// Convert to PhysX math

physx::PxVec2 Vec2_to_PxVec2( const LVecBase2f &vec )
{
	return physx::PxVec2( vec[0], vec[1] );
}


physx::PxVec3 Vec3_to_PxVec3( const LVecBase3f &vec )
{
	return physx::PxVec3( vec[0], vec[1], vec[2] );
}

physx::PxVec4 Vec4_to_PxVec4( const LVecBase4f &vec )
{
	return physx::PxVec4( vec[0], vec[1], vec[2], vec[3] );
}

physx::PxMat33 Mat3_to_PxMat33( const LMatrix3f &mat )
{
	return physx::PxMat33( Vec3_to_PxVec3( mat.get_col( 0 ) ),
			       Vec3_to_PxVec3( mat.get_col( 1 ) ),
			       Vec3_to_PxVec3( mat.get_col( 2 ) ) );
}

physx::PxMat44 Mat4_to_PxMat44( const LMatrix4f &mat )
{
	return physx::PxMat44( Vec4_to_PxVec4( mat.get_col( 0 ) ),
			       Vec4_to_PxVec4( mat.get_col( 1 ) ),
			       Vec4_to_PxVec4( mat.get_col( 2 ) ),
			       Vec4_to_PxVec4( mat.get_col( 2 ) ) );
}

// Convert from PhysX math

LVector2f PxVec2_to_Vec2( const physx::PxVec2 &vec )
{
	return LVector2f( vec[0], vec[1] );
}

LVector3f PxVec3_to_Vec3( const physx::PxVec3 &vec )
{
	return LVector3f( vec[0], vec[1], vec[2] );
}

LVector4f PxVec4_to_Vec4( const physx::PxVec4 &vec )
{
	return LVector4f( vec[0], vec[1], vec[2], vec[3] );
}

// FIXME: this might be wrong
LMatrix3f PxMat33_to_Mat3( const physx::PxMat33 &mat )
{
	physx::PxMat33 trans = mat.getTranspose();
	return LMatrix3f( PxVec3_to_Vec3( trans.column0 ),
			  PxVec3_to_Vec3( trans.column1 ),
			  PxVec3_to_Vec3( trans.column2 ) );
}

// FIXME: this might be wrong
LMatrix4f PxMat44_to_Mat4( const physx::PxMat44 &mat )
{
	physx::PxMat44 trans = mat.getTranspose();
	return LMatrix4f( PxVec4_to_Vec4( trans.column0 ),
			  PxVec4_to_Vec4( trans.column1 ),
			  PxVec4_to_Vec4( trans.column2 ),
			  PxVec4_to_Vec4( trans.column3 ) );
}
