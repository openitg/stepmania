#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenSelectCourse

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "ScreenSelectCourse.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "GameManager.h"
#include "RageMusic.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "AnnouncerManager.h"
#include "GameState.h"
#include "RageMusic.h"
#include "CodeDetector.h"


#define BANNER_FRAME_X		THEME->GetMetricF("ScreenSelectCourse","BannerFrameX")
#define BANNER_FRAME_Y		THEME->GetMetricF("ScreenSelectCourse","BannerFrameY")
#define BANNER_X			THEME->GetMetricF("ScreenSelectCourse","BannerX")
#define BANNER_Y			THEME->GetMetricF("ScreenSelectCourse","BannerY")
#define BANNER_WIDTH		THEME->GetMetricF("ScreenSelectCourse","BannerWidth")
#define BANNER_HEIGHT		THEME->GetMetricF("ScreenSelectCourse","BannerHeight")
#define STAGES_X			THEME->GetMetricF("ScreenSelectCourse","StagesX")
#define STAGES_Y			THEME->GetMetricF("ScreenSelectCourse","StagesY")
#define TIME_X				THEME->GetMetricF("ScreenSelectCourse","TimeX")
#define TIME_Y				THEME->GetMetricF("ScreenSelectCourse","TimeY")
#define CONTENTS_X			THEME->GetMetricF("ScreenSelectCourse","ContentsX")
#define CONTENTS_Y			THEME->GetMetricF("ScreenSelectCourse","ContentsY")
#define WHEEL_X				THEME->GetMetricF("ScreenSelectCourse","WheelX")
#define WHEEL_Y				THEME->GetMetricF("ScreenSelectCourse","WheelY")
#define SCORE_X( p )		THEME->GetMetricF("ScreenSelectCourse",ssprintf("ScoreP%dX",p+1))
#define SCORE_Y( i )		THEME->GetMetricF("ScreenSelectCourse",ssprintf("ScoreP%dY",i+1))
#define HELP_TEXT			THEME->GetMetric("ScreenSelectCourse","HelpText")
#define TIMER_SECONDS		THEME->GetMetricI("ScreenSelectCourse","TimerSeconds")


const float TWEEN_TIME		= 0.5f;

const ScreenMessage SM_GoToPrevScreen		=	ScreenMessage(SM_User+1);
const ScreenMessage SM_GoToNextScreen		=	ScreenMessage(SM_User+2);



ScreenSelectCourse::ScreenSelectCourse()
{
	LOG->Trace( "ScreenSelectCourse::ScreenSelectCourse()" );
 
	MUSIC->LoadAndPlayIfNotAlready( THEME->GetPathTo("Sounds","select course music") );

	m_bMadeChoice = false;
	m_bGoToOptions = false;

	m_Menu.Load(
		THEME->GetPathTo("Graphics","select course background"), 
		THEME->GetPathTo("Graphics","select course top edge"),
		HELP_TEXT, true, TIMER_SECONDS 
		);
	this->AddChild( &m_Menu );

	m_Banner.SetXY( BANNER_X, BANNER_Y );
	m_Banner.SetCroppedSize( BANNER_WIDTH, BANNER_HEIGHT );
	this->AddChild( &m_Banner );

	m_sprBannerFrame.Load( THEME->GetPathTo("Graphics","select course info frame") );
	m_sprBannerFrame.SetXY( BANNER_FRAME_X, BANNER_FRAME_Y );
	this->AddChild( &m_sprBannerFrame );

	m_textNumStages.LoadFromFont( THEME->GetPathTo("Fonts","header1") );
	m_textNumStages.SetXY( STAGES_X, STAGES_Y );
	m_textNumStages.TurnShadowOff();
	this->AddChild( &m_textNumStages );

	m_textTime.LoadFromFont( THEME->GetPathTo("Fonts","header1") );
	m_textTime.SetXY( TIME_X, TIME_Y );
	m_textTime.TurnShadowOff();
	this->AddChild( &m_textTime );

	m_CourseContentsFrame.SetXY( CONTENTS_X, CONTENTS_Y );
	this->AddChild( &m_CourseContentsFrame );

	m_MusicWheel.SetXY( WHEEL_X, WHEEL_Y );
	this->AddChild( &m_MusicWheel );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;	// skip

		m_sprHighScoreFrame[p].Load( THEME->GetPathTo("Graphics","select music score frame") );
		m_sprHighScoreFrame[p].StopAnimating();
		m_sprHighScoreFrame[p].SetState( p );
		m_sprHighScoreFrame[p].SetXY( SCORE_X(p), SCORE_Y(p) );
		this->AddChild( &m_sprHighScoreFrame[p] );

		m_HighScore[p].SetXY( SCORE_X(p), SCORE_Y(p) );
		m_HighScore[p].SetZoom( 0.6f );
		m_HighScore[p].SetDiffuse( PlayerToColor(p) );
		this->AddChild( &m_HighScore[p] );
	}	

	m_textHoldForOptions.LoadFromFont( THEME->GetPathTo("Fonts","Stage") );
	m_textHoldForOptions.SetXY( CENTER_X, CENTER_Y );
	m_textHoldForOptions.SetText( "press START again for options" );
	m_textHoldForOptions.SetZoom( 1 );
	m_textHoldForOptions.SetZoomY( 0 );
	m_textHoldForOptions.SetDiffuse( D3DXCOLOR(1,1,1,0) );
	this->AddChild( &m_textHoldForOptions );


	m_soundSelect.Load( THEME->GetPathTo("Sounds","menu start") );
	m_soundOptionsChange.Load( THEME->GetPathTo("Sounds","select music change options") );

	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo("select course intro") );

	MUSIC->LoadAndPlayIfNotAlready( THEME->GetPathTo("Sounds","select course music") );

	UpdateOptionsDisplays();

	AfterCourseChange();
	TweenOnScreen();
	m_Menu.TweenOnScreenFromMenu( SM_None );
}


