#include "global.h"
#include "MovieTexture_FFMpeg.h"

#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "SDL_utils.h"
#include "SDL_endian.h"

namespace avcodec
{
#if defined(_WIN32)
#include "ffmpeg/include/ffmpeg/avformat.h"
#else
#include <ffmpeg/avformat.h>
#endif
};

#if defined(_WIN32)
	#pragma comment(lib, "ffmpeg/lib/avcodec.lib")
	#pragma comment(lib, "ffmpeg/lib/avformat.lib")
#endif

struct AVPixelFormat_t
{
	int bpp;
	int masks[4];
	avcodec::PixelFormat pf;
	bool HighColor;
	bool ByteSwapOnLittleEndian;
} AVPixelFormats[] = {
	{ 
		/* This format is really ARGB, and is affected by endianness, unlike PIX_FMT_RGB24
		 * and PIX_FMT_BGR24. */
		32,
		{ 0x00FF0000,
		  0x0000FF00,
		  0x000000FF,
		  0xFF000000 },
		avcodec::PIX_FMT_RGBA32,
		true,
		false
	},
	{ 
		24,
		{ 0xFF0000,
		  0x00FF00,
		  0x0000FF,
		  0x000000 },
		avcodec::PIX_FMT_RGB24,
		true,
		true
	},
	{ 
		24,
		{ 0x0000FF,
		  0x00FF00,
		  0xFF0000,
		  0x000000 },
		avcodec::PIX_FMT_BGR24,
		true,
		true
	},
	{
		16,
		{ 0x7C00,
		  0x03E0,
		  0x001F,
		  0x8000 },
		avcodec::PIX_FMT_RGB555,
		false,
		false
	},
	{ 0, { 0,0,0,0 }, avcodec::PIX_FMT_NB, true, false }
};

static void FixLilEndian()
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	static bool Initialized = false;
	if( Initialized )
		return; 
	Initialized = true;

	for( int i = 0; i < AVPixelFormats[i].bpp; ++i )
	{
		AVPixelFormat_t &pf = AVPixelFormats[i];

		if( !pf.ByteSwapOnLittleEndian )
			continue;

		for( int mask = 0; mask < 4; ++mask)
		{
			int m = pf.masks[mask];
			switch( pf.bpp )
			{
				case 24: m = mySDL_Swap24(m); break;
				case 32: m = SDL_Swap32(m); break;
				default: ASSERT(0);
			}
			pf.masks[mask] = m;
		}
	}
#endif
}

static int FindCompatibleAVFormat( PixelFormat &pixfmt, bool HighColor )
{
	for( int i = 0; AVPixelFormats[i].bpp; ++i )
	{
		AVPixelFormat_t &fmt = AVPixelFormats[i];
		if( fmt.HighColor != HighColor )
			continue;

		pixfmt = DISPLAY->FindPixelFormat( fmt.bpp,
				fmt.masks[0],
				fmt.masks[1],
				fmt.masks[2],
				fmt.masks[3] );

		if( pixfmt == NUM_PIX_FORMATS )
			continue;

		return i;
	}

	return -1;
}

class FFMpeg_Helper
{
public:
	avcodec::AVFormatContext *m_fctx;
	avcodec::AVStream *m_stream;
	bool GetNextTimestamp;
	float CurrentTimestamp, Last_IP_Timestamp;

	float LastFrameDelay;

	float pts, last_IP_pts;

	avcodec::AVPacket pkt;
	int current_packet_offset;

	avcodec::AVFrame frame;

	FFMpeg_Helper();
	int GetFrame();
	void Init();

private:
	int ReadPacket();
	int DecodePacket();
};

FFMpeg_Helper::FFMpeg_Helper()
{
	m_fctx=NULL;
	m_stream=NULL;
	current_packet_offset = -1;
	Init();
}

