#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: FontManager

 Desc: Interface for loading and releasing textures.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
-----------------------------------------------------------------------------
*/


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "FontManager.h"
#include "Font.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageException.h"
#include "RageTimer.h"


FontManager*	FONT	= NULL;

//-----------------------------------------------------------------------------
// constructor/destructor
//-----------------------------------------------------------------------------
FontManager::FontManager()
{
}

FontManager::~FontManager()
{
	// delete all textures
	POSITION pos = m_mapPathToFont.GetStartPosition();
	CString sFontFilePath;
	Font* pFont;

	while( pos != NULL )  // iterate over all k/v pairs in map
	{
		m_mapPathToFont.GetNextAssoc( pos, sFontFilePath, pFont );
		LOG->Trace( "FONT LEAK: '%s', RefCount = %d.", sFontFilePath, pFont->m_iRefCount );
		SAFE_DELETE( pFont );
	}

	m_mapPathToFont.RemoveAll();
}


//-----------------------------------------------------------------------------
// Load/Unload textures from disk
//-----------------------------------------------------------------------------
Font* FontManager::LoadFont( CString sFontOrTextureFilePath, CString sChars )
{
	sFontOrTextureFilePath.MakeLower();

//	LOG->Trace( "FontManager::LoadFont(%s).", sFontFilePath );


	// Convert the path to lowercase so that we don't load duplicates.
	// Really, this does not solve the duplicate problem.  We could have to copies
	// of the same bitmap if there are equivalent but different paths
	// (e.g. "graphics\blah.png" and "..\stepmania\graphics\blah.png" ).

	Font* pFont = NULL;

	if( m_mapPathToFont.Lookup( sFontOrTextureFilePath, pFont ) )	// if the texture already exists in the map
	{
//		LOG->Trace( ssprintf("FontManager: The Font '%s' now has %d references.", sFontFilePath, pFont->m_iRefCount) );
		pFont->m_iRefCount++;
	}
	else	// the texture is not already loaded
	{
		CString sDrive, sDir, sFName, sExt;
		splitpath( FALSE, sFontOrTextureFilePath, sDrive, sDir, sFName, sExt );

		if( sChars == "" )
			pFont = (Font*) new Font( sFontOrTextureFilePath );
		else
			pFont = (Font*) new Font( sFontOrTextureFilePath, sChars );

//		LOG->Trace( "FontManager: Loading '%s' from disk.", sFontFilePath);

		m_mapPathToFont.SetAt( sFontOrTextureFilePath, pFont );
	}

	return pFont;
}


bool FontManager::IsFontLoaded( CString sFontFilePath )
{
	sFontFilePath.MakeLower();

	Font* pFont;

	if( m_mapPathToFont.Lookup( sFontFilePath, pFont ) )	// if the texture exists in the map
		return true;
	else
		return false;
}	

void FontManager::UnloadFont( CString sFontFilePath )
{
	sFontFilePath.MakeLower();

//	LOG->Trace( "FontManager::UnloadTexture(%s).", sFontFilePath );

	if( sFontFilePath == "" )
	{
//		LOG->Trace( "FontManager::UnloadTexture(): tried to Unload a blank" );
		return;
	}
	
	Font* pFont;

	if( m_mapPathToFont.Lookup( sFontFilePath, pFont ) )	// if the texture exists in the map
	{
		pFont->m_iRefCount--;
		if( pFont->m_iRefCount == 0 )		// there are no more references to this texture
		{
//			LOG->Trace( "FontManager: '%s' will be deleted.  It has %d references.", sFontFilePath, pFont->m_iRefCount );
			SAFE_DELETE( pFont );		// free the texture
			m_mapPathToFont.RemoveKey( sFontFilePath );	// and remove the key in the map
		}
		else
		{
//			LOG->Trace( ssprintf("FontManager: '%s' will not be deleted.  It still has %d references.", sFontFilePath, pFont->m_iRefCount) );
		}

	}
	else	// lookup failed
	{
		throw RageException( ssprintf("Tried to Unload a font that wasn't loaded. '%s'", sFontFilePath) );
	}
	
}
