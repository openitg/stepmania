#ifndef RAGE_SOUND_DSOUND
#define RAGE_SOUND_DSOUND

#include "RageSoundDriver.h"
#include "DSoundHelpers.h"
#include "RageThreads.h"
#include "RageTimer.h"

struct IDirectSound;
struct IDirectSoundBuffer;

class RageSound_DSound: public RageSoundDriver
{
	/* The only place that takes sounds out of INACTIVE is StartMixing; this mutex
	 * serializes inactive sounds. */
	RageMutex m_InactiveSoundMutex;

	struct stream {
	    /* Actual audio stream: */
		DSoundBuf *pcm;

	    /* Sound object that's playing on this stream, or NULL if this
	     * channel is available: */
	    RageSoundBase *snd;

		enum {
			INACTIVE,
			SETUP,
			PLAYING,
			FLUSHING,
			FINISHED
		} state;

		int64_t flush_pos; /* state == FLUSHING only */

		RageTimer start_time;
		bool GetData( bool init, bool &bEOF );

	    stream() { pcm = NULL; snd = NULL; state=INACTIVE; }
		~stream();
	};

	/* Pool of available streams. */
	vector<stream *> stream_pool;

	DSound ds;

	RageMutex m_Mutex;

	bool shutdown; /* tells the MixerThread to shut down */
	static int MixerThread_start(void *p);
	void MixerThread();
	RageThread MixingThread;

	/* virtuals: */
	void StartMixing( RageSoundBase *snd );	/* used by RageSound */
	void StopMixing( RageSoundBase *snd );		/* used by RageSound */
	int64_t GetPosition( const RageSoundBase *snd ) const;
	void Update(float delta);

	int GetSampleRate( int rate ) const { return rate; }

public:
	RageSound_DSound();
	~RageSound_DSound();
};

#endif

/*
 * Copyright (c) 2002-2004 by the person(s) listed below.  All rights reserved.
 *
 * Glenn Maynard
 */
