#include "global.h"
#include "CommandLineActions.h"
#include "RageFile.h"
#include "RageUtil.h"
#include "IniFile.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "LuaManager.h"
#include "ProductInfo.h"
#include "DateTime.h"
#include "Foreach.h"
#include "arch/Dialog/Dialog.h"
#include "RageFileManager.h"
#include "SpecialFiles.h"
#include "FileDownload.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "Preference.h"
#include "JsonUtil.h"

const RString INSTALLER_LANGUAGES_DIR = "Themes/_Installer/Languages/";

void Parse( const RString &sDir, PlayAfterLaunchInfo &out )
{
	vector<RString> vsDirParts;
	split( sDir, "/", vsDirParts, true );
	if( vsDirParts.size() == 3 && vsDirParts[0].EqualsNoCase("Songs") )
		out.sSongDir = "/" + sDir;
	else if( vsDirParts.size() == 2 && vsDirParts[0].EqualsNoCase("Themes") )
		out.sTheme = vsDirParts[1];
}


static const RString TEMP_ZIP_MOUNT_POINT = "/@temp-zip/";
const RString TEMP_OS_MOUNT_POINT = "/@temp-os/";

void InstallSmzip( const RString &sZipFile, PlayAfterLaunchInfo &out )
{
	if( !FILEMAN->Mount( "zip", sZipFile, TEMP_ZIP_MOUNT_POINT ) )
		FAIL_M("Failed to mount " + sZipFile );

	vector<RString> vsFiles;
	{
		vector<RString> vsRawFiles;
		GetDirListingRecursive( TEMP_ZIP_MOUNT_POINT, "*", vsRawFiles);
		
		vector<RString> vsPrettyFiles;
		FOREACH_CONST( RString, vsRawFiles, s )
		{
			if( GetExtension(*s).EqualsNoCase("ctl") )
				continue;
			
			vsFiles.push_back( *s);

			RString s2 = s->Right( s->length() - TEMP_ZIP_MOUNT_POINT.length() );
			vsPrettyFiles.push_back( s2 );
		}
		sort( vsPrettyFiles.begin(), vsPrettyFiles.end() );
		if( Dialog::OKCancel( "Install these files from " + Basename(sZipFile) + "?\n\n" + join("\n", vsPrettyFiles) ) != Dialog::ok )
			return;
	}

	RString sResult = "Fake Success extracting";
	FOREACH_CONST( RString, vsFiles, sSrcFile )
	{
		RString sDestFile = *sSrcFile;
		sDestFile = sDestFile.Right( sDestFile.length() - TEMP_ZIP_MOUNT_POINT.length() );

		RString sDir, sThrowAway;
		splitpath( sDestFile, sDir, sThrowAway, sThrowAway );

		Parse( sDir, out );

		FILEMAN->CreateDir( sDir );

		if( !FileCopy( *sSrcFile, sDestFile ) )
		{
			sResult = "Error extracting " + sDestFile;
			break;
		}
	}
	FILEMAN->Unmount( "zip", sZipFile, TEMP_ZIP_MOUNT_POINT );
	
	Dialog::OK("Successfully installed " + Basename(sZipFile) );
}

void InstallSmzipOsArg( const RString &sOsZipFile, LoadingWindow *pLW, PlayAfterLaunchInfo &out )
{
	pLW->SetText("Installing " + sOsZipFile );

	RString sOsDir, sFilename, sExt;
	splitpath( sOsZipFile, sOsDir, sFilename, sExt );

	if( !FILEMAN->Mount( "dir", sOsDir, TEMP_OS_MOUNT_POINT ) )
		FAIL_M("Failed to mount " + sOsDir );
	InstallSmzip( TEMP_OS_MOUNT_POINT + sFilename + sExt, out );

	FILEMAN->Unmount( "dir", sOsDir, TEMP_OS_MOUNT_POINT );
}


struct FileCopyResult
{
	FileCopyResult( RString _sFile, RString _sComment ) : sFile(_sFile), sComment(_sComment) {}
	RString sFile, sComment;
};

Preference<RString> g_sCookie( "Cookie", "" );

static RString DownloadFile( const RString &sUri, RString sDestFile, LoadingWindow *pLW )
{
	FileTransfer fd;
	fd.StartDownload( sUri, sDestFile );
	while( true )
	{
		float fSleepSeconds = 0.1f;
		usleep( int( fSleepSeconds * 1000000.0 ) );
		RString sStatus = fd.Update( fSleepSeconds );
		pLW->SetText( sUri + "\n" + sStatus );
		if( fd.IsFinished() )
			break;
	}
	//int iResponse = fd.GetResponseCode();
	return fd.GetResponse();
}