void FFMpeg_Helper::Init()
{
	GetNextTimestamp = true;
	CurrentTimestamp = 0, Last_IP_Timestamp = 0;
	LastFrameDelay = 0;
	pts = last_IP_pts = -1;

	if( current_packet_offset != -1 )
	{
		avcodec::av_free_packet( &pkt );
		current_packet_offset = -1;
	}
}

/* Read until we get a frame, EOF or error.  Return -1 on error, 0 on EOF, 1 if we have a frame. */
int FFMpeg_Helper::GetFrame()
{
	while( 1 )
	{
		int ret = DecodePacket();
		if( ret == 1 )
			return 1;
		if( ret == -1 )
			return -1;

		ASSERT( ret == 0 );
		ret = ReadPacket();
		if( ret <= 0 )
			return ret; /* error or EOF */
	}
}

/* Read a packet.  Return -1 on error, 0 on EOF, 1 on OK. */
int FFMpeg_Helper::ReadPacket()
{
	while( 1 )
	{
		CHECKPOINT;
		if( current_packet_offset != -1 )
		{
			current_packet_offset = -1;
			avcodec::av_free_packet( &pkt );
		}

		int ret = avcodec::av_read_packet(m_fctx, &pkt);
		if( ret == -1 )
			return 0;

		if( ret < 0 )
		{
			/* XXX ? */
//			LOG->Warn("AVCodec: Error decoding %s: %i",
//					GetID().filename.c_str(), ret );
			return -1;
		}
		
		if( pkt.stream_index == m_stream->index )
		{
			current_packet_offset = 0;
			return 1;
		}

		/* It's not for the video stream; ignore it. */
		avcodec::av_free_packet( &pkt );
	}
}


/* Decode data from the current packet.  Return -1 on error, 0 if the packet is finished,
 * and 1 if we have a frame (we may have more data in the packet). */
int FFMpeg_Helper::DecodePacket()
{
	if( current_packet_offset == -1 )
		return 0; /* no packet */

	while( current_packet_offset < pkt.size )
	{
		if ( GetNextTimestamp )
		{
			if (pkt.pts != AV_NOPTS_VALUE)
				pts = (float)pkt.pts * m_fctx->pts_num / m_fctx->pts_den;
			else
				pts = -1;
			GetNextTimestamp = false;
		}

		int got_frame;
		CHECKPOINT;
		int len = avcodec::avcodec_decode_video(
				&m_stream->codec, 
				&frame, &got_frame,
				pkt.data + current_packet_offset,
				pkt.size - current_packet_offset );
		CHECKPOINT;

		if (len < 0)
		{
			LOG->Warn("avcodec_decode_video: %i", len);
			return -1; // XXX
		}

		current_packet_offset += len;

		if (!got_frame)
			continue;

		GetNextTimestamp = true;

		/* Length of this frame: */
		LastFrameDelay = (float)m_stream->codec.frame_rate_base / m_stream->codec.frame_rate;
		LastFrameDelay += frame.repeat_pict * (LastFrameDelay * 0.5f);

		if ( m_stream->codec.has_b_frames &&
				frame.pict_type != FF_B_TYPE )
		{
			swap( pts, last_IP_pts );
		}

		if (pts != -1)
		{
			CurrentTimestamp = pts;
		}
		else
		{
			/* If the timestamp is zero, this frame is to be played at the
			 * time of the last frame plus the length of the last frame. */
			CurrentTimestamp += LastFrameDelay;
		}

		return 1;
	}

	return 0; /* packet done */
}

void MovieTexture_FFMpeg::ConvertFrame()
{
	avcodec::AVPicture pict;
	pict.data[0] = (unsigned char *)m_img->pixels;
	pict.linesize[0] = m_img->pitch;

	avcodec::img_convert(&pict, AVPixelFormats[m_AVTexfmt].pf,
			(avcodec::AVPicture *) &decoder->frame, decoder->m_stream->codec.pix_fmt, 
			decoder->m_stream->codec.width, decoder->m_stream->codec.height);
}

