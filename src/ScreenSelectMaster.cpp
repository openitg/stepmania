#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenSelectMaster

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenSelectMaster.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "ThemeManager.h"
#include "RageSounds.h"
#include "GameState.h"
#include "AnnouncerManager.h"
#include "ModeChoice.h"
#include "ActorUtil.h"
#include "RageLog.h"

#define NUM_ICON_PARTS							THEME->GetMetricI(m_sName,"NumIconParts")
#define NUM_PREVIEW_PARTS						THEME->GetMetricI(m_sName,"NumPreviewParts")
#define NUM_CURSOR_PARTS						THEME->GetMetricI(m_sName,"NumCursorParts")
#define SHARED_PREVIEW_AND_CURSOR				THEME->GetMetricB(m_sName,"SharedPreviewAndCursor")
#define NUM_CHOICES_ON_PAGE_1					THEME->GetMetricI(m_sName,"NumChoicesOnPage1")
#define CURSOR_OFFSET_X_FROM_ICON( p, part )	THEME->GetMetricF(m_sName,ssprintf("CursorPart%dP%dOffsetXFromIcon",part+1,p+1))
#define CURSOR_OFFSET_Y_FROM_ICON( p, part )	THEME->GetMetricF(m_sName,ssprintf("CursorPart%dP%dOffsetYFromIcon",part+1,p+1))
#define PRE_SWITCH_PAGE_SECONDS					THEME->GetMetricF(m_sName,"PreSwitchPageSeconds")
#define POST_SWITCH_PAGE_SECONDS				THEME->GetMetricF(m_sName,"PostSwitchPageSeconds")
#define EXTRA_SLEEP_AFTER_TWEEN_OFF_SECONDS		THEME->GetMetricF(m_sName,"ExtraSleepAfterTweenOffSeconds")
#define OPTION_ORDER( dir )						THEME->GetMetric (m_sName,"OptionOrder"+CString(dir))

/* OptionOrderLeft=0:1,1:2,2:3,3:4 */
const char *ScreenSelectMaster::dirs[NUM_DIRS] =
{
	"Up", "Down", "Left", "Right", "Auto"
};

const ScreenMessage SM_PlayPostSwitchPage = (ScreenMessage)(SM_User+1);