static void HandleStepManiaProtocolArg( const RString &sUri, LoadingWindow *pLW, PlayAfterLaunchInfo &out )
{
	pLW->SetText("Installing " + sUri );

	RString sResponse = DownloadFile( sUri, "", pLW );
	Json::Value root;
	if( !JsonUtil::LoadFromStringShowErrors( root, sResponse) )
		return;

	// Parse the JSON response, make a list of all packages need to be downloaded.
	vector<RString> vsUrls;
	{
		if( root["Cookie"].isString() )
			g_sCookie.Set( root["Cookie"].asString() );
		Json::Value require = root["Require"];
		if( require.isArray() )
		{
			for( unsigned i=0; i<require.size(); i++)
			{
				Json::Value iter = require[i];
				if( iter["Dir"].isString() )
				{
					RString sDir = iter["Dir"].asString();
					Parse( sDir, out );
					if( DoesFileExist( sDir ) )
						continue;
				}

				RString sUri;
				if( iter["Uri"].isString() )
				{
					sUri = iter["Uri"].asString();
					vsUrls.push_back( sUri );
				}
			}
		}
	}	

	/*
	{
		// TODO: Validate that this zip contains files for this version of StepMania

		bool bFileExists = DoesFileExist( SpecialFiles::PACKAGES_DIR + sFilename + sExt );
		if( FileCopy( TEMP_MOUNT_POINT + sFilename + sExt, SpecialFiles::PACKAGES_DIR + sFilename + sExt ) )
			vSucceeded.push_back( FileCopyResult(*s,bFileExists ? "overwrote existing file" : "") );
		else
			vFailed.push_back( FileCopyResult(*s,ssprintf("error copying file to '%s'",sOsDir.c_str())) );

	}
	*/


	FOREACH_CONST( RString, vsUrls, s )
	{
		RString sDestFile = SpecialFiles::CACHE_DIR + "Downloads/" + Basename(*s);
		if( DownloadFile( *s, sDestFile, pLW ) )
		{
			InstallSmzip( sDestFile, out );
		}
		FILEMAN->Remove( sDestFile );	// Harmless if this fails because download didn't finish
	}
}

static void DoNsis()
{
	RageFile out;
	if( !out.Open( "nsis_strings_temp.inc", RageFile::WRITE ) )
		RageException::Throw( "Error opening file for write." );
	
	vector<RString> vs;
	GetDirListing( INSTALLER_LANGUAGES_DIR + "*.ini", vs, false, false );
	FOREACH_CONST( RString, vs, s )
	{
		RString sThrowAway, sLangCode;
		splitpath( *s, sThrowAway, sLangCode, sThrowAway );
		const LanguageInfo *pLI = GetLanguageInfo( sLangCode );
		
		RString sLangNameUpper = pLI->szEnglishName;
		sLangNameUpper.MakeUpper();
		
		IniFile ini;
		if( !ini.ReadFile( INSTALLER_LANGUAGES_DIR + *s ) )
			RageException::Throw( "Error opening file for read." );
		FOREACH_CONST_Child( &ini, child )
		{
			FOREACH_CONST_Attr( child, attr )
			{
				RString sName = attr->first;
				RString sValue = attr->second;
				sValue.Replace( "\\n", "$\\n" );
				RString sLine = ssprintf( "LangString %s ${LANG_%s} \"%s\"", sName.c_str(), sLangNameUpper.c_str(), sValue.c_str() );
				out.PutLine( sLine );
			}
		}
	}
}

bool IsStepManiaProtocol(RString arg)
{
	// for now, only load from the StepMania domain until the security implications of this feature are better understood.
	return BeginsWith(arg,"stepmania://beta.stepmania.com/");
}

bool IsSmzip(RString arg)
{
	RString ext = GetExtension(arg);
	return ext.EqualsNoCase("smzip") || ext.EqualsNoCase("zip");
}

PlayAfterLaunchInfo DoInstalls(LoadingWindow *pLW)
{
	PlayAfterLaunchInfo ret;
	for( int i = 1; i<g_argc; ++i )
	{
		RString s = g_argv[i];
		if( IsStepManiaProtocol(s) )
			HandleStepManiaProtocolArg(s, pLW, ret);
		else if( IsSmzip(s) )
			InstallSmzipOsArg(s, pLW, ret);
	}
	return ret;
}


static void DoLuaInformation()
{
	// TODO: fix me
	/*
	XNode *pNode = LuaHelpers::GetLuaInformation();
	pNode->AppendAttr( "xmlns", "http://www.stepmania.com" );
	pNode->AppendAttr( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
	pNode->AppendAttr( "xsi:schemaLocation", "http://www.stepmania.com Lua.xsd" );

	pNode->AppendChild( "Version", PRODUCT_ID_VER );
	pNode->AppendChild( "Date", DateTime::GetNowDate().GetString() );

	XmlFileUtil::SaveToFile( pNode, "Lua.xml", "Lua.xsl" );

	delete pNode;
	*/
}

PlayAfterLaunchInfo CommandLineActions::Handle(LoadingWindow* pLW)
{
	PlayAfterLaunchInfo ret = DoInstalls(pLW);
	bool bExitAfter = false;
	if( GetCommandlineArgument("ExportNsisStrings") )
	{
		DoNsis();
		bExitAfter = true;
	}
	if( GetCommandlineArgument("ExportLuaInformation") )
	{
		DoLuaInformation();
		bExitAfter = true;
	}
	if( bExitAfter )
		exit(0);
	return ret;
}


/*
 * (c) 2006 Chris Danford, Steve Checkoway
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

