#include "global.h"

#include "ExportPackage.h"
#include "RageUtil.h"
#include "RageFileManager.h"
#include "SpecialFiles.h"
#include "RageLog.h"
#include "Song.h"
#include "FileDownload.h"
#include "CreateZip.h"
#include "ScreenPrompt.h"
#include "FileDownload.h"
#include "XmlFile.h"
#include "arch/ArchHooks/ArchHooks.h"

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

void StripIgnoredSmzipFiles( vector<RString> &vsFilesInOut )
{
	for( int i=vsFilesInOut.size()-1; i>=0; i-- )
	{
		const RString &sFile = vsFilesInOut[i];

		bool bEraseThis = false;
		bEraseThis |= EndsWith( sFile, "smzip.ctl" );
		bEraseThis |= EndsWith( sFile, ".old" );
		bEraseThis |= EndsWith( sFile, "Thumbs.db" );
		bEraseThis |= EndsWith( sFile, ".DS_Store" );
		bEraseThis |= (sFile.find("CVS") != string::npos);
		bEraseThis |= (sFile.find(".svn") != string::npos);

		if( bEraseThis )
			vsFilesInOut.erase( vsFilesInOut.begin()+i );
	}
}

bool ExportDir( RString sSmzipFile, RString sDirToExport, RString &sErrorOut )
{
	//sSmzipFile = "/simple1.zip";

	RageFile f;
	// TODO: Mount Desktop/ for each OS
	if( !f.Open(sSmzipFile, RageFile::WRITE) )
	{
		sErrorOut = ssprintf( "Couldn't open %s for writing: %s", sSmzipFile.c_str(), f.GetError().c_str() );
		return false;
	}

	CreateZip zip;
	zip.Start(&f);

	vector<RString> vs;
	GetDirListingRecursive( sDirToExport, "*", vs );
	StripIgnoredSmzipFiles( vs );
	FOREACH( RString, vs, s )
	{
		if( !zip.AddFile( *s ) )
		{
			sErrorOut = ssprintf( "Couldn't add file: %s", s->c_str() );
			return false;
		}
	}

	if( !zip.Finish() )
	{
		sErrorOut = ssprintf( "Couldn't write to file %s", sSmzipFile.c_str(), f.GetError().c_str() );
		return false;
	}
	
	return true;
}

RString PublishSong( const Song *pSong )
{
	{
		FileTransfer fd;
		fd.StartDownload( "http://www.stepmania.com/api.php?action=is_logged_in", "");
		fd.Finish();
		if( fd.GetResponseCode() != 200 )
			return "Bad response code is_logged_in " + fd.GetResponseCode();
		XNode xml;
		PARSEINFO pi;
		xml.Load( fd.GetResponse(), &pi );
		if( pi.error_occur )
			return "Error parsing is_logged_in " + pi.error_string;
		if( xml.m_sName != "is_logged_in" )
			return "Unexpected response in is_logged_in";
		bool bIsLoggedIn = xml.m_sValue == "1";
		if( !bIsLoggedIn )
		{
			HOOKS->GoToURL("http://www.stepmania.com/launch.php");
			return "You must log into StepMania.com. Launching.";
		}
	}

	RString sDirToExport = pSong->GetSongDir();
	RString sPackageName = ReplaceInvalidFileNameChars( sDirToExport + ".smzip" );

	RString sSmzipFile = SpecialFiles::CACHE_DIR + "Uploads/" + sPackageName;

	RString sErrorOut;
	if( !ExportDir(sSmzipFile, sDirToExport, sErrorOut) )
		return "Failed to export '" + sDirToExport + "' to '" + sSmzipFile + "'";

	RString sUrl = "unknown url";
	{
		FileTransfer ft;
		ft.StartUpload( "http://www.stepmania.com/api.php?action=upload_song", sSmzipFile, "" );
		ft.Finish();
		if( ft.GetResponseCode() != 200 )
			return "Bad response code upload_song " + ft.GetResponseCode();
		XNode xml;
		PARSEINFO pi;
		xml.Load( ft.GetResponse(), &pi );
		if( pi.error_occur )
			return "Error parsing upload_song " + pi.error_string;
		if( xml.m_sName != "upload_song" )
			return "Unexpected response in upload_song";
		if( xml.GetChildValue("url", sUrl) )
			HOOKS->GoToURL(sUrl);
	}
		
	return "Published as '" + sUrl + "'";
}

RString ExportSong( const Song *pSong )
{
	RString sDirToExport = pSong->GetSongDir();
	RString sPackageName = ReplaceInvalidFileNameChars( sDirToExport + ".smzip" );

	RString sSmzipFile = SpecialFiles::DESKTOP_DIR + sPackageName;

	RString sErrorOut;
	if( !ExportDir(sSmzipFile, sDirToExport, sErrorOut) )
		return "Failed to export '" + sDirToExport + "' to '" + sSmzipFile + "'";

	return "Exported as '" + sSmzipFile + "'";
}

void ExportPackage::PublishSongWithUI( const Song *pSong )
{
	RString sResult = PublishSong( pSong );
	ScreenPrompt::Prompt( SM_None, sResult );
}
void ExportPackage::ExportSongWithUI( const Song *pSong )
{
	RString sResult = ExportSong( pSong );
	ScreenPrompt::Prompt( SM_None, sResult );
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
