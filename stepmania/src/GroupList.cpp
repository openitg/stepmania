#include "global.h"
#include "GroupList.h"
#include "ThemeManager.h"
#include "SongManager.h"
#include "RageLog.h"
#include "ThemeMetric.h"
#include "ActorUtil.h"

/* If this actor is used anywhere other than SelectGroup, we
 * can add a setting that changes which metric group we pull
 * settings out of, so it can be configured separately. */
static const ThemeMetric<float>			START_X						("GroupList","StartX");
static const ThemeMetric<float>			START_Y						("GroupList","StartY");
static const ThemeMetric<float>			SPACING_X					("GroupList","SpacingX");
static const ThemeMetric<float>			SPACING_Y					("GroupList","SpacingY");

const int MAX_GROUPS_ONSCREEN = 7;

GroupList::GroupList()
{
	m_iSelection = m_iTop = 0;
}

GroupList::~GroupList()
{
	for( unsigned i = 0; i < m_sprButtons.size(); ++i )
	{
		delete m_sprButtons[i];
		delete m_textLabels[i];
		delete m_ButtonFrames[i];
	}
}


bool GroupList::ItemIsOnScreen( int n ) const
{
	const int offset = n - m_iTop;
	return offset >= 0 && offset < MAX_GROUPS_ONSCREEN;
}

void GroupList::Load( const vector<RString>& asGroupNames )
{
	m_asLabels = asGroupNames;

	m_Frame.SetName( "Frame" );
	this->AddChild( &m_Frame );
	ActorUtil::LoadAllCommands( m_Frame, "GroupList" );

	for( unsigned i=0; i < m_asLabels.size(); i++ )
	{
		Sprite *button = new Sprite;
		button->SetName( "Button" );
		ActorUtil::LoadAllCommands( *button, "GroupList" );

		BitmapText *label = new BitmapText;
		label->SetName( "Label" );
		ActorUtil::LoadAllCommands( *label, "GroupList" );

		ActorFrame *frame = new ActorFrame;
		frame->SetName( "ButtonFrame" );
		ActorUtil::LoadAllCommands( *frame, "GroupList" );

		m_sprButtons.push_back( button );
		m_textLabels.push_back( label );
		m_ButtonFrames.push_back( frame );

		button->Load( THEME->GetPathG("GroupList","bar") );
		label->LoadFromFont( THEME->GetPathF("GroupList","label") );
		label->SetShadowLength( 2 );
		label->SetText( SONGMAN->ShortenGroupName( asGroupNames[i] ) );
		
		frame->AddChild( button );
		frame->AddChild( label );
		m_Frame.AddChild( frame );

		button->SetXY( START_X + i*SPACING_X, START_Y + i*SPACING_Y );
		label->SetXY( START_X + i*SPACING_X, START_Y + i*SPACING_Y );

		if( i == 0 )
		{
			label->SetRainbowScroll( true );
		}
		else 
		{
			label->SetRainbowScroll( false );
			label->SetDiffuse( SONGMAN->GetSongGroupColor(asGroupNames[i]) );
		}

		m_bHidden.push_back( ItemIsOnScreen(i) );

		ResetTextSize( i );
	}

	AfterChange();
}

void GroupList::ResetTextSize( int i )
{
	BitmapText *label = m_textLabels[i];
	const float fTextWidth = (float)label->GetUnzoomedWidth();
	const float fButtonWidth = m_sprButtons[i]->GetZoomedWidth();

	float fZoom = fButtonWidth/fTextWidth;
	fZoom = min( fZoom, 0.8f );
	
	label->SetZoomX( fZoom );
	label->SetZoomY( 0.8f );
}

void GroupList::BeforeChange()
{
	m_ButtonFrames[m_iSelection]->PlayCommand( "LoseFocus" );
}


