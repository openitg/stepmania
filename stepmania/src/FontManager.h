#ifndef FONTMANAGER_H
#define FONTMANAGER_H
/*
-----------------------------------------------------------------------------
 Class: FontManager

 Desc: Manages Loading and Unloading of fonts.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Font.h"

#include <map>

//-----------------------------------------------------------------------------
// FontManager Class Declarations
//-----------------------------------------------------------------------------
class FontManager
{
public:
	FontManager();
	~FontManager();

	Font* LoadFont( CString sFontOrTextureFilePath, CString sChars = "" );
	void UnloadFont( Font *fp );

protected:
	// map from file name to a texture holder
	map<CString, Font*> m_mapPathToFont;
	static CString FontManager::GetPageNameFromFileName(const CString &fn);
};

extern FontManager*	FONT;	// global and accessable from anywhere in our program


#endif
