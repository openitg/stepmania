/* RageSoundReader_Resample_Good - fast audio resampling. */

#ifndef RAGE_SOUND_READER_RESAMPLE_GOOD_H
#define RAGE_SOUND_READER_RESAMPLE_GOOD_H

#include "RageSoundReader.h"
#include "RageSoundReader_Resample.h"

class RageSoundResampler_Polyphase;

/* This class changes the sampling rate of a sound. */
class RageSoundReader_Resample_Good: public RageSoundReader_Resample
{
public:
	/* We own source. */
	void Open(SoundReader *source);
	int GetLength() const;
	int GetLength_Fast() const;
	int SetPosition_Accurate(int ms);
	int SetPosition_Fast(int ms);
	int Read(char *buf, unsigned len);
	RageSoundReader_Resample_Good();
	virtual ~RageSoundReader_Resample_Good();
	SoundReader *Copy() const;

	/* Change the actual sample rate of a sound. */
	void SetSampleRate( int hz );
	void SetHighQuality( bool hq ) { }

	int GetSampleRate() const { return m_iSampleRate; }
	unsigned GetNumChannels() const { return m_pSource->GetNumChannels(); }
	bool IsStreamingFromDisk() const { return m_pSource->IsStreamingFromDisk(); }

private:
	void Reset();
	void ReopenResampler();

	vector<RageSoundResampler_Polyphase *> resamplers; /* one per channel */

	SoundReader *m_pSource;
	int m_iSampleRate;
};

#endif

/*
 * (c) 2006 Glenn Maynard
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
