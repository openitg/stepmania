/* GameInput - An input event specific to an InputScheme defined by a logical controller and button. */

#ifndef GAME_INPUT_H
#define GAME_INPUT_H

#include "EnumHelper.h"

class InputScheme;

enum GameController
{
	GAME_CONTROLLER_1 = 0,	// left controller
	GAME_CONTROLLER_2,	// right controller
	NUM_GameController,	// leave this at the end
	GameController_Invalid,
};
#define FOREACH_GameController( gc ) FOREACH_ENUM2( GameController, gc )

typedef int GameButton;
RString GameButtonToString( const InputScheme* pInputs, GameButton i );
RString GameButtonToLocalizedString( const InputScheme* pInputs, GameButton i );
GameButton StringToGameButton( const InputScheme* pInputs, const RString& s );

const GameButton NUM_GameButton = 20;
const GameButton GameButton_Invalid = NUM_GameButton+1;
#define FOREACH_GameButton( gb ) FOREACH_ENUM2( GameButton, gb )

enum	// DanceButtons
{
	DANCE_BUTTON_LEFT,
	DANCE_BUTTON_RIGHT,
	DANCE_BUTTON_UP,
	DANCE_BUTTON_DOWN,
	DANCE_BUTTON_UPLEFT,
	DANCE_BUTTON_UPRIGHT,
	DANCE_BUTTON_START,
	DANCE_BUTTON_SELECT,
	DANCE_BUTTON_BACK,
	DANCE_BUTTON_MENULEFT,
	DANCE_BUTTON_MENURIGHT,
	DANCE_BUTTON_MENUUP,
	DANCE_BUTTON_MENUDOWN,
	DANCE_BUTTON_COIN,
	DANCE_BUTTON_OPERATOR,
	NUM_DANCE_BUTTONS,		// leave this at the end
};

enum	// PumpButtons
{
	PUMP_BUTTON_UPLEFT,
	PUMP_BUTTON_UPRIGHT,
	PUMP_BUTTON_CENTER,
	PUMP_BUTTON_DOWNLEFT,
	PUMP_BUTTON_DOWNRIGHT,
	PUMP_BUTTON_START,
	PUMP_BUTTON_SELECT,
	PUMP_BUTTON_BACK,
	PUMP_BUTTON_MENULEFT,
	PUMP_BUTTON_MENURIGHT,
	PUMP_BUTTON_MENUUP,
	PUMP_BUTTON_MENUDOWN,
	PUMP_BUTTON_COIN,
	PUMP_BUTTON_OPERATOR,
	NUM_PUMP_BUTTONS,		// leave this at the end
};

enum	// EZ2Buttons
{
	EZ2_BUTTON_FOOTUPLEFT,
	EZ2_BUTTON_FOOTUPRIGHT,
	EZ2_BUTTON_FOOTDOWN,
	EZ2_BUTTON_HANDUPLEFT,
	EZ2_BUTTON_HANDUPRIGHT,
	EZ2_BUTTON_HANDLRLEFT,
	EZ2_BUTTON_HANDLRRIGHT,
	EZ2_BUTTON_START,
	EZ2_BUTTON_SELECT,
	EZ2_BUTTON_BACK,
	EZ2_BUTTON_MENULEFT,
	EZ2_BUTTON_MENURIGHT,
	EZ2_BUTTON_MENUUP,
	EZ2_BUTTON_MENUDOWN,
	EZ2_BUTTON_COIN,
	EZ2_BUTTON_OPERATOR,
	NUM_EZ2_BUTTONS,		// leave this at the end
};

enum	// ParaButtons
{
	PARA_BUTTON_LEFT,
	PARA_BUTTON_UPLEFT,
	PARA_BUTTON_UP,
	PARA_BUTTON_UPRIGHT,
	PARA_BUTTON_RIGHT,
	PARA_BUTTON_START,
	PARA_BUTTON_SELECT,
	PARA_BUTTON_BACK,
	PARA_BUTTON_MENULEFT,
	PARA_BUTTON_MENURIGHT,
	PARA_BUTTON_MENUUP,
	PARA_BUTTON_MENUDOWN,
	PARA_BUTTON_COIN,
	PARA_BUTTON_OPERATOR,
	NUM_PARA_BUTTONS,		// leave this at the end
};

enum // 3DDX Buttons
{
	DS3DDX_BUTTON_HANDLEFT,
	DS3DDX_BUTTON_FOOTDOWNLEFT,
	DS3DDX_BUTTON_FOOTUPLEFT,
	DS3DDX_BUTTON_HANDUP,
	DS3DDX_BUTTON_HANDDOWN,
	DS3DDX_BUTTON_FOOTUPRIGHT,
	DS3DDX_BUTTON_FOOTDOWNRIGHT,
	DS3DDX_BUTTON_HANDRIGHT,
	DS3DDX_BUTTON_START,
	DS3DDX_BUTTON_SELECT,
	DS3DDX_BUTTON_BACK,
	DS3DDX_BUTTON_MENULEFT,
	DS3DDX_BUTTON_MENURIGHT,
	DS3DDX_BUTTON_MENUUP,
	DS3DDX_BUTTON_MENUDOWN,
	DS3DDX_BUTTON_COIN,
	DS3DDX_BUTTON_OPERATOR,
	NUM_DS3DDX_BUTTONS, // leave this at the end.
};