ScreenSelectMaster::ScreenSelectMaster( CString sClassName ) : ScreenSelect( sClassName )
{
	int p, i;

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		m_iChoice[p] = 0;
		m_bChosen[p] = false;
	}

	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( i=0; i<NUM_CURSOR_PARTS; i++ )
		{
			CString sFName = ssprintf("%s Cursor Part%d", m_sName.c_str(),i+1);
			m_sprCursor[i][0].SetName( ssprintf("CursorPart%d",i+1) );
			m_sprCursor[i][0].Load( THEME->GetPathToG(sFName) );
			this->AddChild( &m_sprCursor[i][0] );
		}
	}
	else
	{
		for( i=0; i<NUM_CURSOR_PARTS; i++ )
		{
			for( p=0; p<NUM_PLAYERS; p++ )
			{
				if( !GAMESTATE->IsPlayerEnabled(p) )
					continue;	// skip
				CString sFName = ssprintf("%s Cursor Part%d P%d", m_sName.c_str(),i+1,p+1);
				m_sprCursor[i][p].SetName( ssprintf("CursorPart%dP%d",i+1,p+1) );
				m_sprCursor[i][p].Load( THEME->GetPathToG(sFName) );
				this->AddChild( &m_sprCursor[i][p] );
			}
		}
	}

	for( unsigned c=0; c<m_aModeChoices.size(); c++ )
	{
//		const ModeChoice& mc = m_aModeChoices[c];

		for( i=0; i<NUM_ICON_PARTS; i++ )
		{
			CString sFName = ssprintf("%s Icon Part%d Choice%d", m_sName.c_str(),i+1,c+1);
			m_sprIcon[i][c].Load( THEME->GetPathToG(sFName) );
			m_sprIcon[i][c]->SetName( ssprintf("IconPart%dChoice%d",i+1,c+1) );
			this->AddChild( m_sprIcon[i][c] );
		}

		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				CString sFName = ssprintf("%s Preview Part%d Choice%d", m_sName.c_str(),i+1,c+1);
				m_sprPreview[i][c][0].Load( THEME->GetPathToG(sFName) );
				m_sprPreview[i][c][0]->SetName( ssprintf("PreviewPart%d",i+1) );
				this->AddChild( m_sprPreview[i][c][0] );
			}
		}
		else
		{
			for( i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				for( p=0; p<NUM_PLAYERS; p++ )
				{
					if( !GAMESTATE->IsPlayerEnabled(p) )
						continue;	// skip
					CString sFName = ssprintf("%s Preview Part%d Choice%d P%d", m_sName.c_str(),i+1,c+1,p+1);
					m_sprPreview[i][c][p].Load( THEME->GetPathToG(sFName) );
					m_sprPreview[i][c][p]->SetName( ssprintf("PreviewPart%dP%d",i+1,p+1) );
					this->AddChild( m_sprPreview[i][c][p] );
				}
			}
		}
	}

	for( int page=0; page<NUM_PAGES; page++ )
	{
		m_sprMore[page].SetName( ssprintf("MorePage%d",page+1) );
		m_sprMore[page].Load( THEME->GetPathToG( ssprintf("%s more page%d",m_sName.c_str(), page+1) ) );
		this->AddChild( &m_sprMore[page] );

		m_sprExplanation[page].Load( THEME->GetPathToG( ssprintf("%s explanation page%d",m_sName.c_str(), page+1) ) );
		m_sprExplanation[page]->SetName( ssprintf("ExplanationPage%d",page+1) );
		this->AddChild( m_sprExplanation[page] );
	}


	for( p=0; p<NUM_PLAYERS; p++ )
	{
		CLAMP( m_iChoice[p], 0, (int)m_aModeChoices.size()-1 );
		m_bChosen[p] = false;
	}
	
	m_soundChange.Load( THEME->GetPathToS( ssprintf("%s change", m_sName.c_str())) );
	m_soundSelect.Load( THEME->GetPathToS( "Common start") );
	m_soundDifficult.Load( ANNOUNCER->GetPathTo("select difficulty challenge") );

	for( int dir = 0; dir < NUM_DIRS; ++dir )
	{
		const CString dirname = dirs[dir];
		const CString order = OPTION_ORDER( dirname );
		vector<CString> parts;
		split( order, ",", parts, true );

		if( parts.size() == 0 )
			continue;
		for( unsigned c = 0; c < m_aModeChoices.size(); ++c )
			m_Next[dir][c] = -1;

		for( unsigned part = 0; part < parts.size(); ++part )
		{
			unsigned from, to;
			if( sscanf( parts[part], "%u:%u", &from, &to ) != 2 )
			{
				LOG->Warn( "%s::OptionOrder%s parse error", m_sName.c_str(), dirname.c_str() );
				continue;
			}

			--from;
			--to;

			if( from >= m_aModeChoices.size() ||
				to >= m_aModeChoices.size() )
			{
				LOG->Warn( "%s::OptionOrder%s out of range", m_sName.c_str(), dirname.c_str() );
				continue;
			}

			m_Next[dir][from] = to;
		}
	}

	this->UpdateSelectableChoices();

	m_fLockInputSecs = TweenOnScreen();
}

void ScreenSelectMaster::Update( float fDelta )
{
	ScreenSelect::Update( fDelta );
	m_fLockInputSecs = max( 0, m_fLockInputSecs-fDelta );
}

