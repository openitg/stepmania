#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenAppearanceOptions

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenAppearanceOptions.h"
#include "RageTextureManager.h"
#include "RageUtil.h"
#include "RageSounds.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"
#include "StepMania.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "AnnouncerManager.h"
#include "NoteSkinManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "SongManager.h"


enum {
	AO_LANGUAGE = 0,
	AO_ANNOUNCER,
	AO_THEME,
	AO_SKIN,
	AO_INSTRUCTIONS,
	AO_CAUTION,
	AO_DANCE_POINTS_FOR_ONI,
	AO_SELECT_GROUP,
	AO_WHEEL_SECTIONS,
	AO_TEN_FOOT_RED,
	AO_COURSE_SORT,
	AO_COURSE_MOVE_RANDOM,
	AO_SHOW_TRANSLATIONS,
	AO_SHOW_LYRICS,
	NUM_APPEARANCE_OPTIONS_LINES
};


OptionRow g_AppearanceOptionsLines[NUM_APPEARANCE_OPTIONS_LINES] = {
	OptionRow( "Language",			true ),
	OptionRow( "Announcer",			true ),
	OptionRow( "Theme",				true ),
	OptionRow( "Default\nNoteSkin",	true ),
	OptionRow( "Instructions",		true, "SKIP","SHOW"),
	OptionRow( "Caution",			true, "SKIP","SHOW"),
	OptionRow( "Oni Score\nDisplay",true, "PERCENT","DANCE POINTS"),
	OptionRow( "Song\nGroup",		true, "ALL MUSIC","CHOOSE"),
	OptionRow( "Wheel\nSections",	true, "NEVER","ALWAYS", "ABC ONLY"),
	OptionRow( "10+ foot\nIn Red",	true, "NO", "YES"),
	OptionRow( "Course\nSort",		true, "# SONGS", "AVG FEET", "TOTAL FEET", "RANKING"),
	OptionRow( "Random\nAt End",	true, "NO","YES"),
	OptionRow( "Translations",		true, "NATIVE","TRANSLITERATE"),
	OptionRow( "Lyrics",			true, "HIDE","SHOW"),
};

ScreenAppearanceOptions::ScreenAppearanceOptions() :
	ScreenOptions("ScreenAppearanceOptions",false)
{
	LOG->Trace( "ScreenAppearanceOptions::ScreenAppearanceOptions()" );

	Init( 
		INPUTMODE_TOGETHER, 
		g_AppearanceOptionsLines, 
		NUM_APPEARANCE_OPTIONS_LINES,
		true );
	m_Menu.m_MenuTimer.Disable();

	SOUND->PlayMusic( THEME->GetPathToS("ScreenAppearanceOptions music") );
}

void ScreenAppearanceOptions::ImportOptions()
{
	unsigned i;

	//
	// fill in language names
	//
	CStringArray arrayLanguages;
	THEME->GetLanguages( arrayLanguages );

	m_OptionRow[AO_LANGUAGE].choices.clear();
	for( i=0; i<arrayLanguages.size(); i++ )
	{
		m_OptionRow[AO_LANGUAGE].choices.push_back( arrayLanguages[i] ); 
	}

	// highlight currently selected announcer
	m_iSelectedOption[0][AO_LANGUAGE] = 0;
	for( i=1; i<m_OptionRow[AO_LANGUAGE].choices.size(); i++ )
		if( 0==stricmp(m_OptionRow[AO_LANGUAGE].choices[i], THEME->GetCurLanguage()) )
			m_iSelectedOption[0][AO_LANGUAGE] = i;


	//
	// fill in announcer names
	//
	CStringArray arrayAnnouncerNames;
	ANNOUNCER->GetAnnouncerNames( arrayAnnouncerNames );

	m_OptionRow[AO_ANNOUNCER].choices.clear();
	m_OptionRow[AO_ANNOUNCER].choices.push_back( "OFF" );
	for( i=0; i<arrayAnnouncerNames.size(); i++ )
	{
		m_OptionRow[AO_ANNOUNCER].choices.push_back( arrayAnnouncerNames[i] ); 
	}

	// highlight currently selected announcer
	m_iSelectedOption[0][AO_ANNOUNCER] = 0;
	for( i=1; i<m_OptionRow[AO_ANNOUNCER].choices.size(); i++ )
		if( 0==stricmp(m_OptionRow[AO_ANNOUNCER].choices[i], ANNOUNCER->GetCurAnnouncerName()) )
			m_iSelectedOption[0][AO_ANNOUNCER] = i;


	//
	// fill in theme names
	//
	CStringArray arrayThemeNames;
	THEME->GetThemeNames( arrayThemeNames );

	m_OptionRow[AO_THEME].choices.clear();
	
	for( i=0; i<arrayThemeNames.size(); i++ )
	{
		m_OptionRow[AO_THEME].choices.push_back( arrayThemeNames[i] ); 
	}

	// highlight currently selected theme
	m_iSelectedOption[0][AO_THEME] = 0;
	for( i=0; i<m_OptionRow[AO_THEME].choices.size(); i++ )
		if( 0==stricmp(m_OptionRow[AO_THEME].choices[i], THEME->GetCurThemeName()) )
			m_iSelectedOption[0][AO_THEME] = i;


	//
	// fill in skin names
	//
	CStringArray arraySkinNames;
	NOTESKIN->GetNoteSkinNames( arraySkinNames );

	m_OptionRow[AO_SKIN].choices.clear();

	for( i=0; i<arraySkinNames.size(); i++ )
	{
		arraySkinNames[i].MakeUpper();
		m_OptionRow[AO_SKIN].choices.push_back( arraySkinNames[i] ); 
	}

	// highlight currently selected skin
	PlayerOptions po;
	po.FromString( PREFSMAN->m_sDefaultModifiers );
	m_iSelectedOption[0][AO_SKIN] = 0;
	for( i=0; i<m_OptionRow[AO_SKIN].choices.size(); i++ )
		if( 0==stricmp(m_OptionRow[AO_SKIN].choices[i], po.m_sNoteSkin) )
			m_iSelectedOption[0][AO_SKIN] = i;


	m_iSelectedOption[0][AO_INSTRUCTIONS]				= PREFSMAN->m_bInstructions? 1:0;
	m_iSelectedOption[0][AO_CAUTION]					= PREFSMAN->m_bShowDontDie? 1:0;
	m_iSelectedOption[0][AO_DANCE_POINTS_FOR_ONI]		= PREFSMAN->m_bDancePointsForOni? 1:0;
	m_iSelectedOption[0][AO_SELECT_GROUP]				= PREFSMAN->m_bShowSelectGroup? 1:0;
	m_iSelectedOption[0][AO_WHEEL_SECTIONS]				= (int)PREFSMAN->m_MusicWheelUsesSections;
	m_iSelectedOption[0][AO_TEN_FOOT_RED]				= PREFSMAN->m_bTenFooterInRed? 1:0;
	m_iSelectedOption[0][AO_COURSE_SORT]				= (int)PREFSMAN->m_iCourseSortOrder;
	m_iSelectedOption[0][AO_COURSE_MOVE_RANDOM]			= PREFSMAN->m_bMoveRandomToEnd? 1:0;
	m_iSelectedOption[0][AO_SHOW_TRANSLATIONS]			= PREFSMAN->m_bShowNative? 0:1;
	m_iSelectedOption[0][AO_SHOW_LYRICS]				= PREFSMAN->m_bShowLyrics;
}

