#include "global.h"
#include "ScreenContinue.h"
#include "ScreenManager.h"
#include "ActorUtil.h"
#include "GameState.h"
#include "RageLog.h"
#include "InputEventPlus.h"
#include "MenuTimer.h"


REGISTER_SCREEN_CLASS( ScreenContinue );

void ScreenContinue::BeginScreen()
{
	GAMESTATE->SetCurrentStyle( NULL );

	// unjoin all players with 0 stages left
	FOREACH_HumanPlayer( p )
	{
		bool bPlayerDone = GAMESTATE->m_iPlayerStageTokens[p] <= 0;
		if( bPlayerDone )
			GAMESTATE->UnjoinPlayer( p );
	}

	ScreenWithMenuElements::BeginScreen();
}

void ScreenContinue::Input( const InputEventPlus &input )
{
	if( input.MenuI == GAME_BUTTON_COIN &&  input.type == IET_FIRST_PRESS )
		ResetTimer();

	if( input.MenuI == MENU_BUTTON_START  &&  input.type == IET_FIRST_PRESS  &&  GAMESTATE->JoinInput(input.pn) )
	{
		SCREENMAN->PlayStartSound();

		bool bAllPlayersAreEnabled = true;
		FOREACH_ENUM( PlayerNumber, p )
		{
			if( !GAMESTATE->IsPlayerEnabled(p) )
				bAllPlayersAreEnabled = false;
		}
		if( bAllPlayersAreEnabled )
		{
			m_MenuTimer->Stop();
			if( !IsTransitioning() )
				StartTransitioningScreen( SM_GoToNextScreen );
		}
		else
		{
			ResetTimer();
		}
		return;	// handled
	}

	if( IsTransitioning() )
		return;

	if( input.type == IET_FIRST_PRESS  &&  GAMESTATE->IsHumanPlayer(input.pn) )
	{
		switch( input.MenuI )
		{
		case GAME_BUTTON_START:
		case MENU_BUTTON_UP:
		case MENU_BUTTON_DOWN:
		case MENU_BUTTON_LEFT:
		case MENU_BUTTON_RIGHT:
			float fSeconds = floorf(m_MenuTimer->GetSeconds()) - 0.0001f;
			fSeconds = max( fSeconds, 0.0001f ); // don't set to 0
			m_MenuTimer->SetSeconds( fSeconds );
			return;	// handled
		}
	}

	ScreenWithMenuElements::Input( input );
}

void ScreenContinue::HandleScreenMessage( const ScreenMessage SM )
{
	RString s = ScreenMessageHelpers::NumberToString( SM );

	if( SM == SM_MenuTimer )
	{
		if( !IsTransitioning() )
			StartTransitioningScreen( SM_GoToNextScreen );
		return;
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

/*
 * (c) 2004 Chris Danford
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