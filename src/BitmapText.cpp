#include "global.h"
/*
-----------------------------------------------------------------------------
 File: BitmapText

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "BitmapText.h"
#include "IniFile.h"
#include "FontManager.h"
#include "RageLog.h"
#include "RageException.h"
#include "RageTimer.h"
#include "RageDisplay.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "GameConstantsAndTypes.h"
#include "Font.h"
#include "ActorUtil.h"	// for HandleParams

/* XXX:
 * We need some kind of font modifier string for metrics.  For example,
 * "valign=top;spacing = x+5,y+2"
 *
 * Better, we could go all the way, drop all of the actor-specific font aliases,
 * and do "font=header2;valign=top;...".
 *
 * However, let's wait until Pango support is in, so we can figure out how to
 * integrate font selection at the same time.
 */
/*
 * Forward planning:
 *
 * This can't handle full CJK fonts; it's not reasonable to load a 2000+ glyph bitmap
 * font into memory.  We want to be able to fall back on a real font renderer when
 * we're missing characters.  However, we make use of custom glyphs, and we don't want
 * custom glyphs and fallback fonts to be mutually exclusive.
 *
 * So, if we have a fallback font renderer active, and we're missing any characters at
 * all, send the text to the fallback renderer.  If it can't render a character
 * (because it's a special-use character), render up to that character, render the
 * special character ourself, then start again on the next character.  The data
 * rendered can be put into textures and put into the verts/tex lists as if it came
 * from a regular font (nothing says we can't put more than one character per quad)
 * and DrawChars won't have to be changed at all.
 *
 * Some mechanism to hint which fallback font size/style a given font wants, to make
 * a best effort to line up font types.  This could be a setting in the font INI.
 */
#define RAINBOW_COLOR(n)	THEME->GetMetricC("BitmapText",ssprintf("RainbowColor%i", n+1))

const int NUM_RAINBOW_COLORS = 7;
RageColor RAINBOW_COLORS[NUM_RAINBOW_COLORS];

BitmapText::BitmapText()
{
	// Loading these theme metrics is slow, so only do it ever 20th time.
	static int iReloadCounter = 0;
	if( iReloadCounter%20==0 )
	{
		for(int i = 0; i < NUM_RAINBOW_COLORS; ++i)
			RAINBOW_COLORS[i] = RAINBOW_COLOR(i);
	}
	iReloadCounter++;

	m_HorizAlign = align_center;
	m_VertAlign = align_middle;

	m_pFont = NULL;

	m_bShadow = true;

	m_bRainbow = false;

	m_iWrapWidthPixels = -1;
	m_fMaxWidth = 0;
}

BitmapText::~BitmapText()
{
	if( m_pFont )
		FONT->UnloadFont( m_pFont );
}

bool BitmapText::LoadFromFont( CString sFontFilePath )
{
	CHECKPOINT_M( ssprintf("BitmapText::LoadFromFontName(%s)", sFontFilePath.c_str()) );

	if( m_pFont ) {
		FONT->UnloadFont( m_pFont );
		m_pFont = NULL;
	}

	// load font
	m_pFont = FONT->LoadFont( sFontFilePath );

	BuildChars();

	return true;
}


bool BitmapText::LoadFromTextureAndChars( CString sTexturePath, CString sChars )
{
	CHECKPOINT_M( ssprintf("BitmapText::LoadFromTextureAndChars(\"%s\",\"%s\")", sTexturePath.c_str(), sChars.c_str()) );

	if( m_pFont ) {
		FONT->UnloadFont( m_pFont );
		m_pFont = NULL;
	}

	// load font
	m_pFont = FONT->LoadFont( sTexturePath, sChars );

	BuildChars();

	return true;
}

