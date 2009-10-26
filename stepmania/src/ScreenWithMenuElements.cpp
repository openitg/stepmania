#include "global.h"
#include "ScreenWithMenuElements.h"
#include "MenuTimer.h"
#include "RageLog.h"
#include "ThemeManager.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "GameSoundManager.h"
#include "MemoryCardDisplay.h"
#include "InputEventPlus.h"

#define TIMER_STEALTH				THEME->GetMetricB(m_sName,"TimerStealth")
#define SHOW_STAGE_DISPLAY			THEME->GetMetricB(m_sName,"ShowStageDisplay")
#define MEMORY_CARD_ICONS			THEME->GetMetricB(m_sName,"MemoryCardIcons")
#define FORCE_TIMER				THEME->GetMetricB(m_sName,"ForceTimer")
#define STOP_MUSIC_ON_BACK			THEME->GetMetricB(m_sName,"StopMusicOnBack")
#define WAIT_FOR_CHILDREN_BEFORE_TWEENING_OUT	THEME->GetMetricB(m_sName,"WaitForChildrenBeforeTweeningOut")

REGISTER_SCREEN_CLASS( ScreenWithMenuElements );
ScreenWithMenuElements::ScreenWithMenuElements()
{
	m_MenuTimer = NULL;
	FOREACH_PlayerNumber( p )
		m_MemoryCardDisplay[p] = NULL;
	m_MenuTimer = NULL;
}

void ScreenWithMenuElements::Init()
{
	PLAY_MUSIC.Load( m_sName, "PlayMusic" );
	MUSIC_ALIGN_BEAT.Load( m_sName, "MusicAlignBeat" );
	CANCEL_TRANSITIONS_OUT.Load( m_sName, "CancelTransitionsOut" );
	TIMER_SECONDS.Load( m_sName, "TimerSeconds" );
	TIMER_METRICS_GROUP.Load( m_sName, "TimerMetricsGroup" );
	Screen::Init();

	ASSERT( this->m_SubActors.empty() );	// don't call Init twice!

	if( MEMORY_CARD_ICONS )
	{
		FOREACH_PlayerNumber( p )
		{
			ASSERT( m_MemoryCardDisplay[p] == NULL );
			m_MemoryCardDisplay[p] = new MemoryCardDisplay;
			m_MemoryCardDisplay[p]->Load( p );
			m_MemoryCardDisplay[p]->SetName( ssprintf("MemoryCardDisplayP%d",p+1) );
			LOAD_ALL_COMMANDS_AND_SET_XY( m_MemoryCardDisplay[p] );
			this->AddChild( m_MemoryCardDisplay[p] );
		}
	}

	if( TIMER_SECONDS != -1 )
	{
		ASSERT( m_MenuTimer == NULL );	// don't load twice
		m_MenuTimer = new MenuTimer;
		m_MenuTimer->Load( TIMER_METRICS_GROUP.GetValue() );
		m_MenuTimer->SetName( "Timer" );
		if( TIMER_STEALTH )
			m_MenuTimer->EnableStealth( true );
		LOAD_ALL_COMMANDS_AND_SET_XY( m_MenuTimer );
		ResetTimer();
		this->AddChild( m_MenuTimer );
	}

	/* TODO: Remove overlay and underlay in favor of more flexible decorations */
	m_sprUnderlay.Load( THEME->GetPathB(m_sName,"underlay") );
	m_sprUnderlay->SetName("Underlay");
	m_sprUnderlay->SetDrawOrder( DRAW_ORDER_UNDERLAY );
	this->AddChild( m_sprUnderlay );
	LOAD_ALL_COMMANDS( m_sprUnderlay );
	
	m_sprOverlay.Load( THEME->GetPathB(m_sName,"overlay") );
	m_sprOverlay->SetName("Overlay");
	m_sprOverlay->SetDrawOrder( DRAW_ORDER_OVERLAY );
	this->AddChild( m_sprOverlay );
	LOAD_ALL_COMMANDS( m_sprOverlay );
	
	/* Experimental: Load "decorations" and make them children of the screen. */
	{
		m_Decorations.LoadB( m_sName, "decorations" );
		ActorFrame *pFrame = dynamic_cast<ActorFrame*>(static_cast<Actor*>(m_Decorations));
		vector<Actor*> children = pFrame->GetChildren();
		if( pFrame )
		{
			FOREACH( Actor*, children, child )
				this->AddChild( *child );
		}
		else
		{
			m_Decorations.Unload();
		}
	}

	m_In.SetName( "In" );
	m_In.Load( THEME->GetPathB(m_sName,"in") );
	m_In.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_In );
	LOAD_ALL_COMMANDS( m_In );

	m_Out.SetName( "Out" );
	m_Out.Load( THEME->GetPathB(m_sName,"out") );
	m_Out.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_Out );
	LOAD_ALL_COMMANDS( m_Out );

	m_Cancel.SetName( "Cancel" );
	m_Cancel.Load( THEME->GetPathB(m_sName,"cancel") );
	m_Cancel.SetDrawOrder( DRAW_ORDER_TRANSITIONS );
	this->AddChild( &m_Cancel );
	LOAD_ALL_COMMANDS( m_Cancel );

	/* Grab the music path here; don't GetPath during BeginScreen. */
	if( PLAY_MUSIC )
		m_sPathToMusic = THEME->GetPathS( m_sName, "music" );
}