void GroupList::AfterChange()
{
	m_Frame.StopTweening();
	m_Frame.PlayCommand( "ScrollTween" );
	m_Frame.SetY( -m_iTop*SPACING_Y );

	for( int i=0; i < (int) m_asLabels.size(); i++ )
	{
		const bool IsHidden = !ItemIsOnScreen(i);
		const bool WasHidden = m_bHidden[i];

		if( IsHidden && !WasHidden )
		{
			m_ButtonFrames[i]->PlayCommand( "HideItem" );
		}
		else if( !IsHidden && WasHidden )
		{
			m_ButtonFrames[i]->PlayCommand( "ShowItem" );
			ResetTextSize( i );
		}

		m_bHidden[i] = IsHidden;
	}

	m_ButtonFrames[m_iSelection]->PlayCommand( "GainFocus" );
}

void GroupList::Up()
{
	BeforeChange();

	if( m_iSelection == 0 )
		SetSelection(m_asLabels.size()-1);
	else
		SetSelection(m_iSelection-1);

	AfterChange();
}

void GroupList::Down()
{
	BeforeChange();

	SetSelection((m_iSelection+1) % m_asLabels.size());
	
	AfterChange();
}

void GroupList::SetSelection( unsigned sel )
{
	BeforeChange();

	m_iSelection=sel;

	if( (int)m_asLabels.size() <= MAX_GROUPS_ONSCREEN ||
		  sel <= MAX_GROUPS_ONSCREEN/2 )
		m_iTop = 0;
	else if ( sel >= m_asLabels.size() - MAX_GROUPS_ONSCREEN/2 )
		m_iTop = m_asLabels.size() - MAX_GROUPS_ONSCREEN;
	else
		m_iTop = sel - MAX_GROUPS_ONSCREEN/2;

	/* The current selection must always be visible. */
	ASSERT( m_iTop <= m_iSelection );
	ASSERT( m_iTop+MAX_GROUPS_ONSCREEN > m_iSelection );

	AfterChange();
}


void GroupList::TweenOnScreen()
{
	for( int i=0; i < (int) m_asLabels.size(); i++ )
	{
		const int offset = max(0, i-m_iTop);
		
		m_sprButtons[i]->SetX( 400 );
		m_textLabels[i]->SetX( 400 );

		m_sprButtons[i]->BeginTweening( 0.1f*offset, TWEEN_ACCELERATE );
		m_textLabels[i]->BeginTweening( 0.1f*offset, TWEEN_ACCELERATE );
		
		m_sprButtons[i]->BeginTweening( 0.2f, TWEEN_ACCELERATE );
		m_textLabels[i]->BeginTweening( 0.2f, TWEEN_ACCELERATE );
		
		m_sprButtons[i]->SetX( 0 );
		m_textLabels[i]->SetX( 0 );

		/* If this item isn't visible, hide it and skip tweens. */
		if( !ItemIsOnScreen(i) )
		{
			m_ButtonFrames[i]->PlayCommand( "HideItem" );
			
			m_sprButtons[i]->FinishTweening();
			m_textLabels[i]->FinishTweening();
		}
	}
}

void GroupList::TweenOffScreen()
{
	for( int i=m_iTop; i < min(m_iTop+MAX_GROUPS_ONSCREEN, (int) m_asLabels.size()); i++ )
	{
		const int offset = max(0, i-m_iTop);
		if( i == m_iSelection )
		{
			m_sprButtons[i]->BeginTweening( 1.0f, TWEEN_DECELERATE );
			m_textLabels[i]->BeginTweening( 1.0f, TWEEN_DECELERATE );
		}
		else
		{
			m_sprButtons[i]->BeginTweening( 0.1f*offset, TWEEN_DECELERATE );
			m_textLabels[i]->BeginTweening( 0.1f*offset, TWEEN_DECELERATE );
		}

		m_sprButtons[i]->BeginTweening( 0.2f, TWEEN_DECELERATE );
		m_textLabels[i]->BeginTweening( 0.2f, TWEEN_DECELERATE );

		m_sprButtons[i]->SetX( 400 );
		m_textLabels[i]->SetX( 400 );
	}
}

/*
 * (c) 2001-2003 Chris Danford, Glenn Maynard
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
