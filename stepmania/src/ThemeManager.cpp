#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: ThemeManager

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ThemeManager.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "RageException.h"
#include "RageTimer.h"
#include "GameState.h"
#include "GameDef.h"
#include "IniFile.h"
#include "RageTimer.h"
#include "Font.h"
#include "FontCharAliases.h"

using namespace std;

ThemeManager*	THEME = NULL;	// global object accessable from anywhere in the program


const CString BASE_THEME_NAME = "default";
const CString THEMES_DIR  = "Themes\\";

ThemeManager::ThemeManager()
{
	m_pIniMetrics = new IniFile;

	/* Update the metric cache on the first call to GetMetric. */
	m_fNextReloadTicks = 0;

	m_sCurThemeName = BASE_THEME_NAME;	// Use the base theme for now.  It's up to PrefsManager to change this.

	CStringArray arrayThemeNames;
	GetAllThemeNames( arrayThemeNames );
}

ThemeManager::~ThemeManager()
{
	delete m_pIniMetrics;
}

void ThemeManager::GetAllThemeNames( CStringArray& AddTo )
{
	GetDirListing( THEMES_DIR+"\\*", AddTo, true );
	
	// strip out the folder called "CVS"
	for( CStringArray::iterator i=AddTo.begin(); i != AddTo.end(); ++i )
	{
		if( !i->CompareNoCase("cvs") ) {
			AddTo.erase(i, i+1);
			break;
		}
	}
}

void ThemeManager::GetThemeNamesForCurGame( CStringArray& AddTo )
{
	GetAllThemeNames( AddTo );

	/*
	// strip out announcers that don't have the current game name in them
	CString sGameName = GAMESTATE->GetCurrentGameDef()->m_szName;
	sGameName.MakeLower();
	for( unsigned i=AddTo.size()-1; i>=0; i-- )
	{
		CString sLowercaseVer = AddTo[i];
		sLowercaseVer.MakeLower();
		if( sLowercaseVer.Find(sGameName)==-1 )
			AddTo.RemoveAt(i);
	}
	*/
}

bool ThemeManager::DoesThemeExist( CString sThemeName )
{
	CStringArray asThemeNames;	
	GetAllThemeNames( asThemeNames );
	for( unsigned i=0; i<asThemeNames.size(); i++ )
	{
		if( !sThemeName.CompareNoCase(asThemeNames[i]) )
			return true;
	}
	return false;
}

void ThemeManager::SwitchTheme( CString sThemeName )
{
	if( !DoesThemeExist(sThemeName) )
		m_sCurThemeName = BASE_THEME_NAME;
	else
		m_sCurThemeName = sThemeName;

	// update hashes for metrics files
	m_uHashForCurThemeMetrics = GetHashForFile( GetMetricsPathFromName(m_sCurThemeName) );
	m_uHashForBaseThemeMetrics = GetHashForFile( GetMetricsPathFromName(BASE_THEME_NAME) );

	// read new metrics.  First read base metrics, then read cur theme's metrics, overriding base theme
	m_pIniMetrics->Reset();
	m_pIniMetrics->SetPath( GetMetricsPathFromName(BASE_THEME_NAME) );
	m_pIniMetrics->ReadFile();
	m_pIniMetrics->SetPath( GetMetricsPathFromName(m_sCurThemeName) );
	m_pIniMetrics->ReadFile();
}

CString ThemeManager::GetThemeDirFromName( CString sThemeName )
{
	return THEMES_DIR + sThemeName + "\\";
}