static avcodec::AVStream *FindVideoStream( avcodec::AVFormatContext *m_fctx )
{
    for( int stream = 0; stream < m_fctx->nb_streams; ++stream )
	{
		avcodec::AVStream *enc = m_fctx->streams[stream];
        if( enc->codec.codec_type == avcodec::CODEC_TYPE_VIDEO )
			return enc;
	}
	return NULL;
}

MovieTexture_FFMpeg::MovieTexture_FFMpeg( RageTextureID ID ):
	RageMovieTexture( ID )
{
	LOG->Trace( "MovieTexture_FFMpeg::MovieTexture_FFMpeg(%s)", ID.filename.c_str() );

	FixLilEndian();

	decoder = new FFMpeg_Helper;

	m_uTexHandle = 0;
	m_bLoop = true;
    m_State = DECODER_QUIT; /* it's quit until we call StartThread */
	m_img = NULL;
	m_ImageWaiting = false;
	m_Rate = 1;
	m_bWantRewind = false;

	m_BufferFinished = SDL_CreateSemaphore(0);

	CreateDecoder();

	LOG->Trace("Resolution: %ix%i (%ix%i, %ix%i)",
			m_iSourceWidth, m_iSourceHeight,
			m_iImageWidth, m_iImageHeight, m_iTextureWidth, m_iTextureHeight);
	LOG->Trace("Bitrate: %i", decoder->m_stream->codec.bit_rate );
	LOG->Trace("Codec pixel format: %i", decoder->m_stream->codec.pix_fmt );

	CreateTexture();
	CreateFrameRects();

	/* Decode one frame, to guarantee that the texture is drawn when this function returns. */
	int ret = decoder->GetFrame();
	if( ret == -1 )
	{
		/* XXX */
		LOG->Trace( "%s: error getting first frame", GetID().filename.c_str() );
		return;
	}
	if( ret == 0 )
	{
		/* There's nothing there. XXX */
		LOG->Trace( "%s: EOF getting first frame", GetID().filename.c_str() );
		return;
	}

	ConvertFrame();
	UpdateFrame();

	CHECKPOINT;

	StartThread();
    Play();
}

static CString averr_ssprintf( int err, const char *fmt, ... )
{
	ASSERT( err < 0 );

	va_list     va;
	va_start(va, fmt);
	CString s = vssprintf( fmt, va );
	va_end(va); 

	CString Error;
	switch( err )
	{
	case AVERROR_IO:			Error = "I/O error"; break;
	case AVERROR_NUMEXPECTED:	Error = "number syntax expected in filename"; break;
	case AVERROR_INVALIDDATA:	Error = "invalid data found"; break;
	case AVERROR_NOMEM:			Error = "not enough memory"; break;
	case AVERROR_NOFMT:			Error = "unknown format"; break;
	case AVERROR_UNKNOWN:		Error = "unknown error"; break;
	default: Error = ssprintf( "unknown error %i", err ); break;
	}

	return s + " (" + Error + ")";
}


void MovieTexture_FFMpeg::CreateDecoder()
{
	avcodec::av_register_all();
	int ret = avcodec::av_open_input_file( &decoder->m_fctx, GetID().filename, NULL, 0, NULL );
	if( ret < 0 )
		RageException::Throw( averr_ssprintf(ret, "AVCodec: Couldn't open \"%s\"", GetID().filename.c_str()) );

	ret = avcodec::av_find_stream_info( decoder->m_fctx );
	if ( ret < 0 )
		RageException::Throw( averr_ssprintf(ret, "AVCodec (%s): Couldn't find codec parameters", GetID().filename.c_str()) );
	
	decoder->m_stream = FindVideoStream( decoder->m_fctx );
	if ( decoder->m_stream == NULL )
		RageException::Throw( "AVCodec (%s): Couldn't find any video streams", GetID().filename.c_str() );

	avcodec::AVCodec *codec = avcodec::avcodec_find_decoder( decoder->m_stream->codec.codec_id );
	if( codec == NULL )
		RageException::Throw( "AVCodec (%s): Couldn't find decoder %i", GetID().filename.c_str(), decoder->m_stream->codec.codec_id );

	LOG->Trace("Opening codec %s", codec->name );
	ret = avcodec::avcodec_open( &decoder->m_stream->codec, codec );
	if ( ret < 0 )
		RageException::Throw( averr_ssprintf(ret, "AVCodec (%s): Couldn't open codec \"%s\"", GetID().filename.c_str(), codec->name) );
}


