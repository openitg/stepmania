/****************************************
ScreenEzSelectPlayer,cpp
Desc: See Header
Copyright (C):
Andrew Livy

NOTES: Although cleaner, can still do with
a polish :)
*****************************************/

/* Includes */

#include "stdafx.h"
#include "ScreenEz2SelectStyle.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "RageMusic.h"
#include "ScreenTitleMenu.h"
#include "ScreenCaution.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "ScreenSelectDifficulty.h"
#include "ScreenSandbox.h"
#include "GameManager.h"
#include "RageLog.h"
#include "AnnouncerManager.h"
#include "GameConstantsAndTypes.h"
#include "Background.h"
#include "ScreenSelectGroup.h"
#include "GameState.h"
#include "RageException.h"
#include "RageTimer.h"

/* Constants */

const ScreenMessage SM_GoToPrevState		=	ScreenMessage(SM_User + 1);
const ScreenMessage SM_GoToNextState		=	ScreenMessage(SM_User + 2);

enum DStyles {
	DS_EASY,
	DS_HARD,
	DS_REAL,
	DS_CLUB
};

const float TWEEN_TIME		= 0.35f;

#define SKIP_SELECT_DIFFICULTY		THEME->GetMetricB("General","SkipSelectDifficulty")


float ez2p_lasttimercheck[2];
int ez2p_bounce=0; // used for the bouncing of the '1p' and '2p' images
int ez2p_direct=0; // direction of the bouncing of the '1p' and '2p' images

/************************************
ScreenEz2SelectStyle (Constructor)
Desc: Sets up the screen display
************************************/

ScreenEz2SelectStyle::ScreenEz2SelectStyle()
{
	LOG->Trace( "ScreenEz2SelectStyle::ScreenEz2SelectStyle()" );

	m_iSelectedStyle=DS_EASY; // start on EASY

	// Load in the sprites we will be working with.
	for( int i=0; i<NUM_EZ2STYLE_GRAPHICS; i++ )
	{
		m_sprBackground[i].Load( THEME->GetPathTo("Graphics",ssprintf("select style preview game %d style %d",GAMESTATE->m_CurGame,i)) );
		m_sprBackground[i].SetXY( CENTER_X, CENTER_Y );
		m_sprBackground[i].SetZoom( 1 );
		this->AddSubActor( &m_sprBackground[i] );
	}

	m_ScrList.SetNumberVisibleElements( 3 );
	m_ScrList.CreateNewElement( "select style info game 2 style 1" );
	m_ScrList.CreateNewElement( "select style info game 2 style 2" );
	m_ScrList.CreateNewElement( "select style info game 2 style 3" );
	m_ScrList.CreateNewElement( "select style info game 2 style 0" ); // Excess so that the user is tricked into thinking
	m_ScrList.CreateNewElement( "select style info game 2 style 1" ); // the list is infinite
	m_ScrList.CreateNewElement( "select style info game 2 style 2" );
	m_ScrList.CreateNewElement( "select style info game 2 style 3" );
	m_ScrList.CreateNewElement( "select style info game 2 style 0" );
	m_ScrList.CreateNewElement( "select style info game 2 style 1" );
	m_ScrList.CreateNewElement( "select style info game 2 style 2" );
	m_ScrList.CreateNewElement( "select style info game 2 style 3" );
	m_ScrList.CreateNewElement( "select style info game 2 style 0" );
	m_ScrList.SetXY(CENTER_X, CENTER_Y);
	m_ScrList.SetCurrentPosition( DS_EASY );

	this->AddSubActor( &m_ScrList );

	m_Menu.Load( 	
		THEME->GetPathTo("Graphics","select style background"), 
		THEME->GetPathTo("Graphics","select style top edge"),
		ssprintf("Press %c on the pad you wish to play on", char(4) ),
		false, true, 40 
		);
	this->AddSubActor( &m_Menu );

	m_soundSelect.Load( THEME->GetPathTo("Sounds","menu start") );

	GAMESTATE->m_bPlayersCanJoin = true;

	m_Menu.TweenOnScreenFromBlack( SM_None );
}

/************************************
~ScreenEz2SelectStyle (Destructor)
Desc: Writes line to log when screen
is terminated.
************************************/
ScreenEz2SelectStyle::~ScreenEz2SelectStyle()
{
	LOG->Trace( "ScreenEz2SelectStyle::~ScreenEz2SelectStyle()" );
}


/************************************
DrawPrimitives
Desc: Draws the screen =P
************************************/

void ScreenEz2SelectStyle::DrawPrimitives()
{
/*	if (m_iSelectedPlayer != 2) // no need to animate graphics if we have no graphics to animate ;)
	{
		AnimateGraphics();
	}
*/
	AnimateBackground();

	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();

}

