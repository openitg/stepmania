#ifndef SCREENSANDBOX_H
#define SCREENSANDBOX_H
/*
-----------------------------------------------------------------------------
 Class: ScreenSandbox

 Desc: Area for testing.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Screen.h"
#include "Sample3dObject.h"
#include "Quad.h"

class ScreenSandbox : public Screen
{
public:
	ScreenSandbox();

	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	void Update(float f);
	Sample3dObject obj;
	Quad m_quad;
};


#endif
