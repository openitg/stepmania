/* RageDisplay_OGL: OpenGL renderer. */

#ifndef RAGE_DISPLAY_OGL_H
#define RAGE_DISPLAY_OGL_H

#include "RageDisplay.h"

class RageDisplay_OGL: public RageDisplay
{
public:
	RageDisplay_OGL();
	virtual ~RageDisplay_OGL();
	virtual RString Init( const VideoModeParams &p, bool bAllowUnacceleratedRenderer );

	virtual RString GetApiDescription() const { return "OpenGL"; }
	virtual void GetDisplayResolutions( DisplayResolutions &out ) const;
	virtual float GetMonitorAspectRatio() const;
	void ResolutionChanged();
	const PixelFormatDesc *GetPixelFormatDesc(PixelFormat pf) const;

	bool SupportsThreadedRendering();
	void BeginConcurrentRenderingMainThread();
	void EndConcurrentRenderingMainThread();
	void BeginConcurrentRendering();
	void EndConcurrentRendering();

	bool BeginFrame();	
	void EndFrame();
	VideoModeParams GetActualVideoModeParams() const;
	void SetBlendMode( BlendMode mode );
	bool SupportsTextureFormat( PixelFormat pixfmt, bool realtime=false );
	bool SupportsPerVertexMatrixScale();
	unsigned CreateTexture( 
		PixelFormat pixfmt, 
		RageSurface* img,
		bool bGenerateMipMaps );
	void UpdateTexture( 
		unsigned iTexHandle, 
		RageSurface* img,
		int xoffset, int yoffset, int width, int height 
		);
	void DeleteTexture( unsigned iTexHandle );
	void ClearAllTextures();
	int GetNumTextureUnits();
	void SetTexture( TextureUnit tu, unsigned iTexture );
	void SetTextureMode( TextureUnit tu, TextureMode tm );
	void SetTextureWrapping( TextureUnit tu, bool b );
	int GetMaxTextureSize() const;
	void SetTextureFiltering( TextureUnit tu, bool b );
	bool SupportsRenderToTexture() const;
	unsigned CreateRenderTarget( const RenderTargetParam &param, int &iTextureWidthOut, int &iTextureHeightOut );
	void SetRenderTarget( unsigned iHandle, bool bPreserveTexture );
	bool IsZWriteEnabled() const;
	bool IsZTestEnabled() const;
	void SetZWrite( bool b );
	void SetZBias( float f );
	void SetZTestMode( ZTestMode mode );
	void ClearZBuffer();
	void SetCullMode( CullMode mode );
	void SetAlphaTest( bool b );
	void SetMaterial( 
		const RageColor &emissive,
		const RageColor &ambient,
		const RageColor &diffuse,
		const RageColor &specular,
		float shininess
		);
	void SetLighting( bool b );
	void SetLightOff( int index );
	void SetLightDirectional( 
		int index, 
		const RageColor &ambient, 
		const RageColor &diffuse, 
		const RageColor &specular, 
		const RageVector3 &dir );

	void SetSphereEnvironmentMapping( bool b );

	RageCompiledGeometry* CreateCompiledGeometry();
	void DeleteCompiledGeometry( RageCompiledGeometry* p );

	// hacks for cell-shaded models
	virtual void SetPolygonMode( PolygonMode pm );
	virtual void SetLineWidth( float fWidth );

	RString GetTextureDiagnostics( unsigned id ) const;

protected:
	void DrawQuadsInternal( const RageSpriteVertex v[], int iNumVerts );
	void DrawQuadStripInternal( const RageSpriteVertex v[], int iNumVerts );
	void DrawFanInternal( const RageSpriteVertex v[], int iNumVerts );
	void DrawStripInternal( const RageSpriteVertex v[], int iNumVerts );
	void DrawTrianglesInternal( const RageSpriteVertex v[], int iNumVerts );
	void DrawCompiledGeometryInternal( const RageCompiledGeometry *p, int iMeshIndex );
	void DrawLineStripInternal( const RageSpriteVertex v[], int iNumVerts, float LineWidth );
	void DrawSymmetricQuadStripInternal( const RageSpriteVertex v[], int iNumVerts );

	RString TryVideoMode( const VideoModeParams &p, bool &bNewDeviceOut );
	RageSurface* CreateScreenshot();
	PixelFormat GetImgPixelFormat( RageSurface* &img, bool &FreeImg, int width, int height, bool bPalettedTexture );
	bool SupportsSurfaceFormat( PixelFormat pixfmt );
	
	void SendCurrentMatrices();
};

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

