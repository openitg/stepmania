#include "global.h"
#include "RageSoundManager.h"
#include "RageSounds.h"
#include "RageSound.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "GameState.h"
#include "TimingData.h"
#include "NotesLoaderSM.h"
#include "PrefsManager.h"

RageSounds *SOUND = NULL;

/*
 * When playing music, automatically search for an SM file for timing data.  If one is
 * found, automatically handle GAMESTATE->m_fSongBeat, etc.
 *
 * modf(GAMESTATE->m_fSongBeat) should always be continuously moving from 0 to 1.  To do
 * this, wait before starting a sound until the fractional portion of the beat will be
 * the same.
 *
 * If PlayMusic(length_sec) is set, peek at the beat, and extend the length so we'll be
 * on the same fractional beat when we loop.  (XXX: should we increase fade_len, too?
 * That would cause the extra pad time to be silence.)
 */
/* Lock this before touching any of these globals. */
static RageMutex *g_Mutex;
static bool g_UpdatingTimer;
static bool g_ThreadedMusicStart = true;
static bool g_Shutdown;

struct MusicPlaying
{
	bool m_TimingDelayed;
	bool m_HasTiming;
	/* The timing data that we're currently using. */
	TimingData m_Timing;

	/* If m_TimingDelayed is true, this will be the timing data for the song that's starting.
	 * We'll copy it to m_Timing once sound is heard. */
	TimingData m_NewTiming;
	RageSound *m_Music;
	MusicPlaying( RageSound *Music )
	{
		m_Timing.AddBPMSegment( BPMSegment(0,120) );
		m_NewTiming.AddBPMSegment( BPMSegment(0,120) );
		m_HasTiming = false;
		m_TimingDelayed = false;
		m_Music = Music;
	}

	~MusicPlaying()
	{
		delete m_Music;
	}
};

struct MusicToPlay
{
	CString file, timing_file;
	bool HasTiming;
	bool force_loop;
	float start_sec, length_sec, fade_len;
	bool pending;
	MusicToPlay() { pending=false; }
};

static MusicToPlay g_MusicToPlay;
static MusicPlaying *g_Playing;
static RageThread MusicThread;

static vector<CString> g_SoundsToPlayOnce, g_SoundsToPlayOnceFromDir;
void StartQueuedSounds()
{
	/* Don't hold the mutex if we don't have to. */
	while( 1 )
	{
		CString sPath;

		g_Mutex->Lock();
		if( g_SoundsToPlayOnce.size() )
		{
			sPath = g_SoundsToPlayOnce.back();
			g_SoundsToPlayOnce.erase( g_SoundsToPlayOnce.begin()+g_SoundsToPlayOnce.size()-1, g_SoundsToPlayOnce.end() );
		}
		g_Mutex->Unlock();

		if( sPath != "" )
			SOUNDMAN->PlayOnce( sPath );
		else
			break;
	}

	while( 1 )
	{
		CString sPath;

		g_Mutex->Lock();
		if( g_SoundsToPlayOnceFromDir.size() )
		{
			sPath = g_SoundsToPlayOnceFromDir.back();
			g_SoundsToPlayOnceFromDir.erase( g_SoundsToPlayOnceFromDir.begin()+g_SoundsToPlayOnceFromDir.size()-1, g_SoundsToPlayOnceFromDir.end() );
		}
		g_Mutex->Unlock();

		if( sPath != "" )
		{
			// make sure there's a slash at the end of this path
			if( sPath.Right(1) != "/" )
				sPath += "/";

			CStringArray arraySoundFiles;
			GetDirListing( sPath + "*.mp3", arraySoundFiles );
			GetDirListing( sPath + "*.wav", arraySoundFiles );
			GetDirListing( sPath + "*.ogg", arraySoundFiles );

			if( arraySoundFiles.empty() )
				return;

			int index = rand() % arraySoundFiles.size();
			SOUNDMAN->PlayOnce( sPath + arraySoundFiles[index] );
		}
		else
			break;
	}

}

