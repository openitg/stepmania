#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenManager

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenManager.h"
#include "IniFile.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "GameState.h"
#include "RageException.h"
#include "RageTimer.h"
#include "ThemeManager.h"
#include "RageDisplay.h"


ScreenManager*	SCREENMAN = NULL;	// global and accessable from anywhere in our program


#define STATS_X					THEME->GetMetricF("ScreenManager","StatsX")
#define STATS_Y					THEME->GetMetricF("ScreenManager","StatsY")
#define CREDITS_X( p )			THEME->GetMetricF("ScreenManager",ssprintf("CreditsP%dX",p+1))
#define CREDITS_Y( p )			THEME->GetMetricF("ScreenManager",ssprintf("CreditsP%dY",p+1))
#define CREDITS_COLOR			THEME->GetMetricC("ScreenManager","CreditsColor")
#define CREDITS_SHADOW_LENGTH	THEME->GetMetricF("ScreenManager","CreditsShadowLength")
#define CREDITS_ZOOM			THEME->GetMetricF("ScreenManager","CreditsZoom")


ScreenManager::ScreenManager()
{
	m_ScreenBuffered = NULL;

	m_textStats.LoadFromFont( THEME->GetPathTo("Fonts","normal") );
	m_textStats.SetXY( STATS_X, STATS_Y );
	m_textStats.SetHorizAlign( Actor::align_right );
	m_textStats.SetVertAlign( Actor::align_top );
	m_textStats.SetZoom( 0.5f );
	m_textStats.SetShadowLength( 2 );

	// set credits properties on RefreshCreditsMessage

	m_textSystemMessage.LoadFromFont( THEME->GetPathTo("Fonts","normal") );
	m_textSystemMessage.SetHorizAlign( Actor::align_left );
	m_textSystemMessage.SetVertAlign( Actor::align_top );
	m_textSystemMessage.SetXY( 4.0f, 4.0f );
	m_textSystemMessage.SetZoom( 0.5f );
	m_textSystemMessage.SetShadowLength( 2 );
	m_textSystemMessage.SetDiffuse( RageColor(1,1,1,0) );
}


ScreenManager::~ScreenManager()
{
	LOG->Trace( "ScreenManager::~ScreenManager()" );

	EmptyDeleteQueue();

	// delete current states
	for( unsigned i=0; i<m_ScreenStack.size(); i++ )
		SAFE_DELETE( m_ScreenStack[i] );
	SAFE_DELETE( m_ScreenBuffered );
}

void ScreenManager::EmptyDeleteQueue()
{
	// delete all ScreensToDelete
	for( unsigned i=0; i<m_ScreensToDelete.size(); i++ )
		SAFE_DELETE( m_ScreensToDelete[i] );

	m_ScreensToDelete.clear();
}

void ScreenManager::Update( float fDeltaTime )
{
	m_textSystemMessage.Update( fDeltaTime );
	m_textStats.Update( fDeltaTime );
	for( int p=0; p<NUM_PLAYERS; p++ )
		m_textCreditInfo[p].Update( fDeltaTime );

	EmptyDeleteQueue();

	// Update all windows in the stack
	for( unsigned i=0; i<m_ScreenStack.size(); i++ ) {
		/* Screens take some time to load.  If we don't do this, then screens
		 * receive an initial update that includes all of the time they spent
		 * loading, which will chop off their tweens.  
		 *
		 * We don't want to simply cap update times; for example, the stage
		 * screen sets a 4 second timer, preps the gameplay screen, and then
		 * displays the prepped screen after the timer runs out; this lets the
		 * load time be masked (as long as the load takes less than 4 seconds).
		 * If we cap that large update delta from the screen load, the update
		 * to load the new screen will come after 4 seconds plus the load time.
		 *
		 * So, let's just drop the first update for every screen.
		 */
		if( m_ScreenStack[i]->IsFirstUpdate() )
			m_ScreenStack[i]->Update( 0 );
		else
			m_ScreenStack[i]->Update( fDeltaTime );
	}

}


void ScreenManager::Restore()
{
	// Draw all CurrentScreens (back to front)
	for( unsigned i=0; i<m_ScreenStack.size(); i++ )
		m_ScreenStack[i]->Restore();
}

void ScreenManager::Invalidate()
{
	for( unsigned i=0; i<m_ScreenStack.size(); i++ )
		m_ScreenStack[i]->Invalidate();
}

