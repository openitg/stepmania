#include "global.h"
#include "LowLevelWindow_SDL.h"
#include "SDL_utils.h"
#include "RageLog.h"
#include "RageDisplay.h" // for REFRESH_DEFAULT

#include "SDL_utils.h"

LowLevelWindow_SDL::LowLevelWindow_SDL()
{
	/* By default, ignore all SDL events.  We'll enable them as we need them.
	 * We must not enable any events we don't actually want, since we won't
	 * query for them and they'll fill up the event queue. 
	 *
	 * This needs to be done after we initialize video, since it's really part
	 * of the SDL video system--it'll be reinitialized on us if we do this first. */
	SDL_EventState(0xFF /*SDL_ALLEVENTS*/, SDL_IGNORE);

	SDL_EventState(SDL_VIDEORESIZE, SDL_ENABLE);
	SDL_EventState(SDL_ACTIVEEVENT, SDL_ENABLE);

	Windowed = false;
}

LowLevelWindow_SDL::~LowLevelWindow_SDL()
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	mySDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
	mySDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
}

void *LowLevelWindow_SDL::GetProcAddress(CString s)
{
	return SDL_GL_GetProcAddress(s);
}

bool LowLevelWindow_SDL::SetVideoMode( bool windowed, int width, int height, int bpp, int rate, bool vsync, CString sWindowTitle, CString sIconFile )
{
	/* We need to preserve the event mask and all events, since they're lost by
	 * SDL_QuitSubSystem(SDL_INIT_VIDEO). */
	vector<SDL_Event> events;
	mySDL_GetAllEvents(events);
	Uint8 EventEnabled[SDL_NUMEVENTS];

	/* Queue up key-up events for all keys that are currently down (eg. alt-enter).
	 * This is normally done by SDL, but since we're shutting down the video system
	 * we're also shutting down the event system. */
	{
		const Uint8 *KeyState = SDL_GetKeyState(NULL);
		for ( SDLKey key=SDLK_FIRST; key<SDLK_LAST; key = (SDLKey)(key+1) )
		{
			if ( KeyState[key] != SDL_PRESSED )
				continue;
			SDL_Event e;
			memset(&e, 0, sizeof(e));
			e.key.type = SDL_KEYUP;
			e.key.keysym.sym = key;
			events.push_back(e);
			LOG->Trace("up %i", key);
		}
	}

	int i;
	for( i = 0; i < SDL_NUMEVENTS; ++i)
		EventEnabled[i] = mySDL_EventState( (Uint8) i, SDL_QUERY );

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_InitSubSystem(SDL_INIT_VIDEO);

	/* Put them back. */
	for( int i = 0; i < SDL_NUMEVENTS; ++i)
		mySDL_EventState((Uint8) i, EventEnabled[i]);
	mySDL_PushEvents(events);

	/* Set SDL window title and icon -before- creating the window */
	SDL_WM_SetCaption(sWindowTitle, "");
	mySDL_WM_SetIcon( sIconFile );


	Windowed = false;

	int flags = SDL_RESIZABLE | SDL_OPENGL; // | SDL_DOUBLEBUF; // no need for DirectDraw to be double-buffered
	if( !windowed )
		flags |= SDL_FULLSCREEN;

	Windowed = windowed;
	SDL_ShowCursor( Windowed );

	ASSERT( bpp == 16 || bpp == 32 );
	switch( bpp )
	{
	case 16:
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		break;
	case 32:
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	}

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

#ifdef SDL_HAS_REFRESH_RATE
	if(rate == REFRESH_DEFAULT)
		SDL_SM_SetRefreshRate(0);
	else
		SDL_SM_SetRefreshRate(rate);
#endif

#if defined(WIN32)
//	mySDL_EventState(SDL_OPENGLRESET, SDL_ENABLE);
#endif

	SDL_Surface *screen = SDL_SetVideoMode(width, height, bpp, flags);
	if(!screen)
		RageException::Throw("SDL_SetVideoMode failed: %s", SDL_GetError());

	bool NewOpenGLContext = true;	// always a new context because we're resetting SDL_Video

	/* XXX: This event only exists in the SDL tree, and is only needed in
	 * Windows.  Eventually, it'll probably get upstreamed, and once it's
	 * in the real branch we can remove this #if. */
#if defined(WIN32)
//	SDL_Event e;
//	if(SDL_GetEvent(e, SDL_OPENGLRESETMASK))
//	{
//		LOG->Trace("New OpenGL context");
//		SDL_WM_SetCaption("StepMania", "StepMania");
//		NewOpenGLContext = true;
//	}

	SDL_WM_SetCaption("StepMania", "StepMania");
	
//	mySDL_EventState(SDL_OPENGLRESET, SDL_IGNORE);
#endif

	{
		/* Find out what we really got. */
		int r,g,b,a, colorbits, depth, stencil;
		
		SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
		SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
		SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
		SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
		SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &colorbits);
		SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
		SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil);
		LOG->Info("Got %i bpp (%i%i%i%i), %i depth, %i stencil",
			colorbits, r, g, b, a, depth, stencil);
	}

	CurrentWidth = screen->w;
	CurrentHeight = screen->h;
	CurrentBPP = bpp;

	return NewOpenGLContext;
}

void LowLevelWindow_SDL::SwapBuffers()
{
	SDL_GL_SwapBuffers();
}

void LowLevelWindow_SDL::Update(float fDeltaTime)
{
	SDL_Event event;
	while(SDL_GetEvent(event, SDL_VIDEORESIZEMASK|SDL_ACTIVEEVENTMASK))
	{
		switch(event.type)
		{
		case SDL_VIDEORESIZE:
			CurrentWidth = event.resize.w;
			CurrentHeight = event.resize.h;

			/* Let DISPLAY know that our resolution has changed. */
			DISPLAY->ResolutionChanged();
			break;
		case SDL_ACTIVEEVENT:
			if( event.active.gain  &&		// app regaining focus
				!DISPLAY->IsWindowed() )	// full screen
			{
				// need to reacquire an OGL context
				DISPLAY->SetVideoMode( 
					DISPLAY->IsWindowed(), 
					DISPLAY->GetWidth(), 
					DISPLAY->GetHeight(), 
					DISPLAY->GetBPP(), 
					0, 
					0,
					"",
					"" );	// FIXME: preserve prefs
			}
			break;
		}
	}
}