ScreenSelectCourse::~ScreenSelectCourse()
{
	LOG->Trace( "ScreenSelectCourse::~ScreenSelectCourse()" );

}

void ScreenSelectCourse::DrawPrimitives()
{
	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();
}

void ScreenSelectCourse::TweenOnScreen()
{
	Actor* pActorsInCourseInfoFrame[] = { &m_sprBannerFrame, &m_Banner, &m_textNumStages, &m_textTime };
	const int iNumActorsInGroupInfoFrame = sizeof(pActorsInCourseInfoFrame) / sizeof(Actor*);
	int i;
	for( i=0; i<iNumActorsInGroupInfoFrame; i++ )
	{
		float fOriginalX = pActorsInCourseInfoFrame[i]->GetX();
		pActorsInCourseInfoFrame[i]->SetX( fOriginalX-400 );
		pActorsInCourseInfoFrame[i]->BeginTweening( TWEEN_TIME, TWEEN_BOUNCE_END );
		pActorsInCourseInfoFrame[i]->SetTweenX( fOriginalX );
	}

	m_CourseContentsFrame.SetXY( CONTENTS_X - 400, CONTENTS_Y );
	m_CourseContentsFrame.BeginTweening( TWEEN_TIME, Actor::TWEEN_BIAS_END );
	m_CourseContentsFrame.SetTweenXY( CONTENTS_X, CONTENTS_Y );

	Actor* pActorsInScore[] = { &m_sprHighScoreFrame[0], &m_sprHighScoreFrame[1], &m_HighScore[0], &m_HighScore[1] };
	const int iNumActorsInScore = sizeof(pActorsInScore) / sizeof(Actor*);
	for( i=0; i<iNumActorsInScore; i++ )
	{
		float fOriginalX = pActorsInScore[i]->GetX();
		pActorsInScore[i]->SetX( fOriginalX+400 );
		pActorsInScore[i]->BeginTweening( TWEEN_TIME, TWEEN_BIAS_END );
		pActorsInScore[i]->SetTweenX( fOriginalX );
	}

	m_MusicWheel.TweenOnScreen();
}

void ScreenSelectCourse::TweenOffScreen()
{
	Actor* pActorsInCourseInfoFrame[] = { &m_sprBannerFrame, &m_Banner, &m_textNumStages, &m_textTime };
	const int iNumActorsInGroupInfoFrame = sizeof(pActorsInCourseInfoFrame) / sizeof(Actor*);
	int i;
	for( i=0; i<iNumActorsInGroupInfoFrame; i++ )
	{
		pActorsInCourseInfoFrame[i]->BeginTweening( TWEEN_TIME, TWEEN_BOUNCE_BEGIN );
		pActorsInCourseInfoFrame[i]->SetTweenX( pActorsInCourseInfoFrame[i]->GetX()-400 );
	}

	m_CourseContentsFrame.BeginTweening( TWEEN_TIME, Actor::TWEEN_BOUNCE_BEGIN );
	m_CourseContentsFrame.SetTweenXY( CONTENTS_X - 400, CONTENTS_Y );

	Actor* pActorsInScore[] = { &m_sprHighScoreFrame[0], &m_sprHighScoreFrame[1], &m_HighScore[0], &m_HighScore[1] };
	const int iNumActorsInScore = sizeof(pActorsInScore) / sizeof(Actor*);
	for( i=0; i<iNumActorsInScore; i++ )
	{
		pActorsInScore[i]->BeginTweening( TWEEN_TIME, TWEEN_BIAS_BEGIN );
		pActorsInScore[i]->SetTweenX( pActorsInScore[i]->GetX()+400 );
	}

	m_MusicWheel.TweenOffScreen();
}