void StartPlayingMusic( const RageTimer &when, const MusicToPlay &ToPlay, MusicPlaying &Playing )
{
	Playing.m_HasTiming = ToPlay.HasTiming;
	Playing.m_TimingDelayed = true;

	Playing.m_Music->Load( ToPlay.file, false );

	RageSoundParams p;
	p.m_StartSecond = ToPlay.start_sec;
	p.m_LengthSeconds = ToPlay.length_sec;
	p.m_FadeLength = ToPlay.fade_len;
	p.StartTime = when;
	if( ToPlay.force_loop )
		p.StopMode = RageSoundParams::M_LOOP;

	Playing.m_Music->SetPositionSeconds( p.m_StartSecond );
	Playing.m_Music->StartPlaying();
}

void StartMusic( MusicToPlay &ToPlay )
{
	LockMutex L( *g_Mutex );

	if( ToPlay.file.empty() )
	{
		if( g_Playing->m_Music->IsPlaying() )
			g_Playing->m_Music->StopPlaying();
		g_Playing->m_Music->Unload();
		return;
	}

	MusicPlaying *NewMusic = new MusicPlaying( new RageSound );
	NewMusic->m_Timing = g_Playing->m_Timing;

	/* See if we can find timing data. */
	ToPlay.HasTiming = false;

	if( IsAFile(ToPlay.timing_file) )
	{
		LOG->Trace("Found '%s'", ToPlay.timing_file.c_str());
		if( SMLoader::LoadTimingFromFile( ToPlay.timing_file, NewMusic->m_NewTiming ) )
			ToPlay.HasTiming = true;
	}

	if( ToPlay.HasTiming && ToPlay.force_loop && ToPlay.length_sec != -1 )
	{
		/* Extend the loop period so it always starts and ends on the same fractional
		 * beat.  That is, if it starts on beat 1.5, and ends on beat 10.2, extend it
		 * to end on beat 10.5.  This way, effects always loop cleanly. */
		float fStartBeat = NewMusic->m_NewTiming.GetBeatFromElapsedTime( ToPlay.start_sec );
		float fEndSec = ToPlay.start_sec + ToPlay.length_sec;
		float fEndBeat = NewMusic->m_NewTiming.GetBeatFromElapsedTime( fEndSec );
		
		const float fStartBeatFraction = fmodfp( fStartBeat, 1 );
		const float fEndBeatFraction = fmodfp( fEndBeat, 1 );

		float fBeatDifference = fStartBeatFraction - fEndBeatFraction;
		if( fBeatDifference < 0 )
			fBeatDifference += 1.0f; /* unwrap */

		fEndBeat += fBeatDifference;

		const float fRealEndSec = NewMusic->m_NewTiming.GetElapsedTimeFromBeat( fEndBeat );
		const float fNewLengthSec = fRealEndSec - ToPlay.start_sec;

		/* Extend the fade_len, so the added time is faded out. */
		ToPlay.fade_len += fNewLengthSec - ToPlay.length_sec;
		ToPlay.length_sec = fNewLengthSec;
	}

	bool StartImmediately = false;
	if( !ToPlay.HasTiming )
	{
		/* This song has no real timing data.  The offset is arbitrary.  Change it so
		 * the beat will line up to where we are now, so we don't have to delay. */
		float fDestBeat = fmodfp( GAMESTATE->m_fSongBeat, 1 );
		float fTime = NewMusic->m_NewTiming.GetElapsedTimeFromBeat( fDestBeat );

		NewMusic->m_NewTiming.m_fBeat0OffsetInSeconds = fTime;

		StartImmediately = true;
	}

	/* If we have an active timer, try to start on the next update.  Otherwise,
	 * start now. */
	if( !g_Playing->m_HasTiming && !g_UpdatingTimer )
		StartImmediately = true;

	RageTimer when; /* zero */
	if( !StartImmediately )
	{
		/* GetPlayLatency returns the minimum time until a sound starts.  That's
		 * common when starting a precached sound, but our sound isn't, so it'll
		 * probably take a little longer.  Nudge the latency up. */
		const float PresumedLatency = SOUND->GetPlayLatency() + 0.040f;
		const float fCurSecond = GAMESTATE->m_fMusicSeconds + PresumedLatency;
		const float fCurBeat = g_Playing->m_Timing.GetBeatFromElapsedTime( fCurSecond );
		const float fCurBeatFraction = fmodfp( fCurBeat,1 );

		/* The beat that the new sound will start on. */
		const float fStartBeat = NewMusic->m_NewTiming.GetBeatFromElapsedTime( ToPlay.start_sec );
		float fStartBeatFraction = fmodfp( fStartBeat, 1 );
		if( fStartBeatFraction < fCurBeatFraction )
			fStartBeatFraction += 1.0f; /* unwrap */

		const float fCurBeatToStartOn = truncf(fCurBeat) + fStartBeatFraction;
		const float fSecondToStartOn = g_Playing->m_Timing.GetElapsedTimeFromBeat( fCurBeatToStartOn );
		const float fMaximumDistance = 2;
		const float fDistance = min( fSecondToStartOn - fCurSecond, fMaximumDistance );

		when = GAMESTATE->m_LastBeatUpdate + PresumedLatency + fDistance;
	}

	/* Important: don't hold the mutex while we load the actual sound. */
	L.Unlock();

	StartPlayingMusic( when, ToPlay, *NewMusic );

	LockMut( *g_Mutex );
	delete g_Playing;
	g_Playing = NewMusic;
}

