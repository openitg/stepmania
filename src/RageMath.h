#ifndef RAGE_MATH_H
#define RAGE_MATH_H
/*
-----------------------------------------------------------------------------
 File: RageMath.h

 Desc: .

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

struct RageVector2;
struct RageVector3;
struct RageVector4;
struct RageMatrix;


void RageVec2Normalize( RageVector2* pOut, const RageVector2* pV );
void RageVec3Normalize( RageVector3* pOut, const RageVector3* pV );
void RageVec3TransformCoord( RageVector3* pOut, const RageVector3* pV, const RageMatrix* pM );
void RageVec4TransformCoord( RageVector4* pOut, const RageVector4* pV, const RageMatrix* pM );
void RageMatrixIdentity( RageMatrix* pOut );
RageMatrix RageMatrixIdentity();
void RageMatrixMultiply( RageMatrix* pOut, const RageMatrix* pA, const RageMatrix* pB );
void RageMatrixTranslation( RageMatrix* pOut, float x, float y, float z );
void RageMatrixScaling( RageMatrix* pOut, float x, float y, float z );
void RageMatrixRotationX( RageMatrix* pOut, float fTheta );
void RageMatrixRotationY( RageMatrix* pOut, float fTheta );
void RageMatrixRotationZ( RageMatrix* pOut, float fTheta );
RageMatrix RageMatrixRotationX( float fTheta );
RageMatrix RageMatrixRotationY( float fTheta );
RageMatrix RageMatrixRotationZ( float fTheta );
void RageMatrixCommand( CString sCommandString, RageMatrix &mat );
void RageQuatFromHPR(RageVector4* pOut, RageVector3 hpr );
void RageMatrixFromQuat( RageMatrix* pOut, const RageVector4 q );
void RageQuatSlerp(RageVector4 *pOut, const RageVector4 &from, const RageVector4 &to, float t);
RageVector4 RageQuatFromH(float theta);
RageVector4 RageQuatFromP(float theta);
RageVector4 RageQuatFromR(float theta);
void RageQuatMultiply( RageVector4* pOut, const RageVector4 &pA, const RageVector4 &pB );
RageMatrix RageLookAt(
	float eyex, float eyey, float eyez,
	float centerx, float centery, float centerz,
	float upx, float upy, float upz );

#endif

// Fix for platforms that don't have these in math.h
#ifndef HAVE_POWF
#define powf(x,y) (float)pow((float)(x), (float)(y))
#endif
  
#ifndef HAVE_SQRTF
#define sqrtf(x) (float)sqrt((float)(x))
#endif

#ifndef HAVE_COSF
#define cosf(x) (float)cos((float)(x))
#endif

#ifndef HAVE_SINF
#define sinf(x) (float)sin((float)(x))
#endif

#ifndef HAVE_TANF
#define tanf(x) (float)tan((float)(x))
#endif

#ifndef HAVE_ACOSF
#define acosf(x) (float)acosh((float)(x))
#endif

#ifndef HAVE_ASINF
#define asinf(x) (float)acosh((float)(x))
#endif

#ifndef HAVE_ATANF
#define atanf(x) (float)atanh((float)(x))
#endif