/************************************
Input
Desc: Handles player input.
************************************/
void ScreenEz2SelectStyle::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenEz2SelectStyle::Input()" );

	if( m_Menu.IsClosing() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}

/************************************
HandleScreenMessage
Desc: Handles Screen Messages and changes
	game states.
************************************/
void ScreenEz2SelectStyle::HandleScreenMessage( const ScreenMessage SM )
{
	Screen::HandleScreenMessage( SM );

	switch( SM )
	{
	case SM_MenuTimer:
		m_soundSelect.PlayRandom();
		GAMESTATE->m_PlayMode = PLAY_MODE_ARCADE;
		SCREENMAN->SetNewScreen( new ScreenSelectGroup );

		break;
	case SM_GoToPrevState:
		MUSIC->Stop();
		SCREENMAN->SetNewScreen( new ScreenTitleMenu );
		break;
	case SM_GoToNextState:
		if( SKIP_SELECT_DIFFICULTY )
			SCREENMAN->SetNewScreen( new ScreenSelectGroup );
		else
			SCREENMAN->SetNewScreen( new ScreenSelectDifficulty );
		break;
	}
}


/************************************
MenuBack
Desc: Actions performed when a player 
presses the button bound to back
************************************/

void ScreenEz2SelectStyle::MenuBack( const PlayerNumber p )
{
	MUSIC->Stop();

	m_Menu.TweenOffScreenToBlack( SM_GoToPrevState, true );
	GAMESTATE->m_CurStyle = STYLE_NONE; // Make sure that both players can scroll around title menu...

//	m_Fade.CloseWipingLeft( SM_GoToPrevState );

//	TweenOffScreen();
}

/************************************
MenuDown
Desc: Actions performed when a player 
presses the button bound to down
************************************/

void ScreenEz2SelectStyle::MenuDown( const PlayerNumber p )
{
	MenuStart(p);
}


/************************************
SetFadedStyles
Desc: Fades out non-highlighted items
depending on the users choice.
************************************/
void ScreenEz2SelectStyle::SetFadedStyles()
{

}

/************************************
MenuRight
Desc: Actions performed when a player 
presses the button bound to right
************************************/
void ScreenEz2SelectStyle::MenuRight( PlayerNumber p )
{
	m_ScrList.ShiftRight();
	if (m_iSelectedStyle == 3) // wrap around
		m_iSelectedStyle = 0;
	else
		m_iSelectedStyle++;

	switch (m_iSelectedStyle)
	{
		case DS_EASY: GAMESTATE->m_CurStyle = STYLE_EZ2_SINGLE;	break;
		case DS_HARD: GAMESTATE->m_CurStyle = STYLE_EZ2_SINGLE_HARD; break;
		case DS_REAL: GAMESTATE->m_CurStyle = STYLE_EZ2_REAL; break;
		case DS_CLUB: GAMESTATE->m_CurStyle = STYLE_EZ2_DOUBLE; break;
	}
}

/************************************
MenuLeft
Desc: Actions performed when a player 
presses the button bound to left
************************************/
void ScreenEz2SelectStyle::MenuLeft( PlayerNumber p )
{
	m_ScrList.ShiftLeft();
	if (m_iSelectedStyle == 0) // wrap around
		m_iSelectedStyle = 3;
	else
		m_iSelectedStyle--;

	switch (m_iSelectedStyle)
	{
		case DS_EASY: GAMESTATE->m_CurStyle = STYLE_EZ2_SINGLE;	break;
		case DS_HARD: GAMESTATE->m_CurStyle = STYLE_EZ2_SINGLE_HARD; break;
		case DS_REAL: GAMESTATE->m_CurStyle = STYLE_EZ2_REAL; break;
		case DS_CLUB: GAMESTATE->m_CurStyle = STYLE_EZ2_DOUBLE; break;
	}
}

/************************************
MenuStart
Desc: Actions performed when a player 
presses the button bound to start
************************************/
void ScreenEz2SelectStyle::MenuStart( PlayerNumber p )
{

//	if( p!=PLAYER_INVALID  && !GAMESTATE->m_bIsJoined[p] )
//	{
//		SOUND->PlayOnceStreamed( THEME->GetPathTo("Sounds","menu start") );
//		GAMESTATE->m_bIsJoined[p] = true;
//		SCREENMAN->RefreshCreditsMessages();
//		m_soundSelect.PlayRandom();
//		return;	// don't fall through
//	}

	m_soundSelect.PlayRandom();
	this->ClearMessageQueue();
	GAMESTATE->m_PlayMode = PLAY_MODE_ARCADE;
//	GAMESTATE->m_bPlayersCanJoin = false;
	m_Menu.TweenOffScreenToMenu( SM_GoToNextState );
}

