#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: Screen

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "GameManager.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "RageSound.h"
#include "ThemeManager.h"
#include "ScreenManager.h"

Screen::Screen( CString sName )
{
	SetName( sName );
	m_bIsTransparent = false;
}

Screen::~Screen()
{

}

void Screen::AddChild( Actor* pActor )
{
	// add only if the actor is on screen
//	float fX = pActor->GetX();
//	float fY = pActor->GetY();
//	if( SCREEN_LEFT>=fX && fX<=SCREEN_RIGHT && SCREEN_TOP>=fY && fY<=SCREEN_BOTTOM )
		ActorFrame::AddChild( pActor );
}

bool Screen::SortMessagesByDelayRemaining(const Screen::QueuedScreenMessage &m1,
										 const Screen::QueuedScreenMessage &m2)
{
	return m1.fDelayRemaining < m2.fDelayRemaining;
}

void Screen::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	/* We need to ensure two things:
	 * 1. Messages must be sent in the order of delay.  If two messages are sent
	 *    simultaneously, one with a .001 delay and another with a .002 delay, the
	 *    .001 delay message must be sent first.
	 * 2. Messages to be delivered simultaneously must be sent in the order queued.
	 * 
	 * Stable sort by time to ensure #2. */
	stable_sort(m_QueuedMessages.begin(), m_QueuedMessages.end(), SortMessagesByDelayRemaining);

	// update the times of queued ScreenMessages and send if timer has expired
	// The order you remove messages in must be very careful!  Sending a message can 
	// potentially clear all m_QueuedMessages, and set a new state!
	/* Also, it might call ClearMessageQueue() to clear a single message type.
	 * This might clear previous messages on the queue.  So, first apply time to
	 * everything. */
	
	unsigned i;
	for( i=0; i<m_QueuedMessages.size(); i++ )
		m_QueuedMessages[i].fDelayRemaining -= fDeltaTime;

	/* Now dispatch messages.  If the number of messages on the queue changes
	 * within HandleScreenMessage, someone cleared messages on the queue.  This
	 * means we have no idea where 'i' is, so start over. Since we applied time
	 * already, this won't cause messages to be mistimed. */
	for( i=0; i<m_QueuedMessages.size(); i++ )
	{
		if( m_QueuedMessages[i].fDelayRemaining > 0.0f )
			continue; /* not yet */

		/* Remove the message from the list. */
		const ScreenMessage SM = m_QueuedMessages[i].SM;
		m_QueuedMessages.erase(m_QueuedMessages.begin()+i);
		i--;

		unsigned size = m_QueuedMessages.size();

		// send this sucker!
		this->HandleScreenMessage( SM );

		/* If the size changed, start over. */
		if(size != m_QueuedMessages.size())
			i = 0;
	}
}

void Screen::MenuBack(	PlayerNumber pn, const InputEventType type )
{
	if(!PREFSMAN->m_bDelayedEscape || type==IET_SLOW_REPEAT || type==IET_FAST_REPEAT)
		MenuBack(pn); 
}

void Screen::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	/* Don't send release messages with the default handler. */
	if(type == IET_RELEASE) return; // don't care

	/* Don't make the user hold the back button if they're pressing escape. */
	if( DeviceI.device == DEVICE_KEYBOARD  &&  DeviceI.button == SDLK_ESCAPE )
	{
		this->MenuBack( MenuI.player );
		return;
	}

	// default input handler used by most menus
	if( !MenuI.IsValid() )
		return;

	if( !GAMESTATE->IsHumanPlayer(MenuI.player) )
		return;

	switch( MenuI.button )
	{
	case MENU_BUTTON_UP:	this->MenuUp( MenuI.player, type );		return;
	case MENU_BUTTON_DOWN:	this->MenuDown( MenuI.player, type );	return;
	case MENU_BUTTON_LEFT:	this->MenuLeft( MenuI.player, type );	return;
	case MENU_BUTTON_RIGHT:	this->MenuRight( MenuI.player, type );	return;
	case MENU_BUTTON_BACK:	this->MenuBack( MenuI.player, type );	return;
	case MENU_BUTTON_START:	this->MenuStart( MenuI.player, type );	return;
	case MENU_BUTTON_COIN:	this->MenuCoin( MenuI.player, type );	return;
	}
}

bool Screen::ChangeCoinModeInput( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS )
		return false;
	if( DeviceI.device == DEVICE_KEYBOARD && DeviceI.button == SDLK_F3 )
	{
		(int&)PREFSMAN->m_iCoinMode = (PREFSMAN->m_iCoinMode+1) % NUM_COIN_MODES;
		/* ResetGame();
				This causes problems on ScreenIntroMovie, which results in the
				movie being restarted and/or becoming out-of-synch -- Miryokuteki */

		CString sMessage = CoinModeToString( (CoinMode)PREFSMAN->m_iCoinMode );
		sMessage.MakeUpper();
		sMessage = "Coin Mode: " + sMessage;
		SCREENMAN->RefreshCreditsMessages();
		SCREENMAN->SystemMessage( sMessage );
		return true;
	}
	return false;
}

