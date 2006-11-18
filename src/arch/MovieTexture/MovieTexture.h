#ifndef MOVIE_TEXTURE_H
#define MOVIE_TEXTURE_H

#include "RageTexture.h"
#include <map>

class RageMovieTexture : public RageTexture
{
public:
	RageMovieTexture( RageTextureID ID ): RageTexture(ID) { }
	virtual ~RageMovieTexture() { }
	virtual RString Init() { return RString(); }
	virtual void Update( float fDeltaTime ) { }

	virtual void Reload() = 0;

	virtual void SetPosition( float fSeconds ) = 0;
	virtual void SetPlaybackRate( float fRate ) = 0;
	virtual void SetLooping( bool looping=true ) { }

	bool IsAMovie() const { return true; }

	static bool GetFourCC( RString fn, RString &handler, RString &type );
};

typedef RageMovieTexture *(*CreateMovieTextureFn)( RageTextureID );
struct RegisterMovieTexture
{
	static map<istring, CreateMovieTextureFn> *g_pRegistrees;
	RegisterMovieTexture( const istring &sName, CreateMovieTextureFn );
};
#define REGISTER_MOVIE_TEXTURE_CLASS( name ) \
	static RageMovieTexture *Create##name( RageTextureID ID ) { return new MovieTexture_##name( ID ); } \
	static RegisterMovieTexture register_##className( #name, Create##name )

#endif

/*
 * (c) 2003-2004 Glenn Maynard
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
