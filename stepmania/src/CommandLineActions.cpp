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
#include "ScreenInstallOverlay.h"

const RString INSTALLER_LANGUAGES_DIR = "Themes/_Installer/Languages/";

vector<CommandLineActions::CommandLineArgs> CommandLineActions::ToProcess;

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

void CommandLineActions::Handle(LoadingWindow* pLW)
{
	CommandLineArgs args;
	for( int i=0; i<g_argc; ++i )
		args.argv.push_back( g_argv[i] );
	ToProcess.push_back( args );
	


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