bool Screen::JoinInput( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( !GAMESTATE->m_bPlayersCanJoin )
		return false;

	if( MenuI.IsValid()  &&  MenuI.button==MENU_BUTTON_START )
	{
		/* If this side is already in, don't re-join (and re-pay!). */
		if(GAMESTATE->m_bSideIsJoined[MenuI.player])
			return false;

		/* subtract coins */
		int iCoinsToCharge = 0;
		if( PREFSMAN->m_iCoinMode == COIN_PAY )
			iCoinsToCharge = PREFSMAN->m_iCoinsPerCredit;
		
		if( PREFSMAN->m_bJointPremium )
			if( GAMESTATE->m_MasterPlayerNumber!=PLAYER_INVALID )	// one side already joined
				iCoinsToCharge = 0;

		if( GAMESTATE->m_iCoins < iCoinsToCharge )
			return false;	// not enough coins
		else
			GAMESTATE->m_iCoins -= iCoinsToCharge;

		GAMESTATE->m_bSideIsJoined[MenuI.player] = true;
		if( GAMESTATE->m_MasterPlayerNumber == PLAYER_INVALID )
			GAMESTATE->m_MasterPlayerNumber = MenuI.player;

		SCREENMAN->RefreshCreditsMessages();

		SOUNDMAN->PlayOnce( THEME->GetPathToS("Common start") );

		return true;
	}

	return false;
}


void Screen::MenuCoin( PlayerNumber pn )
{
	// This is now handled globally by Stepmania.cpp  --  Miryokuteki
}

void Screen::PostScreenMessage( const ScreenMessage SM, float fDelay )
{
	ASSERT( fDelay >= 0.0 );

	QueuedScreenMessage QSM;
	QSM.SM = SM;
	QSM.fDelayRemaining = fDelay;
	m_QueuedMessages.push_back( QSM );
}

void Screen::ClearMessageQueue()
{
	m_QueuedMessages.clear(); 
}

void Screen::ClearMessageQueue( const ScreenMessage SM )
{
	for( int i=m_QueuedMessages.size()-1; i>=0; i-- )
		m_QueuedMessages.erase( m_QueuedMessages.begin()+i ); 
}


// Screen classes
#include "ScreenAppearanceOptions.h"
#include "ScreenCaution.h"
#include "ScreenEdit.h"
#include "ScreenEditMenu.h"
#include "ScreenEvaluation.h"
#include "ScreenEz2SelectPlayer.h"
#include "ScreenSelectMode.h"
#include "ScreenGameOver.h"
#include "ScreenGameplay.h"
#include "ScreenGraphicOptions.h"
#include "ScreenHowToPlay.h"
#include "ScreenInputOptions.h"
#include "ScreenMachineOptions.h"
#include "ScreenMapControllers.h"
#include "ScreenMusicScroll.h"
#include "ScreenPlayerOptions.h"
#include "ScreenSandbox.h"
#include "ScreenSelectCourse.h"
#include "ScreenSelectDifficulty.h"
#include "ScreenSelectDifficultyEX.h"
#include "ScreenSelectGame.h"
#include "ScreenSelectGroup.h"
#include "ScreenSelectMusic.h"
#include "ScreenSelectStyle5th.h"
#include "ScreenSelectStyle.h"
#include "ScreenSongOptions.h"
#include "ScreenSoundOptions.h"
#include "ScreenStage.h"
#include "ScreenTest.h"
#include "ScreenTestFonts.h"
#include "ScreenTestSound.h"
#include "ScreenTitleMenu.h"
#include "ScreenEz2SelectMusic.h"
#include "ScreenWarning.h"
#include "ScreenRanking.h"
#include "ScreenMemoryCard.h"
#include "ScreenCompany.h"
#include "ScreenIntroMovie.h"
#include "ScreenAlbums.h"
#include "ScreenLogo.h"
#include "ScreenUnlock.h"
#include "ScreenDemonstration.h"
#include "ScreenInstructions.h"
#include "ScreenNameEntry.h"
#include "ScreenJukebox.h"
#include "ScreenJukeboxMenu.h"
#include "ScreenOptionsMenu.h"
#include "ScreenGameplayOptions.h"
#include "ScreenStyleSplash.h"
#include "ScreenAutogenOptions.h"
#include "ScreenCredits.h"
#include "ScreenSelectCharacter.h"

