#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: RageLog

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "RageException.h"
#include "RageUtil.h"

#include "dxerr8.h"
#pragma comment(lib, "DxErr8.lib")


RageException::RageException( const char *fmt, ...)
{
    va_list	va;
    va_start(va, fmt);
    m_sError = vssprintf( fmt, va );
#ifdef _DEBUG
	MessageBox( NULL, m_sError, "Fatal Error", MB_OK );
	DebugBreak();
#endif
}

RageException::RageException( HRESULT hr, const char *fmt, ...)
{
    va_list	va;
    va_start(va, fmt);
    m_sError = vssprintf( fmt, va );
	m_sError += ssprintf( "(%s)", DXGetErrorString8(hr) );
#ifdef _DEBUG
	MessageBox( NULL, m_sError, "Fatal Error", MB_OK );
	DebugBreak();
#endif
}

const char* RageException::what() const
{
	return m_sError;
}
