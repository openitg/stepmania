#include "global.h"
#include "RageTypes.h"
#include "RageUtil.h"
	
bool RageColor::FromString( const RString &str )
{
	int result = sscanf( str, "%f,%f,%f,%f", &r, &g, &b, &a );
	if( result == 3 )
	{
		a = 1;
		return true;
	}
	if( result == 4 )
		return true;
	
	int ir=255, ib=255, ig=255, ia=255;
	result = sscanf( str, "#%2x%2x%2x%2x", &ir, &ig, &ib, &ia );
	if( result >= 3 )
	{
		r = ir / 255.0f; g = ig / 255.0f; b = ib / 255.0f;
		if( result == 4 )
			a = ia / 255.0f;
		else
			a = 1;
		return true;
	}
	
	r=1; b=1; g=1; a=1;
	return false;
}
RString RageColor::ToString() const
{
	return ssprintf("%x%x%x%x", (int)(r*255), (int)(g*255), (int)(b*255), (int)(a*255));
}

/*
 * Copyright (c) 2001-2002 Chris Danford
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
