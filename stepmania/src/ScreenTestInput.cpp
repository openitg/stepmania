#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenTestInput

 Desc: Where the player maps device input to pad input.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenTestInput.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "GameManager.h"
#include "GameState.h"
#include "RageSounds.h"
#include "ThemeManager.h"
#include "RageDisplay.h"



ScreenTestInput::ScreenTestInput( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenTestInput::ScreenTestInput()" );
	

	m_textInputs.LoadFromFont( THEME->GetPathToF("Common normal") );
	m_textInputs.SetText( "" );
	m_textInputs.SetXY( CENTER_X, CENTER_Y );
	m_textInputs.SetDiffuse( RageColor(1,1,1,1) );
	m_textInputs.SetZoom( 0.8f );
	this->AddChild( &m_textInputs );

	SOUND->PlayMusic( THEME->GetPathToS("ScreenTestInput music") );
}



ScreenTestInput::~ScreenTestInput()
{
	LOG->Trace( "ScreenTestInput::~ScreenTestInput()" );
}


void ScreenTestInput::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );

	CStringArray asInputs;

	DeviceInput di;

	for( int d=0; d<NUM_INPUT_DEVICES; d++ )
	{
		for( int b=0; b<NUM_DEVICE_BUTTONS[d]; b++ )
		{
			di.device = (InputDevice)d;
			di.button = b;

			if( INPUTFILTER->IsBeingPressed(di) )
			{
				CString sTemp;
				sTemp += di.GetDescription();
				
				GameInput gi;
				if( INPUTMAPPER->DeviceToGame(di,gi) )
				{
					CString sName = GAMESTATE->GetCurrentGameDef()->m_szButtonNames[gi.button];
					CString sSecondary = GAMESTATE->GetCurrentGameDef()->m_szSecondaryFunction[gi.button];
					
					sTemp += ssprintf("  (Controller %d %s)  %s", gi.controller+1, sName.c_str(), sSecondary.c_str() );
				}
				else
				{
					sTemp += "  (not mapped)";
				}

				asInputs.push_back( sTemp );
			}
		}
	}

	m_textInputs.SetText( join( "\n ", asInputs ) );
}


void ScreenTestInput::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenTestInput::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default handler
}

void ScreenTestInput::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToNextScreen:
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		break;
	}
}

void ScreenTestInput::MenuStart( PlayerNumber pn )
{
	MenuBack(pn);
}

void ScreenTestInput::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		SCREENMAN->PlayStartSound();
		StartTransitioning( SM_GoToPrevScreen );		
	}
}