void ScreenWithMenuElements::BeginScreen()
{
	Screen::BeginScreen();

	m_In.Reset();
	m_Out.Reset();
	m_Cancel.Reset();

	TweenOnScreen();

	this->SortByDrawOrder();
	m_In.StartTransitioning( SM_DoneFadingIn );

	SOUND->PlayOnceFromAnnouncer( m_sName+" intro" );
	StartPlayingMusic();

	/* Evaluate FirstUpdateCommand. */
	this->PlayCommand( "FirstUpdate" );

	/* If AutoJoin and a player is already joined, then try to join a player.  (If no players
	 * are joined, they'll join on the first JoinInput.) */
	if( GAMESTATE->GetCoinMode() == CoinMode_Pay && GAMESTATE->m_bAutoJoin.Get() )
	{
               if( GAMESTATE->GetNumSidesJoined() > 0 && GAMESTATE->JoinPlayers() )
			SCREENMAN->PlayStartSound();
	}
}

void ScreenWithMenuElements::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_MenuTimer )
	{
		FOREACH_HumanPlayer(p)
		{
			InputEventPlus iep;
			iep.pn = p;
			MenuStart( iep );
		}
	}

	Screen::HandleScreenMessage( SM );
}

ScreenWithMenuElements::~ScreenWithMenuElements()
{
	SAFE_DELETE( m_MenuTimer );
	FOREACH_PlayerNumber( p )
	{
		if( m_MemoryCardDisplay[p] != NULL )
			SAFE_DELETE( m_MemoryCardDisplay[p] );
	}
}

void ScreenWithMenuElements::SetHelpText( RString s )
{
	Message msg("SetHelpText");
	msg.SetParam( "Text", s );
	this->HandleMessage( msg );
}

void ScreenWithMenuElements::StartPlayingMusic()
{
	/* Some screens should leave the music alone (eg. ScreenPlayerOptions music 
	 * sample left over from ScreenSelectMusic). */
	if( PLAY_MUSIC )
	{
		GameSoundManager::PlayMusicParams pmp;
		pmp.sFile = m_sPathToMusic;
		pmp.bAlignBeat = MUSIC_ALIGN_BEAT;
		SOUND->PlayMusic( pmp );
	}
}

void ScreenWithMenuElements::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
}