void ScreenSelectMaster::HandleScreenMessage( const ScreenMessage SM )
{
	ScreenSelect::HandleScreenMessage( SM );

	switch( SM )
	{
	case SM_PlayPostSwitchPage:
		{
			float fSecs = 0;

			if( SHARED_PREVIEW_AND_CURSOR )
			{
				for( int i=0; i<NUM_CURSOR_PARTS; i++ )
				{
					m_sprCursor[i][0].SetXY( GetCursorX((PlayerNumber)0,i), GetCursorY((PlayerNumber)0,i) );
					fSecs = max( fSecs, COMMAND( m_sprCursor[i][0], "PostSwitchPage" ) );
				}
			}
			else
			{
				for( int i=0; i<NUM_CURSOR_PARTS; i++ )
					for( int p=0; p<NUM_PLAYERS; p++ )
						if( GAMESTATE->IsPlayerEnabled(p) )
						{
							m_sprCursor[i][p].SetXY( GetCursorX((PlayerNumber)p,i), GetCursorY((PlayerNumber)p,i) );
							fSecs = max( fSecs, COMMAND( m_sprCursor[i][p], "PostSwitchPage" ) );
						}
			}

			if( SHARED_PREVIEW_AND_CURSOR )
			{
				for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
					fSecs = max( fSecs, COMMAND( m_sprPreview[i][m_iChoice[0]][0], "PostSwitchPage" ) );
			}
			else
			{
				for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
					for( int p=0; p<NUM_PLAYERS; p++ )
						if( GAMESTATE->IsPlayerEnabled(p) )
							fSecs = max( fSecs, COMMAND( m_sprPreview[i][m_iChoice[p]][p], "PostSwitchPage" ) );
			}

			m_fLockInputSecs = POST_SWITCH_PAGE_SECONDS;
		}
		break;
	case SM_BeginFadingOut:
		float fSecs = TweenOffScreen();
		/* This can be used to allow overlap between the main tween-off and the MenuElements
		 * tweenoff. */
		fSecs += EXTRA_SLEEP_AFTER_TWEEN_OFF_SECONDS;
		fSecs = max( fSecs, 0 );
		SCREENMAN->PostMessageToTopScreen( SM_AllDoneChoosing, fSecs );	// nofify parent that we're finished
		m_Menu.StopTimer();
		break;
	}
}

int ScreenSelectMaster::GetSelectionIndex( PlayerNumber pn )
{
	return m_iChoice[pn];
}

void ScreenSelectMaster::UpdateSelectableChoices()
{
	for( unsigned c=0; c<m_aModeChoices.size(); c++ )
		for( int i=0; i<NUM_ICON_PARTS; i++ )
			COMMAND( m_sprIcon[i][c], m_aModeChoices[c].IsPlayable()? "Enabled":"Disabled" );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;

		if( !m_aModeChoices[m_iChoice[p]].IsPlayable() )
			Move( (PlayerNumber) p, DIR_AUTO );
		ASSERT( m_aModeChoices[m_iChoice[p]].IsPlayable() );
	}
}

bool ScreenSelectMaster::Move( PlayerNumber pn, Dirs dir )
{
	const int start = m_iChoice[pn];
	int iSwitchToIndex = m_iChoice[pn];
	do
	{
		iSwitchToIndex = m_Next[dir][iSwitchToIndex];
		if( iSwitchToIndex == -1 )
			return false; // can't go that way
		if( iSwitchToIndex == start )
			return false; // went full circle and none found
	}
	while( !m_aModeChoices[iSwitchToIndex].IsPlayable() );

	return ChangeSelection( pn, iSwitchToIndex );
}

void ScreenSelectMaster::MenuLeft( PlayerNumber pn )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( Move(pn, DIR_LEFT) )
		m_soundChange.Play();
}

void ScreenSelectMaster::MenuRight( PlayerNumber pn )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( Move(pn, DIR_RIGHT) )
		m_soundChange.Play();
}

void ScreenSelectMaster::MenuUp( PlayerNumber pn )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( Move(pn, DIR_UP) )
		m_soundChange.Play();
}

void ScreenSelectMaster::MenuDown( PlayerNumber pn )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( Move(pn, DIR_DOWN) )
		m_soundChange.Play();
}

