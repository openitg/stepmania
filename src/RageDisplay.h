/* RageDisplay - Renderer base class. */

#ifndef RAGEDISPLAY_H
#define RAGEDISPLAY_H

#include "RageTypes.h"
#include "ModelTypes.h"
#include <set>

class DisplayResolution;
typedef set<DisplayResolution> DisplayResolutions;

const int REFRESH_DEFAULT = 0;
struct RageSurface;
enum TextureUnit 
{
	TextureUnit_1,
	TextureUnit_2,
	TextureUnit_3,
	TextureUnit_4,
	NUM_TextureUnit
};

// RageCompiledGeometry holds vertex data in a format that is most efficient 
// for the graphics API.
class RageCompiledGeometry
{
public:
	virtual ~RageCompiledGeometry();

	void Set( const vector<msMesh> &vMeshes, bool bNeedsNormals );

	virtual void Allocate( const vector<msMesh> &vMeshes ) = 0;	// allocate space
	virtual void Change( const vector<msMesh> &vMeshes ) = 0;	// new data must be the same size as was passed to Set()
	virtual void Draw( int iMeshIndex ) const = 0;
	
protected:
	size_t GetTotalVertices() const { if( m_vMeshInfo.empty() ) return 0; return m_vMeshInfo.back().iVertexStart + m_vMeshInfo.back().iVertexCount; }
	size_t GetTotalTriangles() const { if( m_vMeshInfo.empty() ) return 0; return m_vMeshInfo.back().iTriangleStart + m_vMeshInfo.back().iTriangleCount; }
	
	struct MeshInfo
	{
		int iVertexStart;
		int iVertexCount;
		int iTriangleStart;
		int iTriangleCount;
		bool m_bNeedsTextureMatrixScale;
	};
	vector<MeshInfo>	m_vMeshInfo;
	bool m_bNeedsNormals;
	bool m_bAnyNeedsTextureMatrixScale;
};

enum PixelFormat
{
	PixelFormat_RGBA8,
	PixelFormat_RGBA4,
	PixelFormat_RGB5A1,
	PixelFormat_RGB5,
	PixelFormat_RGB8,
	PixelFormat_PAL,
	/* The above formats differ between OpenGL and D3D. These are provided as
	* alternatives for OpenGL that match some format in D3D.  Don't use them
	* directly; they'll be matched automatically by FindPixelFormat. */
	PixelFormat_BGR8,
	PixelFormat_A1BGR5,
	NUM_PixelFormat,
	PixelFormat_Invalid
};
const RString& PixelFormatToString( PixelFormat i );

class VideoModeParams
{
public:
	// Initialize with a constructor so to guarantee all paramters
	// are filled (in case new params are added).
	VideoModeParams( 
		bool windowed_,
		int width_,
		int height_,
		int bpp_,
		int rate_,
		bool vsync_,
		bool interlaced_,
		bool bSmoothLines_,
		bool bTrilinearFiltering_,
		bool bAnisotropicFiltering_,
		RString sWindowTitle_,
		RString sIconFile_,
		bool PAL_,
		float fDisplayAspectRatio_
	)
	{
		windowed = windowed_;
		width = width_;
		height = height_;
		bpp = bpp_;
		rate = rate_;
		vsync = vsync_;
		interlaced = interlaced_;
		bSmoothLines = bSmoothLines_;
		bTrilinearFiltering = bTrilinearFiltering_;
		bAnisotropicFiltering = bAnisotropicFiltering_;
		sWindowTitle = sWindowTitle_;
		sIconFile = sIconFile_;
		PAL = PAL_;
		fDisplayAspectRatio = fDisplayAspectRatio_;
	}
	VideoModeParams() {}

	bool windowed;
	int width;
	int height;
	int bpp;
	int rate;
	bool vsync;
	bool bSmoothLines;
	bool bTrilinearFiltering;
	bool bAnisotropicFiltering;
	bool interlaced;
	bool PAL;
	float fDisplayAspectRatio;
	RString sWindowTitle;
	RString sIconFile;
};

