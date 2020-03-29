//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _SSE_H
#define _SSE_H

#include "common_config.h"

#include <aa_luse.h>

extern _BSPEXPORT float _SSE_Sqrt( float x );
extern _BSPEXPORT float _SSE_RSqrtAccurate( float a );
extern _BSPEXPORT float _SSE_RSqrtFast( float x );
extern _BSPEXPORT float FASTCALL _SSE_VectorNormalize( LVector3& vec );
extern _BSPEXPORT void FASTCALL _SSE_VectorNormalizeFast( LVector3& vec );
extern _BSPEXPORT float _SSE_InvRSquared( const float* v );
extern _BSPEXPORT void _SSE_SinCos( float x, float* s, float* c );
extern _BSPEXPORT float _SSE_cos( float x );
extern _BSPEXPORT void _SSE2_SinCos( float x, float* s, float* c );
extern _BSPEXPORT float _SSE2_cos( float x );
extern _BSPEXPORT void VectorTransformSSE( const float *in1, const LMatrix4& in2, float *out1 );
extern _BSPEXPORT void VectorRotateSSE( const float *in1, const LMatrix4& in2, float *out1 );

#endif // _SSE_H