/* Delete the decoder.  The decoding thread must be stopped. */
void MovieTexture_FFMpeg::DestroyDecoder()
{
	avcodec::avcodec_close( &decoder->m_stream->codec );
	avcodec::av_close_input_file( decoder->m_fctx );
	decoder->m_fctx = NULL;
	decoder->m_stream = NULL;
}

/* Delete the surface and texture.  The decoding thread must be stopped, and this
 * is normally done after destroying the decoder. */
void MovieTexture_FFMpeg::DestroyTexture()
{
	if( m_img )
	{
		SDL_FreeSurface( m_img );
		m_img=NULL;
	}
	if(m_uTexHandle)
	{
		DISPLAY->DeleteTexture( m_uTexHandle );
		m_uTexHandle = 0;
	}
}


void MovieTexture_FFMpeg::CreateTexture()
{
    if( m_uTexHandle )
        return;

	CHECKPOINT;

	RageTextureID actualID = GetID();
	actualID.iAlphaBits = 0;

	/* Cap the max texture size to the hardware max. */
	actualID.iMaxSize = min( actualID.iMaxSize, DISPLAY->GetMaxTextureSize() );

	m_iSourceWidth  = decoder->m_stream->codec.width;
	m_iSourceHeight = decoder->m_stream->codec.height;

	/* image size cannot exceed max size */
	m_iImageWidth = min( m_iSourceWidth, actualID.iMaxSize );
	m_iImageHeight = min( m_iSourceHeight, actualID.iMaxSize );

	/* Texture dimensions need to be a power of two; jump to the next. */
	m_iTextureWidth = power_of_two(m_iImageWidth);
	m_iTextureHeight = power_of_two(m_iImageHeight);

	/* TODO: We could get a big speed bonus by doing the above if the
	 * hardware renderer can handle YUV textures.  I think D3D can do
	 * this, as well as OpenGL on the Mac, but we don't have any infrastructure
	 * for this right now. */
    PixelFormat pixfmt;
	bool PreferHighColor = (TEXTUREMAN->GetMovieColorDepth() == 32);
	m_AVTexfmt = FindCompatibleAVFormat( pixfmt, PreferHighColor );

	if( m_AVTexfmt == -1 )
		m_AVTexfmt = FindCompatibleAVFormat( pixfmt, !PreferHighColor );

	if( m_AVTexfmt == -1 )
	{
		/* No dice.  Use the first avcodec format of the preferred bit depth,
		 * and let the display system convert. */
		for( m_AVTexfmt = 0; AVPixelFormats[m_AVTexfmt].bpp; ++m_AVTexfmt )
			if( AVPixelFormats[m_AVTexfmt].HighColor == PreferHighColor )
				break;
		ASSERT( AVPixelFormats[m_AVTexfmt].bpp );

		switch( TEXTUREMAN->GetMovieColorDepth() )
		{
		default:
			ASSERT(0);
		case 16:
			if( DISPLAY->SupportsTextureFormat(FMT_RGB5) )
				pixfmt = FMT_RGB5;
			else
				pixfmt = FMT_RGBA4; // everything supports RGBA4

			break;

		case 32:
			if( DISPLAY->SupportsTextureFormat(FMT_RGB8) )
				pixfmt = FMT_RGB8;
			else if( DISPLAY->SupportsTextureFormat(FMT_RGBA8) )
				pixfmt = FMT_RGBA8;
			else if( DISPLAY->SupportsTextureFormat(FMT_RGB5) )
				pixfmt = FMT_RGB5;
			else
				pixfmt = FMT_RGBA4; // everything supports RGBA4
			break;
		}
	}
	
	if( !m_img )
	{
		const AVPixelFormat_t *pfd = &AVPixelFormats[m_AVTexfmt];

		LOG->Trace("format %i, %08x %08x %08x %08x",
			pfd->bpp, pfd->masks[0], pfd->masks[1], pfd->masks[2], pfd->masks[3]);

		m_img = SDL_CreateRGBSurfaceSane(SDL_SWSURFACE, m_iTextureWidth, m_iTextureHeight,
			pfd->bpp, pfd->masks[0], pfd->masks[1], pfd->masks[2], pfd->masks[3]);
	}

    m_uTexHandle = DISPLAY->CreateTexture( pixfmt, m_img );
}



