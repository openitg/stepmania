//	from VirtualDub
//	Copyright (C) 1998-2001 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef CRASH_H
#define CRASH_H
#include <windows.h>
extern long __stdcall CrashHandler(struct _EXCEPTION_POINTERS *ExceptionInfo);

/* Exactly as advertised.  (This will bring up the crash handler even
 * in the debugger.) */
void NORETURN debug_crash();

void do_backtrace( const void **buf, size_t size, HANDLE hProcess, HANDLE hThread, const CONTEXT *pContext );
void SymLookup( const void *ptr, char *buf );
void NORETURN Crash_BacktraceThread( HANDLE hThread );

#endif
