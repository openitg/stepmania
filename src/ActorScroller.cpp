#include "global.h"
#include "ActorScroller.h"
#include "ActorCollision.h"
#include "RageUtil.h"
#include "RageDisplay.h"
#include "IniFile.h"
#include "arch/Dialog/Dialog.h"
#include "RageLog.h"
#include "ActorUtil.h"

// lua start
LUA_REGISTER_CLASS( ActorScroller )
// lua end

/* Tricky: We need ActorFrames created in XML to auto delete their children.
 * We don't want classes that derive from ActorFrame to auto delete their 
 * children.  The name "ActorFrame" is widely used in XML, so we'll have
 * that string instead create an ActorFrameAutoDeleteChildren object.
 */
//REGISTER_ACTOR_CLASS( ActorScroller )
REGISTER_ACTOR_CLASS_WITH_NAME( ActorScrollerAutoDeleteChildren, ActorScroller )


ActorScroller::ActorScroller()
{
	m_bLoaded = false;
	m_fCurrentItem = 0;
	m_fDestinationItem = 0;
	m_fSecondsPerItem = 1;
	m_fNumItemsToDraw = 7;

	m_vRotationDegrees = RageVector3(0,0,0);
	m_vTranslateTerm0 = RageVector3(0,0,0);
	m_vTranslateTerm1 = RageVector3(0,0,0);
	m_vTranslateTerm2 = RageVector3(0,0,0);
}

void ActorScroller::Load( 
	float fSecondsPerItem, 
	float fNumItemsToDraw, 
	const RageVector3	&vRotationDegrees,
	const RageVector3	&vTranslateTerm0,
	const RageVector3	&vTranslateTerm1,
	const RageVector3	&vTranslateTerm2
	)
{
	m_fSecondsPerItem = fSecondsPerItem;
	m_fNumItemsToDraw = fNumItemsToDraw;
	m_vRotationDegrees = vRotationDegrees;
	m_vTranslateTerm0 = vTranslateTerm0;
	m_vTranslateTerm1 = vTranslateTerm1;
	m_vTranslateTerm2 = vTranslateTerm2;

	m_bLoaded = true;
}

void ActorScroller::LoadFromNode( const CString &sDir, const XNode *pNode )
{
	ActorFrame::LoadFromNode( sDir, pNode );

	bool bUseScroller = false;
	pNode->GetAttrValue( "UseScroller", bUseScroller );
	if( !bUseScroller )
		return;

#define GET_VALUE( szName, valueOut ) \
	if( !pNode->GetAttrValue( szName, valueOut ) ) { \
		CString sError = ssprintf("Animation in '%s' is missing the value Scroller::%s", sDir.c_str(), szName); \
		LOG->Warn( sError ); \
		Dialog::OK( sError ); \
	}

	float fSecondsPerItem = 1;
	float fNumItemsToDraw = 0;
	RageVector3	vRotationDegrees = RageVector3(0,0,0);
	RageVector3	vTranslateTerm0 = RageVector3(0,0,0);
	RageVector3	vTranslateTerm1 = RageVector3(0,0,0);
	RageVector3	vTranslateTerm2 = RageVector3(0,0,0);
	float fItemPaddingStart = 0;
	float fItemPaddingEnd = 0;

	GET_VALUE( "SecondsPerItem", fSecondsPerItem );
	GET_VALUE( "NumItemsToDraw", fNumItemsToDraw );
	GET_VALUE( "RotationDegreesX", vRotationDegrees.x );
	GET_VALUE( "RotationDegreesY", vRotationDegrees.y );
	GET_VALUE( "RotationDegreesZ", vRotationDegrees.z );
	GET_VALUE( "TranslateTerm0X", vTranslateTerm0.x );
	GET_VALUE( "TranslateTerm0Y", vTranslateTerm0.y );
	GET_VALUE( "TranslateTerm0Z", vTranslateTerm0.z );
	GET_VALUE( "TranslateTerm1X", vTranslateTerm1.x );
	GET_VALUE( "TranslateTerm1Y", vTranslateTerm1.y );
	GET_VALUE( "TranslateTerm1Z", vTranslateTerm1.z );
	GET_VALUE( "TranslateTerm2X", vTranslateTerm2.x );
	GET_VALUE( "TranslateTerm2Y", vTranslateTerm2.y );
	GET_VALUE( "TranslateTerm2Z", vTranslateTerm2.z );
	GET_VALUE( "ItemPaddingStart", fItemPaddingStart );
	GET_VALUE( "ItemPaddingEnd", fItemPaddingEnd );
#undef GET_VALUE

	Load( 
		fSecondsPerItem,
		fNumItemsToDraw,
		vRotationDegrees,
		vTranslateTerm0,
		vTranslateTerm1,
		vTranslateTerm2 );
	SetCurrentAndDestinationItem( -fItemPaddingStart );
	SetDestinationItem( m_SubActors.size()-1+fItemPaddingEnd );
}

void ActorScroller::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	if( m_fHibernateSecondsLeft > 0 )
		return;	// early abort

	if( m_fSecondsPerItem > 0 )
		fapproach( m_fCurrentItem, m_fDestinationItem, fDeltaTime/m_fSecondsPerItem );
}

void ActorScroller::DrawPrimitives()
{
	// Optimization:  If we weren't loaded, then fall back to the ActorFrame logic
	if( !m_bLoaded )
	{
		ActorFrame::DrawPrimitives();
	}
	else
	{
		for( unsigned i=0; i<m_SubActors.size(); i++ )
		{
			float fItemOffset = i - m_fCurrentItem;

			if( fabsf(fItemOffset) > m_fNumItemsToDraw / 2 )
				continue;

			DISPLAY->PushMatrix();

			if( m_vRotationDegrees.x )
				DISPLAY->RotateX( m_vRotationDegrees.x*fItemOffset );
			if( m_vRotationDegrees.y )
				DISPLAY->RotateY( m_vRotationDegrees.y*fItemOffset );
			if( m_vRotationDegrees.z )
				DISPLAY->RotateZ( m_vRotationDegrees.z*fItemOffset );
			
			RageVector3 vTranslation = 
				m_vTranslateTerm0 +								// m_vTranslateTerm0*itemOffset^0
				m_vTranslateTerm1 * fItemOffset +				// m_vTranslateTerm1*itemOffset^1
				m_vTranslateTerm2 * fItemOffset*fItemOffset;	// m_vTranslateTerm2*itemOffset^2
			DISPLAY->Translate( 
				vTranslation.x,
				vTranslation.y,
				vTranslation.z
				);

			m_SubActors[i]->Draw();
			
			DISPLAY->PopMatrix();
		}
	}
}

/*
 * (c) 2003-2004 Chris Danford
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