void MovieTexture_FFMpeg::DecoderThread()
{
	RageTimer Timer;
	float Clock = 0;
	bool FrameSkipMode = false;

	/* Movie decoding is bursty.  We burst decode a frame, then we sleep, then we burst
	 * to YUV->RGB convert, then we wait for the frame to move, and we repeat.  */
//	if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL))
//		LOG->Warn(werr_ssprintf(GetLastError(), "Failed to set sound thread priority"));

	/* Windows likes to boost priority when processes come out of a wait state.  We don't
	 * want that, since it'll result in us having a small priority boost after each movie
	 * frame, resulting in skips in the gameplay thread. */
//	if(!SetThreadPriorityBoost(GetCurrentThread(), TRUE))
//		LOG->Warn(werr_ssprintf(GetLastError(), "SetThreadPriorityBoost failed"));

	CHECKPOINT;

	while( m_State != DECODER_QUIT )
	{
		if( m_State == PAUSE_DECODER )
		{
			SDL_Delay( 5 );
			
			/* The video isn't running; skip time. */
			Timer.GetDeltaTime();
			continue;
		}

		CHECKPOINT;

		/* We're playing.  Update the clock. */
		Clock += Timer.GetDeltaTime() * m_Rate;
		
		/* Read a frame. */
		int ret = decoder->GetFrame();
		if( ret == -1 )
			return;

		if( m_bWantRewind && decoder->CurrentTimestamp == 0 )
			m_bWantRewind = false; /* ignore */

		if( ret == 0 )
		{
			/* EOF. */
			if( !m_bLoop )
				return;

			LOG->Trace( "File \"%s\" looping", GetID().filename.c_str() );
			m_bWantRewind = true;
		}

		if( m_bWantRewind )
		{
			m_bWantRewind = false;

			/* Restart. */
			DestroyDecoder();
			CreateDecoder();

			decoder->Init();
			Clock = 0;
			continue;
		}

		/* We got a frame. */
		const float Offset = decoder->CurrentTimestamp - Clock;
	
		/* If we're ahead, we're decoding too fast; delay. */
		if( Offset > 0 )
		{
			SDL_Delay( int(1000*Offset) );
			if( FrameSkipMode )
			{
				/* We're caught up; stop skipping frames. */
				LOG->Trace( "stopped skipping frames" );
				FrameSkipMode = false;
			}
		} else {
			/* We're behind by -Offset seconds.  
			 *
			 * If we're just slightly behind, don't worry about it; we'll simply
			 * not sleep, so we'll move as fast as we can to catch up.
			 *
			 * If we're far behind, we're short on CPU and we need to do something
			 * about it.  We have at least two options:
			 *
			 * 1: We can skip texture updates.  This is a big bottleneck on many
			 * systems.
			 *
			 * 2: If that's not enough, we can play with hurry_up.
			 *
			 * If we hit a threshold, start skipping frames via #1.  If we do that,
			 * don't stop once we hit the threshold; keep doing it until we're fully
			 * caught up.
			 *
			 * I'm not sure when we should do #2.  Also, we should try to notice if
			 * we simply don't have enough CPU for the video; it's better to just
			 * stay in frame skip mode than to enter and exit it constantly, but we
			 * don't want to do that due to a single timing glitch.
			 *
			 * XXX: is there a setting for hurry_up we can use when we're going to ignore
			 * a frame to make it take less time?
			 */
			const float FrameSkipThreshold = 0.5f;

			if( -Offset >= FrameSkipThreshold && !FrameSkipMode )
			{
				LOG->Trace( "(%s) Time is %f, and the movie is at %f.  Entering frame skip mode.",
					GetID().filename.c_str(), Clock, decoder->CurrentTimestamp);
				FrameSkipMode = true;
			}
		}

		if( FrameSkipMode && decoder->m_stream->codec.frame_number % 2 )
			continue; /* skip */
		
		/* Convert it. */
		ConvertFrame();

		/* Signal the main thread to update the image on the next Update. */
		m_ImageWaiting=true;

		CHECKPOINT;
		SDL_SemWait( m_BufferFinished );
		CHECKPOINT;

		/* If the frame wasn't used, then we must be shutting down. */
		ASSERT( !m_ImageWaiting || m_State == DECODER_QUIT );
	}
	CHECKPOINT;
}

