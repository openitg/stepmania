#include "global.h"
#include "TextBanner.h"
#include "RageUtil.h"
#include "song.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "SongManager.h"
#include "RageTextureManager.h"
#include "ActorUtil.h"
#include "ThemeMetric.h"
#include "Command.h"

ThemeMetric<CString>		ARTIST_PREPEND_STRING			("TextBanner","ArtistPrependString");
ThemeMetric<apActorCommands> TWO_LINES_TITLE_COMMAND		("TextBanner","TwoLinesTitleCommand");
ThemeMetric<apActorCommands> TWO_LINES_SUBTITLE_COMMAND		("TextBanner","TwoLinesSubtitleCommand");
ThemeMetric<apActorCommands> TWO_LINES_ARTIST_COMMAND		("TextBanner","TwoLinesArtistCommand");
ThemeMetric<apActorCommands> THREE_LINES_TITLE_COMMAND		("TextBanner","ThreeLinesTitleCommand");
ThemeMetric<apActorCommands> THREE_LINES_SUBTITLE_COMMAND	("TextBanner","ThreeLinesSubtitleCommand");
ThemeMetric<apActorCommands> THREE_LINES_ARTIST_COMMAND		("TextBanner","ThreeLinesArtistCommand");

void TextBanner::Init()
{
	if( m_bInitted )
		return;
	m_bInitted = true;

	ASSERT( m_sName != "" );

	SET_XY_AND_ON_COMMAND( m_textTitle );
	SET_XY_AND_ON_COMMAND( m_textSubTitle );
	SET_XY_AND_ON_COMMAND( m_textArtist );
}

TextBanner::TextBanner()
{
	m_textTitle.SetName( "Title" );
	m_textTitle.LoadFromFont( THEME->GetPathF("TextBanner","text") );
	this->AddChild( &m_textTitle );

	m_textSubTitle.SetName( "Subtitle" );
	m_textSubTitle.LoadFromFont( THEME->GetPathF("TextBanner","text") );
	this->AddChild( &m_textSubTitle );

	m_textArtist.SetName( "Artist" );
	m_textArtist.LoadFromFont( THEME->GetPathF("TextBanner","text") );
	this->AddChild( &m_textArtist );

	m_bInitted = false;
}


void TextBanner::LoadFromString( 
	CString sDisplayTitle, CString sTranslitTitle, 
	CString sDisplaySubTitle, CString sTranslitSubTitle, 
	CString sDisplayArtist, CString sTranslitArtist )
{
	Init();

	m_textTitle.SetText( sDisplayTitle, sTranslitTitle );
	m_textSubTitle.SetText( sDisplaySubTitle, sTranslitSubTitle );
	m_textArtist.SetText( sDisplayArtist, sTranslitArtist );

	bool bTwoLines = sDisplaySubTitle.size() == 0;

	if( bTwoLines )
	{
		m_textTitle.RunCommands2( TWO_LINES_TITLE_COMMAND, this );
		m_textSubTitle.RunCommands2( TWO_LINES_SUBTITLE_COMMAND, this );
		m_textArtist.RunCommands2( TWO_LINES_ARTIST_COMMAND, this );
	}
	else
	{
		m_textTitle.RunCommands2( THREE_LINES_TITLE_COMMAND, this );
		m_textSubTitle.RunCommands2( THREE_LINES_SUBTITLE_COMMAND, this );
		m_textArtist.RunCommands2( THREE_LINES_ARTIST_COMMAND, this );
	}
}

void TextBanner::LoadFromSong( const Song* pSong )
{
	Init();

	CString sDisplayTitle = pSong ? pSong->GetDisplayMainTitle() : CString("");
	CString sTranslitTitle = pSong ? pSong->GetTranslitMainTitle() : CString("");
	CString sDisplaySubTitle = pSong ? pSong->GetDisplaySubTitle() : CString("");
	CString sTranslitSubTitle = pSong ? pSong->GetTranslitSubTitle() : CString("");
	CString sDisplayArtist = pSong ? (CString)ARTIST_PREPEND_STRING + pSong->GetDisplayArtist() : CString("");
	CString sTranslitArtist = pSong ? (CString)ARTIST_PREPEND_STRING + pSong->GetTranslitArtist() : CString("");

	LoadFromString( 
		sDisplayTitle, sTranslitTitle, 
		sDisplaySubTitle, sTranslitSubTitle, 
		sDisplayArtist, sTranslitArtist );
}

/*
 * (c) 2001-2004 Chris Danford
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