void ScreenAppearanceOptions::ExportOptions()
{
	PREFSMAN->SaveGamePrefsToDisk();
	PREFSMAN->SaveGlobalPrefsToDisk();

	int iSelectedAnnouncer = m_iSelectedOption[0][AO_ANNOUNCER];
	CString sNewAnnouncer = m_OptionRow[AO_ANNOUNCER].choices[iSelectedAnnouncer];
	if( iSelectedAnnouncer == 0 )
		sNewAnnouncer = "";
	ANNOUNCER->SwitchAnnouncer( sNewAnnouncer );

	int iSelectedTheme = m_iSelectedOption[0][AO_THEME];
	CString sNewTheme = m_OptionRow[AO_THEME].choices[iSelectedTheme];
	if( THEME->GetCurThemeName() != sNewTheme )
	{
		THEME->SwitchThemeAndLanguage( sNewTheme, THEME->GetCurLanguage() );
		ApplyGraphicOptions();	// reset graphics to apply new window title and icon

		SONGMAN->UpdateRankingCourses(); // update ranking courses
	}
	TEXTUREMAN->DoDelayedDelete();	// delete all textures that don't have references

    int iSelectedSkin = m_iSelectedOption[0][AO_SKIN];
    CString sNewSkin = m_OptionRow[AO_SKIN].choices[iSelectedSkin];

	CString sModifiers = PREFSMAN->m_sDefaultModifiers;
	PlayerOptions po;
	po.FromString( sModifiers );
	SongOptions so;
	so.FromString( sModifiers );
	po.m_sNoteSkin = sNewSkin;
	CStringArray as;
	if( po.GetString() != "" )
		as.push_back( po.GetString() );
	if( so.GetString() != "" )
		as.push_back( so.GetString() );
	PREFSMAN->m_sDefaultModifiers				= join(", ",as);

	PREFSMAN->m_bInstructions					= !!m_iSelectedOption[0][AO_INSTRUCTIONS];
	PREFSMAN->m_bShowDontDie					= !!m_iSelectedOption[0][AO_CAUTION];
	PREFSMAN->m_bShowSelectGroup				= !!m_iSelectedOption[0][AO_SELECT_GROUP];
	(int&)PREFSMAN->m_MusicWheelUsesSections	=   m_iSelectedOption[0][AO_WHEEL_SECTIONS];
	PREFSMAN->m_bShowNative						=  !m_iSelectedOption[0][AO_SHOW_TRANSLATIONS];
	PREFSMAN->m_bShowLyrics						= !!m_iSelectedOption[0][AO_SHOW_LYRICS];
	PREFSMAN->m_bDancePointsForOni				= !!m_iSelectedOption[0][AO_DANCE_POINTS_FOR_ONI];
	PREFSMAN->m_bTenFooterInRed					= !!m_iSelectedOption[0][AO_TEN_FOOT_RED];
	(int&)PREFSMAN->m_iCourseSortOrder			=   m_iSelectedOption[0][AO_COURSE_SORT];
	PREFSMAN->m_bMoveRandomToEnd				= !!m_iSelectedOption[0][AO_COURSE_MOVE_RANDOM];

	PREFSMAN->SaveGamePrefsToDisk();
	PREFSMAN->SaveGlobalPrefsToDisk();
}

void ScreenAppearanceOptions::GoToPrevState()
{
	SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
}

void ScreenAppearanceOptions::GoToNextState()
{
	PREFSMAN->SaveGamePrefsToDisk();
	GoToPrevState();
}