struct RenderTargetParam
{
	RenderTargetParam():
		iWidth(0),
		iHeight(0),
		bWithDepthBuffer(false),
		bWithAlpha(false)
	{
	}

	/* The dimensions of the actual render target, analogous to a window size: */
	int iWidth, iHeight;

	bool bWithDepthBuffer;
	bool bWithAlpha;
};

class RageDisplay
{
	friend class RageTexture;

public:

	struct PixelFormatDesc {
		int bpp;
		unsigned int masks[4];
	};

	virtual const PixelFormatDesc *GetPixelFormatDesc( PixelFormat pf ) const = 0;

	RageDisplay();
	virtual ~RageDisplay() { }

	virtual RString Init( const VideoModeParams &p, bool bAllowUnacceleratedRenderer ) = 0;

	virtual RString GetApiDescription() const = 0;
	virtual void GetDisplayResolutions( DisplayResolutions &out ) const = 0;
	virtual float GetMonitorAspectRatio() const = 0;

	// Don't override this.  Override TryVideoMode() instead.
	// This will set the video mode to be as close as possible to params.
	// Return true if device was re-created and we need to reload textures.
	RString SetVideoMode( VideoModeParams p, bool &bNeedReloadTextures );

	/* Call this when the resolution has been changed externally: */
	virtual void ResolutionChanged();

	virtual bool BeginFrame();
	virtual void EndFrame();
	virtual VideoModeParams GetActualVideoModeParams() const = 0;
	bool IsWindowed() const { return this->GetActualVideoModeParams().windowed; }
	
	virtual void SetBlendMode( BlendMode mode ) = 0;

	virtual bool SupportsTextureFormat( PixelFormat pixfmt, bool realtime=false ) = 0;
	virtual bool SupportsThreadedRendering() { return false; }
	virtual bool SupportsPerVertexMatrixScale() = 0;

	// If threaded rendering is supported, these will be called from the rendering
	// thread before and after rendering.
	virtual void BeginConcurrentRenderingMainThread() { }
	virtual void EndConcurrentRenderingMainThread() { }
	virtual void BeginConcurrentRendering();
	virtual void EndConcurrentRendering() { }

	/* return 0 if failed or internal texture resource handle 
	 * (unsigned in OpenGL, texture pointer in D3D) */
	virtual unsigned CreateTexture( 
		PixelFormat pixfmt,			// format of img and of texture in video mem
		RageSurface* img, 		// must be in pixfmt
		bool bGenerateMipMaps
		) = 0;
	/* Return a texture handle to the default texture, an opaque, all-white,
	 * immutable texture.  Deleting or updating this texture has no effect. */
	virtual unsigned CreateTextureDefault() = 0;
	virtual void UpdateTexture( 
		unsigned iTexHandle, 
		RageSurface* img,
		int xoffset, int yoffset, int width, int height 
		) = 0;
	virtual void DeleteTexture( unsigned iTexHandle ) = 0;
	virtual void ClearAllTextures() = 0;
	virtual int GetNumTextureUnits() = 0;
	virtual void SetTexture( TextureUnit tu, unsigned iTexture ) = 0;
	virtual void SetTextureMode( TextureUnit tu, TextureMode tm ) = 0;
	virtual void SetTextureWrapping( TextureUnit tu, bool b ) = 0;
	virtual int GetMaxTextureSize() const = 0;
	virtual void SetTextureFiltering( TextureUnit tu, bool b ) = 0;
	virtual void SetEffectMode( EffectMode effect ) { }
	virtual bool IsEffectModeSupported( EffectMode effect ) { return effect == EffectMode_Normal; }

	bool SupportsRenderToTexture() const { return false; }
	
	/*
	 * Create a render target, returning a texture handle.  In addition to normal
	 * texture functions, this can be passed to SetRenderTarget.  Delete with
	 * DeleteTexture.  (UpdateTexture is not permitted.)  Returns 0 if render-to-
	 * texture is unsupported.
	 */
	virtual unsigned CreateRenderTarget( const RenderTargetParam &param, int &iTextureWidthOut, int &iTextureHeightOut ) { return 0; }