void BitmapText::BuildChars()
{
	/* If we don't have a font yet, we'll do this when it loads. */
	if(m_pFont == NULL)
		return;

	/* calculate line lengths and widths */
	m_size.x = 0;

	m_iLineWidths.clear();
	for( unsigned l=0; l<m_wTextLines.size(); l++ )	// for each line
	{
		m_iLineWidths.push_back(m_pFont->GetLineWidthInSourcePixels( m_wTextLines[l] ));
		m_size.x = max( m_size.x, m_iLineWidths.back() );
	}

	verts.clear();
	tex.clear();
	
	if(m_wTextLines.empty()) return;

	m_size.y = float(m_pFont->GetHeight() * m_wTextLines.size());
	unsigned i;
	int MinSpacing = 0;

	/* The height (from the origin to the baseline): */
	int Padding = max(m_pFont->GetLineSpacing(), MinSpacing) - m_pFont->GetHeight();

	/* There's padding between every line: */
	m_size.y += Padding * (m_wTextLines.size()-1);

	int iY;	//	 the top position of the first row of characters
	switch( m_VertAlign )
	{
	case align_top:		iY = 0;					break;
	case align_middle:	iY = -(int)roundf(m_size.y/2.0f);	break;
	case align_bottom:	iY = -(int)m_size.y;	break;
	default:			ASSERT( false );		return;
	}

	for( i=0; i<m_wTextLines.size(); i++ )		// foreach line
	{
		iY += m_pFont->GetHeight();
		const wstring &szLine = m_wTextLines[i];
		const int iLineWidth = m_iLineWidths[i];
		
		int iX;
		switch( m_HorizAlign )
		{
		case align_left:	iX = 0;				break;
		case align_center:	iX = -(int)roundf(iLineWidth/2.0f);	break;
		case align_right:	iX = -iLineWidth;	break;
		default:			ASSERT( false );	return;
		}

		for( unsigned j=0; j<szLine.size(); j++ )	// for each character in the line
		{
			RageSpriteVertex v[4];
			const glyph &g = m_pFont->GetGlyph(szLine[j]);

			/* set vertex positions */
			v[0].p = RageVector3( iX+g.hshift,			iY+g.fp->vshift,		  0 );	// top left
			v[1].p = RageVector3( iX+g.hshift,			iY+g.fp->vshift+g.height, 0 );	// bottom left
			v[2].p = RageVector3( iX+g.hshift+g.width,	iY+g.fp->vshift+g.height, 0 );	// bottom right
			v[3].p = RageVector3( iX+g.hshift+g.width,	iY+g.fp->vshift,		  0 );	// top right

			/* Advance the cursor. */
			iX += g.hadvance;

			/* set texture coordinates */
			v[0].t = RageVector2( g.rect.left,	g.rect.top );
			v[1].t = RageVector2( g.rect.left,	g.rect.bottom );
			v[2].t = RageVector2( g.rect.right,	g.rect.bottom );
			v[3].t = RageVector2( g.rect.right,	g.rect.top );

			verts.insert(verts.end(), &v[0], &v[4]);
			tex.push_back(g.GetTexture());
		}

		/* The amount of padding a line needs: */
		iY += Padding;
	}
}

void BitmapText::DrawChars()
{
	unsigned uNumGlyphs = tex.size();
	unsigned uStartGlyph = (unsigned) SCALE( m_pTempState->crop.left, 0.f, 1.f, 0, (float) uNumGlyphs );
	unsigned uEndGlyph = (unsigned) SCALE( m_pTempState->crop.right, 0.f, 1.f, (float) uNumGlyphs, 0 );

	ASSERT( uStartGlyph <= uNumGlyphs );
	ASSERT( uEndGlyph <= uNumGlyphs );
	for(unsigned start = uStartGlyph; start < uEndGlyph; )
	{
		unsigned end = start;
		while(end < uEndGlyph && tex[end] == tex[start])
			end++;
		DISPLAY->SetTexture( tex[start] );
		RageSpriteVertex &start_vertex = verts[start*4];
		int iNumVertsToDraw = (end-start)*4;
		DISPLAY->DrawQuads( &start_vertex, iNumVertsToDraw );
		
		start = end;
	}
}

