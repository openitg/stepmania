#ifndef SCREEN_EVALUATION_H
#define SCREEN_EVALUATION_H

/*
-----------------------------------------------------------------------------
 Class: ScreenEvaluation

 Desc: Shows the user their score after gameplay has ended.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "Screen.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "GradeDisplay.h"
#include "MenuElements.h"
#include "Banner.h"
#include "ScoreDisplayNormal.h"
#include "Banner.h"
#include "DifficultyIcon.h"
#include "PercentageDisplay.h"
#include "GraphDisplay.h"
#include "ComboGraph.h"
#include "BGAnimation.h"
#include "ActorUtil.h"


const int MAX_SONGS_TO_SHOW = 5;	// In summary, we show last 3 stages, plus extra stages if passed
const int NUM_JUDGE_LINES =	9;	// marvelous, perfect, great, good, boo, miss, ok, max_combo, error
const int NUM_SCORE_LINES = 2;	// score, time

// sound sequences for the evaluation screen
struct EvalSoundSequence
{
	float fTime;
	RageSound sSound;
};

class ScreenEvaluation : public Screen
{
public:
	enum Type	{ stage, summary, course };
	ScreenEvaluation( CString sClassName );
	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

//	virtual void TweenOnScreen();
	virtual void TweenOffScreen();
	virtual void MenuLeft( PlayerNumber pn );
	virtual void MenuRight( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
	virtual void MenuStart( PlayerNumber pn );

protected:
	CString				m_sName;
	Type				m_Type;

	float m_fScreenCreateTime;

	BGAnimation m_bgFailedBack;
	BGAnimation m_bgFailedOverlay;
	BGAnimation m_bgPassedOverlay;

	MenuElements		m_Menu;

	// banner area
	Banner				m_LargeBanner;
	Sprite				m_sprLargeBannerFrame;
	Sprite				m_sprStage;
	DifficultyIcon		m_DifficultyIcon[NUM_PLAYERS];
	BitmapText			m_textPlayerOptions[NUM_PLAYERS];
	Banner				m_SmallBanner[MAX_SONGS_TO_SHOW];
	Sprite				m_sprSmallBannerFrame[MAX_SONGS_TO_SHOW];

	// grade area
	Sprite				m_sprGradeFrame[NUM_PLAYERS];
	GradeDisplay		m_Grades[NUM_PLAYERS];

	// graph area
	GraphDisplay		m_Graph[NUM_PLAYERS];

	// combo area
	ComboGraph			m_Combo[NUM_PLAYERS];

	// points area
	bool				m_bNewSongsUnlocked;
	PercentageDisplay	m_Percent[NUM_PLAYERS];
	Sprite				m_sprPercentFrame[NUM_PLAYERS];

	// bonus area
	Sprite				m_sprBonusFrame[NUM_PLAYERS];
	Sprite				m_sprPossibleBar[NUM_PLAYERS][NUM_RADAR_CATEGORIES];
	Sprite				m_sprActualBar[NUM_PLAYERS][NUM_RADAR_CATEGORIES];

	// survived area
	Sprite				m_sprSurvivedFrame[NUM_PLAYERS];
	BitmapText			m_textSurvivedNumber[NUM_PLAYERS];

	// win area
	Sprite				m_sprWinFrame[NUM_PLAYERS];
	Sprite				m_sprWin[NUM_PLAYERS];

	// judgment area
	Sprite				m_sprJudgeLabels[NUM_JUDGE_LINES];

	BitmapText			m_textJudgeNumbers[NUM_JUDGE_LINES][NUM_PLAYERS];

	// score area
	Sprite				m_sprScoreLabel;
	BitmapText			m_textScore[NUM_PLAYERS];

	// time area
	Sprite				m_sprTimeLabel;
	BitmapText			m_textTime[NUM_PLAYERS];

	// extra area
	Sprite				m_sprMachineRecord[NUM_PLAYERS];
	Sprite				m_sprPersonalRecord[NUM_PLAYERS];
	bool				m_bTryExtraStage;
	Sprite				m_sprTryExtraStage;
	AutoActor			m_FullCombo[NUM_PLAYERS];
	bool m_bFailed;

	// sound effects for other gametypes

	RageSound	m_sndPassFail;	// sound played if the player passes or fails
	bool m_bPassFailTriggered; // has the pass / fail sound been played yet?
	RageTimer m_timerSoundSequences; // timer used for triggering sounds.
	vector<EvalSoundSequence> m_SoundSequences; // a sequence of sounds to be played (although they're stored in no particular order!)	
};

#endif