void ScreenManager::Draw()
{
	// Draw all CurrentScreens (back to front)
	for( unsigned i=0; i<m_ScreenStack.size(); i++ )
		m_ScreenStack[i]->Draw();
	
	if( m_textSystemMessage.GetDiffuse().a != 0 )
		m_textSystemMessage.Draw();

	if( PREFSMAN  &&  PREFSMAN->m_bShowStats )
	{
		/* If FPS == 0, we don't have stats yet. */
		m_textStats.SetText( ssprintf(DISPLAY->GetFPS()?
			"%i FPS\n%i av FPS\n%i VPF":
			"-- FPS\n-- av FPS\n-- VPF",
			DISPLAY->GetFPS(), DISPLAY->GetCumFPS(), 
			DISPLAY->GetVPF()) );

		m_textStats.Draw();
	}
	for( int p=0; p<NUM_PLAYERS; p++ )
		m_textCreditInfo[p].Draw();

}


void ScreenManager::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
//	LOG->Trace( "ScreenManager::Input( %d-%d, %d-%d, %d-%d, %d-%d )", 
//		DeviceI.device, DeviceI.button, GameI.controller, GameI.button, MenuI.player, MenuI.button, StyleI.player, StyleI.col );

	// pass input only to topmost state
	if( !m_ScreenStack.empty() )
		m_ScreenStack.back()->Input( DeviceI, type, GameI, MenuI, StyleI );
}


// Screen classes
#include "ScreenAppearanceOptions.h"
#include "ScreenCaution.h"
#include "ScreenEdit.h"
#include "ScreenEditMenu.h"
#include "ScreenEvaluation.h"
#include "ScreenEz2SelectPlayer.h"
#include "ScreenSelectMode.h"
#include "ScreenGameOver.h"
#include "ScreenGameplay.h"
#include "ScreenGraphicOptions.h"
#include "ScreenHowToPlay.h"
#include "ScreenInputOptions.h"
#include "ScreenMachineOptions.h"
#include "ScreenMapControllers.h"
#include "ScreenMusicScroll.h"
#include "ScreenPlayerOptions.h"
#include "ScreenSelectCourse.h"
#include "ScreenSelectDifficulty.h"
#include "ScreenSelectGame.h"
#include "ScreenSelectGroup.h"
#include "ScreenSelectMusic.h"
#include "ScreenSelectStyle5th.h"
#include "ScreenSelectStyle.h"
#include "ScreenSongOptions.h"
#include "ScreenStage.h"
#include "ScreenTest.h"
#include "ScreenTestFonts.h"
#include "ScreenTestSound.h"
#include "ScreenTitleMenu.h"
#include "ScreenEz2SelectMusic.h"
#include "ScreenWarning.h"
#include "ScreenHighScores.h"
#include "ScreenMemoryCard.h"
#include "ScreenCompany.h"
#include "ScreenAlbums.h"
#include "ScreenLogo.h"
#include "ScreenUnlock.h"
#include "ScreenDemonstration.h"
#include "ScreenInstructions.h"
#include "ScreenPrompt.h"
#include "ScreenTextEntry.h"

