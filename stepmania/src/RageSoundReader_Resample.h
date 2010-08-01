/* RageSoundReader_Resample - Change the sampling rate of a sound. */

#ifndef RAGE_SOUND_READER_RESAMPLE
#define RAGE_SOUND_READER_RESAMPLE

#include "RageSoundReader.h"

class RageSoundReader_Resample: public SoundReader
{
public:
	enum ResampleQuality
	{
		RESAMP_FAST,
		RESAMP_NORMAL,
		RESAMP_HIGHQUALITY,
		RESAMP_INVALID
	};

	/* We own source. */
	virtual void Open( SoundReader *pSource ) = 0;

	/* Change the actual sample rate of a sound. */
	virtual void SetSampleRate( int iRate ) = 0;

	/* If q = RESAMP_INVALID, the preferred resampler will be used. */
	static RageSoundReader_Resample *MakeResampler( ResampleQuality q = RESAMP_INVALID );
};

#endif

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