#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: ScreenSelectDifficulty.cpp

 Desc: Testing the Screen class.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

#include "ScreenSelectDifficulty.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "PrefsManager.h"
#include "ScreenSelectGroup.h"
#include "ScreenTitleMenu.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "AnnouncerManager.h"
#include "ScreenSelectCourse.h"
#include "GameState.h"


const float LOCK_INPUT_TIME = 0.30f;	// lock input while waiting for tweening to complete


#define MORE_PAGE1_X		THEME->GetMetricF("SelectDifficulty","MorePage1X")
#define MORE_PAGE1_Y		THEME->GetMetricF("SelectDifficulty","MorePage1Y")
#define MORE_PAGE2_X		THEME->GetMetricF("SelectDifficulty","MorePage2X")
#define MORE_PAGE2_Y		THEME->GetMetricF("SelectDifficulty","MorePage2Y")
#define EXPLANATION_PAGE1_X	THEME->GetMetricF("SelectDifficulty","ExplanationPage1X")
#define EXPLANATION_PAGE1_Y	THEME->GetMetricF("SelectDifficulty","ExplanationPage1Y")
#define EXPLANATION_PAGE2_X	THEME->GetMetricF("SelectDifficulty","ExplanationPage2X")
#define EXPLANATION_PAGE2_Y	THEME->GetMetricF("SelectDifficulty","ExplanationPage2Y")
#define EASY_X				THEME->GetMetricF("SelectDifficulty","EasyX")
#define EASY_Y				THEME->GetMetricF("SelectDifficulty","EasyY")
#define MEDIUM_X			THEME->GetMetricF("SelectDifficulty","MediumX")
#define MEDIUM_Y			THEME->GetMetricF("SelectDifficulty","MediumY")
#define HARD_X				THEME->GetMetricF("SelectDifficulty","HardX")
#define HARD_Y				THEME->GetMetricF("SelectDifficulty","HardY")
#define ONI_X				THEME->GetMetricF("SelectDifficulty","OniX")
#define ONI_Y				THEME->GetMetricF("SelectDifficulty","OniY")
#define ENDLESS_X			THEME->GetMetricF("SelectDifficulty","EndlessX")
#define ENDLESS_Y			THEME->GetMetricF("SelectDifficulty","EndlessY")
#define ARROW_OFFSET_P1_X	THEME->GetMetricF("SelectDifficulty","ArrowOffsetP1X")
#define ARROW_OFFSET_P1_Y	THEME->GetMetricF("SelectDifficulty","ArrowOffsetP1Y")
#define ARROW_OFFSET_P2_X	THEME->GetMetricF("SelectDifficulty","ArrowOffsetP2X")
#define ARROW_OFFSET_P2_Y	THEME->GetMetricF("SelectDifficulty","ArrowOffsetP2Y")
#define ARROW_SHADOW_LENGTH_X		THEME->GetMetricF("SelectDifficulty","ArrowShadowLengthX")
#define ARROW_SHADOW_LENGTH_Y		THEME->GetMetricF("SelectDifficulty","ArrowShadowLengthY")


float MORE_X( int iIndex ) {
	switch( iIndex ) {
	case 0:		return MORE_PAGE1_X;
	case 1:		return MORE_PAGE2_X;
	default:	ASSERT(0);	return 0;
	}
}
float MORE_Y( int iIndex ) {
	switch( iIndex ) {
	case 0:		return MORE_PAGE1_Y;
	case 1:		return MORE_PAGE2_Y;
	default:	ASSERT(0);	return 0;
	}
}
float EXPLANATION_X( int iIndex ) {
	switch( iIndex ) {
	case 0:		return EXPLANATION_PAGE1_X;
	case 1:		return EXPLANATION_PAGE2_X;
	default:	ASSERT(0);	return 0;
	}
}
float EXPLANATION_Y( int iIndex ) {
	switch( iIndex ) {
	case 0:		return EXPLANATION_PAGE1_Y;
	case 1:		return EXPLANATION_PAGE2_Y;
	default:	ASSERT(0);	return 0;
	}
}
float ITEM_X( int iItemIndex ) {
	switch( iItemIndex ) {
	case 0:		return EASY_X;
	case 1:		return MEDIUM_X;
	case 2:		return HARD_X;
	case 3:		return ONI_X;
	case 4:		return ENDLESS_X;
	default:	ASSERT(0);	return 0;
	}
}
float ITEM_Y( int iItemIndex ) {
	switch( iItemIndex ) {
	case 0:		return EASY_Y;
	case 1:		return MEDIUM_Y;
	case 2:		return HARD_Y;
	case 3:		return ONI_Y;
	case 4:		return ENDLESS_Y;
	default:	ASSERT(0);	return 0;
	}
}
float ARROW_X( int iItemIndex, int p ) {
	switch( p ) {
	case PLAYER_1:	return ITEM_X(iItemIndex) + ARROW_OFFSET_P1_X;
	case PLAYER_2:	return ITEM_X(iItemIndex) + ARROW_OFFSET_P2_X;
	default:	ASSERT(0);	return 0;
	}
}
float ARROW_Y( int iItemIndex, int p ) {
	switch( p ) {
	case PLAYER_1:	return ITEM_Y(iItemIndex) + ARROW_OFFSET_P1_Y;
	case PLAYER_2:	return ITEM_Y(iItemIndex) + ARROW_OFFSET_P2_Y;
	default:	ASSERT(0);	return 0;
	}
}