	/*
	 * Set the render target, or 0 to resume rendering to the framebuffer.  An active render
	 * target may not be used as a texture.  If bPreserveTexture is true, the contents
	 * of the texture will be preserved from the previous call; otherwise, cleared.  If
	 * bPreserveTexture is true the first time a render target is used, behave as if
	 * bPreserveTexture was false.
	 */
	virtual void SetRenderTarget( unsigned iHandle, bool bPreserveTexture = true ) { }
			
	virtual bool IsZTestEnabled() const = 0;
	virtual bool IsZWriteEnabled() const = 0;
	virtual void SetZWrite( bool b ) = 0;
	virtual void SetZTestMode( ZTestMode mode ) = 0;
	virtual void SetZBias( float f ) = 0;
	virtual void ClearZBuffer() = 0;

	virtual void SetCullMode( CullMode mode ) = 0;
	
	virtual void SetAlphaTest( bool b ) = 0;
	
	virtual void SetMaterial( 
		const RageColor &emissive,
		const RageColor &ambient,
		const RageColor &diffuse,
		const RageColor &specular,
		float shininess
		) = 0;

	virtual void SetLighting( bool b ) = 0;
	virtual void SetLightOff( int index ) = 0;
	virtual void SetLightDirectional( 
		int index, 
		const RageColor &ambient, 
		const RageColor &diffuse, 
		const RageColor &specular, 
		const RageVector3 &dir ) = 0;

	virtual void SetSphereEnvironmentMapping( TextureUnit tu, bool b ) = 0;

	virtual RageCompiledGeometry* CreateCompiledGeometry() = 0;
	virtual void DeleteCompiledGeometry( RageCompiledGeometry* p ) = 0;

	void DrawQuads( const RageSpriteVertex v[], int iNumVerts );
	void DrawQuadStrip( const RageSpriteVertex v[], int iNumVerts );
	void DrawFan( const RageSpriteVertex v[], int iNumVerts );
	void DrawStrip( const RageSpriteVertex v[], int iNumVerts );
	void DrawTriangles( const RageSpriteVertex v[], int iNumVerts );
	void DrawCompiledGeometry( const RageCompiledGeometry *p, int iMeshIndex, const vector<msMesh> &vMeshes );
	void DrawLineStrip( const RageSpriteVertex v[], int iNumVerts, float LineWidth );
	void DrawSymmetricQuadStrip( const RageSpriteVertex v[], int iNumVerts );
	void DrawCircle( const RageSpriteVertex &v, float radius );

	void DrawQuad( const RageSpriteVertex v[] ) { DrawQuads(v,4); } /* alias. upper-left, upper-right, lower-left, lower-right */

	// hacks for cell-shaded models
	virtual void SetPolygonMode( PolygonMode pm ) {}
	virtual void SetLineWidth( float fWidth ) {}

	enum GraphicsFileFormat
	{
		SAVE_LOSSLESS, // bmp
		SAVE_LOSSY_LOW_QUAL, // jpg
		SAVE_LOSSY_HIGH_QUAL // jpg
	};
	bool SaveScreenshot( RString sPath, GraphicsFileFormat format );

	virtual RString GetTextureDiagnostics( unsigned id ) const { return RString(); }
	virtual RageSurface* CreateScreenshot() = 0;	// allocates a surface.  Caller must delete it.

protected:
	virtual void DrawQuadsInternal( const RageSpriteVertex v[], int iNumVerts ) = 0;
	virtual void DrawQuadStripInternal( const RageSpriteVertex v[], int iNumVerts ) = 0;
	virtual void DrawFanInternal( const RageSpriteVertex v[], int iNumVerts ) = 0;
	virtual void DrawStripInternal( const RageSpriteVertex v[], int iNumVerts ) = 0;
	virtual void DrawTrianglesInternal( const RageSpriteVertex v[], int iNumVerts ) = 0;
	virtual void DrawCompiledGeometryInternal( const RageCompiledGeometry *p, int iMeshIndex ) = 0;
	virtual void DrawLineStripInternal( const RageSpriteVertex v[], int iNumVerts, float LineWidth );
	virtual void DrawSymmetricQuadStripInternal( const RageSpriteVertex v[], int iNumVerts ) = 0;
	virtual void DrawCircleInternal( const RageSpriteVertex &v, float radius );

