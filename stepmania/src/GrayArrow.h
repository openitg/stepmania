/*
-----------------------------------------------------------------------------
 Class: GrayArrow

 Desc: A gray arrow that "receives" ColorArrows.

 Copyright (c) 2001-2002 by the names listed below.  All rights reserved.
	Ben Nordstrom
	Chris Danford
-----------------------------------------------------------------------------
*/


#ifndef _GrayArrow_H_
#define _GrayArrow_H_


#include "Sprite.h"
#include "Steps.h"

class GrayArrow : public Sprite
{
public:
	GrayArrow();

	virtual void  SetBeat( const float fSongBeat );
	void Step();
};

#endif 