CString ThemeManager::GetPathToFont( CString sAssetCategory, CString sFileName ) 
{
try_element_again:
	sAssetCategory.MakeLower();
	sFileName.MakeLower();

	const CString sCurrentThemeDir = GetThemeDirFromName( m_sCurThemeName );
	const CString sDefaultThemeDir = GetThemeDirFromName( BASE_THEME_NAME );	

	CStringArray asPossibleElementFilePaths;

	///////////////////////////////////////
	// Search both the current theme and the default theme dirs for this element
	///////////////////////////////////////

	GetDirListing( sCurrentThemeDir + "Fonts\\"+sFileName + "*",
					asPossibleElementFilePaths, false, true );

	/* Weed out false matches.  (For example, this gets rid of "normal2" when
	 * we're really looking for "normal".) */
	int i;
	for(i = 0; i < int(asPossibleElementFilePaths.size()); ) {
		if(Font::GetFontName(sFileName) != Font::GetFontName(asPossibleElementFilePaths[i]))
			asPossibleElementFilePaths.erase(asPossibleElementFilePaths.begin()+i);
		else i++;
	}

	if(asPossibleElementFilePaths.empty())
		GetDirListing( sDefaultThemeDir + "Fonts\\"+ sFileName + "*",
						asPossibleElementFilePaths, false, true );

	for(i = 0; i < int(asPossibleElementFilePaths.size()); ) {
		if(Font::GetFontName(sFileName) != Font::GetFontName(asPossibleElementFilePaths[i]))
			asPossibleElementFilePaths.erase(asPossibleElementFilePaths.begin()+i);
		else i++;
	}

	/* If it's empty, we found nothing. */
	if( asPossibleElementFilePaths.empty() )
	{
#ifdef _DEBUG
		CString sMessage = ssprintf("The theme element %s/%s is missing.",sAssetCategory.GetString(),sFileName.GetString());
		switch( MessageBox(NULL, sMessage, "ThemeManager", MB_ABORTRETRYIGNORE ) )
		{
		case IDRETRY:
			goto try_element_again;
			break;
		case IDABORT:
#endif
			RageException::Throw( "Theme element '%s/%s' could not be found in '%s' or '%s'.", 
				sAssetCategory.GetString(),
				sFileName.GetString(), 
				GetThemeDirFromName(m_sCurThemeName).GetString(), 
				GetThemeDirFromName(BASE_THEME_NAME).GetString() );
#ifdef _DEBUG
		case IDIGNORE:
			LOG->Warn( 
				"Theme element '%s/%s' could not be found in '%s' or '%s'.", 
				sAssetCategory.GetString(),
				sFileName.GetString(), 
				GetThemeDirFromName(m_sCurThemeName), 
				GetThemeDirFromName(BASE_THEME_NAME) );
			return GetPathTo( sAssetCategory, "_missing" );
			break;
		}
#endif
	}

	if( asPossibleElementFilePaths[0].GetLength() > 5  &&  asPossibleElementFilePaths[0].Right(5) == "redir" )	// this is a redirect file
	{
		CString sNewFilePath = DerefRedir(asPossibleElementFilePaths[0]);
		
		/* backwards-compatibility hack */
		if( sAssetCategory == "fonts" )
			sNewFilePath.Replace(" 16x16.png", "");

		/* Search again.  For example, themes/default/Fonts/foo might redir
		 * to "Hello"; but "Hello" might be overridden in themes/hot pink/Fonts/Hello. */

		/* Strip off the path. */
		unsigned pos = sNewFilePath.find_last_of("/\\");
		if(pos != sNewFilePath.npos) sNewFilePath = sNewFilePath.substr(pos+1);
		sFileName = sNewFilePath;

		/* XXX check for loops */
		goto try_element_again;
	}

	return asPossibleElementFilePaths[0];
}
CString ThemeManager::GetPathToOptional( CString sAssetCategory, CString sFileName ) 
{
try_element_again:
	sAssetCategory.MakeLower();
	sFileName.MakeLower();

	const CString sCurrentThemeDir = GetThemeDirFromName( m_sCurThemeName );
	const CString sDefaultThemeDir = GetThemeDirFromName( BASE_THEME_NAME );	

	CStringArray asPossibleElementFilePaths;

	///////////////////////////////////////
	// Search both the current theme and the default theme dirs for this element
	///////////////////////////////////////
	static const char *graphic_masks[] = {
		"*.sprite", "*.png", "*.jpg", "*.bmp", "*.gif", "*.redir",
		"*.avi", "*.mpg", "*.mpeg", NULL
	};
	static const char *sound_masks[] = { ".set", ".mp3", ".ogg", ".wav", ".redir", NULL };
	static const char *font_masks[] = { "*.png", ".redir", NULL };
	static const char *numbers_masks[] = { "*.png", ".redir", NULL };
	static const char *bganimations_masks[] = { ".", ".redir", NULL };
	static const char *blank_mask[] = { "", NULL };
	const char **asset_masks = NULL;
	if( sAssetCategory == "graphics" ) asset_masks = graphic_masks;
	else if( sAssetCategory == "sounds" ) asset_masks = sound_masks;
//	else if( sAssetCategory == "fonts" ) asset_masks = font_masks;
	else if( sAssetCategory == "numbers" ) asset_masks = numbers_masks;
	else if( sAssetCategory == "bganimations" ) asset_masks = bganimations_masks;
	else ASSERT(0); // Unknown theme asset category

	/* If the theme asset name has an extension, don't add
	 * a mask.  This should only happen with redirs. */
	if(sFileName.find_last_of('.') != sFileName.npos)
		asset_masks = blank_mask;

	int i;
	CString path;

	for(i = 0; asset_masks[i]; ++i)
	{
		path = sCurrentThemeDir;
		GetDirListing( path + sAssetCategory+"\\"+sFileName + asset_masks[i],
						asPossibleElementFilePaths, false, true );
	}

	if(asPossibleElementFilePaths.empty())
	for(i = 0; asset_masks[i]; ++i)
	{
		path = sDefaultThemeDir;

		GetDirListing( path + sAssetCategory+"\\"+sFileName + asset_masks[i],
						asPossibleElementFilePaths, false, true );
	}

	if(asPossibleElementFilePaths.empty())
		return "";

	if( asPossibleElementFilePaths[0].GetLength() > 5  &&  asPossibleElementFilePaths[0].Right(5) == "redir" )	// this is a redirect file
	{
		CString sNewFilePath = DerefRedir(asPossibleElementFilePaths[0]);
		
		if( sAssetCategory == "fonts" )
		{
			/* backwards-compatibility hack */
			if( sAssetCategory == "fonts" )
				sNewFilePath.Replace(" 16x16.png", "");
		}
		else if( !DoesFileExist(sNewFilePath) ) 
		{
			CString message = ssprintf(
						"The redirect '%s' points to the file '%s', which does not exist."
						"Verify that this redirect is correct.",
						asPossibleElementFilePaths[0].GetString(), sNewFilePath.GetString());

#ifdef _DEBUG
			if( MessageBox(NULL, message.GetString(), "ThemeManager", MB_RETRYCANCEL ) == IDRETRY)
				goto try_element_again;
#endif
			RageException::Throw( "%s", message.GetString() ); 
		}

		/* Search again.  For example, themes/default/Fonts/foo might redir
		 * to "Hello"; but "Hello" might be overridden in themes/hot pink/Fonts/Hello. */

		/* Strip off the path. */
		unsigned pos = sNewFilePath.find_last_of("/\\");
		if(pos != sNewFilePath.npos) sNewFilePath = sNewFilePath.substr(pos+1);
		sFileName = sNewFilePath;

		/* XXX check for loops */
		goto try_element_again;
	}

	return asPossibleElementFilePaths[0];
}