Screen* ScreenManager::MakeNewScreen( CString sClassName )
{
#define RETURN_IF_MATCH(className)	if(0==stricmp(sClassName,"##className##")) return new className
	Screen *ret = NULL;

	if(		 0==stricmp(sClassName, "ScreenAppearanceOptions") )ret = new ScreenAppearanceOptions;
	else if( 0==stricmp(sClassName, "ScreenCaution") )			ret = new ScreenCaution;
	else if( 0==stricmp(sClassName, "ScreenEdit") )				ret = new ScreenEdit;
	else if( 0==stricmp(sClassName, "ScreenEditMenu") )			ret = new ScreenEditMenu;
	else if( 0==stricmp(sClassName, "ScreenEvaluation") )		ret = new ScreenEvaluation;
	else if( 0==stricmp(sClassName, "ScreenFinalEvaluation") )	ret = new ScreenFinalEvaluation;
	else if( 0==stricmp(sClassName, "ScreenEz2SelectPlayer") )	ret = new ScreenEz2SelectPlayer;
	else if( 0==stricmp(sClassName, "ScreenSelectMode") )		ret = new ScreenSelectMode;
	else if( 0==stricmp(sClassName, "ScreenGameOver") )			ret = new ScreenGameOver;
	else if( 0==stricmp(sClassName, "ScreenGameplay") )			ret = new ScreenGameplay;
	else if( 0==stricmp(sClassName, "ScreenGraphicOptions") )	ret = new ScreenGraphicOptions;
	else if( 0==stricmp(sClassName, "ScreenHowToPlay") )		ret = new ScreenHowToPlay;
	else if( 0==stricmp(sClassName, "ScreenInputOptions") )		ret = new ScreenInputOptions;
	else if( 0==stricmp(sClassName, "ScreenMachineOptions") )	ret = new ScreenMachineOptions;
	else if( 0==stricmp(sClassName, "ScreenMapControllers") )	ret = new ScreenMapControllers;
	else if( 0==stricmp(sClassName, "ScreenInputOptions") )		ret = new ScreenInputOptions;
	else if( 0==stricmp(sClassName, "ScreenMusicScroll") )		ret = new ScreenMusicScroll;
	else if( 0==stricmp(sClassName, "ScreenPlayerOptions") )	ret = new ScreenPlayerOptions;
	else if( 0==stricmp(sClassName, "ScreenSelectCourse") )		ret = new ScreenSelectCourse;
	else if( 0==stricmp(sClassName, "ScreenSelectDifficulty") )	ret = new ScreenSelectDifficulty;
	else if( 0==stricmp(sClassName, "ScreenSelectGame") )		ret = new ScreenSelectGame;
	else if( 0==stricmp(sClassName, "ScreenSelectGroup") )		ret = new ScreenSelectGroup;
	else if( 0==stricmp(sClassName, "ScreenSelectMusic") )		ret = new ScreenSelectMusic;
	else if( 0==stricmp(sClassName, "ScreenSelectStyle5th") )	ret = new ScreenSelectStyle5th;
	else if( 0==stricmp(sClassName, "ScreenSelectStyle") )		ret = new ScreenSelectStyle;
	else if( 0==stricmp(sClassName, "ScreenSongOptions") )		ret = new ScreenSongOptions;
	else if( 0==stricmp(sClassName, "ScreenStage") )			ret = new ScreenStage;
	else if( 0==stricmp(sClassName, "ScreenTest") )				ret = new ScreenTest;
	else if( 0==stricmp(sClassName, "ScreenTestFonts") )		ret = new ScreenTestFonts;
	else if( 0==stricmp(sClassName, "ScreenTestSound") )		ret = new ScreenTestSound;
	else if( 0==stricmp(sClassName, "ScreenTitleMenu") )		ret = new ScreenTitleMenu;
	else if( 0==stricmp(sClassName, "ScreenEz2SelectMusic") )	ret = new ScreenEz2SelectMusic;
	else if( 0==stricmp(sClassName, "ScreenWarning") )			ret = new ScreenWarning;
	else if( 0==stricmp(sClassName, "ScreenHighScores") )		ret = new ScreenHighScores;
	else if( 0==stricmp(sClassName, "ScreenMemoryCard") )		ret = new ScreenMemoryCard;
	else if( 0==stricmp(sClassName, "ScreenCompany") )			ret = new ScreenCompany;
	else if( 0==stricmp(sClassName, "ScreenAlbums") )			ret = new ScreenAlbums;
	else if( 0==stricmp(sClassName, "ScreenLogo") )				ret = new ScreenLogo;
	else if( 0==stricmp(sClassName, "ScreenUnlock") )			ret = new ScreenUnlock;
	else if( 0==stricmp(sClassName, "ScreenDemonstration") )	ret = new ScreenDemonstration;
	else if( 0==stricmp(sClassName, "ScreenInstructions") )		ret = new ScreenInstructions;
	else
		RageException::Throw( "Invalid Screen class name '%s'", sClassName.GetString() );

	/* Give a null update to the new screen.  This bumps everything into its
	 * initial tween state, if any, so we don't show stuff at an incorrect
	 * position for a frame. 
	 *
	 * XXX: Can't do this here, since this update might cause another screen
	 * to be loaded; that'll land back here, and the screen list gets messed
	 * up.  We need to make sure Update(0) is called some time before the
	 * first Draw(). */
//	ret->Update(0);

	/* Loading probably took a little while.  Let's reset stats.  This prevents us
	 * from displaying an unnaturally low FPS value, and the next FPS value we
	 * display will be accurate, which makes skips in the initial tween-ins more
	 * apparent. */
	DISPLAY->ResetStats();

	/* This is a convenient time to clean up our song cache. */
	SONGMAN->CleanCourses();

	return ret;
}

void ScreenManager::PrepNewScreen( CString sClassName )
{
	ASSERT(m_ScreenBuffered == NULL);
	m_ScreenBuffered = MakeNewScreen(sClassName);
}

void ScreenManager::LoadPreppedScreen()
{
	ASSERT( m_ScreenBuffered != NULL);
	SetNewScreen( m_ScreenBuffered  );
	m_ScreenBuffered = NULL;
}