/************************************
TweenOffScreen
Desc: Squashes graphics before the screen
changes state.
************************************/
void ScreenEz2SelectStyle::TweenOffScreen()
{

}

/************************************
AnimateGraphics
Desc: Animates the 1p/2p selection
************************************/
void ScreenEz2SelectStyle::AnimateGraphics()
{

	//if (bounce < 10 && direct == 0 && wait == 2) // Bounce 1p/2p up
/*	if (TIMER->GetTimeSinceStart() > ez2p_lasttimercheck[0] + 0.01f && ez2p_direct == 0)
	{
		ez2p_lasttimercheck[0] = TIMER->GetTimeSinceStart();
		ez2p_bounce+=1;
	
		m_sprPly[2].SetXY( OPT_XP[2], OPT_YP[2] - ez2p_bounce);
		m_sprPly[3].SetXY( OPT_XP[3], OPT_YP[3] - ez2p_bounce);


		if (ez2p_bounce == 10)
		{
			ez2p_direct = 1;
		}
	}
	else if (TIMER->GetTimeSinceStart() > ez2p_lasttimercheck[0] + 0.01f && ez2p_direct == 1) // bounce 1p/2p down
	{
		ez2p_lasttimercheck[0] = TIMER->GetTimeSinceStart();
		ez2p_bounce-=1;

		m_sprPly[2].SetXY( OPT_XP[2], OPT_YP[2] - ez2p_bounce);
		m_sprPly[3].SetXY( OPT_XP[3], OPT_YP[3] - ez2p_bounce);

		if (ez2p_bounce == 0)
		{
			ez2p_direct = 0;
		}
	}
*/
}

/************************************
AnimateBackground
Desc: Animates the Background
************************************/
void ScreenEz2SelectStyle::AnimateBackground()
{
	if ((m_iSelectedStyle == 0) || (m_iSelectedPlayer == 2 && m_iSelectedStyle == 3)) // EASY background
	{
		m_sprBackground[3].SetHeight(SCREEN_HEIGHT * 1.7f);
		m_sprBackground[3].SetWidth(SCREEN_WIDTH * 1.7f);
		m_sprBackground[3].SetEffectSpinning(1.0f);
	}
	else if (m_iSelectedStyle == 3 && m_iSelectedPlayer != 2) // CLUB background
	{
		m_sprBackground[3].SetHeight(0);
		m_sprBackground[3].SetWidth(0);
		m_sprBackground[3].SetEffectNone();
		m_sprBackground[2].SetHeight(SCREEN_HEIGHT * 3.3f);
		m_sprBackground[2].SetWidth(SCREEN_WIDTH * 3.3f);
		m_sprBackground[2].SetEffectSpinning(0.5f);
		m_sprBackground[2].SetXY( CENTER_X, -250 );
	}
	else if (m_iSelectedStyle == 2) // REAL background
	{
		m_sprBackground[3].SetHeight(0);
		m_sprBackground[3].SetWidth(0);
		m_sprBackground[3].SetEffectNone();
		m_sprBackground[2].SetHeight(0);
		m_sprBackground[2].SetWidth(0);
		m_sprBackground[2].SetEffectNone();
		m_sprBackground[1].SetHeight(SCREEN_HEIGHT * 1.7f);
		m_sprBackground[1].SetWidth(SCREEN_WIDTH * 1.7f);
		m_sprBackground[1].SetEffectSpinning(2.1f);
	}
	else if (m_iSelectedStyle == 1) // HARD background
	{
		m_sprBackground[3].SetHeight(0);
		m_sprBackground[3].SetWidth(0);
		m_sprBackground[3].SetEffectNone();
		m_sprBackground[2].SetHeight(0);
		m_sprBackground[2].SetWidth(0);
		m_sprBackground[2].SetEffectNone();
		m_sprBackground[1].SetHeight(0);
		m_sprBackground[1].SetWidth(0);
		m_sprBackground[1].SetEffectNone();
		m_sprBackground[0].SetHeight(SCREEN_HEIGHT * 1.7f);
		m_sprBackground[0].SetWidth(SCREEN_WIDTH * 1.7f);
		m_sprBackground[0].SetEffectSpinning(1.0f);
	}

}

/************************************
TweenPlyOffScreen
Desc: Squashes Player Graphics off screen
When selected.
************************************/
void ScreenEz2SelectStyle::TweenPlyOffScreen()
{

}