CString ThemeManager::GetPathTo( CString sAssetCategory, CString sFileName ) 
{
	if(sAssetCategory == "Fonts")
		return GetPathToFont(sAssetCategory, sFileName);

#ifdef _DEBUG
try_element_again:
#endif

	CString ret = GetPathToOptional(sAssetCategory, sFileName);

	/* If it's empty, we found nothing. */
	if( !ret.empty() )
		return ret;

#ifdef _DEBUG
	CString sMessage = ssprintf("The theme element %s/%s is missing.",sAssetCategory.GetString(),sFileName.GetString());
	switch( MessageBox(NULL, sMessage, "ThemeManager", MB_ABORTRETRYIGNORE ) )
	{
	case IDRETRY:
		goto try_element_again;
		break;
	case IDABORT:
		goto abort;
	case IDIGNORE:
		LOG->Warn( 
			"Theme element '%s/%s' could not be found in '%s' or '%s'.", 
			sAssetCategory.GetString(),
			sFileName.GetString(), 
			GetThemeDirFromName(m_sCurThemeName).GetString(), 
			GetThemeDirFromName(BASE_THEME_NAME).GetString() );
		return GetPathTo( sAssetCategory, "_missing" );
	}
#endif

abort:
	RageException::Throw( "Theme element '%s/%s' could not be found in '%s' or '%s'.", 
		sAssetCategory.GetString(),
		sFileName.GetString(), 
		GetThemeDirFromName(m_sCurThemeName).GetString(), 
		GetThemeDirFromName(BASE_THEME_NAME).GetString() );
}