/* sText is UTF-8.  If not all of the characters in sText are available in the
 * font, sAlternateText will be used instead.  If there are unavailable characters
 * in sAlternateText, too, just use sText. */
void BitmapText::SetText( CString sText, CString sAlternateText, int iWrapWidthPixels )
{
	ASSERT( m_pFont );

	if(StringWillUseAlternate(sText, sAlternateText))
		sText = sAlternateText;

	if( iWrapWidthPixels == -1 )	// wrap not specified
		iWrapWidthPixels = m_iWrapWidthPixels;

	if(m_sText == sText && iWrapWidthPixels==m_iWrapWidthPixels)
		return;
	m_sText = sText;
	m_iWrapWidthPixels = iWrapWidthPixels;


	// Break the string into lines.
	//
	m_wTextLines.clear();

	if( iWrapWidthPixels == -1 )
	{
		split( CStringToWstring(sText), L"\n", m_wTextLines, false );
	}
	else
	{
		//
		// Break sText into lines that don't exceed iWrapWidthPixels
		// (if only one word fits on the line, it may be larger than iWrapWidthPixels ).
		//
		// TODO: Investigate whether this works in all languages
		/* It doesn't.  I can add Japanese wrapping, at least.  We could handle hyphens
		 * and soft hyphens and pretty easily, too. -glenn */
		// TODO: Move this wrapping logic into Font
		CStringArray asWords;
		split( sText, " ", asWords );

		CString sCurLine;
		int iCurLineWidth = 0;

		/* Note that GetLineWidthInSourcePixels does not include horizontal overdraw
		 * right now (eg. italic fonts), so it's possible to go slightly over. */
		for( unsigned i=0; i<asWords.size(); i++ )
		{
			const CString &sWord = asWords[i];
			int iWidthWord = m_pFont->GetLineWidthInSourcePixels( CStringToWstring(sWord) );

			if( sCurLine.empty() )
			{
				sCurLine = sWord;
				iCurLineWidth = iWidthWord;
			}
			else
			{
				CString sToAdd = " " + sWord;
				int iWidthToAdd = m_pFont->GetLineWidthInSourcePixels(L" ") + iWidthWord;
				if( iCurLineWidth + iWidthToAdd <= iWrapWidthPixels )	// will fit on current line
				{
					sCurLine += sToAdd;
					iCurLineWidth += iWidthToAdd;
				}
				else
				{
					m_wTextLines.push_back( CStringToWstring(sCurLine) );
					sCurLine = sWord;
					iCurLineWidth = iWidthWord;
				}
			}
		}
		m_wTextLines.push_back( CStringToWstring(sCurLine) );
	}

	BuildChars();
	UpdateBaseZoom();
}

void BitmapText::SetMaxWidth( float MaxWidth )
{
	m_fMaxWidth = MaxWidth;
	UpdateBaseZoom();
}

void BitmapText::UpdateBaseZoom()
{
	if( m_fMaxWidth == 0 )
	{
		this->SetBaseZoomX( 1 );
		return;
	}

	const float Width = GetUnzoomedWidth();

	/* Avoid division by zero. */
	if( !Width )
		return;

	/* Never decrease the zoom. */
	const float Zoom = min( 1, m_fMaxWidth/Width );
	this->SetBaseZoomX( Zoom );
}

bool BitmapText::StringWillUseAlternate(CString sText, CString sAlternateText) const
{
	ASSERT( m_pFont );

	/* Can't use the alternate if there isn't one. */
	if(!sAlternateText.size()) return false;

	/* False if the alternate isn't needed. */
	if(m_pFont->FontCompleteForString(CStringToWstring(sText))) return false;

	/* False if the alternate is also incomplete. */
	if(!m_pFont->FontCompleteForString(CStringToWstring(sAlternateText))) return false;

	return true;
}