enum // BEAT Buttons
{
	BEAT_BUTTON_KEY1,
	BEAT_BUTTON_KEY2,
	BEAT_BUTTON_KEY3,
	BEAT_BUTTON_KEY4,
	BEAT_BUTTON_KEY5,
	BEAT_BUTTON_KEY6,
	BEAT_BUTTON_KEY7,
	BEAT_BUTTON_SCRATCHUP,
	/* XXX special case: this button is an alias of BEAT_BUTTON_SCRATCHUP for track
	 * matching. */
	BEAT_BUTTON_SCRATCHDOWN,
	BEAT_BUTTON_START,
	BEAT_BUTTON_SELECT,
	BEAT_BUTTON_BACK,
	BEAT_BUTTON_MENULEFT,
	BEAT_BUTTON_MENURIGHT,
	BEAT_BUTTON_MENUUP,
	BEAT_BUTTON_MENUDOWN,
	BEAT_BUTTON_COIN,
	BEAT_BUTTON_OPERATOR,
	NUM_BEAT_BUTTONS, // leave this at the end.
};

enum	// ManiaxButtons
{
	MANIAX_BUTTON_HANDUPLEFT,
	MANIAX_BUTTON_HANDUPRIGHT,
	MANIAX_BUTTON_HANDLRLEFT,
	MANIAX_BUTTON_HANDLRRIGHT,
	MANIAX_BUTTON_START,
	MANIAX_BUTTON_SELECT,
	MANIAX_BUTTON_BACK,
	MANIAX_BUTTON_MENULEFT,
	MANIAX_BUTTON_MENURIGHT,
	MANIAX_BUTTON_MENUUP,
	MANIAX_BUTTON_MENUDOWN,
	MANIAX_BUTTON_COIN,
	MANIAX_BUTTON_OPERATOR,
	NUM_MANIAX_BUTTONS,		// leave this at the end
};

enum	// TechnoButtons
{
	TECHNO_BUTTON_LEFT,
	TECHNO_BUTTON_RIGHT,
	TECHNO_BUTTON_UP,
	TECHNO_BUTTON_DOWN,
	TECHNO_BUTTON_UPLEFT,
	TECHNO_BUTTON_UPRIGHT,
	TECHNO_BUTTON_CENTER,
	TECHNO_BUTTON_DOWNLEFT,
	TECHNO_BUTTON_DOWNRIGHT,
	TECHNO_BUTTON_START,
	TECHNO_BUTTON_SELECT,
	TECHNO_BUTTON_BACK,
	TECHNO_BUTTON_MENULEFT,
	TECHNO_BUTTON_MENURIGHT,
	TECHNO_BUTTON_MENUUP,
	TECHNO_BUTTON_MENUDOWN,
	TECHNO_BUTTON_COIN,
	TECHNO_BUTTON_OPERATOR,
	NUM_TECHNO_BUTTONS,		// leave this at the end
};

enum	// PnM Buttons
{
	POPN_BUTTON_LEFT_WHITE,
	POPN_BUTTON_LEFT_YELLOW,
	POPN_BUTTON_LEFT_GREEN,
	POPN_BUTTON_LEFT_BLUE,
	POPN_BUTTON_RED,
	POPN_BUTTON_RIGHT_BLUE,
	POPN_BUTTON_RIGHT_GREEN,
	POPN_BUTTON_RIGHT_YELLOW,
	POPN_BUTTON_RIGHT_WHITE,
	POPN_BUTTON_START,
	POPN_BUTTON_SELECT,
	POPN_BUTTON_BACK,
	POPN_BUTTON_MENULEFT,
	POPN_BUTTON_MENURIGHT,
	POPN_BUTTON_MENUUP,
	POPN_BUTTON_MENUDOWN,
	POPN_BUTTON_COIN,
	POPN_BUTTON_OPERATOR,
	NUM_POPN_BUTTONS,		// leave this at the end
};

enum	// LightsButtons
{
	LIGHTS_BUTTON_MARQUEE_UP_LEFT,
	LIGHTS_BUTTON_MARQUEE_UP_RIGHT,
	LIGHTS_BUTTON_MARQUEE_LR_LEFT,
	LIGHTS_BUTTON_MARQUEE_LR_RIGHT,
	LIGHTS_BUTTON_BUTTONS_LEFT,
	LIGHTS_BUTTON_BUTTONS_RIGHT,
	LIGHTS_BUTTON_BASS_LEFT,
	LIGHTS_BUTTON_BASS_RIGHT,
	LIGHTS_BUTTON_START,
	LIGHTS_BUTTON_SELECT,
	LIGHTS_BUTTON_BACK,
	LIGHTS_BUTTON_MENULEFT,
	LIGHTS_BUTTON_MENURIGHT,
	LIGHTS_BUTTON_MENUUP,
	LIGHTS_BUTTON_MENUDOWN,
	LIGHTS_BUTTON_COIN,
	LIGHTS_BUTTON_OPERATOR,
	NUM_LIGHTS_BUTTONS,		// leave this at the end
};


struct GameInput
{
	GameInput(): controller(GameController_Invalid), button(GameButton_Invalid) { }

	GameInput( GameController c, GameButton b ): controller(c), button(b) { }

	GameController	controller;
	GameButton	button;

	bool operator==( const GameInput &other ) const { return controller == other.controller && button == other.button; };
	bool operator<( const GameInput &other ) const
	{
		if( controller < other.controller )
			return true;
		else if( controller > other.controller )
			return false;
		return button < other.button;
	}

	inline bool IsValid() const { return controller != GameController_Invalid; };
	inline void MakeInvalid() { controller = GameController_Invalid; button = GameButton_Invalid; };

	RString ToString( const InputScheme* pInputs ) const;
	bool FromString( const InputScheme* pInputs, const RString &s );
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
