#include "global.h"
#include "MovieTexture_FFMpeg.h"

#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "SDL_utils.h"
#include "SDL_endian.h"


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
	{ 0, { 0,0,0,0 }, avcodec::PIX_FMT_NB, true }
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

	m_uTexHandle = 0;
	m_bLoop = true;
    m_State = DECODER_QUIT; /* it's quit until we call StartThread */
	m_bLoop = true;
	m_img = NULL;
	m_ImageWaiting = false;
	m_Rate = 1;
	m_Position = 0;

	m_BufferFinished = SDL_CreateSemaphore(0);
	m_OneFrameDecoded = SDL_CreateSemaphore(0);

	CreateDecoder();

	LOG->Trace("Resolution: %ix%i (%ix%i, %ix%i)",
			m_iSourceWidth, m_iSourceHeight,
			m_iImageWidth, m_iImageHeight, m_iTextureWidth, m_iTextureHeight);
	LOG->Trace("Bitrate: %i", m_stream->codec.bit_rate );
	LOG->Trace("Codec pixel format: %i", m_stream->codec.pix_fmt );

	CreateTexture();
	StartThread();

	CreateFrameRects();

	/* Wait until we decode one frame, to guarantee that the texture is
	 * drawn when this function returns. */
    ASSERT( m_State == PAUSE_DECODER );
	CHECKPOINT;
    m_State = PLAYING_ONE;
	SDL_SemWait( m_OneFrameDecoded );
	CHECKPOINT;
	m_State = PAUSE_DECODER;
    CheckFrame();
	CHECKPOINT;

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
	RageTextureID actualID = GetID();

	actualID.iAlphaBits = 0;

	avcodec::av_register_all();
	int ret = avcodec::av_open_input_file( &m_fctx, actualID.filename, NULL, 0, NULL );
	if( ret < 0 )
		RageException::Throw( averr_ssprintf(ret, "AVCodec: Couldn't open \"%s\"", actualID.filename.c_str()) );

	ret = avcodec::av_find_stream_info( m_fctx );
	if ( ret < 0 )
		RageException::Throw( averr_ssprintf(ret, "AVCodec: Couldn't find codec parameters") );
	
	m_stream = FindVideoStream( m_fctx );
	if ( m_stream == NULL )
		RageException::Throw( averr_ssprintf(ret, "AVCodec: Couldn't find video stream") );

	avcodec::AVCodec *codec = avcodec::avcodec_find_decoder( m_stream->codec.codec_id );
	if( codec == NULL )
		RageException::Throw( averr_ssprintf(ret, "AVCodec: Couldn't find decoder") );

	LOG->Trace("Opening codec %s", codec->name );
	ret = avcodec::avcodec_open( &m_stream->codec, codec );
	if ( ret < 0 )
		RageException::Throw( averr_ssprintf(ret, "AVCodec: Couldn't open decoder") );

	/* Cap the max texture size to the hardware max. */
	actualID.iMaxSize = min( actualID.iMaxSize, DISPLAY->GetMaxTextureSize() );

	m_iSourceWidth  = m_stream->codec.width;
	m_iSourceHeight = m_stream->codec.height;

	/* image size cannot exceed max size */
	m_iImageWidth = min( m_iSourceWidth, actualID.iMaxSize );
	m_iImageHeight = min( m_iSourceHeight, actualID.iMaxSize );

	/* Texture dimensions need to be a power of two; jump to the next. */
	m_iTextureWidth = power_of_two(m_iImageWidth);
	m_iTextureHeight = power_of_two(m_iImageHeight);
}


