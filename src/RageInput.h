#ifndef RAGEINPUT_H
#define RAGEINPUT_H
/*
-----------------------------------------------------------------------------
 File: RageInput.h

 Desc: Wrapper for SDL's input routines.  Generates InputEvents.
-----------------------------------------------------------------------------
*/

#include "RageInputDevice.h"

class InputHandler;
class RageInput
{
	vector<InputHandler *> Devices;

public:
	RageInput();
	~RageInput();

	void Update( float fDeltaTime );
	void GetDevicesAndDescriptions(vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut);
	void WindowReset();
};

extern RageInput*			INPUTMAN;	// global and accessable from anywhere in our program

#endif
/*
 * Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
 *	Chris Danford
 *  Glenn Maynard
 */
