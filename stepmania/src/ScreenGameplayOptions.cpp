#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenGameplayOptions

 Desc: See header.

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Chris Gomez
-----------------------------------------------------------------------------
*/

#include "ScreenGameplayOptions.h"
#include "RageUtil.h"
#include "RageSoundManager.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"
#include "StepMania.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "ThemeManager.h"


enum {
	GO_SOLO_SINGLE,
	GO_HIDDEN_SONGS,
	GO_EASTER_EGGS,
	GO_MARVELOUS,
	GO_PICK_EXTRA_STAGE,
	GO_UNLOCK_SYSTEM,
	NUM_GAMEPLAY_OPTIONS_LINES
};

OptionRow g_GameplayOptionsLines[NUM_GAMEPLAY_OPTIONS_LINES] = {
	OptionRow( "Solo\nSingles",			"OFF","ON" ),
	OptionRow( "Hidden\nSongs",			"OFF","ON" ),
	OptionRow( "Easter\nEggs",			"OFF","ON" ),
	OptionRow( "Marvelous\nTiming",		"NEVER","COURSES ONLY","ALWAYS" ),
	OptionRow( "Pick Extra\nStage",		"OFF","ON" ),
	OptionRow( "Unlock\nSystem",		"OFF","ON" )
};

ScreenGameplayOptions::ScreenGameplayOptions() :
	ScreenOptions("ScreenGameplayOptions",false)
{
	LOG->Trace( "ScreenGameplayOptions::ScreenGameplayOptions()" );

	Init( 
		INPUTMODE_BOTH, 
		g_GameplayOptionsLines, 
		NUM_GAMEPLAY_OPTIONS_LINES,
		false, true );
	m_Menu.m_MenuTimer.Disable();

	SOUNDMAN->PlayMusic( THEME->GetPathToS("ScreenMachineOptions music") );
}

void ScreenGameplayOptions::ImportOptions()
{
	m_iSelectedOption[0][GO_SOLO_SINGLE]			= PREFSMAN->m_bSoloSingle ? 1:0;
	m_iSelectedOption[0][GO_HIDDEN_SONGS]			= PREFSMAN->m_bHiddenSongs ? 1:0;
	m_iSelectedOption[0][GO_EASTER_EGGS]			= PREFSMAN->m_bEasterEggs ? 1:0;
	m_iSelectedOption[0][GO_MARVELOUS]				= PREFSMAN->m_iMarvelousTiming;
	m_iSelectedOption[0][GO_PICK_EXTRA_STAGE]		= PREFSMAN->m_bPickExtraStage? 1:0;
	m_iSelectedOption[0][GO_UNLOCK_SYSTEM]			= PREFSMAN->m_bUseUnlockSystem? 1:0;
}

void ScreenGameplayOptions::ExportOptions()
{
	PREFSMAN->m_bSoloSingle				= m_iSelectedOption[0][GO_SOLO_SINGLE] == 1;
	PREFSMAN->m_bHiddenSongs			= m_iSelectedOption[0][GO_HIDDEN_SONGS]	== 1;
	PREFSMAN->m_bEasterEggs			= m_iSelectedOption[0][GO_EASTER_EGGS] == 1;
	PREFSMAN->m_iMarvelousTiming	= m_iSelectedOption[0][GO_MARVELOUS];
	PREFSMAN->m_bPickExtraStage		= m_iSelectedOption[0][GO_PICK_EXTRA_STAGE] == 1;
	PREFSMAN->m_bUseUnlockSystem	= m_iSelectedOption[0][GO_UNLOCK_SYSTEM]	== 1;
}

void ScreenGameplayOptions::GoToPrevState()
{
	SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
}

void ScreenGameplayOptions::GoToNextState()
{
	PREFSMAN->SaveGlobalPrefsToDisk();
	GoToPrevState();
}