int MusicThread_start( void *p )
{
	while( !g_Shutdown )
	{
		SDL_Delay( 10 );

		StartQueuedSounds();

		LockMutex L( *g_Mutex );
		if( !g_MusicToPlay.pending )
			continue;

		/* We have a sound to start.  Don't keep the lock while we do this; if another
		 * music tries to start in the meantime, it'll cause a skip. */
		MusicToPlay ToPlay = g_MusicToPlay;
		g_MusicToPlay.pending = false;

		L.Unlock();

		StartMusic( ToPlay );
	}

	return 0;
}

RageSounds::RageSounds()
{
	/* Init RageSoundMan first: */
	ASSERT( SOUNDMAN );

	g_Mutex = new RageMutex;
	g_Playing = new MusicPlaying( new RageSound );

	g_UpdatingTimer = false;

	if( g_ThreadedMusicStart )
	{
		g_Shutdown = false;
		MusicThread.SetName( "MusicThread" );
		MusicThread.Create( MusicThread_start, this );
	}
}

RageSounds::~RageSounds()
{
	if( g_ThreadedMusicStart )
	{
		/* Signal the mixing thread to quit. */
		g_Shutdown = true;
		LOG->Trace("Shutting down music start thread ...");
		MusicThread.Wait();
		LOG->Trace("Music start thread shut down.");
	}

	delete g_Playing;
	delete g_Mutex;
}


void RageSounds::Update( float fDeltaTime )
{
	LockMut( *g_Mutex );

	if( !g_UpdatingTimer )
		return;

	if( !g_Playing->m_Music->IsPlaying() )
	{
		/* There's no song playing.  Fake it. */
		CHECKPOINT_M( ssprintf("%f, delta %f", GAMESTATE->m_fMusicSeconds, fDeltaTime) );
		GAMESTATE->UpdateSongPosition( GAMESTATE->m_fMusicSeconds + fDeltaTime, g_Playing->m_Timing );
		return;
	}

	/* There's a delay between us calling Play() and the sound actually playing.
	 * During this time, approximate will be true.  Keep using the previous timing
	 * data until we get a non-approximate time, indicating that the sound has actually
	 * started playing. */
	bool approximate;
	RageTimer tm;
	const float fSeconds = g_Playing->m_Music->GetPositionSeconds( &approximate, &tm );

	if( g_Playing->m_TimingDelayed )
	{
		if( approximate )
		{
			/* We're still waiting for the new sound to start playing, so keep using the
			 * old timing data and fake the time. */
			CHECKPOINT_M( ssprintf("%f, delta %f", GAMESTATE->m_fMusicSeconds, fDeltaTime) );
			GAMESTATE->UpdateSongPosition( GAMESTATE->m_fMusicSeconds + fDeltaTime, g_Playing->m_Timing );
			return;
		}
		else
		{
			/* We've passed the start position of the new sound, so we should be OK.
			 * Load up the new timing data. */
			g_Playing->m_Timing = g_Playing->m_NewTiming;
			g_Playing->m_TimingDelayed = false;
		}
	}
	else if( PREFSMAN->m_bLogSkips )
	{
		const float fExpectedTimePassed = (tm - GAMESTATE->m_LastBeatUpdate) * g_Playing->m_Music->GetPlaybackRate();
		const float fSoundTimePassed = fSeconds - GAMESTATE->m_fMusicSeconds;
		const float fDiff = fExpectedTimePassed - fSoundTimePassed;

		static CString sLastFile = "";
		const CString ThisFile = g_Playing->m_Music->GetLoadedFilePath();

		/* If fSoundTimePassed < 0, the sound has probably looped. */
		if( sLastFile == ThisFile && fSoundTimePassed >= 0 && fabsf(fDiff) > 0.003f )
			LOG->Trace("Song position skip in %s: expected %.3f, got %.3f (cur %f, prev %f) (%.3f difference)",
				Basename(ThisFile).c_str(), fExpectedTimePassed, fSoundTimePassed, fSeconds, GAMESTATE->m_fMusicSeconds, fDiff );
		sLastFile = ThisFile;
	}

	CHECKPOINT_M( ssprintf("%f", fSeconds) );
	GAMESTATE->UpdateSongPosition( fSeconds, g_Playing->m_Timing, tm );
}


