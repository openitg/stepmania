/*
-----------------------------------------------------------------------------
 File: RageUtil.h

 Desc: Helper and error-controlling function used throughout the program.

 Copyright (c) 2001 Chris Danford.  All rights reserved.
-----------------------------------------------------------------------------
*/

#ifndef _RAGEUTIL_H_
#define _RAGEUTIL_H_


#include "dxerr8.h"
#pragma comment(lib, "DxErr8.lib")


//-----------------------------------------------------------------------------
// SAFE_ Macros
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }


//-----------------------------------------------------------------------------
// Other Macros
//-----------------------------------------------------------------------------
#define RECTWIDTH(rect)   ((rect).right  - (rect).left)
#define RECTHEIGHT(rect)  ((rect).bottom - (rect).top)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define clamp(val,low,high)		( max( (low), min((val),(high)) ) )


//-----------------------------------------------------------------------------
// Misc helper functions
//-----------------------------------------------------------------------------

// Simple function for generating random numbers
inline float randomf( float low=-1.0f, float high=1.0f )
{
    return low + ( high - low ) * ( (FLOAT)rand() ) / RAND_MAX;
}
inline int roundf( float f )	{ return (int)((f)+0.5f); };
inline int roundf( double f )	{ return (int)((f)+0.5);  };


bool IsAnInt( CString s );


CString ssprintf( LPCTSTR fmt, ...);
CString vssprintf( LPCTSTR fmt, va_list argList );


// Splits a Path into 4 parts (Directory, Drive, Filename, Extention).  Supports UNC path names.
// param1: Whether the Supplied Path (PARAM2) contains a directory name only
//            or a file name (Reason: some directories will end with "xxx.xxx"
//            which is like a file name).
void splitpath(BOOL UsingDirsOnly, CString Path, CString& Drive, CString& Dir, CString& FName, CString& Ext);

void splitrelpath( CString Path, CString& Dir, CString& FName, CString& Ext );

// Splits a CString into an CStringArray according the Deliminator.  Supports UNC path names.
void split(CString Source, CString Deliminator, CStringArray& AddIt, bool bIgnoreEmpty = true );

// Joins a CStringArray to create a CString according the Deliminator.
CString join(CString Deliminator, CStringArray& Source);

void GetDirListing( CString sPath, CStringArray &AddTo, bool bOnlyDirs=false );

bool DoesFileExist( CString sPath );
DWORD GetFileSizeInBytes( CString sFilePath );

int CompareCStrings(const void *arg1, const void *arg2);
void SortCStringArray( CStringArray &AddTo, BOOL bSortAcsending = TRUE );

//-----------------------------------------------------------------------------
// Log helpers
//-----------------------------------------------------------------------------
void RageLogStart();
void RageLog( LPCTSTR fmt, ...);
void RageLogHr( HRESULT hr, LPCTSTR fmt, ...);

//-----------------------------------------------------------------------------
// Error helpers
//-----------------------------------------------------------------------------
void DisplayErrorAndDie( CString sError );

#define RageError(str)		DisplayErrorAndDie( ssprintf(     "%s\n\n%s(%d)", str,						 __FILE__, (DWORD)__LINE__) )
#define RageErrorHr(str,hr)	DisplayErrorAndDie( ssprintf("%s (%s)\n\n%s(%d)", str, DXGetErrorString8(hr), __FILE__, (DWORD)__LINE__) )



LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);
HINSTANCE GotoURL(LPCTSTR url);


#endif
