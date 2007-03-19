/* ScreenSelectMusic - Choose a Song and Steps. */

#ifndef SCREEN_SELECT_MUSIC_H
#define SCREEN_SELECT_MUSIC_H

#include "ScreenWithMenuElements.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "GameConstantsAndTypes.h"
#include "MusicWheel.h"
#include "FadingBanner.h"
#include "RageUtil_BackgroundLoader.h"
#include "ThemeMetric.h"
#include "RageTexturePreloader.h"
#include "TimingData.h"
#include "GameInput.h"
#include "OptionsList.h"

class ScreenSelectMusic : public ScreenWithMenuElements
{
public:
	ScreenSelectMusic();
	virtual ~ScreenSelectMusic();
	virtual void Init();
	virtual void BeginScreen();

	virtual void Update( float fDeltaTime );
	virtual void Input( const InputEventPlus &input );
	virtual void HandleScreenMessage( const ScreenMessage SM );
	virtual bool AllowLateJoin() const { return true; }

	virtual void MenuStart( const InputEventPlus &input );
	virtual void MenuBack( const InputEventPlus &input );

	/* ScreenWithMenuElements override: never play music here; we do it ourself. */
	virtual void StartPlayingMusic() { }
		
	bool GetGoToOptions() const { return m_bGoToOptions; }

	//
	// Lua
	//
	virtual void PushSelf( lua_State *L );

protected:
	virtual bool GenericTweenOn() const { return true; }
	virtual bool GenericTweenOff() const { return true; }
	void UpdateSelectButton( PlayerNumber pn, bool bBeingPressed );

	void ChangeDifficulty( PlayerNumber pn, int dir );

	void AfterStepsOrTrailChange( const vector<PlayerNumber> &vpns );
	void SwitchToPreferredDifficulty();
	void AfterMusicChange();

	void CheckBackgroundRequests( bool bForce );
	bool DetectCodes( const InputEventPlus &input );

	vector<Steps*>			m_vpSteps;
	vector<Trail*>			m_vpTrails;
	int				m_iSelection[NUM_PLAYERS];

	ThemeMetric<float> SAMPLE_MUSIC_DELAY;
	ThemeMetric<bool> DO_ROULETTE_ON_MENU_TIMER;
	ThemeMetric<bool> ALIGN_MUSIC_BEATS;
	ThemeMetric<RString> CODES;
	ThemeMetric<RString> MUSIC_WHEEL_TYPE;
	ThemeMetric<bool> OPTIONS_MENU_AVAILABLE;
	DynamicThemeMetric<bool> SELECT_MENU_AVAILABLE;
	DynamicThemeMetric<bool> MODE_MENU_AVAILABLE;
	ThemeMetric<bool> USE_OPTIONS_LIST;
	ThemeMetric<bool> TWO_PART_SELECTION;

	enum SelectionState
	{
		SelectionState_SelectingSong,
		SelectionState_SelectingSteps,
		SelectionState_Finalized
	};
	bool CanChangeSong() const { return m_SelectionState == SelectionState_SelectingSong; }
	bool CanChangeSteps() const { return TWO_PART_SELECTION ? m_SelectionState == SelectionState_SelectingSteps : m_SelectionState == SelectionState_SelectingSong; }
	SelectionState GetNextSelectionState() const
	{
		switch( m_SelectionState )
		{
			DEFAULT_FAIL( m_SelectionState );
		case SelectionState_SelectingSong:
			return TWO_PART_SELECTION ? SelectionState_SelectingSteps : SelectionState_Finalized;
		case SelectionState_SelectingSteps:
			return SelectionState_Finalized;
		}
	}

	GameButton m_GameButtonPreviousSong;
	GameButton m_GameButtonNextSong;

	RString m_sSectionMusicPath;
	RString m_sSortMusicPath;
	RString m_sRouletteMusicPath;
	RString m_sRandomMusicPath;
	RString m_sCourseMusicPath;
	RString m_sFallbackCDTitlePath;

	FadingBanner		m_Banner;
	Sprite			m_sprCDTitleFront, m_sprCDTitleBack;
	Sprite			m_sprHighScoreFrame[NUM_PLAYERS];
	BitmapText		m_textHighScore[NUM_PLAYERS];
	MusicWheel		m_MusicWheel;
	OptionsList		m_OptionsList[NUM_PLAYERS];
	void OpenOptionsList( PlayerNumber pn );
	void CloseOptionsList( PlayerNumber pn );

	SelectionState		m_SelectionState;
	bool			m_bStepsSelected[NUM_PLAYERS];	// only used in SelectionState_SelectingSteps
	bool			m_bGoToOptions;
	RString			m_sSampleMusicToPlay;
	TimingData		*m_pSampleMusicTimingData;
	float			m_fSampleStartSeconds, m_fSampleLengthSeconds;
	bool			m_bAllowOptionsMenu, m_bAllowOptionsMenuRepeat;
	bool			m_bSelectIsDown[NUM_PLAYERS];
	bool			m_bAcceptSelectRelease[NUM_PLAYERS];

	RageSound		m_soundStart;
	RageSound		m_soundDifficultyEasier;
	RageSound		m_soundDifficultyHarder;
	RageSound		m_soundOptionsChange;
	RageSound		m_soundLocked;

	BackgroundLoader	m_BackgroundLoader;
	RageTexturePreloader	m_TexturePreload;
};

#endif

/*
 * (c) 2001-2004 Chris Danford
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