#include "global.h"
#include "ArchHooks_Win32.h"

#include "archutils/win32/tls.h"
#include "archutils/win32/crash.h"
#include "archutils/win32/DebugInfoHunt.h"

ArchHooks_Win32::ArchHooks_Win32()
{
	SetUnhandledExceptionFilter(CrashHandler);
	InitThreadData("Main thread");
	VDCHECKPOINT;
}

void ArchHooks_Win32::Log(CString str, bool important)
{
	/* It's OK to send to both of these; it'll let us know when important
	 * events occurred in crash dumps if they're recent. */
	if(important)
		StaticLog(str);

	CrashLog(str);
}

void ArchHooks_Win32::AdditionalLog(CString str)
{
	AdditionalLog(str);
}

void ArchHooks_Win32::DumpDebugInfo()
{
	/* This is a good time to do the debug search: before we actually
	 * start OpenGL (in case something goes wrong). */
	SearchForDebugInfo();
}

void ArchHooks_Win32::MessageBoxOK( CString sMessage )
{
	MessageBox(NULL, sMessage, "StepMania", MB_OK );
}

ArchHooks::MessageBoxResult ArchHooks_Win32::MessageBoxAbortRetryIgnore( CString sMessage )
{
	switch( MessageBox(NULL, sMessage, "StepMania", MB_ABORTRETRYIGNORE ) )
	{
	case IDABORT:	return abort;
	case IDRETRY:	return retry;
	default:	ASSERT(0);
	case IDIGNORE:	return ignore;
	}
} 

/*
 * Copyright (c) 2002-2003 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
