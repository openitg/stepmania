#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: TransitionKeepAlive

 Desc: See header.

 Copyright (c) 2001-2002 by the persons listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

#include "RageUtil.h"

#include "TransitionKeepAlive.h"
#include "GameConstantsAndTypes.h"
#include "ThemeManager.h"


const float KEEP_ALIVE_FORWARD_TRANSITION_TIME	=	0.8f;
const float KEEP_ALIVE_BACKWARD_TRANSITION_TIME	=	0.4f;

TransitionKeepAlive::TransitionKeepAlive()
{
	m_sprLogo.Load( THEME->GetPathTo(GRAPHIC_KEEP_ALIVE) );
	m_sprLogo.SetXY( CENTER_X, CENTER_Y );

	m_rect.StretchTo( CRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT) );
}

void TransitionKeepAlive::Update( float fDeltaTime )
{
	// hack:  Smooth out the big hickups.
	fDeltaTime = min( fDeltaTime, 1/15.0f );

	Transition::Update( fDeltaTime );
}


void TransitionKeepAlive::RenderPrimitives()
{
	if( m_TransitionState == closing_left )	// we're going back
	{
		// Draw a fade

		const float fPercentageOpaque = 1 - GetPercentageOpen();
		if( fPercentageOpaque == 0 )
			return;	// draw nothing

		D3DXCOLOR colorTemp = m_Color * fPercentageOpaque;
		m_rect.SetDiffuseColor( colorTemp );
		m_rect.Draw();
	}
	else	// we're going forward
	{
		// draw keep alive graphic

		float fPercentClosed = 1 - this->GetPercentageOpen();
		fPercentClosed = min( 1, fPercentClosed*2 );
		const float fPercentColor = fPercentClosed;
		const float fPercentAlpha = min( fPercentClosed * 2, 1 );

		m_sprLogo.SetDiffuseColor( D3DXCOLOR(fPercentColor,fPercentColor,fPercentColor,fPercentAlpha) );
		m_sprLogo.SetZoomY( fPercentClosed );
		if( fPercentClosed > 0 )
			m_sprLogo.Draw();
	}
}


void TransitionKeepAlive::OpenWipingRight( WindowMessage send_when_done )
{
	this->SetTransitionTime( KEEP_ALIVE_FORWARD_TRANSITION_TIME );

	Transition::OpenWipingRight( send_when_done );
}

void TransitionKeepAlive::OpenWipingLeft(  WindowMessage send_when_done )
{
	this->SetTransitionTime( KEEP_ALIVE_BACKWARD_TRANSITION_TIME );

	Transition::OpenWipingLeft( send_when_done );
}

void TransitionKeepAlive::CloseWipingRight(WindowMessage send_when_done )
{
	this->SetTransitionTime( KEEP_ALIVE_FORWARD_TRANSITION_TIME );

	Transition::CloseWipingRight( send_when_done );
}

void TransitionKeepAlive::CloseWipingLeft( WindowMessage send_when_done )
{
	this->SetTransitionTime( KEEP_ALIVE_BACKWARD_TRANSITION_TIME );

	Transition::CloseWipingLeft( send_when_done );
}

