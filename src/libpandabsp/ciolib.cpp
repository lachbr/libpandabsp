/**
 * PANDA3D BSP LIBRARY
 * 
 * Copyright (c) Brian Lach <brianlach72@gmail.com>
 * All rights reserved.
 *
 * @file ciolib.cpp
 * @author Brian Lach
 * @date July 12, 2019
 */

#ifdef CIO

#include "ciolib.h"

static const LVector3 LeftA( 0.06, 0.0, 0.14 );
static const LVector3 LeftB( -0.13, 0.0, 0.1 );
static const LVector3 LeftC( -0.05, 0.0, 0.0 );
static const LVector3 LeftD( 0.06, 0.0, 0.0 );
static const LVector3 RightA( 0.13, 0.0, 0.1 );
static const LVector3 RightB( -0.06, 0.0, 0.14 );
static const LVector3 RightC( -0.06, 0.0, 0.0 );
static const LVector3 RightD( 0.05, 0.0, 0.0 );
static const LVector3 LeftAD( LeftA[0] - LeftA[2] * ( LeftD[0] - LeftA[0] ) / ( LeftD[2] - LeftA[2] ), 0.0, 0.0 );
static const LVector3 LeftBC( LeftB[0] - LeftB[2] * ( LeftC[0] - LeftB[0] ) / ( LeftC[2] - LeftB[2] ), 0.0, 0.0 );
static const LVector3 RightAD( RightA[0] - RightA[2] * ( RightD[0] - RightA[0] ) / ( RightD[2] - RightA[2] ), 0.0, 0.0 );
static const LVector3 RightBC( RightB[0] - RightB[2] * ( RightC[0] - RightB[0] ) / ( RightC[2] - RightB[2] ), 0.0, 0.0 );
static const LVector3 LeftMid = ( LeftA + LeftB + LeftC + LeftD ) / 4.0;
static const LVector3 RightMid = ( RightA + RightB + RightC + RightD ) / 4.0;

void CIOLib::set_pupil_direction( float x, float y, LVector3 &left, LVector3 &right )
{
	float		x2, y2;
	LVector3	left0, left1, left2, right0, right1, right2;

	if ( y < 0.0 )
	{
		y2 = -y;
		left1 = LeftAD + ( LeftD - LeftAD ) * y2;
		left2 = LeftBC + ( LeftC - LeftBC ) * y2;
		right1 = RightAD + ( RightD - RightAD ) * y2;
		right2 = RightBC + ( RightC - RightBC ) * y2;
	}
	else
	{
		y2 = y;
		left1 = LeftAD + ( LeftA - LeftAD ) * y2;
		left2 = LeftBC + ( LeftB - LeftBC ) * y2;
		right1 = RightAD + ( RightA - RightAD ) * y2;
		right2 = RightBC + ( RightB - RightBC ) * y2;
	}

	left0 = LVector3( 0.0, 0.0, left1[2] - left1[0] * ( left2[2] - left1[2] ) / ( left2[0] - left1[0] ) );
	right0 = LVector3( 0.0, 0.0, right1[2] - right1[0] * ( right2[2] - right1[2] ) / ( right2[0] - right1[0] ) );
	
	if (x < 0.0)
	{
		x2 = -x;
		left = left0 + ( left2 - left0 ) * x2;
		right = right0 + ( right2 - right0 ) * x2;
	
	}
	else
	{
		x2 = x;
		left = left0 + ( left1 - left0 ) * x2;
		right = right0 + ( right1 - right0 ) * x2;
	}
}

LVector2 CIOLib::look_pupils_at( const NodePath &node, const LVector3 &point, const NodePath &eyes )
{
	LVector3 eye_point = point;
	if ( !node.is_empty() )
	{
		LMatrix4 mat = node.get_mat( eyes );
		eye_point = mat.xform_point( point );
	}
	float distance = 1.0f;
	float recip_z = 1.0f / std::max( 0.1f, eye_point[1] );
	float x = distance * eye_point[0] * recip_z;
	float y = distance * eye_point[2] * recip_z;
	x = std::min( std::max( x, -1.0f ), 1.0f );
	y = std::min( std::max( y, -1.0f ), 1.0f );

	return LVector2( x, y );
}

#endif // CIO