Screen* Screen::Create( CString sClassName )
{
	Screen *ret = NULL;

#define IS( name )	(sClassName.CompareNoCase(name)==0)

	if(		 IS("ScreenAppearanceOptions") )	ret = new ScreenAppearanceOptions;
	else if( IS("ScreenCaution") )				ret = new ScreenCaution;
	else if( IS("ScreenEdit") )					ret = new ScreenEdit;
	else if( IS("ScreenEditMenu") )				ret = new ScreenEditMenu;
	else if( IS("ScreenEvaluationStage") )		ret = new ScreenEvaluationStage;
	else if( IS("ScreenEvaluationSummary") )	ret = new ScreenEvaluationSummary;
	else if( IS("ScreenEvaluationNonstop") )	ret = new ScreenEvaluationNonstop;
	else if( IS("ScreenEvaluationOni") )		ret = new ScreenEvaluationOni;
	else if( IS("ScreenEvaluationEndless") )	ret = new ScreenEvaluationEndless;
	else if( IS("ScreenEvaluationBattle") )		ret = new ScreenEvaluationBattle;
	else if( IS("ScreenEvaluationRave") )		ret = new ScreenEvaluationRave;
	else if( IS("ScreenEz2SelectPlayer") )		ret = new ScreenEz2SelectPlayer;
	else if( IS("ScreenSelectMode") )			ret = new ScreenSelectMode;
	else if( IS("ScreenGameOver") )				ret = new ScreenGameOver;
	else if( IS("ScreenGameplay") )				ret = new ScreenGameplay;
	else if( IS("ScreenGraphicOptions") )		ret = new ScreenGraphicOptions;
	else if( IS("ScreenHowToPlay") )			ret = new ScreenHowToPlay;
	else if( IS("ScreenInputOptions") )			ret = new ScreenInputOptions;
	else if( IS("ScreenMachineOptions") )		ret = new ScreenMachineOptions;
	else if( IS("ScreenMapControllers") )		ret = new ScreenMapControllers;
	else if( IS("ScreenInputOptions") )			ret = new ScreenInputOptions;
	else if( IS("ScreenMusicScroll") )			ret = new ScreenMusicScroll;
	else if( IS("ScreenPlayerOptions") )		ret = new ScreenPlayerOptions;
	else if( IS("ScreenSandbox") )				ret = new ScreenSandbox;
	else if( IS("ScreenSelectCourse") )			ret = new ScreenSelectCourse;
	else if( IS("ScreenSelectDifficulty") )		ret = new ScreenSelectDifficulty;
	else if( IS("ScreenSelectDifficultyEX") )	ret = new ScreenSelectDifficultyEX;
	else if( IS("ScreenSelectGame") )			ret = new ScreenSelectGame;
	else if( IS("ScreenSelectGroup") )			ret = new ScreenSelectGroup;
	else if( IS("ScreenSelectMusic") )			ret = new ScreenSelectMusic;
	else if( IS("ScreenSelectStyle5th") )		ret = new ScreenSelectStyle5th;
	else if( IS("ScreenSelectStyle") )			ret = new ScreenSelectStyle;
	else if( IS("ScreenSongOptions") )			ret = new ScreenSongOptions;
	else if( IS("ScreenStage") )				ret = new ScreenStage;
	else if( IS("ScreenTest") )					ret = new ScreenTest;
	else if( IS("ScreenTestFonts") )			ret = new ScreenTestFonts;
	else if( IS("ScreenTestSound") )			ret = new ScreenTestSound;
	else if( IS("ScreenTitleMenu") )			ret = new ScreenTitleMenu;
	else if( IS("ScreenEz2SelectMusic") )		ret = new ScreenEz2SelectMusic;
	else if( IS("ScreenWarning") )				ret = new ScreenWarning;
	else if( IS("ScreenRanking") )				ret = new ScreenRanking;
	else if( IS("ScreenMemoryCard") )			ret = new ScreenMemoryCard;
	else if( IS("ScreenCompany") )				ret = new ScreenCompany;
	else if( IS("ScreenIntroMovie") )			ret = new ScreenIntroMovie;
	else if( IS("ScreenAlbums") )				ret = new ScreenAlbums;
	else if( IS("ScreenLogo") )					ret = new ScreenLogo;
	else if( IS("ScreenUnlock") )				ret = new ScreenUnlock;
	else if( IS("ScreenDemonstration") )		ret = (ScreenGameplay*)new ScreenDemonstration;
	else if( IS("ScreenInstructions") )			ret = new ScreenInstructions;
	else if( IS("ScreenNameEntry") )			ret = new ScreenNameEntry;
	else if( IS("ScreenJukebox") )				ret = new ScreenJukebox;
	else if( IS("ScreenJukeboxMenu") )			ret = new ScreenJukeboxMenu;
	else if( IS("ScreenOptionsMenu") )			ret = new ScreenOptionsMenu;
	else if( IS("ScreenSoundOptions") )			ret = new ScreenSoundOptions;
	else if( IS("ScreenGameplayOptions") )		ret = new ScreenGameplayOptions;
	else if( IS("ScreenStyleSplash") )			ret = new ScreenStyleSplash;
	else if( IS("ScreenAutogenOptions") )		ret = new ScreenAutogenOptions;
	else if( IS("ScreenCredits") )				ret = new ScreenCredits;
	else if( IS("ScreenSelectCharacter") )		ret = new ScreenSelectCharacter;
	else
		RageException::Throw( "Invalid Screen class name '%s'", sClassName.GetString() );
	return ret;
}