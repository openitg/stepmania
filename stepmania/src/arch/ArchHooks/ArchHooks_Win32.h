#ifndef ARCH_HOOKS_WIN32_H
#define ARCH_HOOKS_WIN32_H

#include "ArchHooks.h"
class RageMutex;

class ArchHooks_Win32: public ArchHooks
{
public:
    ArchHooks_Win32();
    ~ArchHooks_Win32();
    void DumpDebugInfo();
    void MessageBoxOKPrivate( CString sMessage, CString ID );
	void MessageBoxErrorPrivate( CString error, CString ID );
    MessageBoxResult MessageBoxAbortRetryIgnorePrivate( CString sMessage, CString ID );
    MessageBoxResult MessageBoxRetryCancelPrivate( CString sMessage, CString ID );
	void RestartProgram();

	int OldThreadPriority;
	RageMutex *TimeCritMutex;
	void EnterTimeCriticalSection();
	void ExitTimeCriticalSection();
	void SetTime( tm newtime );
};

#undef ARCH_HOOKS
#define ARCH_HOOKS ArchHooks_Win32

#endif
/*
 * Copyright (c) 2002-2003 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
