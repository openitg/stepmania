#include "global.h"
/*
 -----------------------------------------------------------------------------
 Class: RageFile
 
 Desc: See header.
 
 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Steve Checkoway
	Glenn Maynard
 -----------------------------------------------------------------------------
 */

#include "global.h"
#include "RageFile.h"
#include "RageUtil.h"
#include "RageFileDriver.h"

void FixSlashesInPlace( CString &sPath )
{
	for( unsigned i = 0; i < sPath.size(); ++i )
		if( sPath[i] == '\\' )
			sPath[i] = '/';
}

CString FixSlashes( CString sPath )
{
	FixSlashesInPlace( sPath );
    return sPath;
}

/*
 * Keep trailing slashes, since that can be used to illustrate that a path always
 * represents a directory.  Not sure if we should always keep leading "." (we do
 * because it's simpler), but be sure to keep the "." if it's all that's there, so
 * collapsing "." doesn't result in "". 
 *
 * foo/bar -> foo/bar
 * foo/bar/ -> foo/bar/
 * foo///bar/// -> foo/bar/
 * foo/bar/./baz -> foo/bar/baz
 * foo/bar/../baz -> foo/baz
 * ./foo -> foo
 * ./ -> .
 * ./// -> .
 */

void CollapsePath( CString &sPath )
{
	/* Don't ignore empty: we do want to keep trailing slashes. */
	CStringArray as;
	split( sPath, "/", as, false );

	for( unsigned i=0; i<as.size(); i++ )
	{
		if( as[i] == ".." && i != 0 )
		{
			as.erase( as.begin()+i-1 );
			as.erase( as.begin()+i-1 );
			i -= 2;
		}
		else if( as[i] == "" && i+1 < as.size() )
		{
			/* Remove empty parts that aren't at the end; "foo//bar/" -> "foo/bar/". */
			as.erase( as.begin()+i );
			i -= 1;
		}
		else if( as[i] == "." && i != 0 )
		{
			as.erase( as.begin()+i );
			i -= 1;
		}
	}
	sPath = join( "/", as );
}

RageFile::RageFile()
{
    m_File = NULL;
	m_BufUsed = 0;
	m_EOF = false;
	m_FilePos = 0;
}
	
RageFile::RageFile( const CString& path, RageFile::OpenMode mode )
{
    m_File = NULL;
	m_BufUsed = 0;
	m_EOF = false;
	m_FilePos = 0;
	Open(path, mode);
}

RageFile::RageFile( const RageFile &cpy )
{
	ResetBuf();

	/* This will copy the file driver, including its internal file pointer. */
	m_File = FILEMAN->CopyFileObj( cpy.m_File, *this );
	m_Path = cpy.m_Path;
	m_Mode = cpy.m_Mode;
	m_Error = cpy.m_Error;
	m_EOF = cpy.m_EOF;
	m_FilePos = cpy.m_FilePos;
	memcpy( this->m_Buffer, cpy.m_Buffer, cpy.m_BufUsed );
	m_BufUsed = cpy.m_BufUsed;
}

CString RageFile::GetPath() const
{
    if ( !IsOpen() )
		return "";

	return m_File->GetDisplayPath();
}

bool RageFile::Open( const CString& path, RageFile::OpenMode mode )
{
	ASSERT( FILEMAN );

	Close();

	m_Path = path;
	FixSlashesInPlace(m_Path);

	m_Mode = mode;
	m_EOF = false;
	m_FilePos = 0;
	ResetBuf();

	int error;
	m_File = FILEMAN->Open( path, mode, *this, error );

	if( m_File == NULL )
	{
		SetError( strerror(error) );
		return false;
	}

    return true;
}

void RageFile::Close()
{
	FILEMAN->Close( m_File );
	m_File = NULL;
}

void RageFile::ResetBuf()
{
	m_BufUsed = 0;
	m_pBuf = m_Buffer;
}

/* Read up to the next \n, and return it in out.  Strip the \n.  If the \n is
 * preceded by a \r (DOS newline), strip that, too. */
int RageFile::GetLine( CString &out )
{
	out = "";

    if ( !IsOpen() )
        RageException::Throw("\"%s\" is not open.", m_Path.c_str());

	if( m_Mode != READ )
		RageException::Throw("\"%s\" is not open for reading", GetPath().c_str());

	bool GotData = false;
	while( 1 )
	{
		bool done = false;

		/* Find the end of the block we'll move to out. */
		char *p = (char *) memchr( m_pBuf, '\n', m_BufUsed );
		bool ReAddCR = false;
		if( p == NULL )
		{
			/* Hack: If the last character of the buffer is \r, then it's likely that an
			 * \r\n has been split across buffers.  Move everything else, then move the
			 * \r to the beginning of the buffer and handle it the next time around the loop. */
			if( m_pBuf[m_BufUsed-1] == '\r' )
			{
				ReAddCR = true;
				--m_BufUsed;
			}

			p = m_pBuf+m_BufUsed; /* everything */
		}
		else
			done = true;

		if( p >= m_pBuf )
		{
			char *RealEnd = p;
			if( done && p > m_pBuf && p[-1] == '\r' )
				--RealEnd; /* not including \r */
			out.append( m_pBuf, RealEnd );

			if( done )
				++p; /* skip \n */

			const int used = p-m_pBuf;
			if( used )
			{
				m_BufUsed -= used;
				m_FilePos += used;
				GotData = true;
				m_pBuf = p;
			}
		}

		if( ReAddCR )
		{
			ASSERT( m_BufUsed == 0 );
			m_pBuf = m_Buffer;
			m_Buffer[m_BufUsed] = '\r';
			++m_BufUsed;
		}

		if( done )
			break;

		/* We need more data. */
		m_pBuf = m_Buffer;

		const int size = m_File->Read( m_pBuf+m_BufUsed, sizeof(m_Buffer)-m_BufUsed );

		/* If we've read data already, then don't mark EOF yet.  Wait until the
		 * next time we're called. */
		if( size == 0 && !GotData )
		{
			m_EOF = true;
			return 0;
		}
		if( size < 0 )
			return -1; // error
		if( size == 0 )
			break; // EOF or error
		m_BufUsed += size;
	}
	return GotData? 1:0;
}

