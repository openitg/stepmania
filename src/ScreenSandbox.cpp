#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 File: ScreenSandbox.h

 Desc: Area for testing.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/


#include "ScreenSandbox.h"
#include "ScreenManager.h"
#include "RageMusic.h"
#include "ScreenTitleMenu.h"
#include "ScreenSelectMusic.h"
#include "GameConstantsAndTypes.h"
#include "ThemeManager.h"
#include "Quad.h"


ScreenSandbox::ScreenSandbox()
{	
	m_text.Load( THEME->GetPathTo(FONT_NORMAL) );
	m_text.SetXY( CENTER_X, CENTER_Y );
	this->AddActor( &m_text );

	m_sound.Load( THEME->GetPathTo(SOUND_MENU_MUSIC) );
	m_sound.SetPositionSeconds( -10 );
	m_sound.Play();

	//this->AddActor( &m_spr );
}


void ScreenSandbox::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
	m_sound.Update( fDeltaTime );
	m_text.SetText( ssprintf("%f", m_sound.GetPositionSeconds()) );
}

void ScreenSandbox::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenSandbox::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( MenuI.IsValid() )
	{
		switch( MenuI.button )
		{
		case MENU_BUTTON_LEFT:
			break;
		case MENU_BUTTON_RIGHT:
			break;
		case MENU_BUTTON_UP:
	//		m_BlurTest.StartFocusing();
			break;
		case MENU_BUTTON_DOWN:
	//		m_BlurTest.StartBlurring();
			break;
		case MENU_BUTTON_BACK:
			//SCREENMAN->SetNewScreen( new ScreenTitleMenu );
			return;
		}
	}

//	m_sprBG.SetEffectCamelion( 5, D3DXCOLOR(1,0.8f,0.8f,1), D3DXCOLOR(1,0.2f,0.2f,1) );
}


void ScreenSandbox::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_DoneClosingWipingLeft:
		break;
	case SM_DoneClosingWipingRight:
		break;
	case SM_DoneOpeningWipingLeft:
		break;
	case SM_DoneOpeningWipingRight:
		break;
	}
}