void ScreenSelectCourse::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenSelectCourse::Input()" );
	
	if( type == IET_RELEASE )	return;		// don't care

	if( m_Menu.IsClosing() )	return;		// ignore

	if( !GameI.IsValid() )		return;		// don't care

	if( m_bMadeChoice  &&  !m_bGoToOptions  &&  MenuI.IsValid()  &&  MenuI.button == MENU_BUTTON_START  &&  !GAMESTATE->IsExtraStage()  &&  !GAMESTATE->IsExtraStage2() )
	{
		m_bGoToOptions = true;
		m_textHoldForOptions.SetText( "Entering Options..." );
		SOUND->PlayOnceStreamed( THEME->GetPathTo("Sounds","menu start") );
		return;
	}
	
	if( m_bMadeChoice )
		return;

	PlayerNumber pn = GAMESTATE->GetCurrentStyleDef()->ControllerToPlayerNumber( GameI.controller );


	if( CodeDetector::DetectAndAdjustOptions(GameI.controller) )
	{
		m_soundOptionsChange.Play();
		UpdateOptionsDisplays();
		return;
	}

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}


void ScreenSelectCourse::HandleScreenMessage( const ScreenMessage SM )
{
	Screen::HandleScreenMessage( SM );

	switch( SM )
	{
	case SM_MenuTimer:
		MenuStart(PLAYER_1);
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
		break;
	case SM_GoToNextScreen:

		// find out if the Next button is being held down on any of the pads
		bool bIsHoldingNext;
		bIsHoldingNext = false;
		int player;
		for( player=0; player<NUM_PLAYERS; player++ )
		{
			MenuInput mi( (PlayerNumber)player, MENU_BUTTON_START );
			if( INPUTMAPPER->IsButtonDown( mi ) )
				bIsHoldingNext = true;
		}

		if( bIsHoldingNext || m_bGoToOptions )
			SCREENMAN->SetNewScreen( "ScreenPlayerOptions" );
		else
			SCREENMAN->SetNewScreen( "ScreenStage" );

		break;
	}
}

void ScreenSelectCourse::MenuLeft( PlayerNumber p, const InputEventType type )
{
	m_MusicWheel.PrevMusic();
	
	AfterCourseChange();
}


void ScreenSelectCourse::MenuRight( PlayerNumber p, const InputEventType type )
{
	m_MusicWheel.NextMusic();

	AfterCourseChange();
}

void ScreenSelectCourse::MenuStart( PlayerNumber p )
{
	// this needs to check whether valid Notes are selected!
	m_MusicWheel.Select();

	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_COURSE:
		SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo("select course comment general") );
	
		TweenOffScreen();

		m_soundSelect.Play();

		m_bMadeChoice = true;

		// show "hold START for options"
		m_textHoldForOptions.SetDiffuse( D3DXCOLOR(1,1,1,0) );
		m_textHoldForOptions.BeginTweening( 0.25f );	// fade in
		m_textHoldForOptions.SetTweenZoomY( 1 );
		m_textHoldForOptions.SetTweenDiffuse( D3DXCOLOR(1,1,1,1) );
		m_textHoldForOptions.BeginTweening( 2.0f );	// sleep
		m_textHoldForOptions.BeginTweening( 0.25f );	// fade out
		m_textHoldForOptions.SetTweenDiffuse( D3DXCOLOR(1,1,1,0) );
		m_textHoldForOptions.SetTweenZoomY( 0 );

		m_Menu.TweenOffScreenToBlack( SM_None, false );

		Course* pCourse = m_MusicWheel.GetSelectedCourse();
		GAMESTATE->m_pCurCourse = pCourse;
		for( int p=0; p<NUM_PLAYERS; p++ )
			pCourse->GetPlayerOptions( &GAMESTATE->m_PlayerOptions[p] );
		pCourse->GetSongOptions( &GAMESTATE->m_SongOptions );

		m_Menu.StopTimer();

		this->SendScreenMessage( SM_GoToNextScreen, 2.5f );
		
		break;
	}
}


void ScreenSelectCourse::MenuBack( PlayerNumber p )
{
	MUSIC->Stop();

	m_Menu.TweenOffScreenToBlack( SM_GoToPrevScreen, true );
}


void ScreenSelectCourse::AfterCourseChange()
{
	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_COURSE:
		{
			Course* pCourse = m_MusicWheel.GetSelectedCourse();

			m_textNumStages.SetText( ssprintf("%d", pCourse->m_iStages) );
			float fTotalSeconds = 0;
			for( int i=0; i<pCourse->m_iStages; i++ )
				fTotalSeconds += pCourse->m_apSongs[i]->m_fMusicLengthSeconds;
			m_textTime.SetText( SecondsToTime(fTotalSeconds) );

			m_Banner.SetFromCourse( pCourse );

			m_CourseContentsFrame.SetFromCourse( pCourse );
		}
		break;
	case TYPE_SECTION:	// if we get here, there are no courses
		break;
	default:
		ASSERT(0);
	}
}

void ScreenSelectCourse::UpdateOptionsDisplays()
{
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( GAMESTATE->IsPlayerEnabled(p) )
		{
			CString s = GAMESTATE->m_PlayerOptions[p].GetString();
			s.Replace( ", ", "\n" );
//			m_textPlayerOptions[p].SetText( s );
		}
	}

	CString s = GAMESTATE->m_SongOptions.GetString();
	s.Replace( ", ", "\n" );
//	m_textSongOptions.SetText( s );
}
