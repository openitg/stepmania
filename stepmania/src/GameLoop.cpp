#include "global.h"
#include "GameLoop.h"
#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageSoundManager.h"
#include "PrefsManager.h"
#include "RageDisplay.h"

#include "arch/ArchHooks/ArchHooks.h"

#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "SongManager.h"
#include "GameState.h"
#include "MemoryCardManager.h"
#include "ScreenManager.h"
#include "InputFilter.h"
#include "InputMapper.h"
#include "RageFileManager.h"
#include "LightsManager.h"
#include "NetworkSyncManager.h"
#include "RageTimer.h"
#include "RageInput.h"

static bool g_bQuitting = false;
static RageTimer g_GameplayTimer;

enum BoostAppPriority { BOOST_NO, BOOST_YES, BOOST_AUTO };	/* auto = do whatever is appropriate for the arch. */
static Preference<BoostAppPriority,int> g_BoostAppPriority( "BoostAppPriority", BOOST_AUTO );

/* experimental: force a specific update rate.  This prevents big 
 * animation jumps on frame skips.  0 to disable. */
static Preference<float> g_fConstantUpdateDeltaSeconds( "ConstantUpdateDeltaSeconds", 0 );

static bool UserQuit()
{
	return g_bQuitting || ArchHooks::UserQuit();
}

void HandleInputEvents( float fDeltaTime );

static float g_fUpdateRate = 1;
void GameLoop::SetUpdateRate( float fUpdateRate )
{
	g_fUpdateRate = fUpdateRate;
}

static void CheckGameLoopTimerSkips( float fDeltaTime )
{
	if( !PREFSMAN->m_bLogSkips )
		return;

	static int iLastFPS = 0;
	int iThisFPS = DISPLAY->GetFPS();

	/* If vsync is on, and we have a solid framerate (vsync == refresh and we've sustained this
	 * for at least one second), we expect the amount of time for the last frame to be 1/FPS. */
	if( iThisFPS != DISPLAY->GetActualVideoModeParams().rate || iThisFPS != iLastFPS )
	{
		iLastFPS = iThisFPS;
		return;
	}

	const float fExpectedTime = 1.0f / iThisFPS;
	const float fDifference = fDeltaTime - fExpectedTime;
	if( fabsf(fDifference) > 0.002f && fabsf(fDifference) < 0.100f )
		LOG->Trace( "GameLoop timer skip: %i FPS, expected %.3f, got %.3f (%.3f difference)",
			iThisFPS, fExpectedTime, fDeltaTime, fDifference );
}

static bool ChangeAppPri()
{
	if( g_BoostAppPriority.Get() == BOOST_NO )
		return false;

	// if using NTPAD don't boost or else input is laggy
#if defined(_WINDOWS)
	if( g_BoostAppPriority == BOOST_AUTO )
	{
		vector<InputDevice> vDevices;
		vector<RString> vDescriptions;

		// This can get called before INPUTMAN is constructed.
		if( INPUTMAN )
		{
			INPUTMAN->GetDevicesAndDescriptions(vDevices,vDescriptions);
			RString sInputDevices = join( ",", vDescriptions );
			if( sInputDevices.find("NTPAD") != string::npos )
			{
				LOG->Trace( "Using NTPAD.  Don't boost priority." );
				return false;
			}
		}
	}
#endif

	/* If -1 and this is a debug build, don't.  It makes the debugger sluggish. */
#ifdef DEBUG
	if( g_BoostAppPriority == BOOST_AUTO )
		return false;
#endif

	return true;
}

static void CheckFocus()
{
	static bool bHasFocus = true;
	
	bool bHasFocusNow = HOOKS->AppHasFocus();
	if( bHasFocus == bHasFocusNow )
		return;
	bHasFocus = bHasFocusNow;

	/* If we lose focus, we may lose input events, especially key releases. */
	INPUTFILTER->Reset();

	if( ChangeAppPri() )
	{
		if( bHasFocus )
			HOOKS->BoostPriority();
		else
			HOOKS->UnBoostPriority();
	}
}