CString ThemeManager::GetMetricsPathFromName( CString sThemeName )
{
	return GetThemeDirFromName( sThemeName ) + "metrics.ini";
}

CString ThemeManager::GetMetric( CString sClassName, CString sValueName )
{
#ifdef _DEBUG
try_metric_again:
#endif
	CString sCurMetricPath = GetMetricsPathFromName(m_sCurThemeName);
	CString sDefaultMetricPath = GetMetricsPathFromName(BASE_THEME_NAME);

	// Is our metric cache out of date?
	// XXX: GTSS wraps every ~40 days.  Need a better way to handler timers like this.
	if (RageTimer::GetTimeSinceStart() >= m_fNextReloadTicks)
	{
		m_fNextReloadTicks = RageTimer::GetTimeSinceStart()+1.0f;
		if( m_uHashForCurThemeMetrics != GetHashForFile(sCurMetricPath)  ||
			m_uHashForBaseThemeMetrics != GetHashForFile(sDefaultMetricPath) )
		{
			SwitchTheme(m_sCurThemeName);	// force a reload of the metrics cache
		}
	}

	CString sValue;
	if( m_pIniMetrics->GetValue(sClassName,sValueName,sValue) )
	{
		sValue.Replace("::","\n");	// "::" means newline since you can't use line breaks in an ini file.

		/* XXX: add a parameter to turn this off if there are some metrics where
		 * we don't want markers */
		FontCharAliases::ReplaceMarkers(sValue);

		return sValue;
	}

#ifdef _DEBUG
	if( IDRETRY == MessageBox(NULL,ssprintf("The theme metric %s-%s is missing.  Correct this and click Retry, or Cancel to break.",sClassName.GetString(),sValueName.GetString()),"ThemeManager",MB_RETRYCANCEL ) )
		goto try_metric_again;
#endif

	RageException::Throw( "Theme metric '%s : %s' could not be found in '%s' or '%s'.", 
		sClassName.GetString(),
		sValueName.GetString(),
		sCurMetricPath.GetString(), 
		sDefaultMetricPath.GetString()
		);
}

int ThemeManager::GetMetricI( CString sClassName, CString sValueName )
{
	return atoi( GetMetric(sClassName,sValueName) );
}

float ThemeManager::GetMetricF( CString sClassName, CString sValueName )
{
	return (float)atof( GetMetric(sClassName,sValueName) );
}

bool ThemeManager::GetMetricB( CString sClassName, CString sValueName )
{
	return atoi( GetMetric(sClassName,sValueName) ) != 0;
}

RageColor ThemeManager::GetMetricC( CString sClassName, CString sValueName )
{
	float r=1,b=1,g=1,a=1;	// initialize in case sscanf fails
	CString sValue = GetMetric(sClassName,sValueName);
	char szValue[40];
	strncpy( szValue, sValue, 39 );
	int result = sscanf( szValue, "%f,%f,%f,%f", &r, &g, &b, &a );
	if( result != 4 )
	{
		LOG->Warn( "The color value '%s' for NoteSkin metric '%s : %s' is invalid.", szValue, sClassName.GetString(), sValueName.GetString() );
		ASSERT(0);
	}

	return RageColor(r,g,b,a);
}