const ScreenMessage SM_GoToPrevState			= ScreenMessage(SM_User + 1);
const ScreenMessage SM_GoToNextState			= ScreenMessage(SM_User + 2);
const ScreenMessage SM_StartTweeningOffScreen	= ScreenMessage(SM_User + 3);
const ScreenMessage SM_StartFadingOut			= ScreenMessage(SM_User + 4);


ScreenSelectDifficulty::ScreenSelectDifficulty()
{
	LOG->Trace( "ScreenSelectDifficulty::ScreenSelectDifficulty()" );

	// Reset the current PlayMode
	GAMESTATE->m_PlayMode = PLAY_MODE_INVALID;
	GAMESTATE->m_bPlayersCanJoin = false;


	int p;
	
	m_Menu.Load(
		THEME->GetPathTo("Graphics","select difficulty background"), 
		THEME->GetPathTo("Graphics","select difficulty top edge"),
		ssprintf("Use %c %c to select, then press START", char(1), char(2)),
		false, true, 40 
		);
	this->AddSubActor( &m_Menu );


	for( int d=0; d<NUM_DIFFICULTY_ITEMS; d++ )
	{
		CString sHeaderFile;
		CString sPictureFile;
		switch( d )
		{
		case 0:	sHeaderFile = "select difficulty easy header";		sPictureFile = "select difficulty easy picture";	break;
		case 1: sHeaderFile = "select difficulty medium header";	sPictureFile = "select difficulty medium picture";	break;
		case 2: sHeaderFile = "select difficulty hard header";		sPictureFile = "select difficulty hard picture";	break;
		case 3: sHeaderFile = "select difficulty oni header";		sPictureFile = "select difficulty oni picture";		break;
		case 4: sHeaderFile = "select difficulty endless header";	sPictureFile = "select difficulty endless picture";	break;
		}
		m_sprPicture[d].Load( THEME->GetPathTo("Graphics",sPictureFile) );
		m_sprPicture[d].SetXY( ITEM_X(d), ITEM_Y(d) );
		m_sprPicture[d].SetVertAlign( align_bottom );
		m_sprPicture[d].TurnShadowOff();
		m_framePages.AddSubActor( &m_sprPicture[d] );

		m_sprHeader[d].Load( THEME->GetPathTo("Graphics",sHeaderFile) );
		m_sprHeader[d].SetXY( ITEM_X(d), ITEM_Y(d) );
		m_sprHeader[d].SetVertAlign( align_top );
		m_sprHeader[d].TurnShadowOff();
		m_framePages.AddSubActor( &m_sprHeader[d] );
	}

	for( p=0; p<NUM_PAGES; p++ )
	{
		m_sprMoreArrows[p].Load( THEME->GetPathTo("Graphics", p==0 ? "select difficulty more page1" : "select difficulty more page2" ) );
		m_sprMoreArrows[p].SetXY( MORE_X(p), MORE_Y(p) );
		m_framePages.AddSubActor( &m_sprMoreArrows[p] );

		m_sprExplanation[p].Load( THEME->GetPathTo("Graphics", "select difficulty explanation") );
		m_sprExplanation[p].SetXY( EXPLANATION_X(p), EXPLANATION_Y(p) );
		m_sprExplanation[p].StopAnimating();
		m_sprExplanation[p].SetState( p );
		m_framePages.AddSubActor( &m_sprExplanation[p] );
	}


	for( p=0; p<NUM_PLAYERS; p++ )
	{
		m_iSelection[p] = 1;	// Select "medium"
		m_bChosen[p] = false;

		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;

		m_sprArrowShadow[p].Load( THEME->GetPathTo("Graphics", "select difficulty arrows") );
		m_sprArrowShadow[p].StopAnimating();
		m_sprArrowShadow[p].SetState( p );
		m_sprArrowShadow[p].TurnShadowOff();
		m_sprArrowShadow[p].SetDiffuseColor( D3DXCOLOR(0,0,0,0.6f) );
		m_framePages.AddSubActor( &m_sprArrowShadow[p] );

		m_sprArrow[p].Load( THEME->GetPathTo("Graphics", "select difficulty arrows") );
		m_sprArrow[p].StopAnimating();
		m_sprArrow[p].SetState( p );
		m_sprArrow[p].TurnShadowOff();
		m_sprArrow[p].SetDiffuseColor( PlayerToColor((PlayerNumber)p) );
		m_sprArrow[p].SetEffectGlowing();
		m_framePages.AddSubActor( &m_sprArrow[p] );

		m_sprOK[p].Load( THEME->GetPathTo("Graphics", "select difficulty ok") );
		m_framePages.AddSubActor( &m_sprOK[p] );
	}

	this->AddSubActor( &m_framePages );
	
	m_soundChange.Load( THEME->GetPathTo("Sounds", "select difficulty change") );
	m_soundSelect.Load( THEME->GetPathTo("Sounds", "menu start") );

	m_bPlayedChallengeSound = false;
	
	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_DIFFICULTY_INTRO) );

	m_Menu.TweenOnScreenFromMenu( SM_None );
	TweenOnScreen();

	if( !MUSIC->IsPlaying()  ||  MUSIC->GetLoadedFilePath() != THEME->GetPathTo("Sounds","select difficulty music") )
	{
		MUSIC->Load( THEME->GetPathTo("Sounds","select difficulty music") );
		MUSIC->Play( true );
	}

	m_fLockInputTime = LOCK_INPUT_TIME;
}


