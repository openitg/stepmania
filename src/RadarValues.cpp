#include "global.h"
#include "RadarValues.h"
#include "ThemeManager.h"
#include "RageUtil.h"
#include "XmlFile.h"


RadarValues::RadarValues()
{
	Init();
}

void RadarValues::Init()
{
	FOREACH_RadarCategory( rc )
		m_fValues[rc] = RADAR_VAL_UNKNOWN;
}

XNode* RadarValues::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->name = "RadarValues";

	// TRICKY: Don't print a remainder for the integer values.
	FOREACH_RadarCategory( rc )
	{
		if( rc >= RADAR_NUM_TAPS_AND_HOLDS )
			pNode->AppendChild( RadarCategoryToString(rc),	(int)m_fValues[rc] );
		else
			pNode->AppendChild( RadarCategoryToString(rc),	m_fValues[rc] );
	}

	return pNode;
}

void RadarValues::LoadFromNode( const XNode* pNode ) 
{
	ASSERT( pNode->name == "RadarValues" );

	CString s;

	FOREACH_RadarCategory( rc )
		pNode->GetChildValue( RadarCategoryToString(rc),	m_fValues[rc] );
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