void GameLoop::GameLoop()
{
	/* People may want to do something else while songs are loading, so do
	 * this after loading songs. */
	if( ChangeAppPri() )
		HOOKS->BoostPriority();

	while( !UserQuit() )
	{
		/*
		 * Update
		 */
		float fDeltaTime = g_GameplayTimer.GetDeltaTime();

		if( g_fConstantUpdateDeltaSeconds > 0 )
			fDeltaTime = g_fConstantUpdateDeltaSeconds;
		
		CheckGameLoopTimerSkips( fDeltaTime );

		fDeltaTime *= g_fUpdateRate;

		CheckFocus();

		/* Update SOUNDMAN early (before any RageSound::GetPosition calls), to flush position data. */
		SOUNDMAN->Update( fDeltaTime );

		/* Update song beat information -before- calling update on all the classes that
		 * depend on it.  If you don't do this first, the classes are all acting on old 
		 * information and will lag.  (but no longer fatally, due to timestamping -glenn) */
		SOUND->Update( fDeltaTime );
		TEXTUREMAN->Update( fDeltaTime );
		GAMESTATE->Update( fDeltaTime );
		SCREENMAN->Update( fDeltaTime );
		MEMCARDMAN->Update( fDeltaTime );
		NSMAN->Update( fDeltaTime );

		/* Important:  Process input AFTER updating game logic, or input will be acting on song beat from last frame */
		HandleInputEvents( fDeltaTime );

		if( INPUTMAN->DevicesChanged() )
		{
			INPUTFILTER->Reset();	// fix "buttons stuck" if button held while unplugged
			INPUTMAN->LoadDrivers();
			RString sMessage;
			if( INPUTMAPPER->CheckForChangedInputDevicesAndRemap(sMessage) )
				SCREENMAN->SystemMessage( sMessage );
		}

		LIGHTSMAN->Update( fDeltaTime );

		/*
		 * Render
		 */
		SCREENMAN->Draw();

		/* If we don't have focus, give up lots of CPU. */
		// XXX: do this in DISPLAY EndFrame?
		if( !HOOKS->AppHasFocus() )
			usleep( 10000 );// give some time to other processes and threads
#if defined(_WINDOWS)
		/* In Windows, we want to give up some CPU for other threads.  Most OS's do
		 * this more intelligently. */
		else
			usleep( 1000 );	// give some time to other processes and threads
#endif
	}

	if( ChangeAppPri() )
		HOOKS->UnBoostPriority();
}

class ConcurrentRenderer
{
public:
	ConcurrentRenderer();
	~ConcurrentRenderer();

private:
	RageThread m_Thread;
	bool m_bShutdown;
	void RenderThread();
	static int StartRenderThread( void *p );
};
static ConcurrentRenderer *g_pConcurrentRenderer = NULL;

ConcurrentRenderer::ConcurrentRenderer()
{
	m_bShutdown = false;

	m_Thread.SetName( "ConcurrentRenderer" );
	m_Thread.Create( StartRenderThread, this );
}

ConcurrentRenderer::~ConcurrentRenderer()
{
	m_bShutdown = true;
	m_Thread.Wait();
}

void ConcurrentRenderer::RenderThread()
{
	ASSERT( SCREENMAN != NULL );

	DISPLAY->BeginConcurrentRendering();
	HOOKS->SetupConcurrentRenderingThread();

	LOG->Trace( "ConcurrentRenderer::RenderThread start" );

	/* This is called during Update().  The next thing the game loop
	 * will do is Draw, so shift operations around to put Draw at the
	 * top.  This makes sure updates are seamless. */
	while( !m_bShutdown )
	{
		SCREENMAN->Draw();

		float fDeltaTime = g_GameplayTimer.GetDeltaTime();
		SCREENMAN->Update( fDeltaTime );
	}
	DISPLAY->EndConcurrentRendering();

	LOG->Trace( "ConcurrentRenderer::RenderThread done" );
}

int ConcurrentRenderer::StartRenderThread( void *p )
{
	((ConcurrentRenderer *) p)->RenderThread();
	return 0;
}

void GameLoop::StartConcurrentRendering()
{
	ASSERT( g_pConcurrentRenderer == NULL );
	g_pConcurrentRenderer = new ConcurrentRenderer;
}

void GameLoop::FinishConcurrentRendering()
{
	SAFE_DELETE( g_pConcurrentRenderer );
}

/*
 * (c) 2001-2005 Chris Danford, Glenn Maynard
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
