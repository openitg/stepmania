#pragma once
/*
-----------------------------------------------------------------------------
 Class: RageError

 Desc: Class for thowing fatal error exceptions

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include <exception>

class RageException : exception
{
public:
	RageException( const char *fmt, ...);
	RageException( HRESULT hr, const char *fmt, ...);

	virtual const char *what() const;

protected:
	CString m_sError;
};

