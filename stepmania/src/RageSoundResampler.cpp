#include "global.h"
#include "RageSoundResampler.h"
#include "RageUtil.h"
#include "RageLog.h"

/*
 * This class handles sound resampling.
 *
 * This isn't very efficient; we write to a static buffer instead of a circular
 * one.  I'll optimize it if it becomes an issue.
 */

const int channels = 2;

RageSoundResampler::RageSoundResampler()
{
	reset();
}

void RageSoundResampler::reset()
{
	at_eof = false;
	memset(prev, 0, sizeof(prev));
	memset(t, 0, sizeof(t));
	outbuf.clear();
	ipos = 0;
}


/* Write data to be converted. */
void RageSoundResampler::write(const void *data_, int bytes)
{
	ASSERT(!at_eof);

	const Sint16 *data = (const Sint16 *) data_;

	const unsigned samples = bytes / sizeof(Sint16);
	const unsigned frames = samples / channels;

	if(InputRate == OutputRate)
	{
		/* Optimization. */
		outbuf.insert(outbuf.end(), data, data+samples);
		return;
	}

#if 0
	/* Much higher quality for 44100->48000 resampling, but way too slow. */
	const int upsamp = 8;
	const float div = float(InputRate*upsamp) / OutputRate;
	for(unsigned f = 0; f < frames; ++f)
	{
		for(int u = 0; u < upsamp; ++u)
		{
			int ipos_begin = (int) roundf(ipos / div);
			int ipos_end = (int) roundf((ipos+1) / div);

			for(int c = 0; c < channels; ++c)
			{
				const float f = 0.5f;
				prev[c] = Sint16(prev[c] * (f) + data[c] * (1-f));
				for(int i = ipos_begin; i < ipos_end; ++i)
					outbuf.push_back(prev[c]);
			}
			ipos++;
		}
		data += channels;
	}
#else
	/* Lerp. */
	const float div = float(InputRate) / OutputRate;
	for(unsigned f = 0; f < frames; ++f)
	{
		for(int c = 0; c < channels; ++c)
		{
			while(t[c] < 1.0f)
			{
				Sint16 s = Sint16(prev[c]*(1-t[c]) + data[c]*t[c]);
				outbuf.push_back(s);
				t[c] += div;
			}

			t[c] -= 1;
			prev[c] = data[c];
		}

		ipos++;
		data += channels;
	}
#endif
}


void RageSoundResampler::eof()
{
	ASSERT(!at_eof);

	/* Write some silence to flush out the real data.  If we don't have any sound,
	 * don't do this, so seeking past end of file doesn't write silence. */
	bool bNeedsFlush = false;
	for( int c = 0; c < channels; ++c )
		if( prev[c] != 0 ) 
			bNeedsFlush = true;

	if( bNeedsFlush )
	{
		const int size = channels*16;
		Sint16 *data = new Sint16[size];
		memset(data, 0, size * sizeof(Sint16));
		write(data, size * sizeof(Sint16));
		delete [] data;
	}

	at_eof = true;
}


int RageSoundResampler::read(void *data, unsigned bytes)
{
	/* Don't be silly. */
	ASSERT( (bytes % sizeof(Sint16)) == 0);

	/* If no data is available, and we're at_eof, return -1. */
	if(outbuf.size() == 0 && at_eof)
		return -1;

	/* Fill as much as we have. */
	int Avail = min(outbuf.size()*sizeof(Sint16), bytes);
	memcpy(data, &outbuf[0], Avail);
	outbuf.erase(outbuf.begin(), outbuf.begin() + Avail/sizeof(Sint16));
	return Avail;
}

/*
 * Copyright (c) 2003 Glenn Maynard
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