bool ScreenSelectMaster::ChangePage( int iNewChoice )
{
	Page newPage = GetPage(iNewChoice);

	// If anyone has already chosen, don't allow changing of pages
	int p;
	for( p=0; p<NUM_PLAYERS; p++ )
		if( GAMESTATE->IsHumanPlayer(p) && m_bChosen[p] )
			return false;

	float fSecs = 0;

	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			fSecs = max( fSecs, COMMAND( m_sprCursor[i][0], "PreSwitchPage" ) );
	}
	else
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			for( int p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsHumanPlayer(p) )
					fSecs = max( fSecs, COMMAND( m_sprCursor[i][p], "PreSwitchPage" ) );
	}

	const CString sIconAndExplanationCommand = ssprintf( "SwitchToPage%d", newPage+1 );

	for( unsigned c=0; c<m_aModeChoices.size(); c++ )
		for( int i=0; i<NUM_ICON_PARTS; i++ )
			fSecs = max( fSecs, COMMAND( m_sprIcon[i][c], sIconAndExplanationCommand ) );

	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			fSecs = max( fSecs, COMMAND( m_sprPreview[i][m_iChoice[p]][0], "PreSwitchPage" ) );
	}
	else
	{
		for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			for( int p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsPlayerEnabled(p) )
					fSecs = max( fSecs, COMMAND( m_sprPreview[i][m_iChoice[p]][p], "PreSwitchPage" ) );
	}

	for( int page=0; page<NUM_PAGES; page++ )
	{
		fSecs = max( fSecs, COMMAND( m_sprExplanation[page], sIconAndExplanationCommand ) );
		fSecs = max( fSecs, COMMAND( m_sprMore[page], sIconAndExplanationCommand ) );
	}



	if( newPage == PAGE_2 )
	{
		// XXX: only play this once (I thought we already did that?)
		// DDR plays it on every change to page 2.  -Chris
		/* That sounds ugly if you go back and forth quickly. -g */
		m_soundDifficult.Stop();
		m_soundDifficult.PlayRandom();
	}

	// change both players
	for( p=0; p<NUM_PLAYERS; p++ )
		m_iChoice[p] = iNewChoice;

	m_fLockInputSecs = PRE_SWITCH_PAGE_SECONDS;
	this->PostScreenMessage( SM_PlayPostSwitchPage, PRE_SWITCH_PAGE_SECONDS );
	return true;
}

bool ScreenSelectMaster::ChangeSelection( PlayerNumber pn, int iNewChoice )
{
	if( GetPage(m_iChoice[pn]) != GetPage(iNewChoice) )
		return ChangePage( iNewChoice );

	bool bMoveAll = SHARED_PREVIEW_AND_CURSOR || GetCurrentPage()!=PAGE_1;

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip

		if( !bMoveAll && p!=pn )
			continue;	// skip

		const int iOldChoice = m_iChoice[p];
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				COMMAND( m_sprPreview[i][iOldChoice][0], "LoseFocus" );
				COMMAND( m_sprPreview[i][iNewChoice][0], "GainFocus" );
			}
		}
		else
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				COMMAND( m_sprPreview[i][iOldChoice][p], "LoseFocus" );
				COMMAND( m_sprPreview[i][iNewChoice][p], "GainFocus" );
			}
		}

		{
			/* XXX: If !SharedPreviewAndCursor, this is incorrect.  (Nothing uses
			 * both icon focus and !SharedPreviewAndCursor right now.) */
			for( int i=0; i<NUM_ICON_PARTS; i++ )
			{
				COMMAND( m_sprIcon[i][iOldChoice], "LoseFocus" );
				COMMAND( m_sprIcon[i][iNewChoice], "GainFocus" );
			}
		}

		m_iChoice[p] = iNewChoice;

		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			{
				COMMAND( m_sprCursor[i][0], "Change" );
				m_sprCursor[i][0].SetXY( GetCursorX((PlayerNumber)0,i), GetCursorY((PlayerNumber)0,i) );
			}
		}
		else
		{
			for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			{
				COMMAND( m_sprCursor[i][p], "Change" );
				m_sprCursor[i][p].SetXY( GetCursorX((PlayerNumber)p,i), GetCursorY((PlayerNumber)p,i) );
			}
		}
	}

	return true;
}