void ScreenWithMenuElements::ResetTimer()
{
	if( m_MenuTimer == NULL )
		return;

	if( TIMER_SECONDS > 0.0f && (PREFSMAN->m_bMenuTimer || FORCE_TIMER) )
	{
		m_MenuTimer->SetSeconds( TIMER_SECONDS );
		m_MenuTimer->Start();
	}
	else
	{
		m_MenuTimer->Disable();
	}
}

void ScreenWithMenuElements::StartTransitioningScreen( ScreenMessage smSendWhenDone )
{
	TweenOffScreen();

	m_Out.StartTransitioning( smSendWhenDone );
	if( WAIT_FOR_CHILDREN_BEFORE_TWEENING_OUT )
	{
		// Time the transition so that it finishes exactly when all actors have 
		// finished tweening.
		float fSecondsUntilFinished = GetTweenTimeLeft();
		float fSecondsUntilBeginOff = max( fSecondsUntilFinished - m_Out.GetTweenTimeLeft(), 0 );
		m_Out.SetHibernate( fSecondsUntilBeginOff );
	}
}

void ScreenWithMenuElements::TweenOnScreen()
{
	this->PlayCommand( "On" );
}

void ScreenWithMenuElements::TweenOffScreen()
{
	if( m_MenuTimer )
	{
		m_MenuTimer->SetSeconds( 0 );
		m_MenuTimer->Stop();
	}

	this->PlayCommand( "Off" );

	// If we're a stacked screen, then there's someone else between us and the
	// background, so don't tween it off.
	if( !SCREENMAN->IsStackedScreen(this) )
		SCREENMAN->PlaySharedBackgroundOffCommand();
}

void ScreenWithMenuElements::Cancel( ScreenMessage smSendWhenDone )
{
	m_sNextScreen = GetPrevScreen();

	if( CANCEL_TRANSITIONS_OUT )
	{
		StartTransitioningScreen( smSendWhenDone );
		COMMAND( m_Out, "Cancel" );
		return;
	}

	if( m_Cancel.IsTransitioning() )
		return;	// ignore

	if( STOP_MUSIC_ON_BACK )
		SOUND->StopMusic();

	if( m_MenuTimer )
		m_MenuTimer->Stop();
	m_Cancel.StartTransitioning( smSendWhenDone );
	COMMAND( m_Cancel, "Cancel" );
}

bool ScreenWithMenuElements::IsTransitioning()
{
	return m_In.IsTransitioning() || m_Out.IsTransitioning() || m_Cancel.IsTransitioning();
}

void ScreenWithMenuElements::StopTimer()
{
	if( m_MenuTimer )
		m_MenuTimer->Stop();
}

REGISTER_SCREEN_CLASS( ScreenWithMenuElementsSimple );

void ScreenWithMenuElementsSimple::MenuStart( const InputEventPlus &input )
{
	if( IsTransitioning() )
		return;
	if( m_fLockInputSecs > 0 )
		return;

	StartTransitioningScreen( SM_GoToNextScreen );
}

void ScreenWithMenuElementsSimple::MenuBack( const InputEventPlus &input )
{
	if( IsTransitioning() )
		return;
	if( m_fLockInputSecs > 0 )
		return;

	Cancel( SM_GoToPrevScreen );
}

// lua start
#include "LuaBinding.h"

class LunaScreenWithMenuElements: public Luna<ScreenWithMenuElements>
{
public:
	static int Cancel( T* p, lua_State *L )		{ p->Cancel( SM_GoToPrevScreen ); return 0; }

	LunaScreenWithMenuElements()
	{
  		ADD_METHOD( Cancel );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenWithMenuElements, Screen )

class LunaScreenWithMenuElementsSimple: public Luna<ScreenWithMenuElementsSimple>
{
public:
	LunaScreenWithMenuElementsSimple()
	{
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenWithMenuElementsSimple, ScreenWithMenuElements )

// lua end

/*
 * (c) 2004 Chris Danford
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
