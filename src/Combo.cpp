#include "global.h"
#include "Combo.h"
#include "ThemeManager.h"
#include "StatsManager.h"
#include "GameState.h"
#include "song.h"
#include "PlayerState.h"
#include "ActorUtil.h"

Combo::Combo()
{
	m_iLastSeenCombo = -1;
	m_pPlayerState = NULL;
	m_pPlayerStageStats = NULL;

	this->SubscribeToMessage( Message_BeatCrossed );
}

void Combo::Load( const PlayerState *pPlayerState, const PlayerStageStats *pPlayerStageStats )
{
	ASSERT( m_SubActors.empty() );	// don't load twice

	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;

	SHOW_COMBO_AT			.Load(m_sName,"ShowComboAt");
	NUMBER_MIN_ZOOM			.Load(m_sName,"NumberMinZoom");
	NUMBER_MAX_ZOOM			.Load(m_sName,"NumberMaxZoom");
	NUMBER_MAX_ZOOM_AT		.Load(m_sName,"NumberMaxZoomAt");
	PULSE_COMMAND			.Load(m_sName,"PulseCommand");
	FULL_COMBO_W3_COMMAND		.Load(m_sName,"FullComboW3Command");
	FULL_COMBO_W2_COMMAND		.Load(m_sName,"FullComboW2Command");
	FULL_COMBO_W1_COMMAND		.Load(m_sName,"FullComboW1Command");
	FULL_COMBO_BROKEN_COMMAND	.Load(m_sName,"FullComboBrokenCommand");
	MISS_COMBO_COMMAND		.Load(m_sName,"MissComboCommand");
	
	m_spr100Milestone.Load( THEME->GetPathG(m_sName,"100milestone") );
	this->AddChild( m_spr100Milestone );

	m_spr1000Milestone.Load( THEME->GetPathG(m_sName,"1000milestone") );
	this->AddChild( m_spr1000Milestone );

	m_sprComboLabel.Load( THEME->GetPathG(m_sName,"label") );
	m_sprComboLabel->SetName( "Label" );
	m_sprComboLabel->SetVisible( false );
	LOAD_ALL_COMMANDS( m_sprComboLabel );
	this->AddChild( m_sprComboLabel );
	m_sprComboLabel->PlayCommand( "On" );

	m_sprMissesLabel.Load( THEME->GetPathG(m_sName,"misses") );
	m_sprMissesLabel->SetName( "Label" );
	m_sprMissesLabel->SetVisible( false );
	LOAD_ALL_COMMANDS( m_sprMissesLabel );
	this->AddChild( m_sprMissesLabel );
	m_sprMissesLabel->PlayCommand( "On" );

	m_textNumber.LoadFromFont( THEME->GetPathF(m_sName,"numbers") );
	m_textNumber.SetName( "Number" );
	m_textNumber.SetVisible( false );
	LOAD_ALL_COMMANDS( m_textNumber );
	this->AddChild( &m_textNumber );
	m_textNumber.PlayCommand( "On" );
}

void Combo::SetCombo( int iCombo, int iMisses )
{
	bool bComboOfMisses = iMisses > 0;
	int iNum = bComboOfMisses ? iMisses : iCombo;
	bool bShowCombo = iNum >= (int)SHOW_COMBO_AT;

	if( !bShowCombo )
	{
		m_sprComboLabel->SetVisible( false );
		m_sprMissesLabel->SetVisible( false );
		m_textNumber.SetVisible( false );
		return;
	}

	if( m_iLastSeenCombo == -1 )	// first update, don't set bIsMilestone=true
		m_iLastSeenCombo = iCombo;

	bool b100Milestone = false;
	bool b1000Milestone = false;
	for( int i=m_iLastSeenCombo+1; i<=iCombo; i++ )
	{
		if( i < 600 )
			b100Milestone |= ((i % 100) == 0);
		else
			b1000Milestone |= ((i % 200) == 0);
	}
	m_iLastSeenCombo = iCombo;

	m_sprComboLabel->SetVisible( !bComboOfMisses );
	m_sprMissesLabel->SetVisible( bComboOfMisses );
	m_textNumber.SetVisible( true );

	RString txt = ssprintf("%d", iNum);
	
	// Do pulse even if the number isn't changing.

	m_textNumber.SetText( txt );
	float fNumberZoom = SCALE(iNum,0.f,(float)NUMBER_MAX_ZOOM_AT,(float)NUMBER_MIN_ZOOM,(float)NUMBER_MAX_ZOOM);
	CLAMP( fNumberZoom, (float)NUMBER_MIN_ZOOM, (float)NUMBER_MAX_ZOOM );
	m_textNumber.FinishTweening();
	m_textNumber.SetZoom( fNumberZoom );
	m_textNumber.RunCommands( PULSE_COMMAND ); 

	AutoActor &sprLabel = bComboOfMisses ? m_sprMissesLabel : m_sprComboLabel;

	sprLabel->FinishTweening();
	sprLabel->RunCommands( PULSE_COMMAND );

	if( b100Milestone )
		m_spr100Milestone->PlayCommand( "Milestone" );
	if( b1000Milestone )
		m_spr1000Milestone->PlayCommand( "Milestone" );

	// don't show a colored combo until 1/4 of the way through the song
	bool bPastMidpoint = GAMESTATE->GetCourseSongIndex()>0 ||
		GAMESTATE->m_fMusicSeconds > GAMESTATE->m_pCurSong->m_fMusicLengthSeconds/4;

	if( bComboOfMisses )
	{
		sprLabel->RunCommands( MISS_COMBO_COMMAND );
		m_textNumber.RunCommands( MISS_COMBO_COMMAND );
	}
	else if( bPastMidpoint )
	{
		if( m_pPlayerStageStats->FullComboOfScore(TNS_W1) )
		{
			sprLabel->RunCommands( FULL_COMBO_W1_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_W1_COMMAND );
		}
		else if( bPastMidpoint && m_pPlayerStageStats->FullComboOfScore(TNS_W2) )
		{
			sprLabel->RunCommands( FULL_COMBO_W2_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_W2_COMMAND );
		}
		else if( bPastMidpoint && m_pPlayerStageStats->FullComboOfScore(TNS_W3) )
		{
			sprLabel->RunCommands( FULL_COMBO_W3_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_W3_COMMAND );
		}
		else
		{
			sprLabel->RunCommands( FULL_COMBO_BROKEN_COMMAND );
			m_textNumber.RunCommands( FULL_COMBO_BROKEN_COMMAND );
		}
	}
	else
	{
		sprLabel->RunCommands( FULL_COMBO_BROKEN_COMMAND );
		m_textNumber.RunCommands( FULL_COMBO_BROKEN_COMMAND );
	}
}

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