ScreenSelectMaster::Page ScreenSelectMaster::GetPage( int iChoiceIndex ) const
{
	return iChoiceIndex < NUM_CHOICES_ON_PAGE_1? PAGE_1:PAGE_2;
}

ScreenSelectMaster::Page ScreenSelectMaster::GetCurrentPage() const
{
	// Both players are guaranteed to be on the same page.
	return GetPage( m_iChoice[GAMESTATE->m_MasterPlayerNumber] );
}


float ScreenSelectMaster::DoMenuStart( PlayerNumber pn )
{
	if( m_bChosen[pn] == true )
		return 0;
	m_bChosen[pn] = true;

	float fSecs = 0;
	for( int page=0; page<NUM_PAGES; page++ )
		fSecs = max( fSecs, OFF_COMMAND( m_sprMore[page] ) );

	for( int i=0; i<NUM_CURSOR_PARTS; i++ )
		fSecs = max( fSecs, COMMAND( m_sprCursor[i][pn], "Choose") );
	return fSecs;
}

void ScreenSelectMaster::MenuStart( PlayerNumber pn )
{
	if( m_fLockInputSecs > 0 )
		return;
	if( m_bChosen[pn] == true )
		return;

	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo(ssprintf("%s comment %s",m_sName.c_str(), m_aModeChoices[m_iChoice[pn]].m_sName.c_str())) );
	m_soundSelect.Play();

	bool bAllDone = true;
	float fSecs = 0;
	if( SHARED_PREVIEW_AND_CURSOR || GetCurrentPage() == PAGE_2 )
	{
		/* Only one player has to pick.  Choose this for all the other players, too. */
		for( int p=0; p<NUM_PLAYERS; p++ )
			if( GAMESTATE->IsHumanPlayer(p) )
			{
				ASSERT( !m_bChosen[p] );
				fSecs = max( fSecs, DoMenuStart( (PlayerNumber)p ) );
			}
	} else {
		fSecs = max( fSecs, DoMenuStart(pn) );
		// check to see if everyone has chosen
		for( int p=0; p<NUM_PLAYERS; p++ )
			if( GAMESTATE->IsHumanPlayer((PlayerNumber)p) )
				bAllDone &= m_bChosen[p];
	}

	if( bAllDone )
		this->PostScreenMessage( SM_BeginFadingOut, fSecs );// tell our owner it's time to move on
}

/*
 * We want all items to always run OnCommand and either GainFocus or LoseFocus on
 * tween-in.  If we only run OnCommand, then it has to contain a copy of either
 * GainFocus or LoseFocus, which implies that the default setting is hard-coded in
 * the theme.  Always run both.
 *
 * However, the actual tween-in is OnCommand; we don't always want to actually run
 * through the Gain/LoseFocus tweens during initial tween-in.  So, we run the focus
 * command first, do a FinishTweening to pop it in place, and then run OnCommand.
 * This means that the focus command should be position neutral; eg. only use "addx",
 * not "x".
 */