	// return RString() if mode change was successful, an error message otherwise.
	// bNewDeviceOut is set true if a new device was created and textures
	// need to be reloaded.
	virtual RString TryVideoMode( const VideoModeParams &p, bool &bNewDeviceOut ) = 0;

	void DrawPolyLine( const RageSpriteVertex &p1, const RageSpriteVertex &p2, float LineWidth );

	// Stuff in RageDisplay.cpp
	void SetDefaultRenderStates();

public:
	/* Statistics */
	int GetFPS() const;
	int GetVPF() const;
	int GetCumFPS() const; /* average FPS since last reset */
	virtual void ResetStats();
	virtual void ProcessStatsOnFlip();
	virtual RString GetStats() const;
	void StatsAddVerts( int iNumVertsRendered );

	/* World matrix stack functions. */
	void PushMatrix();
	void PopMatrix();
	void Translate( float x, float y, float z );
	void TranslateWorld( float x, float y, float z );
	void Scale( float x, float y, float z );
	void RotateX( float deg );
	void RotateY( float deg );
	void RotateZ( float deg );
	void SkewX( float fAmount );
	void MultMatrix( const RageMatrix &f ) { this->PostMultMatrix(f); } /* alias */
	void PostMultMatrix( const RageMatrix &f );
	void PreMultMatrix( const RageMatrix &f );
	void LoadIdentity();

	/* Texture matrix functions */
	void TexturePushMatrix();
	void TexturePopMatrix();
	void TextureTranslate( float x, float y );
	void TextureTranslate( const RageVector2 &v ) { this->TextureTranslate( v.x, v.y ); }	

	/* Projection and View matrix stack functions. */
	void CameraPushMatrix();
	void CameraPopMatrix();
	void LoadMenuPerspective( float fFOVDegrees, float fWidth, float fHeight, float fVanishPointX, float fVanishPointY );
	void LoadLookAt( float fov, const RageVector3 &Eye, const RageVector3 &At, const RageVector3 &Up );

	/* Centering matrix */
	void CenteringPushMatrix();
	void CenteringPopMatrix();
	void ChangeCentering( int trans_x, int trans_y, int add_width, int add_height );

	RageSurface *CreateSurfaceFromPixfmt( PixelFormat pixfmt, void *pixels, int width, int height, int pitch );
	PixelFormat FindPixelFormat( int bpp, int Rmask, int Gmask, int Bmask, int Amask, bool realtime=false );

protected:
	RageMatrix GetPerspectiveMatrix( float fovy, float aspect, float zNear, float zFar );

	// Different for D3D and OpenGL.  Not sure why they're not compatible. -Chris
	virtual RageMatrix GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf ); 
	virtual RageMatrix GetFrustumMatrix( float l, float r, float b, float t, float zn, float zf ); 

	//
	// Matrix that adjusts position and scale of image on the screen
	//
	RageMatrix GetCenteringMatrix( float fTranslateX, float fTranslateY, float fAddWidth, float fAddHeight ) const;
	void UpdateCentering();

	// Called by the RageDisplay derivitives
	const RageMatrix* GetCentering() const;
	const RageMatrix* GetProjectionTop() const;
	const RageMatrix* GetViewTop() const;
	const RageMatrix* GetWorldTop() const;
	const RageMatrix* GetTextureTop() const;

	//
	// To limit the framerate, call FrameLimitBeforeVsync before waiting
	// for vsync and FrameLimitAfterVsync after.
	//
	void FrameLimitBeforeVsync( int iFPS );
	void FrameLimitAfterVsync();
};


extern RageDisplay*		DISPLAY;	// global and accessable from anywhere in our program

#endif
/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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