ScreenSelectDifficulty::~ScreenSelectDifficulty()
{
	LOG->Trace( "ScreenSelectDifficulty::~ScreenSelectDifficulty()" );

}


void ScreenSelectDifficulty::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenSelectDifficulty::Input()" );

	if( m_Menu.IsClosing() )
		return;

	if( m_fLockInputTime > 0 )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}

void ScreenSelectDifficulty::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
	m_fLockInputTime = max( m_fLockInputTime-fDeltaTime, 0 );
}

void ScreenSelectDifficulty::DrawPrimitives()
{
	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();
}

void ScreenSelectDifficulty::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_MenuTimer:
		{
			for( int p=0; p<NUM_PLAYERS; p++ )
				MenuStart( (PlayerNumber)p );
		}
		break;
	case SM_GoToPrevState:
		SCREENMAN->SetNewScreen( new ScreenTitleMenu );
		break;
	case SM_GoToNextState:
		{
			for( int p=0; p<NUM_PLAYERS; p++ )
				switch( m_iSelection[p] )
				{
				case 0:	GAMESTATE->m_PreferredDifficultyClass[p] = CLASS_EASY;	break;
				case 1:	GAMESTATE->m_PreferredDifficultyClass[p] = CLASS_MEDIUM;	break;
				case 2:	GAMESTATE->m_PreferredDifficultyClass[p] = CLASS_HARD;	break;
				}
		}

		switch( m_iSelection[PLAYER_1] )
		{
		case 0:
		case 1:
		case 2:	// something on page 1 was chosen
			GAMESTATE->m_PlayMode = PLAY_MODE_ARCADE;
			SCREENMAN->SetNewScreen( new ScreenSelectGroup );
			break;
		case 3:
			GAMESTATE->m_PlayMode = PLAY_MODE_ONI;
			SCREENMAN->SetNewScreen( new ScreenSelectCourse );
			break;
		case 4:
			GAMESTATE->m_PlayMode = PLAY_MODE_ENDLESS;
			SCREENMAN->SetNewScreen( new ScreenSelectCourse );
			break;
		default:
			ASSERT(0);	// bad selection
		}
		break;
	case SM_StartTweeningOffScreen:
		TweenOffScreen();
		this->SendScreenMessage( SM_StartFadingOut, 0.8f );
		break;
	case SM_StartFadingOut:
		m_Menu.TweenOffScreenToMenu( SM_GoToNextState );
		break;
	}
}

