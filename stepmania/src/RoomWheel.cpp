#include "global.h"
#include "RoomWheel.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ScreenTextEntry.h"
#include "LocalizedString.h"
#include "NetworkSyncManager.h"
#include "ScreenManager.h"

AutoScreenMessage( SM_BackFromRoomName )
AutoScreenMessage( SM_RoomInfoRetract )
AutoScreenMessage( SM_RoomInfoDeploy )

ThemeMetric<float>		TEXT_X;
ThemeMetric<float>		TEXT_Y;
ThemeMetric<float>		TEXT_WIDTH;
ThemeMetric<apActorCommands>	TEXT_ON_COMMAND;

ThemeMetric<float>	DESC_X;
ThemeMetric<float>	DESC_Y;
ThemeMetric<float>	DESC_WIDTH;

RoomWheel::~RoomWheel()
{
	FOREACH( WheelItemBaseData*, m_CurWheelItemData, i )
		SAFE_DELETE( *i );
	m_CurWheelItemData.clear();
}

void RoomWheel::Load( RString sType ) 
{
	WheelBase::Load( sType );

	m_offset = 0;
	LOG->Trace( "RoomWheel::Load('%s')", sType.c_str() );

	AddPerminateItem( new RoomWheelItemData(TYPE_GENERIC, "Create Room", "Create a new game room", THEME->GetMetricC( m_sName, "CreateRoomColor")) );

	BuildWheelItemsData( m_CurWheelItemData );
	RebuildWheelItems();
}

WheelItemBase *RoomWheel::MakeItem()
{
	return new RoomWheelItem;
}

RoomWheelItem::RoomWheelItem( RString sType ):
	WheelItemBase( sType )
{
	Load( sType );
}

RoomWheelItem::RoomWheelItem( const RoomWheelItem &cpy ):
	WheelItemBase( cpy ),
	m_sprBar( cpy.m_sprBar ),
	m_text( cpy.m_text ),
	m_Desc( cpy.m_Desc )
{
	if( cpy.GetNumChildren() != 0 )
	{
		this->AddChild( m_sprBar );
		this->AddChild( &m_text );
		this->AddChild( &m_Desc );
	}
}

void RoomWheelItem::Load( RString sType )
{
	TEXT_X			.Load(sType,"TextX");
	TEXT_Y			.Load(sType,"TextY");
	TEXT_ON_COMMAND	.Load(sType,"TextOnCommand");

	m_sprBar.Load( THEME->GetPathG(sType,"bar") );
	m_sprBar->SetXY( 0, 0 );
	this->AddChild( m_sprBar );
	m_sprBar->SetVisible(false);

	m_text.LoadFromFont( THEME->GetPathF(sType,"text") );
	m_text.SetShadowLength( 0 );
	m_text.SetXY( TEXT_X, TEXT_Y );
	m_text.RunCommands( TEXT_ON_COMMAND );
	this->AddChild( &m_text );


	DESC_X		.Load(sType,"DescX");
	DESC_Y		.Load(sType,"DescY");
	DESC_WIDTH	.Load(sType,"DescWidth");
	TEXT_WIDTH	.Load(sType,"TextWidth");

	m_text.SetHorizAlign( align_left );
	m_text.SetMaxWidth(TEXT_WIDTH);

	m_Desc.SetName( "Desc" );
	ActorUtil::LoadAllCommands( m_Desc, "RoomWheelItem" );
	m_Desc.LoadFromFont( THEME->GetPathF("RoomWheel","text") );
	m_Desc.SetHorizAlign( align_left );
	m_Desc.SetShadowLength( 0 );
	m_Desc.SetMaxWidth( DESC_WIDTH );
	m_Desc.SetXY( DESC_X, DESC_Y);
	m_Desc.PlayCommand( "On" );
	this->AddChild( &m_Desc );
}

void RoomWheel::BuildWheelItemsData( vector<WheelItemBaseData*> &arrayWheelItemDatas )
{
	if( arrayWheelItemDatas.empty() )
		arrayWheelItemDatas.push_back( new RoomWheelItemData(TYPE_GENERIC, "- EMPTY -", "", RageColor(1,0,0,1)) );
}

void RoomWheel::AddPerminateItem( RoomWheelItemData *itemdata  )
{
	++m_offset;
	AddItem( itemdata );
}

void RoomWheel::AddItem( WheelItemBaseData *pItemData )
{
	m_CurWheelItemData.push_back( pItemData );
	int iVisible = FirstVisibleIndex();
	int iIndex = m_CurWheelItemData.size();

	if( m_bEmpty )
	{
		m_bEmpty = false;
		// Remove the - Empty - field when we add an object from an empty state.
		RemoveItem( 0 );
	}

	// If the item was shown in the wheel, rebuild the wheel
	if( 0 <= iIndex - iVisible && iIndex - iVisible < NUM_WHEEL_ITEMS )
		RebuildWheelItems();
}

void RoomWheel::RemoveItem( int index )
{
	index += m_offset;

	if( m_bEmpty || index >= (int)m_CurWheelItemData.size() )
		return;

	vector<WheelItemBaseData *>::iterator i = m_CurWheelItemData.begin();
	i += index;

	// If this item's data happened to be last selected, make it NULL.
	if( m_LastSelection == *i )
		m_LastSelection = NULL;

	SAFE_DELETE( *i );
	m_CurWheelItemData.erase( i );

	if( m_CurWheelItemData.size() < 1 )
	{
		m_bEmpty = true;
		m_CurWheelItemData.push_back( new WheelItemBaseData(TYPE_GENERIC, "- EMPTY -", RageColor(1,0,0,1)) );
	}

	RebuildWheelItems();
}


static LocalizedString ENTER_ROOM_NAME( "RoomWheel", "Enter room name" );
bool RoomWheel::Select()
{
	SCREENMAN->PostMessageToTopScreen( SM_RoomInfoRetract, 0 );

	if( m_iSelection > 0 )
		return WheelBase::Select();
	if( m_iSelection == 0 )
	{
		// Since this is not actually an option outside of this wheel, NULL is a good idea.
		m_LastSelection = NULL;
		ScreenTextEntry::TextEntry( SM_BackFromRoomName, ENTER_ROOM_NAME, "", 255 );
	}
	return false;
}

void RoomWheelItem::LoadFromWheelItemData( const WheelItemBaseData *pWID, int iIndex, bool bHasFocus )
{
	WheelItemBase::LoadFromWheelItemData( pWID, iIndex, bHasFocus );

	m_text.SetText( pWID->m_sText );
	m_text.SetDiffuseColor( pWID->m_color );
	
	const RoomWheelItemData *tmpdata = dynamic_cast<const RoomWheelItemData*>( pWID );
	WheelItemBase::LoadFromWheelItemData( pWID, iIndex, bHasFocus );
	m_Desc.SetText( tmpdata->m_sDesc );
	m_Desc.SetDiffuseColor( pWID->m_color );
}

void RoomWheel::Move( int n )
{
	if( n == 0 && m_iSelection >= m_offset )
	{
		const RoomWheelItemData* data = GetItem( m_iSelection-m_offset );
		if( data != NULL )
			SCREENMAN->PostMessageToTopScreen( SM_RoomInfoDeploy, 0 );
	}
	else
	{
		SCREENMAN->PostMessageToTopScreen( SM_RoomInfoRetract, 0 );
	}

	WheelBase::Move( n );
}

unsigned int RoomWheel::GetNumItems() const
{
	return m_CurWheelItemData.size() - m_offset;
}


/*
 * (c) 2004 Josh Allen
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