void ScreenManager::SetNewScreen( Screen *pNewScreen )
{
	RefreshCreditsMessages();

	// move current screen(s) to ScreenToDelete
	m_ScreensToDelete.insert(m_ScreensToDelete.end(), m_ScreenStack.begin(), m_ScreenStack.end());

	m_ScreenStack.clear();
	m_ScreenStack.push_back( pNewScreen );
}

void ScreenManager::SetNewScreen( CString sClassName )
{
	/* If we prepped a screen but didn't use it, nuke it. */
	SAFE_DELETE( m_ScreenBuffered );
	/* Explicitely flush the directory cache each time we load a new screen.
	 * Perhaps we should only do this in debug? */
	FlushDirCache();

	RageTimer t;
	
	// It makes sense that ScreenManager should allocate memory for a new screen since it 
	// deletes it later on.  This also convention will reduce includes because screens won't 
	// have to include each other's headers of other screens.
	Screen* pNewScreen = MakeNewScreen(sClassName);
	LOG->Trace( "Loaded %s in %f", sClassName.GetString(), t.GetDeltaTime());

	SetNewScreen( pNewScreen );
}

void ScreenManager::Prompt( ScreenMessage SM_SendWhenDone, CString sText, bool bYesNo, bool bDefaultAnswer, void(*OnYes)(), void(*OnNo)() )
{
	FlushDirCache();

	// add the new state onto the back of the array
	m_ScreenStack.push_back( new ScreenPrompt(SM_SendWhenDone, sText, bYesNo, bDefaultAnswer, OnYes, OnNo) );
}

void ScreenManager::TextEntry( ScreenMessage SM_SendWhenDone, CString sQuestion, CString sInitialAnswer, void(*OnOK)(CString sAnswer), void(*OnCanel)() )
{	
	FlushDirCache();

	// add the new state onto the back of the array
	m_ScreenStack.push_back( new ScreenTextEntry(SM_SendWhenDone, sQuestion, sInitialAnswer, OnOK, OnCanel) );
}

void ScreenManager::PopTopScreen( ScreenMessage SM )
{
	Screen* pScreenToPop = m_ScreenStack.back();	// top menu
	//pScreenToPop->HandleScreenMessage( SM_LosingInputFocus );
	m_ScreenStack.erase(m_ScreenStack.end()-1, m_ScreenStack.end());
	m_ScreensToDelete.push_back( pScreenToPop );
	
	Screen* pNewTopScreen = m_ScreenStack[m_ScreenStack.size()-1];
	pNewTopScreen->HandleScreenMessage( SM );
}

void ScreenManager::SendMessageToTopScreen( ScreenMessage SM, float fDelay )
{
	Screen* pTopScreen = m_ScreenStack.back();
	pTopScreen->SendScreenMessage( SM, fDelay );
}


void ScreenManager::SystemMessage( CString sMessage )
{
	// Look for an open spot
	m_textSystemMessage.StopTweening();
	m_textSystemMessage.SetText( sMessage );
	m_textSystemMessage.SetDiffuse( RageColor(1,1,1,1) );
	m_textSystemMessage.BeginTweening( 5 );
	m_textSystemMessage.BeginTweening( 1 );
	m_textSystemMessage.SetTweenDiffuse( RageColor(1,1,1,0) );

	LOG->Trace( "WARNING:  Didn't find an empty system messages slot." );
}

/*
void ScreenManager::OverrideCreditsMessage( PlayerNumber pn, CString sNewString )
{
	m_textCreditInfo[p].SetText( sNewString );
}
*/

void ScreenManager::RefreshCreditsMessages()
{
	// update joined
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		m_textCreditInfo[p].LoadFromFont( THEME->GetPathTo("Fonts","credits") );
		m_textCreditInfo[p].SetXY( CREDITS_X(p), CREDITS_Y(p) );
		m_textCreditInfo[p].SetZoom( CREDITS_ZOOM );
		m_textCreditInfo[p].SetDiffuse( CREDITS_COLOR );
		m_textCreditInfo[p].SetShadowLength( CREDITS_SHADOW_LENGTH );

		if(      GAMESTATE->m_bSideIsJoined[p] )	m_textCreditInfo[p].SetText( "" );
		else if( GAMESTATE->m_bPlayersCanJoin )	m_textCreditInfo[p].SetText( "PRESS START" );
		else									m_textCreditInfo[p].SetText( "CREDIT(S) 0 / 0" );
	}
}
