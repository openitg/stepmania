#include "global.h"

#include "ExportPackage.h"
#include "RageUtil.h"
#include "RageFileManager.h"
#include "SpecialFiles.h"
#include "RageLog.h"
#include "Song.h"
#include "FileDownload.h"
#include "CreateZip.h"

static RString ReplaceInvalidFileNameChars( RString sOldFileName )
{
	RString sNewFileName = sOldFileName;
	const char charsToReplace[] = { 
		' ', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', 
		'+', '=', '[', ']', '{', '}', '|', ':', '\"', '\\',
		'<', '>', ',', '?', '/' 
	};
	for( unsigned i=0; i<sizeof(charsToReplace); i++ )
		sNewFileName.Replace( charsToReplace[i], '_' );
	return sNewFileName;
}

bool ExportPackage::PublishSong( const Song *pSong, RString &sErrorOut )
{
	RString sDirToExport = pSong->GetSongDir();
	RString sPackageName = ReplaceInvalidFileNameChars( sDirToExport + ".smzip" );

	RString sSmzipFile = SpecialFiles::CACHE_DIR + "Uploads/" + sPackageName + ".smzip";

	if( ExportPackage(sSmzipFile, sDirToExport, sErrorOut) )
		return false;

	FileTransfer ft;
	ft.StartUpload( "http://www.stepmania.com/upload/", sSmzipFile );
	
	return true;
}

bool ExportPackage::ExportPackage( RString sSmzipFile, RString sDirToExport, RString &sErrorOut )
{
	RageFile f;
	// TODO: Mount Desktop/ for each OS
	if( !f.Open(sSmzipFile, RageFile::WRITE) )
	{
		sErrorOut = ssprintf( "Couldn't open %s for writing: %s", sSmzipFile.c_str(), f.GetError().c_str() );
		return false;
	}

	/*
	RageFileObjZip zip( &f );
	zip.Start();
	zip.SetGlobalComment( sComment );

	vector<RString> vs;
	GetDirListingRecursive( sDirToExport, "*", vs );
	SMPackageUtil::StripIgnoredSmzipFiles( vs );
	FOREACH( RString, vs, s )
	{
		if( !zip.AddFile( *s ) )
		{
			sErrorOut = ssprintf( "Couldn't add file: %s", s->c_str() );
			return false;
		}
	}

	if( zip.Finish() == -1 )
	{
		sErrorOut = ssprintf( "Couldn't write to file %s", fn.c_str(), f.GetError().c_str() );
		return false;
	}
	*/
	HZIP hz = CreateZip("simple1.zip",0);
	ZipAdd(hz,"adad\\znsimple.bmp",  "simple.bmp");
	ZipAdd(hz,"znsimple.txt",  "simple.txt");
	CloseZip(hz);

	return true;
}

/*
 * (c) 2010 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
