#ifndef MOVIE_TEXTURE_H
#define MOVIE_TEXTURE_H

#include "RageTexture.h"

class RageMovieTexture : public RageTexture
{
public:
	RageMovieTexture( RageTextureID ID ): RageTexture(ID) { }
	virtual ~RageMovieTexture() { }
	virtual void Update(float fDeltaTime) { }

	virtual void Reload() = 0;

	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void SetPosition( float fSeconds ) = 0;
	virtual void SetPlaybackRate( float fRate ) = 0;
	virtual bool IsPlaying() const = 0;
	virtual void SetLooping(bool looping=true) { }

	bool IsAMovie() const { return true; }

	static bool GetFourCC( CString fn, CString &handler, CString &type );
};

RageMovieTexture *MakeRageMovieTexture(RageTextureID ID);

#endif
