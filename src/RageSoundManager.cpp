/* Handle and provide an interface to the sound driver.  Delete sounds that
 * have been detached from their owner when they're finished playing.  Distribute Update
 * calls to all sounds. */
#include "global.h"

#include "RageSoundManager.h"
#include "RageException.h"
#include "RageUtil.h"
#include "RageSound.h"
#include "RageLog.h"
#include "RageTimer.h"

#include "arch/arch.h"
#include "arch/Sound/RageSoundDriver.h"
#include "SDL_audio.h"

RageSoundManager *SOUNDMAN = NULL;

RageSoundManager::RageSoundManager(CString drivers)
{
	/* needs to be done first */
	SOUNDMAN = this;

	try
	{
		MixVolume = 1.0f;

		driver = MakeRageSoundDriver(drivers);
		if(!driver)
			RageException::Throw("Couldn't find a sound driver that works");
	} catch(...) {
		SOUNDMAN = NULL;
		throw;
	}
}

RageSoundManager::~RageSoundManager()
{
	/* Clear any sounds that we own and havn't freed yet. */
	set<RageSound *>::iterator j = owned_sounds.begin();
	while(j != owned_sounds.end())
		delete *(j++);

	delete driver;
}

void RageSoundManager::StartMixing(RageSound *snd)
{
	driver->StartMixing(snd);
}

void RageSoundManager::StopMixing(RageSound *snd)
{
	driver->StopMixing(snd);

	/* The sound is finished, and should be deleted if it's in owned_sounds.
	 * However, this call might be *from* the sound itself, and it'd be
	 * a mess to delete it while it's on the call stack.  Instead, put it
	 * in a queue to delete, and delete it on the next update. */
	if(owned_sounds.find(snd) != owned_sounds.end()) {
		sounds_to_delete.insert(snd);
		owned_sounds.erase(snd);
	}
}

int RageSoundManager::GetPosition(const RageSound *snd) const
{
	return driver->GetPosition(snd);
}

void RageSoundManager::Update(float delta)
{
	while(!sounds_to_delete.empty())
	{
		delete *sounds_to_delete.begin();
		sounds_to_delete.erase(sounds_to_delete.begin());
	}

	for(set<RageSound *>::iterator i = all_sounds.begin();
		i != all_sounds.end(); ++i)
		(*i)->Update(delta);

	driver->Update(delta);
}

float RageSoundManager::GetPlayLatency() const
{
	return driver->GetPlayLatency();
}

int RageSoundManager::GetDriverSampleRate( int rate ) const
{
	return driver->GetSampleRate( rate );
}

RageSound *RageSoundManager::PlaySound(RageSound &snd)
{
	RageSound *sound_to_play;
	if(!snd.IsPlaying())
		sound_to_play = &snd;
	else
	{
		sound_to_play = new RageSound(snd);

		/* We're responsible for freeing it. */
		owned_sounds.insert(sound_to_play);
	}

	// Move to the start position.
	sound_to_play->SetPositionSeconds();
	sound_to_play->StartPlaying();

	return sound_to_play;
}

void RageSoundManager::StopPlayingSound(RageSound &snd)
{
	/* Stop playing all playing sounds derived from the same parent as snd. */
	vector<RageSound *> snds;
	GetCopies(snd, snds);
	for(vector<RageSound *>::iterator i = snds.begin(); i != snds.end(); i++)
	{
		if((*i)->IsPlaying())
			(*i)->StopPlaying();
	}
}

void RageSoundManager::GetCopies(RageSound &snd, vector<RageSound *> &snds)
{
	RageSound *parent = snd.GetOriginal();

	snds.clear();
	for(set<RageSound *>::iterator i = playing_sounds.begin();
		i != playing_sounds.end(); i++)
		if((*i)->GetOriginal() == parent)
			snds.push_back(*i);
}


void RageSoundManager::PlayOnce( CString sPath )
{
	/* We want this to start quickly, so don't try to prebuffer it. */
	RageSound *snd = new RageSound;
	snd->Load(sPath, false);

	/* We're responsible for freeing it. */
	owned_sounds.insert(snd);

	snd->Play();
}

void RageSoundManager::PlayOnceFromDir( CString sDir )
{
	if( sDir == "" )
		return;

	// make sure there's a slash at the end of this path
	if( sDir.Right(1) != "/" )
		sDir += "/";

	CStringArray arraySoundFiles;
	GetDirListing( sDir + "*.mp3", arraySoundFiles );
	GetDirListing( sDir + "*.wav", arraySoundFiles );
	GetDirListing( sDir + "*.ogg", arraySoundFiles );

	if( arraySoundFiles.empty() )
		return;

	int index = rand() % arraySoundFiles.size();
	SOUNDMAN->PlayOnce( sDir + arraySoundFiles[index] );
}

/* Standalone helpers: */

/* Mix audio.  This is from SDL, but doesn't depend on the sound being
 * initialized. */
void RageSoundManager::MixAudio(Sint16 *dst, const Sint16 *src, Uint32 len, float volume)
{
	if ( volume == 0 )
		return;

	int factor = int(volume * 256);
	len /= 2;
	while ( len-- ) {
		Sint16 src1 = *src;
		src1 = Sint16((src1*factor)/256);
		Sint16 src2 = *dst;

		int dst_sample = src1+src2;
		dst_sample = clamp(dst_sample, -32768, 32767);
		*dst = Sint16(dst_sample);

		src++;
		dst++;
	}
}


void RageSoundManager::SetPrefs(float MixVol)
{
	MixVolume = MixVol;
	driver->VolumeChanged();
}

void RageSoundManager::AttenuateBuf( Sint16 *buf, int samples, float vol )
{
	while( samples-- )
	{
		*buf = Sint16( (*buf) * vol );
		++buf;
	}
}

	
SoundMixBuffer::SoundMixBuffer()
{
	bufsize = used = 0;
	mixbuf = NULL;
	SetVolume( SOUNDMAN->GetMixVolume() );
}

SoundMixBuffer::~SoundMixBuffer()
{
	free(mixbuf);
}

void SoundMixBuffer::SetVolume( float f )
{
	vol = int(256*f);
}

void SoundMixBuffer::write(const Sint16 *buf, unsigned size)
{
	if(bufsize < size)
	{
		mixbuf = (Sint32 *) realloc(mixbuf, sizeof(Sint32) * size);
		bufsize = size;
	}

	if(used < size)
	{
		memset(mixbuf + used, 0, (size - used) * sizeof(Sint32));
		used = size;
	}

	for(unsigned pos = 0; pos < size; ++pos)
	{
		/* Scale volume and add. */
		mixbuf[pos] += buf[pos] * vol;
	}
}

void SoundMixBuffer::read(Sint16 *buf)
{
	for(unsigned pos = 0; pos < used; ++pos)
	{
		Sint32 out = (mixbuf[pos]) / 256;
		out = clamp(out, -32768, 32767);
		buf[pos] = Sint16(out);
	}

	used = 0;
}
