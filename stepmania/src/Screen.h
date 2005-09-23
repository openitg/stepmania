/* Screen - Class that holds a screen-full of Actors. */

#ifndef SCREEN_H
#define SCREEN_H

#include "ActorFrame.h"
#include "ScreenMessage.h"
#include "InputFilter.h"
#include "ThemeMetric.h"
#include "PlayerNumber.h"

class InputEventPlus;
struct MenuInput;
class Screen;
typedef Screen* (*CreateScreenFn)(const CString& sClassName);
void RegisterScreenClass( const CString& sClassName, CreateScreenFn pfn );

// Each Screen class should have a REGISTER_SCREEN_CLASS in its CPP file.
#define REGISTER_SCREEN_CLASS( className ) \
	static Screen* Create##className( const CString &sName ) { Screen *pRet = new className( sName ); pRet->Init(); return pRet; } \
	struct Register##className { \
		Register##className() { RegisterScreenClass( #className,Create##className); } \
	}; \
	static Register##className register_##className;

enum ScreenType
{
	attract,
	game_menu,
	gameplay,
	system_menu
};

class Screen : public ActorFrame
{
public:
	Screen( CString sName );	// enforce that all screens have m_sName filled in
	virtual ~Screen();

	/* This is called immediately after construction, to allow initializing after all derived classes
	 * exist. */
	virtual void Init();

	/* This is called immediately before the screen is used. */
	virtual void BeginScreen();

	virtual void Update( float fDeltaTime );
	virtual bool OverlayInput( const InputEventPlus &input );
	virtual void Input( const InputEventPlus &input );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	void PostScreenMessage( const ScreenMessage SM, float fDelay );
	void ClearMessageQueue();
	void ClearMessageQueue( const ScreenMessage SM );	// clear of a specific SM

	virtual bool UsesBackground() const { return true; }	// override and set false if this screen shouldn't load a background
	virtual ScreenType GetScreenType() const { return ALLOW_OPERATOR_MENU_BUTTON ? game_menu : system_menu; }

	static bool JoinInput( const MenuInput &MenuI );	// return true if a player joined

	//
	// Lua
	//
	virtual void PushSelf( lua_State *L );

protected:
	// structure for holding messages sent to a Screen
	struct QueuedScreenMessage {
		ScreenMessage SM;  
		float fDelayRemaining;
	};
	vector<QueuedScreenMessage>	m_QueuedMessages;
	static bool SortMessagesByDelayRemaining(const QueuedScreenMessage &m1, const QueuedScreenMessage &m2);

	ThemeMetric<bool>	ALLOW_OPERATOR_MENU_BUTTON;

	virtual CString GetNextScreen() const;
	virtual CString GetPrevScreen() const;

	// If these are left blank, the NextScreen and PrevScreen metrics will be used.
	CString m_sNextScreen, m_sPrevScreen;
	ScreenMessage m_smSendOnPop;

public:

	// let subclass override if they want
	virtual void MenuUp(	const InputEventPlus &input );
	virtual void MenuDown(	const InputEventPlus &input );
	virtual void MenuLeft(	const InputEventPlus &input );
	virtual void MenuRight( const InputEventPlus &input );
	virtual void MenuStart( const InputEventPlus &input );
	virtual void MenuSelect( const InputEventPlus &input );
	virtual void MenuBack(	const InputEventPlus &input );
	virtual void MenuCoin(	const InputEventPlus &input );

	virtual void MenuUp(	PlayerNumber pn )	{}
	virtual void MenuDown(	PlayerNumber pn )	{}
	virtual void MenuLeft(	PlayerNumber pn )	{}
	virtual void MenuRight( PlayerNumber pn )	{}
	virtual void MenuStart( PlayerNumber pn )	{}
	virtual void MenuSelect( PlayerNumber pn )	{}
	virtual void MenuBack(	PlayerNumber pn )	{}
	virtual void MenuCoin(	PlayerNumber pn );
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
