#include "stdafx.h"
#include "GroupList.h"
#include "ThemeManager.h"
#include "SongManager.h"

#define BUTTON_X			THEME->GetMetricF("SelectGroup","ButtonX")
#define BUTTON_START_Y		THEME->GetMetricF("SelectGroup","ButtonStartY")
#define BUTTON_SPACING_Y	THEME->GetMetricF("SelectGroup","ButtonSpacingY")
#define BUTTON_SELECTED_X	THEME->GetMetricF("SelectGroup","ButtonSelectedX")

GroupList::GroupList()
{
	m_iSelection = m_iTop = 0;
}

void GroupList::DoneAddingGroups()
{
	int i;

	for( i=0; i<min(m_textLabels.GetSize(), MAX_GROUPS_ONSCREEN); i++ )
	{
		m_sprButton[i].Load( THEME->GetPathTo("Graphics","select group button") );
		m_sprButton[i].SetXY( BUTTON_X, BUTTON_START_Y + i*BUTTON_SPACING_Y );
		this->AddSubActor( &m_sprButton[i] );
		this->AddSubActor( &m_screenLabels[i] );
	}

	for( i=0; i<min(m_textLabels.GetSize(), MAX_GROUPS_ONSCREEN); i++ )
	{
		m_screenLabels[i].LoadFromFont( THEME->GetPathTo("Fonts","normal") );
		m_screenLabels[i].SetXY( BUTTON_X, BUTTON_START_Y + i*BUTTON_SPACING_Y );
		m_screenLabels[i].SetZoom( 0.8f );
		m_screenLabels[i].SetShadowLength( 2 );

		CString sGroupName = m_textLabels[i];
	}

	SetLabels();
	AfterChange();
}

void GroupList::SetLabels()
{
	for( int i=0; i<min(m_textLabels.GetSize(), MAX_GROUPS_ONSCREEN); i++ )
	{
		CString &label = m_textLabels[m_iTop+i];
		m_screenLabels[i].SetText( SONGMAN->ShortenGroupName( label ) );
		
		if( m_iTop+i == 0 )	m_screenLabels[i].TurnRainbowOn();
		else {
			m_screenLabels[i].TurnRainbowOff();
			m_screenLabels[i].SetDiffuseColor( SONGMAN->GetGroupColor(label) );
		}
	}
}

void GroupList::BeforeChange()
{
	int iSel = m_iSelection-m_iTop;

	m_sprButton[iSel].BeginTweening( 0.2f );
	m_sprButton[iSel].SetTweenX( BUTTON_X );
	m_sprButton[iSel].SetEffectNone();

	m_screenLabels[iSel].BeginTweening( 0.2f );
	m_screenLabels[iSel].SetTweenX( BUTTON_X );
	m_screenLabels[iSel].SetEffectNone();
}


void GroupList::AfterChange()
{
	int iSel = m_iSelection-m_iTop;

	m_sprButton[iSel].BeginTweening( 0.2f );
	m_sprButton[iSel].SetTweenX( BUTTON_SELECTED_X );
	m_sprButton[iSel].SetEffectGlowing();

	m_screenLabels[iSel].BeginTweening( 0.2f );
	m_screenLabels[iSel].SetTweenX( BUTTON_SELECTED_X );
	m_screenLabels[iSel].SetEffectGlowing();
}

void GroupList::Up()
{
	BeforeChange();

	if( m_iSelection == 0 )
		SetSelection(m_textLabels.GetSize()-1);
	else
		SetSelection(m_iSelection-1);

	AfterChange();
}

void GroupList::Down()
{
	BeforeChange();

	SetSelection((m_iSelection+1) % m_textLabels.GetSize());
	
	AfterChange();
}

void GroupList::AddGroup(CString name)
{
	m_textLabels.Add(name);
}

void GroupList::SetSelection( int sel )
{
	BeforeChange();

	if( sel == m_iSelection ) ;
	else if( sel == m_iSelection+1 ) {
		if( m_iSelection >= MAX_GROUPS_ONSCREEN/2 ) 
			m_iTop++;
	} else if( sel == m_iSelection-1 ) {
		if(m_iSelection < m_textLabels.GetSize() - MAX_GROUPS_ONSCREEN/2)
			m_iTop--;
	} else {
		/* We're jumping somewhere else; just put the top somewhere
		 * reasonable. */
		m_iTop = sel - MAX_GROUPS_ONSCREEN/2;
	}

	m_iSelection=sel;
	m_iTop = clamp( m_iTop, 0, m_textLabels.GetSize()-MAX_GROUPS_ONSCREEN );

	/* The current selection must always be visible. */
	ASSERT( m_iTop <= m_iSelection );
	ASSERT( m_iTop+MAX_GROUPS_ONSCREEN > m_iSelection );

	SetLabels();
	AfterChange();
}


void GroupList::TweenOnScreen()
{
	for( int i=0; i<min(m_textLabels.GetSize(), MAX_GROUPS_ONSCREEN); i++ )
	{
		m_sprButton[i].SetX( BUTTON_X+400 );
		m_sprButton[i].BeginTweeningQueued( 0.1f*i, TWEEN_BOUNCE_END );
		m_sprButton[i].BeginTweeningQueued( 0.2f, TWEEN_BOUNCE_END );
		m_sprButton[i].SetTweenX( BUTTON_X );

		m_screenLabels[i].SetX( BUTTON_X+400 );
		m_screenLabels[i].BeginTweeningQueued( 0.1f*i, TWEEN_BOUNCE_END );
		m_screenLabels[i].BeginTweeningQueued( 0.2f, TWEEN_BOUNCE_END );
		m_screenLabels[i].SetTweenX( BUTTON_X );
	}
}

void GroupList::TweenOffScreen()
{
	for( int i=0; i<min(m_textLabels.GetSize(), MAX_GROUPS_ONSCREEN); i++ )
	{
		if( i == m_iSelection )
			m_sprButton[i].BeginTweeningQueued( 1.0f, TWEEN_BOUNCE_BEGIN );
		else
			m_sprButton[i].BeginTweeningQueued( 0.1f*i, TWEEN_BOUNCE_BEGIN );
		m_sprButton[i].BeginTweeningQueued( 0.2f, TWEEN_BOUNCE_BEGIN );
		m_sprButton[i].SetTweenX( BUTTON_X+400 );

		if( i == m_iSelection )
			m_screenLabels[i].BeginTweeningQueued( 1.0f, TWEEN_BOUNCE_BEGIN );
		else
			m_screenLabels[i].BeginTweeningQueued( 0.1f*i, TWEEN_BOUNCE_BEGIN );
		m_screenLabels[i].BeginTweeningQueued( 0.2f, TWEEN_BOUNCE_BEGIN );
		m_screenLabels[i].SetTweenX( BUTTON_X+400 );
	}

}