void ScreenSelectDifficulty::MenuLeft( const PlayerNumber p )
{
	if( m_iSelection[p] == 0 )	// can't go left any more
		return;
	if( m_bChosen[p] )
		return;

	ChangeTo( p, m_iSelection[p], m_iSelection[p]-1 );
}


void ScreenSelectDifficulty::MenuRight( const PlayerNumber p )
{
	if( m_iSelection[p] == NUM_DIFFICULTY_ITEMS-1 )	// can't go right any more
		return;
	if( m_bChosen[p] )
		return;

	ChangeTo( p, m_iSelection[p], m_iSelection[p]+1 );
}

bool ScreenSelectDifficulty::IsItemOnPage2( int iItemIndex )
{
	ASSERT( iItemIndex >= 0  &&  iItemIndex < NUM_DIFFICULTY_ITEMS );

	return iItemIndex >= NUM_DIFFICULTY_CLASSES;
}

bool ScreenSelectDifficulty::SelectedSomethingOnPage2()
{
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( GAMESTATE->IsPlayerEnabled(p)  &&  IsItemOnPage2(m_iSelection[p]) )
			return true;
	}
	return false;
}

void ScreenSelectDifficulty::ChangeTo( const PlayerNumber pn, int iSelectionWas, int iSelectionIs )
{
	bool bChangedPagesFrom1To2 = iSelectionWas < 3  &&  iSelectionIs >= 3;
	bool bChangedPagesFrom2To1 = iSelectionWas >= 3  &&  iSelectionIs < 3;
	bool bChangedPages = bChangedPagesFrom1To2 || bChangedPagesFrom2To1;
	bool bSelectedSomethingOnPage1 = iSelectionIs < 3;
	bool bSelectedSomethingOnPage2 = iSelectionIs >= 3;

	if( bSelectedSomethingOnPage2 )
	{
		// change both players
		for( int p=0; p<NUM_PLAYERS; p++ )
			m_iSelection[p] = iSelectionIs;
	}
	else if( bChangedPagesFrom2To1 )
	{
		// change only the player who pressed the button
		for( int p=0; p<NUM_PLAYERS; p++ )
			m_iSelection[p] = 2;
	}
	else	// moving around in page 1
	{
		// change only the player who pressed the button
		m_iSelection[pn] = iSelectionIs;

	}


	if( !m_bPlayedChallengeSound  &&  bChangedPagesFrom1To2 )
	{
		SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_DIFFICULTY_CHALLENGE) );
		m_bPlayedChallengeSound = true;
	}

	if( bChangedPagesFrom1To2 || bChangedPagesFrom2To1 )
	{
		m_framePages.BeginTweening( 0.2f );
		m_framePages.SetTweenX( bSelectedSomethingOnPage1 ? 0.0f : -SCREEN_WIDTH );
	}

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( bSelectedSomethingOnPage2 || bChangedPagesFrom2To1 || p==pn )
		{
			m_sprArrow[p].BeginTweening( 0.2f, bChangedPages ? TWEEN_LINEAR : TWEEN_BIAS_BEGIN );
			m_sprArrow[p].SetTweenX( ARROW_X(m_iSelection[p],(PlayerNumber)p) - ARROW_SHADOW_LENGTH_X );
			m_sprArrow[p].SetTweenY( ARROW_Y(m_iSelection[p],(PlayerNumber)p) - ARROW_SHADOW_LENGTH_Y );

			m_sprArrowShadow[p].BeginTweening( 0.2f, bChangedPages ? TWEEN_LINEAR : TWEEN_BIAS_BEGIN );
			m_sprArrowShadow[p].SetTweenX( ARROW_X(m_iSelection[p],(PlayerNumber)p) );
			m_sprArrowShadow[p].SetTweenY( ARROW_Y(m_iSelection[p],(PlayerNumber)p) );
		}
	}

	m_soundChange.PlayRandom();
}

