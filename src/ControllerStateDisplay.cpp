#include "global.h"
#include "ControllerStateDisplay.h"
#include "EnumHelper.h"
#include "RageUtil.h"
#include "RageInputDevice.h"
#include "ThemeManager.h"
#include "InputMapper.h"
#include "InputFilter.h"
#include "RageLog.h"
#include "LuaBinding.h"

static const char *ControllerStateButtonNames[] = {
	"Up",
	"Down",
	"Left",
	"Right",
};
XToString( ControllerStateButton, NUM_ControllerStateButton );

static const DeviceButton ControllerStateButtonToDeviceButton[] = {
	JOY_UP,
	JOY_DOWN,
	JOY_LEFT,
	JOY_RIGHT,
};
// TODO: Generalize for all game types
static const GameButton ControllerStateButtonToGameButton[] = {
	DANCE_BUTTON_UP,
	DANCE_BUTTON_DOWN,
	DANCE_BUTTON_LEFT,
	DANCE_BUTTON_RIGHT,
};

REGISTER_ACTOR_CLASS( ControllerStateDisplay )

ControllerStateDisplay::ControllerStateDisplay()
{
	m_bIsLoaded = false;

	this->AddChild( &m_sprFrame );
	FOREACH_ENUM2( ControllerStateButton, b )
		this->AddChild( &m_Buttons[b].spr );
}

void ControllerStateDisplay::LoadMultiPlayer( MultiPlayer mp )
{
	m_bIsLoaded = true;

	m_sprFrame.Load( THEME->GetPathG("ControllerStateDisplay", "frame") );

	FOREACH_ENUM2( ControllerStateButton, b )
	{
		Button &button = m_Buttons[ b ];

		//LOG->Warn( "csd: %d %d", mp, b );

		RString sPath = THEME->GetPathG( "ControllerStateDisplay", ControllerStateButtonToString(b) );
		button.spr.Load( sPath );
		
		button.di = DeviceInput( INPUTMAPPER->MultiPlayerToInputDevice(mp), ControllerStateButtonToDeviceButton[b] );
	}
}

void ControllerStateDisplay::LoadGameController( GameController gc )
{
	m_bIsLoaded = true;

	m_sprFrame.Load( THEME->GetPathG("ControllerStateDisplay", "frame") );

	FOREACH_ENUM2( ControllerStateButton, b )
	{
		Button &button = m_Buttons[ b ];

		//LOG->Warn( "csd: %d %d", mp, b );

		RString sPath = THEME->GetPathG( "ControllerStateDisplay", ControllerStateButtonToString(b) );
		button.spr.Load( sPath );
		
		button.gi = GameInput( gc, ControllerStateButtonToGameButton[b] );
	}
}

void ControllerStateDisplay::Update( float fDelta )
{
	FOREACH_ENUM2( ControllerStateButton, b )
	{
		Button &button = m_Buttons[ b ];

		bool bVisible = false;

		if( button.gi.IsValid() )
			bVisible = INPUTMAPPER->IsButtonDown(button.gi);
		else if( button.di.IsValid() )
			bVisible = INPUTFILTER->IsBeingPressed(button.di);

		button.spr.SetVisible( bVisible );
	}
}

// lua start
class LunaControllerStateDisplay: public Luna<ControllerStateDisplay>
{
public:
	LunaControllerStateDisplay() { LUA->Register( Register ); }

	static int LoadGameController( T* p, lua_State *L )	{ p->LoadGameController( (GameController)IArg(1) ); return 0; }

	static void Register(lua_State *L) 
	{
		ADD_METHOD( LoadGameController );
		Luna<T>::Register( L );
	}
};

LUA_REGISTER_DERIVED_CLASS( ControllerStateDisplay, Actor )

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
