#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenPrompt

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/


#include "ScreenPrompt.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "RageMusic.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"

const float QUESTION_X	=	CENTER_X;
const float QUESTION_Y	=	CENTER_Y - 60;

const float PROMPT_X	=	CENTER_X;
const float PROMPT_Y	=	CENTER_Y + 120;


ScreenPrompt::ScreenPrompt( ScreenMessage SM_SendWhenDone, CString sText, bool bYesNoPrompt, bool bDefaultAnswer, void(*OnYes)(), void(*OnNo)() )
{
	m_SMSendWhenDone = SM_SendWhenDone;
	m_bYesNoPrompt = bYesNoPrompt;
	m_bAnswer = bDefaultAnswer;
	m_pOnYes = OnYes;
	m_pOnNo = OnNo;


	m_Fade.SetTransitionTime( 0.5f );
	m_Fade.SetDiffuseColor( D3DXCOLOR(0,0,0,0.7f) );
	m_Fade.SetOpened();
	m_Fade.CloseWipingRight();
	this->AddSubActor( &m_Fade );

	m_textQuestion.LoadFromFont( THEME->GetPathTo("Fonts","normal") );
	m_textQuestion.SetText( sText );
	m_textQuestion.SetXY( QUESTION_X, QUESTION_Y );
	this->AddSubActor( &m_textQuestion );

	m_rectAnswerBox.SetDiffuseColor( D3DXCOLOR(0.5f,0.5f,1.0f,0.7f) );
	this->AddSubActor( &m_rectAnswerBox );

	m_textAnswer[0].LoadFromFont( THEME->GetPathTo("Fonts","header1") );
	m_textAnswer[1].LoadFromFont( THEME->GetPathTo("Fonts","header1") );
	m_textAnswer[0].SetY( PROMPT_Y );
	m_textAnswer[1].SetY( PROMPT_Y );
	this->AddSubActor( &m_textAnswer[0] );
	this->AddSubActor( &m_textAnswer[1] );

	

	if( m_bYesNoPrompt )
	{
		m_textAnswer[0].SetText( "NO" );
		m_textAnswer[1].SetText( "YES" );
		m_textAnswer[0].SetX( PROMPT_X+50 );
		m_textAnswer[1].SetX( PROMPT_X-50 );
	}
	else
	{
		m_textAnswer[0].SetText( "OK" );
		m_textAnswer[0].SetX( PROMPT_X );
	}

	m_rectAnswerBox.SetXY( m_textAnswer[m_bAnswer].GetX(), m_textAnswer[m_bAnswer].GetY() );
	m_rectAnswerBox.SetZoomX( m_textAnswer[m_bAnswer].GetWidestLineWidthInSourcePixels()+10.0f );
	m_rectAnswerBox.SetZoomY( 30 );

	m_textAnswer[m_bAnswer].SetEffectGlowing();

	SOUND->PlayOnceStreamed( THEME->GetPathTo("Sounds","menu prompt") );
}

void ScreenPrompt::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
}

void ScreenPrompt::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenPrompt::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( m_Fade.IsOpening() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenPrompt::HandleScreenMessage( const ScreenMessage SM )
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
		SCREENMAN->PopTopScreen( m_SMSendWhenDone );
		break;
	}
}

void ScreenPrompt::MenuLeft( PlayerNumber p )
{
	if( !m_bYesNoPrompt )
		return;

	MenuRight( p );
}

void ScreenPrompt::MenuRight( PlayerNumber p )
{
	if( !m_bYesNoPrompt )
		return;

	m_textAnswer[m_bAnswer].SetEffectNone();
	m_bAnswer = !m_bAnswer;
	m_textAnswer[m_bAnswer].SetEffectGlowing();

	m_rectAnswerBox.BeginTweening( 0.2f );
	m_rectAnswerBox.SetTweenXY( m_textAnswer[m_bAnswer].GetX(), m_textAnswer[m_bAnswer].GetY() );
	m_rectAnswerBox.SetTweenZoomX( m_textAnswer[m_bAnswer].GetWidestLineWidthInSourcePixels()+10.0f );

	SOUND->PlayOnceStreamed( THEME->GetPathTo("Sounds","edit change line") );
}

void ScreenPrompt::MenuStart( PlayerNumber p )
{
	m_Fade.OpenWipingRight( SM_DoneOpeningWipingRight );

	m_textQuestion.BeginTweening( 0.2f );
	m_textQuestion.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );

	m_rectAnswerBox.BeginTweening( 0.2f );
	m_rectAnswerBox.SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );

	m_textAnswer[m_bAnswer].SetEffectNone();

	m_textAnswer[0].BeginTweening( 0.2f );
	m_textAnswer[0].SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );
	m_textAnswer[1].BeginTweening( 0.2f );
	m_textAnswer[1].SetTweenDiffuseColor( D3DXCOLOR(1,1,1,0) );

	SOUND->PlayOnceStreamed( THEME->GetPathTo("Sounds","menu start") );

	if( m_bAnswer )
		m_pOnYes();
	else
		m_pOnNo();
}

void ScreenPrompt::MenuBack( PlayerNumber p )
{

}
