/*
-----------------------------------------------------------------------------
 File: ScreenRaveOptions

 Desc: Select a song.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenOptions.h"

class ScreenRaveOptions : public ScreenOptions
{
public:
	ScreenRaveOptions( CString sName );

private:
	void ImportOptions();
	void ExportOptions();

	void GoToNextState();
	void GoToPrevState();
};

