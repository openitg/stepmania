#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: MenuTimer

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "MenuTimer.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "ThemeManager.h"
#include "ScreenManager.h"

const float TIMER_SECONDS = 40;

MenuTimer::MenuTimer()
{
	m_fSecondsLeft = TIMER_SECONDS;

	m_textDigit1.Load( THEME->GetPathTo(FONT_TIMER_NUMBERS) );
	m_textDigit1.TurnShadowOff();
	m_textDigit1.SetXY( -20, 0 );
	this->AddActor( &m_textDigit1 );

	m_textDigit2.Load( THEME->GetPathTo(FONT_TIMER_NUMBERS) );
	m_textDigit2.TurnShadowOff();
	m_textDigit2.SetXY( +20, 0 );
	this->AddActor( &m_textDigit2 );
}


void MenuTimer::Update( float fDeltaTime ) 
{ 
	ActorFrame::Update( fDeltaTime );

	float fOldSecondsLeft = m_fSecondsLeft;
	float fNewSecondsLeft = fOldSecondsLeft - fDeltaTime;

	if( fOldSecondsLeft > 5  &&  fNewSecondsLeft < 5 )	// transition to below 5
	{
		m_textDigit1.SetEffectGlowing( 10, D3DXCOLOR(1,0,0,0), D3DXCOLOR(1,0,0,1) );
		m_textDigit2.SetEffectGlowing( 10, D3DXCOLOR(1,0,0,0), D3DXCOLOR(1,0,0,1) );
	}
	else if( fOldSecondsLeft > 0  &&  fNewSecondsLeft < 0 )	// transition to below 0
	{
		SCREENMAN->SendMessageToTopScreen( SM_MenuTimer, 0 );
	}

	m_fSecondsLeft = fNewSecondsLeft;
	m_fSecondsLeft = max( 0, m_fSecondsLeft );

	m_textDigit1.SetText( ssprintf("%d", ((int)m_fSecondsLeft)/10) ); 
	m_textDigit2.SetText( ssprintf("%d", ((int)m_fSecondsLeft)%10) ); 

	// "flip" the numbers
	float fRemainder = m_fSecondsLeft - (int)m_fSecondsLeft;
	float fDistFromNearestNumber = min( fRemainder, 1-fRemainder );	// this is between 0 and 0.5;

	if( m_fSecondsLeft < 4.5f )
	{
		m_textDigit1.SetZoomX( min(1, fDistFromNearestNumber*8) ); 
		m_textDigit2.SetZoomX( min(1, fDistFromNearestNumber*8) ); 
	}
}


void MenuTimer::StopTimer()
{

}

void MenuTimer::StallTimer()
{

}