MovieTexture_FFMpeg::~MovieTexture_FFMpeg()
{
	StopThread();
	DestroyDecoder();
	DestroyTexture();

	delete decoder;

	SDL_DestroySemaphore( m_BufferFinished );
}

void MovieTexture_FFMpeg::Update(float fDeltaTime)
{
	/* Note that if there's an image waiting, we *must* signal m_BufferFinished, or
	 * the decoder thread may sit around waiting for it, even though Pause and Play
	 * calls, causing the clock to keep running. */
	if( !m_ImageWaiting )
		return;

	CHECKPOINT;

	UpdateFrame();
	m_ImageWaiting = false;
	SDL_SemPost(m_BufferFinished);
}

void MovieTexture_FFMpeg::UpdateFrame()
{
    /* Just in case we were invalidated: */
    CreateTexture();

	CHECKPOINT;
	DISPLAY->UpdateTexture(
        m_uTexHandle,
        m_img,
        0, 0,
        m_iImageWidth, m_iImageHeight );
    CHECKPOINT;
}

void MovieTexture_FFMpeg::Reload()
{
}

void MovieTexture_FFMpeg::StartThread()
{
	ASSERT( m_State == DECODER_QUIT );
	m_State = PAUSE_DECODER;
	m_DecoderThread.SetName( ssprintf("MovieTexture_FFMpeg(%s)", GetID().filename.c_str()) );
	m_DecoderThread.Create( DecoderThread_start, this );
}

void MovieTexture_FFMpeg::StopThread()
{
	LOG->Trace("Shutting down decoder thread ...");

	if( !m_DecoderThread.IsCreated() )
		return;

	m_State = DECODER_QUIT;

	/* Make sure we don't deadlock waiting for m_BufferFinished. */
	SDL_SemPost(m_BufferFinished);
	CHECKPOINT;
	m_DecoderThread.Wait();
	CHECKPOINT;
	
	m_ImageWaiting = false;

	/* Clear the above post, if the thread didn't. */
	SDL_SemTryWait(m_BufferFinished);

	LOG->Trace("Decoder thread shut down.");
}

void MovieTexture_FFMpeg::Play()
{
    m_State = PLAYING;
}

void MovieTexture_FFMpeg::Pause()
{
    m_State = PAUSE_DECODER;
}

void MovieTexture_FFMpeg::SetPosition( float fSeconds )
{
    ASSERT( m_State != DECODER_QUIT );

	/* We can reset to 0, but I don't think this API supports fast seeking
	 * yet.  I don't think we ever actually seek except to 0 right now,
	 * anyway. XXX */
	if( fSeconds != 0 )
	{
		LOG->Warn( "MovieTexture_FFMpeg::SetPosition(%f): non-0 seeking unsupported; ignored", fSeconds );
		return;
	}

	LOG->Trace( "Seek to %f (from %f)", fSeconds );
	m_bWantRewind = true;
}

