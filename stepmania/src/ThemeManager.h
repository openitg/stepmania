#pragma once
/*
-----------------------------------------------------------------------------
 Class: ThemeManager

 Desc: Manages which graphics and sounds are chosed to load.  Every time 
	a sound or graphic is loaded, it gets the path from the ThemeManager.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "RageUtil.h"


enum ThemeElement { 
	GRAPHIC_ALL_MUSIC_BANNER,
	GRAPHIC_ARROWS_LEFT,
	GRAPHIC_ARROWS_RIGHT,
	GRAPHIC_CAUTION,
	GRAPHIC_GAMEPLAY_CLOSING_STAR,
	GRAPHIC_DANCER_P1,
	GRAPHIC_DANCER_P2,
	GRAPHIC_GAMEPLAY_BOTTOM_FRAME,
	GRAPHIC_GAMEPLAY_DIFFICULTY_FRAME,
	GRAPHIC_GAMEPLAY_TOP_FRAME,
	GRAPHIC_DIFFICULTY_ICONS,
	GRAPHIC_EDIT_BACKGROUND,
	GRAPHIC_EDIT_SNAP_INDICATOR,
	GRAPHIC_MUSIC_SCROLL_BACKGROUND,
	GRAPHIC_FALLBACK_BACKGROUND,
	GRAPHIC_FALLBACK_BANNER,
	GRAPHIC_FALLBACK_CD_TITLE,
	GRAPHIC_GAME_OPTIONS_BACKGROUND,
	GRAPHIC_GAME_OPTIONS_TOP_EDGE,
	GRAPHIC_GAMEPLAY_CLEARED,
	GRAPHIC_GAMEPLAY_COMBO,		
	GRAPHIC_GAMEPLAY_DANGER_BACKGROUND,
	GRAPHIC_GAMEPLAY_DANGER_TEXT,
	GRAPHIC_GAMEPLAY_FAILED,	
	GRAPHIC_GAMEPLAY_JUDGEMENT,	
	GRAPHIC_GAMEPLAY_READY,	
	GRAPHIC_GRAPHIC_OPTIONS_BACKGROUND,
	GRAPHIC_GRAPHIC_OPTIONS_TOP_EDGE,
	GRAPHIC_GAMEPLAY_HERE_WE_GO,	
	GRAPHIC_KEEP_ALIVE,	
	GRAPHIC_LIFEMETER_FRAME,
	GRAPHIC_LIFEMETER_PILLS,
	GRAPHIC_MENU_BOTTOM_EDGE,
	GRAPHIC_MUSIC_SORT_ICONS,
	GRAPHIC_MUSIC_STATUS_ICONS,
	GRAPHIC_GAMEPLAY_OPENING_STAR,
	GRAPHIC_PAD_DOUBLE,
	GRAPHIC_PAD_SINGLE,	
	GRAPHIC_PAD_SOLO,
	GRAPHIC_PLAYER_OPTIONS_BACKGROUND,
	GRAPHIC_PLAYER_OPTIONS_TOP_EDGE,
	GRAPHIC_RESULTS_BACKGROUND,
	GRAPHIC_RESULTS_BANNER_FRAME,
	GRAPHIC_RESULTS_BONUS_FRAME_P1,
	GRAPHIC_RESULTS_BONUS_FRAME_P2,
	GRAPHIC_RESULTS_GRADE_FRAME,
	GRAPHIC_RESULTS_GRADES,	
	GRAPHIC_RESULTS_JUDGE_LABELS,
	GRAPHIC_RESULTS_SCORE_LABELS,
	GRAPHIC_RESULTS_TOP_EDGE,
	GRAPHIC_SCROLLBAR_PARTS,		
	GRAPHIC_SELECT_DIFFICULTY_ARROW_P1,
	GRAPHIC_SELECT_DIFFICULTY_ARROW_P2,
	GRAPHIC_SELECT_DIFFICULTY_BACKGROUND,
	GRAPHIC_SELECT_DIFFICULTY_EASY_HEADER,
	GRAPHIC_SELECT_DIFFICULTY_EASY_PICTURE,
	GRAPHIC_SELECT_DIFFICULTY_EXPLANATION,
	GRAPHIC_SELECT_DIFFICULTY_HARD_HEADER,
	GRAPHIC_SELECT_DIFFICULTY_HARD_PICTURE,
	GRAPHIC_SELECT_DIFFICULTY_MEDIUM_HEADER,
	GRAPHIC_SELECT_DIFFICULTY_MEDIUM_PICTURE,
	GRAPHIC_SELECT_DIFFICULTY_OK,
	GRAPHIC_SELECT_DIFFICULTY_TOP_EDGE,
	GRAPHIC_SELECT_GAME_BACKGROUND,
	GRAPHIC_SELECT_GAME_TOP_EDGE,
	GRAPHIC_SELECT_GROUP_BACKGROUND,
	GRAPHIC_SELECT_GROUP_BUTTON,
	GRAPHIC_SELECT_GROUP_CONTENTS_HEADER,
	GRAPHIC_SELECT_GROUP_EXPLANATION,
	GRAPHIC_SELECT_GROUP_INFO_FRAME,
	GRAPHIC_SELECT_GROUP_TOP_EDGE,
	GRAPHIC_SELECT_MODE_ARCADE_HEADER,
	GRAPHIC_SELECT_MODE_ARCADE_PICTURE,
	GRAPHIC_SELECT_MODE_ARROW,
	GRAPHIC_SELECT_MODE_BACKGROUND,
	GRAPHIC_SELECT_MODE_EXPLANATION,
	GRAPHIC_SELECT_MODE_FREE_PLAY_HEADER,
	GRAPHIC_SELECT_MODE_FREE_PLAY_PICTURE,
	GRAPHIC_SELECT_MODE_NONSTOP_HEADER,
	GRAPHIC_SELECT_MODE_NONSTOP_PICTURE,
	GRAPHIC_SELECT_MODE_OK,
	GRAPHIC_SELECT_MODE_TOP_EDGE,
	GRAPHIC_SELECT_MUSIC_BACKGROUND,
	GRAPHIC_SELECT_MUSIC_DIFFICULTY_FRAME,
	GRAPHIC_SELECT_MUSIC_INFO_FRAME,
	GRAPHIC_SELECT_MUSIC_METER_FRAME,
	GRAPHIC_SELECT_MUSIC_OPTION_ICONS,
	GRAPHIC_SELECT_MUSIC_RADAR_BASE,
	GRAPHIC_SELECT_MUSIC_RADAR_WORDS,
	GRAPHIC_SELECT_MUSIC_ROULETTE_BANNER,
	GRAPHIC_SELECT_MUSIC_SCORE_FRAME,
	GRAPHIC_SELECT_MUSIC_SECTION_BAR,
	GRAPHIC_SELECT_MUSIC_SMALL_GRADES,	
	GRAPHIC_SELECT_MUSIC_SONG_BAR,
	GRAPHIC_SELECT_MUSIC_SONG_HIGHLIGHT,
	GRAPHIC_SELECT_MUSIC_TOP_EDGE,
	GRAPHIC_SELECT_STYLE_BACKGROUND,
	GRAPHIC_SELECT_STYLE_TOP_EDGE,
	GRAPHIC_SONG_OPTIONS_BACKGROUND,
	GRAPHIC_SONG_OPTIONS_TOP_EDGE,
	GRAPHIC_STAGE_UNDERSCORE,
	GRAPHIC_STEPS_DESCRIPTION,
	GRAPHIC_STYLE_ICONS,
	GRAPHIC_SYNCHRONIZE_BACKGROUND,
	GRAPHIC_SYNCHRONIZE_TOP_EDGE,
	GRAPHIC_TITLE_MENU_BACKGROUND,
	GRAPHIC_TITLE_MENU_LOGO,

	SOUND_FAILED,	
	SOUND_ASSIST,	
	SOUND_SELECT,	
	SOUND_SWITCH_STYLE,	
	SOUND_SWITCH_MUSIC,	
	SOUND_SWITCH_SORT,	
	SOUND_EXPAND,	
	SOUND_SWITCH_STEPS,	
	SOUND_TITLE_CHANGE,	
	SOUND_MENU_SWOOSH,	
	SOUND_MENU_BACK,	
	SOUND_INVALID,	
	SOUND_EDIT_CHANGE_LINE,	
	SOUND_EDIT_CHANGE_SNAP,	
	SOUND_SELECT_DIFFICULTY_CHANGE,	
	SOUND_SELECT_MODE_CHANGE,	
	SOUND_ENDING_MUSIC,	
	SOUND_MENU_MUSIC,	

	FONT_HEADER1,	
	FONT_HEADER2,	
	FONT_NORMAL,
	FONT_COMBO_NUMBERS,	
	FONT_METER,
	FONT_SCORE_NUMBERS,	
	FONT_TEXT_BANNER,	
	FONT_STAGE,	

	NUM_THEME_ELEMENTS	// leave this at the end
};






class ThemeManager
{
public:
	ThemeManager();

	void GetThemeNames( CStringArray& AddTo );
	CString GetCurrentThemeName() { return m_sCurThemeName; }
	void SwitchTheme( CString sThemeName );		// return false if theme doesn't exist
	void AssertThemeIsComplete( CString sThemeName );		// return false if theme doesn't exist
	CString GetPathTo( ThemeElement te );
	CString GetPathTo( ThemeElement te, CString sThemeName );

protected:
	CString GetThemeDirFromName( CString sAnnouncerName );
	CString GetElementDir( CString sThemeName );

	CString m_sCurThemeName;
};



extern ThemeManager*	THEME;	// global and accessable from anywhere in our program