void ScreenSelectDifficulty::MenuStart( PlayerNumber pn )
{
	if( m_bChosen[pn] == true )
		return;
	m_bChosen[pn] = true;

	m_soundSelect.PlayRandom();
	int iSelection = m_iSelection[pn];

	switch( iSelection )
	{
	case 0:	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_DIFFICULTY_COMMENT_EASY) );	break;
	case 1:	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_DIFFICULTY_COMMENT_MEDIUM) );	break;
	case 2:	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_DIFFICULTY_COMMENT_HARD) );	break;
	case 3:	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_DIFFICULTY_COMMENT_ONI) );	break;
	case 4:	SOUND->PlayOnceStreamedFromDir( ANNOUNCER->GetPathTo(ANNOUNCER_SELECT_DIFFICULTY_COMMENT_ENDLESS) );	break;
	}

	if( iSelection >= 3 )	// chose something on page 2
	{
		// choose this for all the other players too
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			if( m_bChosen[p] )
				continue;
		
			MenuStart( (PlayerNumber)p );
		}
	}

	m_sprArrow[pn].BeginTweeningQueued( 0.2f );
	m_sprArrow[pn].BeginTweeningQueued( 0.2f );
	m_sprArrow[pn].SetTweenX( ARROW_X(iSelection, pn) );
	m_sprArrow[pn].SetTweenY( ARROW_Y(iSelection, pn) );

	m_sprOK[pn].SetX( ARROW_X(iSelection, pn) );
	m_sprOK[pn].SetY( ARROW_Y(iSelection, pn) );
	m_sprOK[pn].SetDiffuseColor( D3DXCOLOR(1,1,1,0) );
	m_sprOK[pn].SetZoom( 2 );

	m_sprOK[pn].BeginTweening( 0.2f );
	m_sprOK[pn].SetTweenZoom( 1 );
	m_sprOK[pn].SetTweenDiffuseColor( D3DXCOLOR(1,1,1,1) );

	m_sprArrowShadow[pn].BeginTweeningQueued( 0.2f );
	m_sprArrowShadow[pn].BeginTweeningQueued( 0.2f );
	m_sprArrowShadow[pn].SetDiffuseColor( D3DXCOLOR(0,0,0,0) );


	// check to see if everyone has chosen
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( GAMESTATE->IsPlayerEnabled((PlayerNumber)p)  &&  m_bChosen[p] == false )
			return;
	}
	this->SendScreenMessage( SM_StartTweeningOffScreen, 0.7f );
}

void ScreenSelectDifficulty::MenuBack( const PlayerNumber p )
{
	m_Menu.TweenOffScreenToBlack( SM_GoToPrevState, true );
}

void ScreenSelectDifficulty::TweenOffScreen()
{
	int p;

	for( p=0; p<NUM_PAGES; p++ )
	{
		m_sprExplanation[p].SetXY( EXPLANATION_X(p), EXPLANATION_Y(p) );
		m_sprExplanation[p].BeginTweening( 0.5, Actor::TWEEN_BOUNCE_BEGIN );
		m_sprExplanation[p].SetTweenXY( EXPLANATION_X(p)+700, EXPLANATION_Y(p) );

		m_sprMoreArrows[p].BeginTweening( 0.5 );
		m_sprMoreArrows[p].SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );
	}

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;

		m_sprArrow[p].BeginTweening( 0.3f );
		m_sprArrow[p].SetTweenZoom( 0 );

		m_sprOK[p].BeginTweening( 0.3f );
		m_sprOK[p].SetTweenZoom( 0 );

		m_sprArrowShadow[p].BeginTweeningQueued( 0.3f );
		m_sprArrowShadow[p].SetTweenDiffuseColor( D3DXCOLOR(0,0,0,0) );
	}

	for( int d=0; d<NUM_DIFFICULTY_ITEMS; d++ )
	{
		if( SelectedSomethingOnPage2() != IsItemOnPage2(d) )	// item isn't on selected page
			continue;	// don't tween

		const float fPauseTime = d*0.2f;

		// pause
		m_sprHeader[d].BeginTweeningQueued( fPauseTime );

		m_sprPicture[d].BeginTweeningQueued( fPauseTime );

		// roll up
		m_sprHeader[d].BeginTweeningQueued( 0.3f, TWEEN_BOUNCE_BEGIN );

		m_sprPicture[d].BeginTweeningQueued( 0.3f, TWEEN_BOUNCE_BEGIN );
		m_sprPicture[d].SetTweenZoomY( 0 );

		// fly off
		m_sprHeader[d].BeginTweeningQueued( 0.4f, TWEEN_BIAS_END );
		m_sprHeader[d].SetTweenXY( ITEM_X(d)-700, ITEM_Y(d) );

		m_sprPicture[d].BeginTweeningQueued( 0.4f, TWEEN_BIAS_END );
		m_sprPicture[d].SetTweenXY( ITEM_X(d)-700, ITEM_Y(d) );
	}
}

