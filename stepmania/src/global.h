/*
-----------------------------------------------------------------------------
 File: global.h

 Desc: Include file for standard system include files.

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Glenn Maynard
-----------------------------------------------------------------------------
*/

#ifndef STDAFX_H
#define STDAFX_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff

/* Platform-specific fixes. */
#if defined(WIN32)
#include "archutils/Win32/arch_setup.h"
#elif defined(PBBUILD)
#include "archutils/Darwin/arch_setup.h"
#endif

/* Make sure everyone has min and max: */
#include <algorithm>

/* Everything will need string for one reason or another: */
#include <string>

/* And vector: */
#include <vector>

#if !defined(MISSING_STDINT_H) /* need to define int64_t if so */
#include <stdint.h>
#endif

#if defined(NEED_CSTDLIB_WORKAROUND)
#define llabs ::llabs
#endif

#if defined(NEED_MINMAX_TEMPLATES)
/* Some old <algorithm>s don't actually define min and max. */
template<class T>
inline const T& max(const T &a, const T &b)			{ return a < b? b:a; }
template<class T, class P>
inline const T& max(const T &a, const T &b, P Pr)	{ return Pr(a, b)? b:a; }
template<class T>
inline const T& min(const T &a, const T &b)			{ return b < a? b:a; }
template<class T, class P>
inline const T& min(const T &a, const T &b, P Pr)	{ return Pr(b, a)? b:a; }
#endif

using namespace std;

#ifdef ASSERT
#undef ASSERT
#endif

/* RageThreads defines (don't pull in all of RageThreads.h here) */
namespace Checkpoints
{
	void SetCheckpoint( const char *file, int line, const char *message );
};
#define CHECKPOINT (Checkpoints::SetCheckpoint(__FILE__, __LINE__, NULL))
#define CHECKPOINT_M(m) (Checkpoints::SetCheckpoint(__FILE__, __LINE__, m))


/* Define a macro to tell the compiler that a function doesn't return.  This just
 * improves compiler warnings.  This should be placed near the beginning of the
 * function prototype (although it looks better near the end, VC only accepts it
 * at the beginning). */
#if defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#elif defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))
#define NORETURN __attribute__ ((__noreturn__))
#else
#define NORETURN
#endif

/* The infinite loop isn't actually reached; it's just there to shut up a warning. */
#ifdef _WINDOWS
	#include "windows.h"
	static inline void NORETURN sm_crash() { DebugBreak(); }	// throws an exception that gets caught by the exception handler
#else
	static inline void NORETURN sm_crash() { *(char*)0=0; while(1); }
#endif 

/* Assertion that sets an optional message and brings up the crash handler, so
 * we get a backtrace.  This should probably be used instead of throwing an
 * exception in most cases we expect never to happen (but not in cases that
 * we do expect, such as DSound init failure.) */
#define FAIL_M(MESSAGE) { CHECKPOINT_M(MESSAGE); sm_crash(); }
#define ASSERT_M(COND, MESSAGE) { if(!(COND)) { FAIL_M(MESSAGE); } }
#define ASSERT(COND) ASSERT_M((COND), "Assertion '" #COND "' failed")

#define RAGE_ASSERT ASSERT /* compat */
#define RAGE_ASSERT_M ASSERT_M /* compat */

#ifdef DEBUG
#define DEBUG_ASSERT(x)	ASSERT(x)
#else
#define DEBUG_ASSERT(x)
#endif

/* Define a macro to tell the compiler that a function has printf() semantics, to
 * aid warning output. */
#if defined(__GNUC__)
#define PRINTF(a,b) __attribute__((format(__printf__,a,b)))
#else
#define PRINTF(a,b)
#endif

/* Use CStdString: */
#include "StdString.h"

/* Include this here to make sure our assertion handler is always
 * used.  (This file is a dependency of most everything anyway,
 * so there's no real problem putting it here.) */
#include "RageException.h"

#if !defined(WIN32)
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

/* Define a few functions if necessary */
#include <math.h>
#ifdef NEED_POWF
inline float powf (float x, float y) { return float(pow(double(x),double(y))); }
#endif

#ifdef NEED_SQRTF
inline float sqrtf(float x) { return float(sqrt(double(x))); }
#endif

#ifdef NEED_SINF
inline float sinf(float x) { return float(sin(double(x))); }
#endif

#ifdef NEED_TANF
inline float tanf(float x) { return float(tan(double(x))); }
#endif

#ifdef NEED_COSF
inline float cosf(float x) { return float(cos(double(x))); }
#endif

#ifdef NEED_ACOSF
inline float acosf(float x) { return float(acos(double(x))); }
#endif

#ifdef NEED_TRUNCF
inline float truncf( float f )	{ return float(int(f)); };
#endif

#ifdef NEED_ROUNDF
inline float roundf( float f )	{ if(f < 0) return truncf(f-0.5f); return truncf(f+0.5f); };
#endif

/* Don't include our own headers here, since they tend to change often. */

#endif
