#include "global.h"
#include "ScreenInstallOverlay.h"
#include "RageFileManager.h"
#include "ScreenManager.h"
#include "Preference.h"
#include "RageLog.h"
#include "FileDownload.h"
#include "json/Value.h"
#include "JsonUtil.h"
#include "SpecialFiles.h"
class Song;
#include "SongManager.h"
#include "GameState.h"
#include "GameManager.h"
#include "CommonMetrics.h"
#include "SongManager.h"
#include "CommandLineActions.h"
#include "ScreenDimensions.h"
#include "InputEventPlus.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "LuaFunctions.h"
#include "FileDownload.h"

struct PlayAfterLaunchInfo
{
	RString sSongDir;
	RString sTheme;
	bool bAnySongChanged;
	bool bAnyThemeChanged;

	PlayAfterLaunchInfo()
	{
		bAnySongChanged = false;
		bAnyThemeChanged = false;
	}

	void OverlayWith( const PlayAfterLaunchInfo &other )
	{
		if( !other.sSongDir.empty() ) sSongDir = other.sSongDir;
		if( !other.sTheme.empty() ) sTheme = other.sTheme;
		bAnySongChanged |= other.bAnySongChanged;
		bAnyThemeChanged |= other.bAnyThemeChanged;
	}
};

static void Parse( const RString &sDir, PlayAfterLaunchInfo &out )
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

static void InstallSmzip( const RString &sZipFile, PlayAfterLaunchInfo &out )
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
	}

	RString sResult = "Success installing " + sZipFile;
	FOREACH_CONST( RString, vsFiles, sSrcFile )
	{
		RString sDestFile = *sSrcFile;
		sDestFile = sDestFile.Right( sDestFile.length() - TEMP_ZIP_MOUNT_POINT.length() );

		RString sDir, sThrowAway;
		splitpath( sDestFile, sDir, sThrowAway, sThrowAway );

		Parse( sDir, out );
		out.bAnySongChanged = true;

		FILEMAN->CreateDir( sDir );

		if( !FileCopy( *sSrcFile, sDestFile ) )
		{
			sResult = "Error extracting " + sDestFile;
			break;
		}
	}
	FILEMAN->Unmount( "zip", sZipFile, TEMP_ZIP_MOUNT_POINT );
	
	SCREENMAN->SystemMessage( sResult );
}

