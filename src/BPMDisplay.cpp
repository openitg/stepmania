#include "global.h"
/*
-----------------------------------------------------------------------------
 File: BPMDisplay.h

 Desc: A graphic displayed in the BPMDisplay during Dancing.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/

#include "BPMDisplay.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Course.h"
#include "StyleDef.h"


#define NORMAL_COLOR		THEME->GetMetricC(m_sName,"NormalColor")
#define CHANGE_COLOR		THEME->GetMetricC(m_sName,"ChangeColor")
#define EXTRA_COLOR			THEME->GetMetricC(m_sName,"ExtraColor")
#define CYCLE				THEME->GetMetricB(m_sName,"Cycle")
#define SEPARATOR			THEME->GetMetric (m_sName,"Separator")
#define NO_BPM_TEXT			THEME->GetMetric (m_sName,"NoBPMText")

BPMDisplay::BPMDisplay()
{
	m_fBPMFrom = m_fBPMTo = 0;
	m_iCurrentBPM = 0;
	m_BPMS.push_back(0);
	m_fPercentInState = 0;
	m_fCycleTime = 1.0f;
}

void BPMDisplay::Load()
{
	m_textBPM.SetName( "Text" );
	m_textBPM.LoadFromNumbers( THEME->GetPathToN(m_sName) );
	SET_XY_AND_ON_COMMAND( m_textBPM );
	m_textBPM.SetDiffuse( NORMAL_COLOR );
	this->AddChild( &m_textBPM );

	m_sprLabel.Load( THEME->GetPathToG(ssprintf("%s label", m_sName.c_str())) );
	m_sprLabel->SetName( "Label" );
	SET_XY_AND_ON_COMMAND( m_sprLabel );
	m_sprLabel->SetDiffuse( NORMAL_COLOR );
	this->AddChild( m_sprLabel );
}

float BPMDisplay::GetActiveBPM() const
{
	return m_fBPMTo + (m_fBPMFrom-m_fBPMTo)*m_fPercentInState;
}

void BPMDisplay::Update( float fDeltaTime ) 
{ 
	ActorFrame::Update( fDeltaTime ); 

	if( !CYCLE )
		return;
	if( m_BPMS.size() == 0 )
		return; /* no bpm */

	m_fPercentInState -= fDeltaTime / m_fCycleTime;
	if( m_fPercentInState < 0 )
	{
		// go to next state
		m_fPercentInState = 1;		// reset timer

		m_iCurrentBPM = (m_iCurrentBPM + 1) % m_BPMS.size();
		m_fBPMFrom = m_fBPMTo;
		m_fBPMTo = m_BPMS[m_iCurrentBPM];

		if(m_fBPMTo == -1)
		{
			m_fBPMFrom = -1;
			m_textBPM.SetText( (RandomFloat(0,1)>0.90) ? "xxx" : ssprintf("%03.0f",RandomFloat(0,600)) ); 
		} else if(m_fBPMFrom == -1)
			m_fBPMFrom = m_fBPMTo;
	}

	// update m_textBPM
	if( m_fBPMTo != -1)
	{
		const float fActualBPM = GetActiveBPM();
		m_textBPM.SetText( ssprintf("%03.0f", fActualBPM) );
	}
}


void BPMDisplay::SetBPMRange( const DisplayBpms &bpms )
{
	ASSERT( !bpms.vfBpms.empty() );

	const vector<float> &BPMS = bpms.vfBpms;

	unsigned i;
	bool AllIdentical = true;
	for( i = 0; i < BPMS.size(); ++i )
	{
		if( i > 0 && BPMS[i] != BPMS[i-1] )
			AllIdentical = false;
	}

	if( !CYCLE )
	{
		int MinBPM=99999999;
		int MaxBPM=-99999999;
		for( i = 0; i < BPMS.size(); ++i )
		{
			MinBPM = min( MinBPM, (int) roundf(BPMS[i]) );
			MaxBPM = max( MaxBPM, (int) roundf(BPMS[i]) );
		}
		if( MinBPM == MaxBPM )
		{
			if( MinBPM == -1 )
				m_textBPM.SetText( "..." ); // random
			else
				m_textBPM.SetText( ssprintf("%i", MinBPM) );
		}
		else
			m_textBPM.SetText( ssprintf("%i%s%i", MinBPM, SEPARATOR.c_str(), MaxBPM) );
	} else {
		m_BPMS.clear();
		for( i = 0; i < BPMS.size(); ++i )
		{
			m_BPMS.push_back(BPMS[i]);
			if( BPMS[i] != -1 )
				m_BPMS.push_back(BPMS[i]); /* hold */
		}

		m_iCurrentBPM = min(1u, m_BPMS.size()); /* start on the first hold */
		m_fBPMFrom = BPMS[0];
		m_fBPMTo = BPMS[0];
		m_fPercentInState = 1;
	}

	m_textBPM.StopTweening();
	m_sprLabel->StopTweening();
	m_textBPM.BeginTweening(0.5f);
	m_sprLabel->BeginTweening(0.5f);
	if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
	{
		m_textBPM.SetDiffuse( EXTRA_COLOR );
		m_sprLabel->SetDiffuse( EXTRA_COLOR );		
	}
	else if( !AllIdentical )
	{
		m_textBPM.SetDiffuse( CHANGE_COLOR );
		m_sprLabel->SetDiffuse( CHANGE_COLOR );
	}
	else
	{
		m_textBPM.SetDiffuse( NORMAL_COLOR );
		m_sprLabel->SetDiffuse( NORMAL_COLOR );
	}
}

void BPMDisplay::CycleRandomly()
{
	DisplayBpms bpms;
	bpms.Add(-1);
	SetBPMRange( bpms );

	m_fCycleTime = 0.2f;
}

void BPMDisplay::NoBPM()
{
	m_BPMS.clear();
	m_textBPM.SetText( NO_BPM_TEXT ); 

	m_textBPM.SetDiffuse( NORMAL_COLOR );
	m_sprLabel->SetDiffuse( NORMAL_COLOR );
}

void BPMDisplay::SetBPM( const Song* pSong )
{
	ASSERT( pSong );
	switch( pSong->m_DisplayBPMType )
	{
	case Song::DISPLAY_ACTUAL:
	case Song::DISPLAY_SPECIFIED:
		{
			DisplayBpms bpms;
			pSong->GetDisplayBpms( bpms );
			SetBPMRange( bpms );
			m_fCycleTime = 1.0f;
		}
		break;
	case Song::DISPLAY_RANDOM:
		CycleRandomly();
		break;
	default:
		ASSERT(0);
	}
}

void BPMDisplay::SetBPM( const Course* pCourse )
{
	ASSERT( pCourse );

	StepsType st = GAMESTATE->GetCurrentStyleDef()->m_StepsType;
	Trail *pTrail = pCourse->GetTrail( st, COURSE_DIFFICULTY_REGULAR );

	ASSERT( pTrail->m_vEntries.size() );

	DisplayBpms bpms;
	pTrail->GetDisplayBpms( bpms );
	
	SetBPMRange( bpms );
	m_fCycleTime = 0.2f;
}

void BPMDisplay::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();
}