void ScreenSelectDifficulty::TweenOnScreen() 
{
	int p;

	for( p=0; p<NUM_PAGES; p++ )
	{
		m_sprExplanation[p].SetXY( EXPLANATION_X(p)+700, EXPLANATION_Y(p) );
		m_sprExplanation[p].BeginTweening( 0.3f, Actor::TWEEN_BOUNCE_END );
		m_sprExplanation[p].SetTweenXY( EXPLANATION_X(p), EXPLANATION_Y(p) );

		m_sprMoreArrows[p].SetDiffuseColor( D3DXCOLOR(1,1,1,0) );
		m_sprMoreArrows[p].BeginTweening( 0.5 );
		m_sprMoreArrows[p].SetTweenDiffuseColor( D3DXCOLOR(1,1,1,1) );
	}

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;

		int iSelection = m_iSelection[p];

		m_sprArrow[p].SetXY( ARROW_X(iSelection,(PlayerNumber)p), ARROW_Y(iSelection,(PlayerNumber)p) );
		m_sprArrow[p].SetDiffuseColor( D3DXCOLOR(1,1,1,0) );
		m_sprArrow[p].SetRotation( D3DX_PI );
		m_sprArrow[p].SetZoom( 2 );
		m_sprArrow[p].BeginTweening( 0.3f );
		m_sprArrow[p].SetTweenDiffuseColor( D3DXCOLOR(1,1,1,1) );
		m_sprArrow[p].SetTweenRotationZ( 0 );
		m_sprArrow[p].SetTweenZoom( 1 );

		m_sprArrowShadow[p].SetXY( ARROW_X(iSelection,(PlayerNumber)p), ARROW_Y(iSelection,(PlayerNumber)p) );
		D3DXCOLOR colorOriginal = m_sprArrowShadow[p].GetDiffuseColor();
		m_sprArrowShadow[p].SetDiffuseColor( D3DXCOLOR(0,0,0,0) );
		m_sprArrowShadow[p].BeginTweening( 0.3f );
		m_sprArrowShadow[p].SetTweenDiffuseColor( colorOriginal );
	}

	for( int d=0; d<NUM_DIFFICULTY_ITEMS; d++ )
	{
		const float fPauseTime = d*0.2f;

		if( SelectedSomethingOnPage2() != IsItemOnPage2(d) )	// item isn't on the current page
			continue;	// don't tween

		// set off screen
		m_sprHeader[d].SetXY( ITEM_X(d)-700, ITEM_Y(d) );
	
		m_sprPicture[d].SetXY( ITEM_X(d)-700, ITEM_Y(d) );
		m_sprPicture[d].SetZoomY( 0 );

		// pause
		m_sprHeader[d].BeginTweeningQueued( fPauseTime );

		m_sprPicture[d].BeginTweeningQueued( fPauseTime );

		// fly on
		m_sprHeader[d].BeginTweeningQueued( 0.5f, TWEEN_BIAS_BEGIN );
		m_sprHeader[d].SetTweenXY( ITEM_X(d), ITEM_Y(d) );

		m_sprPicture[d].BeginTweeningQueued( 0.5f, TWEEN_BIAS_BEGIN );
		m_sprPicture[d].SetTweenXY( ITEM_X(d), ITEM_Y(d) );

		// roll down
		m_sprHeader[d].BeginTweeningQueued( 0.3f, TWEEN_BOUNCE_END );

		m_sprPicture[d].BeginTweeningQueued( 0.3f, TWEEN_BOUNCE_END );
		m_sprPicture[d].SetTweenZoomY( 1 );

	}
}