CString RageSounds::GetMusicPath() const
{
	LockMut( *g_Mutex );
	return g_Playing->m_Music->GetLoadedFilePath();
}

/* This function should not touch the disk at all. */
void RageSounds::PlayMusic( const CString &file, const CString &timing_file, bool force_loop, float start_sec, float length_sec, float fade_len )
{
	LockMut( *g_Mutex );
//	LOG->Trace("play '%s' (current '%s')", file.c_str(), g_Playing->m_Music->GetLoadedFilePath().c_str());
	if( g_Playing->m_Music->IsPlaying() && !g_Playing->m_Music->GetLoadedFilePath().CompareNoCase(file) )
		return;		// do nothing

	MusicToPlay ToPlay;

	ToPlay.file = file;
	ToPlay.force_loop = force_loop;
	ToPlay.start_sec = start_sec;
	ToPlay.length_sec = length_sec;
	ToPlay.fade_len = fade_len;
	ToPlay.timing_file = timing_file;
	ToPlay.pending = true;

	/* If no timing file was specified, look for one in the same place as the music file. */
	if( ToPlay.timing_file == "" )
		ToPlay.timing_file = SetExtension( file, "sm" );

	if( g_ThreadedMusicStart )
	{
		g_MusicToPlay = ToPlay;
	}
	else
		StartMusic( ToPlay );
}

void RageSounds::HandleSongTimer( bool on )
{
	LockMut( *g_Mutex );
	g_UpdatingTimer = on;
}

void RageSounds::PlayOnce( CString sPath )
{
	g_Mutex->Lock();
	g_SoundsToPlayOnce.push_back( sPath );
	g_Mutex->Unlock();

	if( !g_ThreadedMusicStart )
		StartQueuedSounds();
}

void RageSounds::PlayOnceFromDir( CString PlayOnceFromDir )
{
	g_Mutex->Lock();
	g_SoundsToPlayOnceFromDir.push_back( PlayOnceFromDir );
	g_Mutex->Unlock();

	if( !g_ThreadedMusicStart )
		StartQueuedSounds();
}

float RageSounds::GetPlayLatency() const
{
	return SOUNDMAN->GetPlayLatency();
}

void RageSounds::TakeOverSound( RageSound *snd, const TimingData *Timing )
{
	MusicPlaying *NewMusic = new MusicPlaying( snd );

	NewMusic->m_TimingDelayed = false;
	NewMusic->m_HasTiming = false;
	if( Timing )
	{
		NewMusic->m_HasTiming = true;
		NewMusic->m_Timing = *Timing;
	}

	LockMut( *g_Mutex );
	delete g_Playing;
	g_Playing = NewMusic;

	Update( 0 );

	const float fSeconds = g_Playing->m_Music->GetPositionSeconds();
	LOG->Trace("Updated: %f sec, %f beat", fSeconds, GAMESTATE->m_fSongBeat );
}

/*
-----------------------------------------------------------------------------
 Copyright (c) 2002-2003 by the person(s) listed below.  All rights reserved.
        Glenn Maynard
-----------------------------------------------------------------------------
*/