CString RageFile::GetLine()
{
	CString ret;
	GetLine( ret );
	return ret;
}

#if defined(_WIN32)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

int RageFile::PutLine( const CString &str )
{
	if( Write(str) == -1 )
		return -1;
	return Write( CString(NEWLINE) );
}


int RageFile::Read( void *buffer, size_t bytes )
{
	if( !IsOpen() )
		RageException::Throw("\"%s\" is not open.", GetPath().c_str());

	if( m_Mode != READ )
		RageException::Throw("\"%s\" is not open for reading", GetPath().c_str());

	int ret = 0;

	int FromBuffer = min( (int) bytes, m_BufUsed );
	memcpy( buffer, m_pBuf, FromBuffer );

	ret += FromBuffer;
	m_FilePos += FromBuffer;
	bytes -= FromBuffer;
	m_BufUsed -= FromBuffer;
	m_pBuf += FromBuffer;
	
	buffer = (char *) buffer + FromBuffer;
	
	if( bytes )
	{
		int FromFile = m_File->Read( buffer, bytes );
		if( FromFile < 0 )
			return -1;
		if( FromFile == 0 )
			m_EOF = true;
		ret += FromFile;
		m_FilePos += FromFile;
	}

	return ret;
}

int RageFile::Seek( int offset )
{
	if( !IsOpen() )
		RageException::Throw("\"%s\" is not open.", GetPath().c_str());

	if( m_Mode != READ )
		RageException::Throw("\"%s\" is not open for reading; can't seek", GetPath().c_str());

	m_EOF = false;

	/* If the new position is within the buffer, just eat the buffered data. */
	int FromBuffer = offset - m_FilePos;
	if( 0 <= FromBuffer && FromBuffer <= m_BufUsed )
	{
		m_FilePos += FromBuffer;
		m_BufUsed -= FromBuffer;
		m_pBuf += FromBuffer;

		return m_FilePos;
	}
	
	/* It's not.  Clear the buffer and do a real seek. */
	ResetBuf();
	
	if( offset == 0 )
	{
		m_File->Rewind();
		return 0;
	}

	int pos = m_File->Seek( offset );
	if( pos == -1 )
		return -1;

	m_FilePos = pos;
	return pos;
}

int RageFile::SeekCur( int offset )
{
	ASSERT( offset >= 0 );

	if( !IsOpen() )
		RageException::Throw("\"%s\" is not open.", GetPath().c_str());

	if( m_Mode != READ )
		RageException::Throw("\"%s\" is not open for reading; can't seek", GetPath().c_str());

	if( !offset || m_EOF )
		return m_FilePos;

	int FromBuffer = min( offset, m_BufUsed );
	m_FilePos += FromBuffer;
	offset -= FromBuffer;
	m_BufUsed -= FromBuffer;
	m_pBuf += FromBuffer;
	
	if( offset )
	{
		int pos = m_File->SeekCur( offset );
		if( pos == -1 )
			return -1;

		m_FilePos = pos;
	}
	return m_FilePos;
}

int RageFile::GetFileSize()
{
	if( !IsOpen() )
		RageException::Throw("\"%s\" is not open.", GetPath().c_str());

	if( m_Mode != READ )
		RageException::Throw("\"%s\" is not open for reading; can't GetFileSize", GetPath().c_str());

	return m_File->GetFileSize();
}

void RageFile::Rewind()
{
	if( !IsOpen() )
		RageException::Throw("\"%s\" is not open.", GetPath().c_str());

	if( m_Mode != READ )
		RageException::Throw("\"%s\" is not open for reading; can't GetFileSize", GetPath().c_str());

	m_EOF = false;

	m_FilePos = 0;
	m_File->Rewind();
}

int RageFile::Read( CString &buffer, size_t bytes )
{
	char *buf = new char[bytes];
	int ret = Read( buf, bytes );
	if( ret != -1 )
		buffer.assign( buf, buf+ret );
	delete [] buf;
	return ret;
}

int RageFile::Write( const void *buffer, size_t bytes )
{
	if( !IsOpen() )
		RageException::Throw("\"%s\" is not open.", GetPath().c_str());

	if( m_Mode != WRITE )
		RageException::Throw("\"%s\" is not open for writing", GetPath().c_str());

	return m_File->Write( buffer, bytes );
}


int RageFile::Write( const void *buffer, size_t bytes, int nmemb )
{
	/* Simple write.  We never return partial writes. */
	int ret = Write( buffer, bytes*nmemb ) / bytes;
	if( ret == -1 )
		return -1;
	return ret / bytes;
}

int RageFile::Flush()
{
	return m_File->Flush();
}

int RageFile::Read( void *buffer, size_t bytes, int nmemb )
{
	const int ret = Read( buffer, bytes*nmemb );
	if( ret == -1 )
		return -1;

	/* If we're reading 10-byte blocks, and we got 27 bytes, we have 7 extra bytes.
	 * Seek back. */
	const int extra = ret % bytes;
	Seek( Tell()-extra );

	return ret/bytes;
}

int RageFile::Seek( int offset, int whence )
{
	switch( whence )
	{
	case SEEK_CUR:
		return SeekCur( (int) offset );
	case SEEK_END:
		offset += GetFileSize();
	}
	return Seek( (int) offset );
}
