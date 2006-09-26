#include "global.h"
#include "GameInput.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "Game.h"
#include "ThemeManager.h"


static const char *GameControllerNames[] = {
	"Controller1",
	"Controller2",
};
XToString( GameController, NUM_GameController );
StringToX( GameController );


RString GameButtonToString( const Game* pGame, GameButton i )
{
	return pGame->m_szButtonNames[i];
}

RString GameButtonToLocalizedString( const Game* pGame, GameButton i )
{
	return THEME->GetString( "GameButton", GameButtonToString(pGame,i) );
}

GameButton StringToGameButton( const Game* pGame, const RString& s )
{
	for( int i=0; i<pGame->m_iButtonsPerController; i++ )
	{
		if( s == GameButtonToString(pGame,(GameButton)i) )
			return (GameButton)i;
	}
	return GAME_BUTTON_INVALID;
}


RString GameInput::ToString( const Game* pGame ) const
{
	return GameControllerToString(controller) + RString("_") + GameButtonToString(pGame,button);
}

bool GameInput::FromString( const Game* pGame, const RString &s )
{ 
	char szController[32] = "";
	char szButton[32] = "";

	if( 2 != sscanf( s, "%31[^_]_%31[^_]", szController, szButton ) )
	{
		controller = GameController_Invalid;
		return false;
	}

	controller = StringToGameController( szController );
	button = StringToGameButton( pGame, szButton );
	return true;
};

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