float ScreenSelectMaster::TweenOnScreen() 
{
	float fSecs = 0;

	for( unsigned c=0; c<m_aModeChoices.size(); c++ )
	{
		for( int i=0; i<NUM_ICON_PARTS; i++ )
		{
			COMMAND( m_sprIcon[i][c], (int(c) == m_iChoice[0])? "GainFocus":"LoseFocus" );
			m_sprIcon[i][c]->FinishTweening();
			fSecs = max( fSecs, SET_XY_AND_ON_COMMAND( m_sprIcon[i][c] ) );
		}

		if( SHARED_PREVIEW_AND_CURSOR )
		{
			int p=0;
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				COMMAND( m_sprPreview[i][c][p], (int(c) == m_iChoice[p])? "GainFocus":"LoseFocus" );
				m_sprPreview[i][c][p]->FinishTweening();
				fSecs = max( fSecs, SET_XY_AND_ON_COMMAND( m_sprPreview[i][c][p] ) );
			}
		}
		else
		{
			for( int p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsPlayerEnabled(p) )
					for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
					{
						COMMAND( m_sprPreview[i][c][p], int(c) == m_iChoice[p]? "GainFocus":"LoseFocus" );
						m_sprPreview[i][c][p]->FinishTweening();
						fSecs = max( fSecs, SET_XY_AND_ON_COMMAND( m_sprPreview[i][c][p] ) );
					}
		}
	}

	// Need to SetXY of Cursor after Icons since it depends on the Icons' positions.
	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
		{
			m_sprCursor[i][0].SetXY( GetCursorX((PlayerNumber)0,i), GetCursorY((PlayerNumber)0,i) );
			fSecs = max( fSecs, ON_COMMAND( m_sprCursor[i][0] ) );
		}
	}
	else
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			for( int p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsPlayerEnabled(p) )
				{
					m_sprCursor[i][p].SetXY( GetCursorX((PlayerNumber)p,i), GetCursorY((PlayerNumber)p,i) );
					fSecs = max( fSecs, ON_COMMAND( m_sprCursor[i][p] ) );
				}
	}


	fSecs = max( fSecs, SET_XY_AND_ON_COMMAND( m_sprExplanation[GetCurrentPage()] ) );
	fSecs = max( fSecs, SET_XY_AND_ON_COMMAND( m_sprMore[GetCurrentPage()] ) );

	this->SortByZ();

	return fSecs;
}

float ScreenSelectMaster::TweenOffScreen()
{
	float fSecs = 0;

	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			fSecs = max( fSecs, OFF_COMMAND( m_sprCursor[i][0] ) );
	}
	else
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			for( int p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsPlayerEnabled(p) )
					fSecs = max( fSecs, OFF_COMMAND( m_sprCursor[i][p] ) );
	}

	for( unsigned c=0; c<m_aModeChoices.size(); c++ )
	{
		if( GetPage(c) != GetCurrentPage() )
			continue;	// skip

		bool SelectedByEitherPlayer = false;
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			if( m_iChoice[0] == (int)c )
				SelectedByEitherPlayer = true;
		}
		else
			for( int p=0; p<NUM_PLAYERS; p++ )
				if( GAMESTATE->IsPlayerEnabled(p) && m_iChoice[p] == (int)c )
					SelectedByEitherPlayer = true;

		for( int i=0; i<NUM_ICON_PARTS; i++ )
		{
			fSecs = max( fSecs, OFF_COMMAND( m_sprIcon[i][c] ) );
			fSecs = max( fSecs, COMMAND( m_sprIcon[i][c], SelectedByEitherPlayer? "OffFocused":"OffUnfocused" ) );
		}


		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				fSecs = max( fSecs, OFF_COMMAND( m_sprPreview[i][c][0] ) );
				fSecs = max( fSecs, COMMAND( m_sprPreview[i][c][0], SelectedByEitherPlayer? "OffFocused":"OffUnfocused" ) );
			}
		}
		else
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
				for( int p=0; p<NUM_PLAYERS; p++ )
					if( GAMESTATE->IsPlayerEnabled(p) )
					{
						fSecs = max( fSecs, OFF_COMMAND( m_sprPreview[i][c][p] ) );
						fSecs = max( fSecs, COMMAND( m_sprPreview[i][c][0], SelectedByEitherPlayer? "OffFocused":"OffUnfocused" ) );
					}
		}
	}

	fSecs = max( fSecs, OFF_COMMAND( m_sprExplanation[GetCurrentPage()] ) );
	fSecs = max( fSecs, OFF_COMMAND( m_sprMore[GetCurrentPage()] ) );

	return fSecs;
}


float ScreenSelectMaster::GetCursorX( PlayerNumber pn, int iPartIndex )
{
	return m_sprIcon[0][m_iChoice[pn]]->GetX() + CURSOR_OFFSET_X_FROM_ICON(pn, iPartIndex);
}

float ScreenSelectMaster::GetCursorY( PlayerNumber pn, int iPartIndex )
{
	return m_sprIcon[0][m_iChoice[pn]]->GetY() + CURSOR_OFFSET_Y_FROM_ICON(pn, iPartIndex);
}