void BitmapText::CropToWidth( int iMaxWidthInSourcePixels )
{
	iMaxWidthInSourcePixels = max( 0, iMaxWidthInSourcePixels );

	for( unsigned l=0; l<m_wTextLines.size(); l++ )	// for each line
	{
		while( m_iLineWidths[l] > iMaxWidthInSourcePixels )
		{
			m_wTextLines[l].erase(m_wTextLines[l].end()-1, m_wTextLines[l].end());
			m_iLineWidths[l] = m_pFont->GetLineWidthInSourcePixels( m_wTextLines[l] );
		}
	}

	BuildChars();
}

// draw text at x, y using colorTop blended down to colorBottom, with size multiplied by scale
void BitmapText::DrawPrimitives()
{
	if( m_wTextLines.empty() )
		return;

	Actor::SetRenderStates();	// set Actor-specified render states
	DISPLAY->SetTextureModeModulate();

	/* Draw if we're not fully transparent or the zbuffer is enabled */
	if( m_pTempState->diffuse[0].a != 0 )
	{
		//////////////////////
		// render the shadow
		//////////////////////
		if( m_bShadow )
		{
			DISPLAY->PushMatrix();
			DISPLAY->TranslateWorld( m_fShadowLength, m_fShadowLength, 0 );	// shift by 5 units

			RageColor dim(0,0,0,0.5f*m_pTempState->diffuse[0].a);	// semi-transparent black

			for( unsigned i=0; i<verts.size(); i++ )
				verts[i].c = dim;
			DrawChars();

			DISPLAY->PopMatrix();
		}

		//////////////////////
		// render the diffuse pass
		//////////////////////
		if( m_bRainbow )
		{
			int color_index = int(RageTimer::GetTimeSinceStart() / 0.200) % NUM_RAINBOW_COLORS;
			for( unsigned i=0; i<verts.size(); i+=4 )
			{
				const RageColor color = RAINBOW_COLORS[color_index];
				for( unsigned j=i; j<i+4; j++ )
					verts[j].c = color;

				color_index = (color_index+1)%NUM_RAINBOW_COLORS;
			}
		}
		else
		{
			for( unsigned i=0; i<verts.size(); i+=4 )
			{
				verts[i+0].c = m_pTempState->diffuse[0];	// top left
				verts[i+1].c = m_pTempState->diffuse[2];	// bottom left
				verts[i+2].c = m_pTempState->diffuse[3];	// bottom right
				verts[i+3].c = m_pTempState->diffuse[1];	// top right
			}
		}

		DrawChars();
	}

	/* render the glow pass */
	if( m_pTempState->glow.a > 0.0001f )
	{
		DISPLAY->SetTextureModeGlow(m_pTempState->glowmode);

		for( unsigned i=0; i<verts.size(); i++ )
			verts[i].c = m_pTempState->glow;
		DrawChars();
	}
}

/* Rebuild when these change. */
void BitmapText::SetHorizAlign( HorizAlign ha )
{
	if(ha == m_HorizAlign) return;
	Actor::SetHorizAlign(ha);
	BuildChars();
}

void BitmapText::SetVertAlign( VertAlign va )
{
	if(va == m_VertAlign) return;
	Actor::SetVertAlign(va);
	BuildChars();
}

void BitmapText::HandleCommand( const CStringArray &asTokens )
{
	HandleParams;

	const CString& sName = asTokens[0];

	// Commands that go in the tweening queue:
	// Commands that take effect immediately (ignoring the tweening queue):
	if( sName=="wrapwidthpixels" )		SetWrapWidthPixels( iParam(1) );
	else if( sName=="maxwidth" )		SetMaxWidth( fParam(1) );
	else
	{
		Actor::HandleCommand( asTokens );
		return;
	}

	CheckHandledParams;
}

void BitmapText::SetWrapWidthPixels( int iWrapWidthPixels )
{
	ASSERT( m_pFont ); // always load a font first
	if( m_iWrapWidthPixels == iWrapWidthPixels )
		return;
	SetText( m_sText, "", iWrapWidthPixels );
}
