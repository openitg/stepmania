#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenSongOptions

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenSongOptions.h"
#include "RageUtil.h"
#include "RageSoundManager.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "RageLog.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "PrefsManager.h"

#define PREV_SCREEN( play_mode )		THEME->GetMetric ("ScreenSongOptions","PrevScreen"+Capitalize(PlayModeToString(play_mode)))
#define NEXT_SCREEN( play_mode )		THEME->GetMetric ("ScreenSongOptions","NextScreen"+Capitalize(PlayModeToString(play_mode)))

enum {
	SO_LIFE = 0,
	SO_DRAIN,
	SO_BAT_LIVES,
	SO_FAIL,
	SO_ASSIST,
	SO_RATE,
	SO_AUTOSYNC,
	NUM_SONG_OPTIONS_LINES
};

OptionRow g_SongOptionsLines[NUM_SONG_OPTIONS_LINES] = {
	OptionRow( "Life\nType",	"BAR","BATTERY" ),	
	OptionRow( "Bar\nDrain",	"NORMAL","NO RECOVER","SUDDEN DEATH" ),	
	OptionRow( "Bat\nLives",	"1","2","3","4","5","6","7","8","9","10" ),	
	OptionRow( "Fail",			"ARCADE","END OF SONG","OFF" ),	
	OptionRow( "Assist\nTick",	"OFF", "ON" ),
	OptionRow( "Rate",			"x0.3","x0.5","x0.7","x0.8","x0.9","x1.0","x1.1","x1.2","x1.3","x1.4","x1.5" ),	
	OptionRow( "Auto\nAdjust",	"OFF", "ON" ),	
};

/* Get the next screen we'll go to when finished. */
CString ScreenSongOptions::GetNextScreen()
{
	return NEXT_SCREEN(GAMESTATE->m_PlayMode);
}


ScreenSongOptions::ScreenSongOptions() :
	ScreenOptions("ScreenSongOptions",true)
{
	LOG->Trace( "ScreenSongOptions::ScreenSongOptions()" );

	Init( INPUTMODE_BOTH, 
		g_SongOptionsLines, 
		NUM_SONG_OPTIONS_LINES,
		false, false );

	/* If we're coming in from "press start for more options", we need a different
	 * fade in. XXX: this is a hack */
	if(PREFSMAN->m_ShowSongOptions == PrefsManager::ASK)
	{
		m_Menu.m_In.Load( THEME->GetPathToB("ScreenSongOptions option in") );
		m_Menu.m_In.StartTransitioning();
	}
}

void ScreenSongOptions::ImportOptions()
{
	SongOptions &so = GAMESTATE->m_SongOptions;

	m_iSelectedOption[0][SO_LIFE] = so.m_LifeType;
	m_iSelectedOption[0][SO_BAT_LIVES] = so.m_iBatteryLives-1;
	m_iSelectedOption[0][SO_FAIL] = so.m_FailType;
	m_iSelectedOption[0][SO_ASSIST] = so.m_bAssistTick;
	m_iSelectedOption[0][SO_AUTOSYNC] = so.m_bAutoSync;

	if(		 so.m_fMusicRate == 0.3f )		m_iSelectedOption[0][SO_RATE] = 0;
	if(		 so.m_fMusicRate == 0.5f )		m_iSelectedOption[0][SO_RATE] = 1;
	if(		 so.m_fMusicRate == 0.7f )		m_iSelectedOption[0][SO_RATE] = 2;
	else if( so.m_fMusicRate == 0.8f )		m_iSelectedOption[0][SO_RATE] = 3;
	else if( so.m_fMusicRate == 0.9f )		m_iSelectedOption[0][SO_RATE] = 4;
	else if( so.m_fMusicRate == 1.0f )		m_iSelectedOption[0][SO_RATE] = 5;
	else if( so.m_fMusicRate == 1.1f )		m_iSelectedOption[0][SO_RATE] = 6;
	else if( so.m_fMusicRate == 1.2f )		m_iSelectedOption[0][SO_RATE] = 7;
	else if( so.m_fMusicRate == 1.3f )		m_iSelectedOption[0][SO_RATE] = 8;
	else if( so.m_fMusicRate == 1.4f )		m_iSelectedOption[0][SO_RATE] = 9;
	else if( so.m_fMusicRate == 1.5f )		m_iSelectedOption[0][SO_RATE] = 10;
	else									m_iSelectedOption[0][SO_RATE] = 5;
}

void ScreenSongOptions::ExportOptions()
{
	SongOptions &so = GAMESTATE->m_SongOptions;

	so.m_LifeType = (SongOptions::LifeType)m_iSelectedOption[0][SO_LIFE];
	so.m_DrainType = (SongOptions::DrainType)m_iSelectedOption[0][SO_DRAIN];
	so.m_iBatteryLives = m_iSelectedOption[0][SO_BAT_LIVES]+1;
	so.m_FailType =	(SongOptions::FailType)m_iSelectedOption[0][SO_FAIL];
	so.m_bAssistTick = !!m_iSelectedOption[0][SO_ASSIST];
	so.m_bAutoSync = !!m_iSelectedOption[0][SO_AUTOSYNC];

	switch( m_iSelectedOption[0][SO_RATE] )
	{
	case 0:	so.m_fMusicRate = 0.3f;	break;
	case 1:	so.m_fMusicRate = 0.5f;	break;
	case 2:	so.m_fMusicRate = 0.7f;	break;
	case 3:	so.m_fMusicRate = 0.8f;	break;
	case 4:	so.m_fMusicRate = 0.9f;	break;
	case 5:	so.m_fMusicRate = 1.0f;	break;
	case 6:	so.m_fMusicRate = 1.1f;	break;
	case 7:	so.m_fMusicRate = 1.2f;	break;
	case 8:	so.m_fMusicRate = 1.3f;	break;
	case 9:	so.m_fMusicRate = 1.4f;	break;
	case 10:so.m_fMusicRate = 1.5f;	break;
	default:	ASSERT( false );
	}
}

void ScreenSongOptions::GoToPrevState()
{
	if( GAMESTATE->m_bEditing )
		SCREENMAN->PopTopScreen( SM_None );
	else
		SCREENMAN->SetNewScreen( PREV_SCREEN(GAMESTATE->m_PlayMode) );
}

void ScreenSongOptions::GoToNextState()
{
	if( GAMESTATE->m_bEditing )
		SCREENMAN->PopTopScreen();
	else
		SCREENMAN->SetNewScreen( NEXT_SCREEN(GAMESTATE->m_PlayMode) );
}