void InstallSmzipOsArg( const RString &sOsZipFile, PlayAfterLaunchInfo &out )
{
	SCREENMAN->SystemMessage("Installing " + sOsZipFile );

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

Preference<RString> g_sUsername( "Username", "" );
Preference<RString> g_sCookie( "Cookie", "" );
static LocalizedString LOGGED_IN_AS	( "ScreenInstallOverlay", "Logged in as %s" );
static LocalizedString PRESS_TO_LOG_IN	( "ScreenInstallOverlay", "Press %s to log in" );
static const DeviceButton g_buttonLogin = KEY_F11;
RString GetStepManiaLoginMessage()
{
	if( g_sUsername.Get().length() > 0 )
		return ssprintf(LOGGED_IN_AS.GetValue(), g_sUsername.Get().c_str());
	const RString &sKey = DeviceButtonToString( g_buttonLogin );
	return ssprintf( PRESS_TO_LOG_IN.GetValue(), sKey.c_str() );
}
LuaFunction( GetStepManiaLoginMessage, GetStepManiaLoginMessage() );

class DownloadTask;
static vector<DownloadTask*> g_pDownloadTasks;
PlayAfterLaunchInfo g_playAfterLaunchInfo;
void HandleLaunch( RString sJson );

class DownloadTask
{
protected:
	FileTransfer *m_pTransfer;
public:
	DownloadTask()
	{
		m_pTransfer = NULL;
	}
	~DownloadTask()
	{
		SAFE_DELETE(m_pTransfer);
	}
	RString GetStatus()
	{
		if( m_pTransfer == NULL )
			return "";
		else
			return m_pTransfer->GetStatus();
	}
	virtual bool UpdateAndIsFinished( float fDeltaSeconds, PlayAfterLaunchInfo &playAfterLaunchInfo ) = 0;
};
class DownloadLaunchTask : public DownloadTask
{
public:
	DownloadLaunchTask( const RString &sUrl )
	{
		m_pTransfer = new FileTransfer();
		m_pTransfer->StartDownload( sUrl, "" );
	}
	bool UpdateAndIsFinished( float fDeltaSeconds, PlayAfterLaunchInfo &playAfterLaunchInfo )
	{
		m_pTransfer->Update( fDeltaSeconds );
		if( m_pTransfer->IsFinished() )
		{
			RString sJsonResponse = m_pTransfer->GetResponse();
			SAFE_DELETE( m_pTransfer );
			HandleLaunch( sJsonResponse );
			return true;
		}
		return false;
	}
};
class DownloadPackagesTask : public DownloadTask
{
	vector<RString> m_vsQueuedPackageUrls;
	RString m_sCurrentPackageTempFile;
	PlayAfterLaunchInfo m_playAfterLaunchInfo;
public:
	DownloadPackagesTask( vector<RString> vsQueuedPackageUrls, PlayAfterLaunchInfo pali )
	{
		m_vsQueuedPackageUrls = vsQueuedPackageUrls;
		m_playAfterLaunchInfo = pali;

		if( m_vsQueuedPackageUrls.size() > 0 )
			SCREENMAN->SystemMessage( "Downloading required .smzip" );

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
		if( !m_vsQueuedPackageUrls.empty() )
		{
			RString sUrl = m_vsQueuedPackageUrls.back();
			m_vsQueuedPackageUrls.pop_back();
			m_sCurrentPackageTempFile = MakeTempFileName(sUrl);
			ASSERT(m_pTransfer == NULL);
			m_pTransfer = new FileTransfer();
			m_pTransfer->StartDownload( sUrl, m_sCurrentPackageTempFile );
		}
	}

	bool UpdateAndIsFinished( float fDeltaSeconds, PlayAfterLaunchInfo &playAfterLaunchInfo )
	{
		m_pTransfer->Update( fDeltaSeconds );
		if( m_pTransfer->IsFinished() )
		{
			SAFE_DELETE( m_pTransfer );
			InstallSmzip( m_sCurrentPackageTempFile, m_playAfterLaunchInfo );
			FILEMAN->Remove( m_sCurrentPackageTempFile );	// Harmless if this fails because download didn't finish
		}
		if( !m_vsQueuedPackageUrls.empty() )
		{
			RString sUrl = m_vsQueuedPackageUrls.back();
			m_vsQueuedPackageUrls.pop_back();
			m_sCurrentPackageTempFile = MakeTempFileName(sUrl);
			ASSERT(m_pTransfer == NULL);
			m_pTransfer = new FileTransfer();
			m_pTransfer->StartDownload( sUrl, m_sCurrentPackageTempFile );
		}

		bool bFinsihed = m_vsQueuedPackageUrls.empty() && m_pTransfer == NULL;
		if( bFinsihed )
		{
			playAfterLaunchInfo = m_playAfterLaunchInfo;
			return true;
		}
		else
		{
			return false;
		}
	}
	static RString MakeTempFileName( RString s )
	{
		return SpecialFiles::CACHE_DIR + "Downloads/" + Basename(s);
	}
};

void HandleLaunch( RString sJson )
{
	Json::Value root;
	RString sError;
	if( !JsonUtil::LoadFromString(root, sJson, sError) )
	{
		SCREENMAN->SystemMessage( sError );
		return;
	}

	// Parse the JSON response, make a list of all packages need to be downloaded.
	if( root["Username"].isString() )
	{
		g_sUsername.Set( root["Username"].asString() );
		SCREENMAN->SystemMessage("Logged in as " + g_sUsername.Get() );
	}
	if( root["Cookie"].isString() )
		g_sCookie.Set( root["Cookie"].asString() );
	Json::Value require = root["Require"];
	vector<RString> vsQueuedPackageUrls;
	PlayAfterLaunchInfo pali;
	if( require.isArray() )
	{
		for( unsigned i=0; i<require.size(); i++)
		{
			Json::Value iter = require[i];
			if( iter["Dir"].isString() )
			{
				RString sDir = iter["Dir"].asString();
				Parse( sDir, pali );
				if( DoesFileExist( sDir ) )
					continue;
			}

			RString sUri;
			if( iter["Uri"].isString() )
			{
				sUri = iter["Uri"].asString();
				vsQueuedPackageUrls.push_back( sUri );
			}
		}
	}
	if( vsQueuedPackageUrls.size() > 0 )
		g_pDownloadTasks.push_back( new DownloadPackagesTask(vsQueuedPackageUrls, pali) );
	else
		g_playAfterLaunchInfo.OverlayWith( pali );
}

static bool HandleStepManiaProtocolLaunch(const RString &arg)
{
	static const RString sBeginning = "stepmania://launch/?";	// "launch" is a dummy server name.  There's a slash after it because browsers will insert it if we leave out the slash between server and '?'
	if( !BeginsWith(arg,sBeginning) )
		return false;
	RString sQueryString = arg.Right( arg.size() - sBeginning.size() );
	RString sJson = FileTransfer::DecodeUrl(sQueryString);
	HandleLaunch(sJson);
	return true;
}

static bool IsStepManiaProtocolFile(const RString &arg)
{
	// for now, only load from the StepMania domain until the security implications of this feature are better understood.
	return BeginsWith(arg,"stepmania://beta.stepmania.com/");
}

static bool IsSmzip(const RString &arg)
{
	RString ext = GetExtension(arg);
	return ext.EqualsNoCase("smzip") || ext.EqualsNoCase("zip");
}

PlayAfterLaunchInfo DoInstalls( CommandLineActions::CommandLineArgs args )
{
	PlayAfterLaunchInfo ret;
	for( int i = 0; i<(int)args.argv.size(); i++ )
	{
		RString s = args.argv[i];
		if( HandleStepManiaProtocolLaunch(s) )
			;
		else if( IsStepManiaProtocolFile(s) )
			g_pDownloadTasks.push_back( new DownloadLaunchTask(s) );
		else if( IsSmzip(s) )
			InstallSmzipOsArg(s, ret);
	}
	return ret;
}


REGISTER_SCREEN_CLASS( ScreenInstallOverlay );

ScreenInstallOverlay::~ScreenInstallOverlay()
{
}

void ScreenInstallOverlay::Init()
{
	Screen::Init();

	m_textStatus.LoadFromFont( THEME->GetPathF("Common", "normal") );
	m_textStatus.SetHorizAlign( Actor::align_left );
	m_textStatus.SetX( SCREEN_LEFT+20 );
	m_textStatus.SetY( SCREEN_BOTTOM-20 );
	m_textStatus.SetZoom( 0.7f );
	m_textStatus.SetVertAlign( Actor::align_bottom );
	this->AddChild( &m_textStatus );
}

bool ScreenInstallOverlay::OverlayInput( const InputEventPlus &input )
{
	if( input.DeviceI.button == g_buttonLogin )
	{
		HOOKS->GoToURL("http://www.stepmania.com/launch.php");
			return true;
	}

	return false;
}


void ScreenInstallOverlay::Update( float fDeltaTime )
{
	Screen::Update(fDeltaTime);
	while( CommandLineActions::ToProcess.size() > 0 )
	{
		CommandLineActions::CommandLineArgs args = CommandLineActions::ToProcess.back();
		CommandLineActions::ToProcess.pop_back();
 		PlayAfterLaunchInfo pali2 = DoInstalls( args );
		g_playAfterLaunchInfo.OverlayWith( pali2 );
	}

	for(int i=g_pDownloadTasks.size()-1; i>=0; --i)
	{
		DownloadTask *p = g_pDownloadTasks[i];
		PlayAfterLaunchInfo pali;
		if( p->UpdateAndIsFinished( fDeltaTime, pali) )
		{
			g_playAfterLaunchInfo.OverlayWith(pali);
			SAFE_DELETE(p);
			g_pDownloadTasks.erase( g_pDownloadTasks.begin()+i );
		}
	}

	{
		vector<RString> vsMessages;
		FOREACH_CONST( DownloadTask*, g_pDownloadTasks, pDT )
		{
			vsMessages.push_back( (*pDT)->GetStatus() );			
		}
		m_textStatus.SetText( join("\n", vsMessages) );
	}

	if( g_playAfterLaunchInfo.bAnySongChanged )
		SONGMAN->Reload( false, NULL );

	if( !g_playAfterLaunchInfo.sSongDir.empty() )
	{
		Song* pSong = NULL;
		GAMESTATE->Reset();
				
		RString sInitialScreen;
		if( g_playAfterLaunchInfo.sSongDir.length() > 0 )
			pSong = SONGMAN->GetSongFromDir( g_playAfterLaunchInfo.sSongDir );
		if( pSong )
		{
			vector<const Style*> vpStyle;
			GAMEMAN->GetStylesForGame( GAMESTATE->m_pCurGame, vpStyle, false );
			GAMESTATE->m_PlayMode.Set( PLAY_MODE_REGULAR );
			GAMESTATE->m_bSideIsJoined[0] = true;
			GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
			GAMESTATE->m_pCurStyle.Set( vpStyle[0] );
			GAMESTATE->m_pCurSong.Set( pSong );
			GAMESTATE->m_pPreferredSong = pSong;
			sInitialScreen = CommonMetrics::SELECT_MUSIC_SCREEN; 
		}
		else
		{
			sInitialScreen = CommonMetrics::INITIAL_SCREEN;
		}

		g_playAfterLaunchInfo = PlayAfterLaunchInfo();

		SCREENMAN->SetNewScreen( sInitialScreen );
	}
}

/*
 * (c) 2001-2005 Chris Danford, Glenn Maynard
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