/* Delete the decoder.  The decoding thread must be stopped. */
void MovieTexture_FFMpeg::DestroyDecoder()
{
	avcodec::avcodec_close( &m_stream->codec );
	avcodec::av_close_input_file( m_fctx );
	m_fctx = NULL;
	m_stream = NULL;
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

	/* If the movie is coming in RGB, we can potentially save a lot
	 * of time by simply sending frame updates to RageDisplay in the
	 * format we're receiving data.  However, I think just about all
	 * formats output data in some form of YUV, so I havn't bothered
	 * with this yet.
	 *
	 * TODO: We could get a big speed bonus by doing the above if the
	 * hardware renderer can handle YUV textures.  I think D3D can do
	 * this, as well as OpenGL on the Mac, but we don't have any infrastructure
	 * for this right now. 
	 *
	 * A hint: http://oss.sgi.com/projects/performer/mail/info-performer/perf-01-06/0017.html */
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
	bool GetNextTimestamp = true;
	float CurrentTimestamp = 0, Last_IP_Timestamp = 0;

	bool FrameSkipMode = false;
	float LastFrameDelay = 0;
	float Clock = 0;
	RageTimer Timer;

	bool FirstFrame = true;
	float pts = 0, last_IP_pts = 0;
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
		
		/* Read a packet. */
		avcodec::AVPacket pkt;
		while( 1 )
		{
			CHECKPOINT;
			int ret = avcodec::av_read_packet(m_fctx, &pkt);
			if( ret == -1 )
			{
				/* EOF. */
				if( !m_bLoop )
					return;

				LOG->Trace( "File \"%s\" looping", GetID().filename.c_str() );

				if( FirstFrame )
				{
					/* It's the first frame, and we're at EOF, which menas there aren't
					 * any frames.  Kick Create(), if needed, and abort. */
					if( m_State == PLAYING_ONE )
						SDL_SemPost( m_OneFrameDecoded );
					return;
				}

				/* Restart. */
				DestroyDecoder();
				CreateDecoder();

				CurrentTimestamp = Last_IP_Timestamp = 0;
				GetNextTimestamp = true;
				FrameSkipMode = false;
				LastFrameDelay = 0;
				Clock = 0;
				FirstFrame = true;
				continue;
			}

			if( ret < 0 )
			{
				/* XXX ? */
				LOG->Warn("AVCodec: Error decoding %s: %i",
						GetID().filename.c_str(), ret );
				return;
			}
			
			if( pkt.stream_index != m_stream->index )
			{
				/* It's not for the video stream; ignore it. */
				avcodec::av_free_packet( &pkt );
				continue;
			}

			break;
		}

		/* Decode the packet. */
		int current_packet_offset = 0;
		while( m_State != DECODER_QUIT && current_packet_offset < pkt.size )
		{
			if ( GetNextTimestamp )
			{
				if (pkt.pts != AV_NOPTS_VALUE)
					pts = (float)pkt.pts * m_fctx->pts_num / m_fctx->pts_den;
				else
					pts = -1;
				GetNextTimestamp = false;
			}

			avcodec::AVFrame frame;
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
				return; // XXX
			}

			current_packet_offset += len;

			if (!got_frame)
				continue;

			FirstFrame=false;

			/* Length of this frame: */
			LastFrameDelay = (float)m_stream->codec.frame_rate_base / m_stream->codec.frame_rate;
			LastFrameDelay += frame.repeat_pict * (LastFrameDelay * 0.5f);

			/* We got a frame.  Convert it. */
			avcodec::AVPicture pict;
			pict.data[0] = (unsigned char *)m_img->pixels;
			pict.linesize[0] = m_img->pitch;

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

			m_Position = CurrentTimestamp;

			const float Offset = CurrentTimestamp - Clock;
			bool SkipThisTextureUpdate = false;
		
			/* If we're ahead, we're decoding too fast; delay. */
			if( Offset > 0 )
			{
				SDL_Delay( int(1000*(CurrentTimestamp - Clock)) );
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
						GetID().filename.c_str(), CurrentTimestamp, Clock );
					FrameSkipMode = true;
				}
			}

			if( FrameSkipMode )
				if( m_stream->codec.frame_number % 2 )
					SkipThisTextureUpdate = true;
			
			if( m_State == PLAYING_ONE )
				SkipThisTextureUpdate = false;

			if( !SkipThisTextureUpdate )
			{
				avcodec::img_convert(&pict, AVPixelFormats[m_AVTexfmt].pf,
						(avcodec::AVPicture *) &frame, m_stream->codec.pix_fmt, 
						m_stream->codec.width, m_stream->codec.height);

				/* Signal the main thread to update the image on the next Update. */
				m_ImageWaiting=true;

				if( m_State == PLAYING_ONE )
					SDL_SemPost( m_OneFrameDecoded );

				CHECKPOINT;
				SDL_SemWait( m_BufferFinished );
				CHECKPOINT;

				/* If the frame wasn't used, then we must be shutting down. */
				ASSERT( !m_ImageWaiting || m_State == DECODER_QUIT );
			}

			GetNextTimestamp = true;
		}
		avcodec::av_free_packet( &pkt );
	}
	CHECKPOINT;
}

MovieTexture_FFMpeg::~MovieTexture_FFMpeg()
{
	StopThread();
	DestroyDecoder();
	DestroyTexture();

	SDL_DestroySemaphore( m_BufferFinished );
	SDL_DestroySemaphore( m_OneFrameDecoded );
}

void MovieTexture_FFMpeg::Update(float fDeltaTime)
{
	CHECKPOINT;

	if( m_State == PLAYING )
		CheckFrame();
}

void MovieTexture_FFMpeg::CheckFrame()
{
	if( !m_ImageWaiting )
		return;

    /* Just in case we were invalidated: */
    CreateTexture();

	CHECKPOINT;
	DISPLAY->UpdateTexture(
        m_uTexHandle,
        m_img,
        0, 0,
        m_iImageWidth, m_iImageHeight );
    CHECKPOINT;

	m_ImageWaiting = false;
	SDL_SemPost(m_BufferFinished);
}

void MovieTexture_FFMpeg::Reload()
{
}

void MovieTexture_FFMpeg::StartThread()
{
	/* If we had a frame waiting from a previous thread start, clear it. */
	m_ImageWaiting = false;

	ASSERT( m_State == DECODER_QUIT );
	m_State = PAUSE_DECODER;
	m_DecoderThread.SetName( ssprintf("MovieTexture_FFMpeg(%s)", GetID().filename.c_str()) );
	m_DecoderThread.Create( DecoderThread_start, this );
}

void MovieTexture_FFMpeg::StopThread()
{
	LOG->Trace("Shutting down decoder thread ...");

	if( m_State == DECODER_QUIT )
		return;

	m_State = DECODER_QUIT;

	/* Make sure we don't deadlock waiting for m_BufferFinished. */
	SDL_SemPost(m_BufferFinished);

	CHECKPOINT;
	m_DecoderThread.Wait();
	CHECKPOINT;
	
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
	if( fSeconds == m_Position )
		return;

	if( fSeconds != 0 )
	{
		LOG->Warn( "MovieTexture_FFMpeg::SetPosition(%f): non-0 seeking unsupported; ignored", fSeconds );
		return;
	}

	LOG->Trace( "Seek to %f (from %f)", fSeconds, m_Position );
    State OldState = m_State;

	/* Recreate the decoder.  Do this without touching the actual texture, and without waiting
	 * for the first frame to be decoded, so we don't take too long. */
	StopThread();
	DestroyDecoder();
	CreateDecoder();
	StartThread();

	m_State = OldState;
}

