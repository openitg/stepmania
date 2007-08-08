#include "global.h"
#include "ScreenSelectProfile.h"
#include "ScreenManager.h"
#include "SongManager.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "AnnouncerManager.h"
#include "song.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "ActorUtil.h"
#include "GameState.h"
#include "MemoryCardManager.h"
#include "RageLog.h"
#include "Style.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "StatsManager.h"
#include "PlayerState.h"
#include "CommonMetrics.h"
#include "InputEventPlus.h"
#include "ScreenDimensions.h"
#include "InputMapper.h"

REGISTER_SCREEN_CLASS( ScreenSelectProfile );

void ScreenSelectProfile::Init()
{
	FOREACH_PlayerNumber( p )
	{
		// no selection initially
		m_iSelectedProfiles[p]=-1;
	}
	ScreenWithMenuElements::Init();
}

void ScreenSelectProfile::Input( const InputEventPlus &input )
{
	if( IsTransitioning() )
		return;
	
	ScreenWithMenuElements::Input( input );
}

bool ScreenSelectProfile::SetProfileIndex( PlayerNumber pn, int iProfileIndex )
{
	if( !GAMESTATE->IsHumanPlayer( pn ) )
	{
		if( iProfileIndex == -1 )
		{
			GAMESTATE->JoinPlayer( pn );
			SCREENMAN->PlayStartSound();
			return true;
		}
		return false;
	}

	if( iProfileIndex > PROFILEMAN->GetNumLocalProfiles() )
		return false;

	// wrong selection
	if( iProfileIndex < -2 )
		return false;

	// unload player
	if( iProfileIndex == -2 )
	{
		PROFILEMAN->UnloadProfile( pn );
		GAMESTATE->UnjoinPlayer( pn );
		MEMCARDMAN->UnlockCard( pn );
		MEMCARDMAN->UnmountCard( pn );
		m_iSelectedProfiles[pn]=-1;
		return true;
	}

	m_iSelectedProfiles[pn]=iProfileIndex;

	return true;
}

bool ScreenSelectProfile::Finish(){
	if( GAMESTATE->GetNumPlayersEnabled() == 0 )
		return false;

	// if profile indexes are the same for both players
	if( ( GAMESTATE->GetNumPlayersEnabled() == 2 ) && ( m_iSelectedProfiles[0] == m_iSelectedProfiles[1] ) && ( m_iSelectedProfiles[0] > 0 ) )
	{
		return false;
	}

	FOREACH_PlayerNumber( p )
	{
		// not all players has made their choices
		if( GAMESTATE->IsHumanPlayer( p ) && ( m_iSelectedProfiles[p] == -1 ) )
			return false;

		// card not ready
		if( ( m_iSelectedProfiles[p] == 0 ) && (MEMCARDMAN->GetCardState( p ) != MemoryCardState_Ready ) )
			return false;

		// profile index too big
		if( m_iSelectedProfiles[p] > PROFILEMAN->GetNumLocalProfiles() )
			return false;
	}

	// all ok - load profiles and go to next screen
	FOREACH_PlayerNumber( p )
	{
		MEMCARDMAN->UnlockCard( p );
		MEMCARDMAN->UnmountCard( p );
		PROFILEMAN->UnloadProfile( p );

		if( m_iSelectedProfiles[p] > 0 )
		{
			PROFILEMAN->m_sDefaultLocalProfileID[p].Set( PROFILEMAN->GetLocalProfileIDFromIndex( m_iSelectedProfiles[p] - 1 ) );
			PROFILEMAN->LoadLocalProfileFromMachine( p );
		}
		if( m_iSelectedProfiles[p] == 0 )
		{
			MEMCARDMAN->WaitForCheckingToComplete();

			MEMCARDMAN->MountCard( p );
			bool bSuccess = PROFILEMAN->LoadProfileFromMemoryCard( p, true );	// load full profile
			MEMCARDMAN->UnmountCard( p );

			/* Lock the card on successful load, so we won't allow it to be changed. */
			if( bSuccess )
				MEMCARDMAN->LockCard( p );
		}
	}
	StartTransitioningScreen( SM_GoToNextScreen );
	return true;
}

// lua start
#include "LuaBinding.h"

class LunaScreenSelectProfile: public Luna<ScreenSelectProfile>
{
public:
	static int SetProfileIndex( T* p, lua_State *L )
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>(L, 1);
		int iProfileIndex = IArg(2);
		bool bRet = p->SetProfileIndex( pn, iProfileIndex );
		LuaHelpers::Push( L, bRet );
		return 1;
	}

	static int GetProfileIndex( T* p, lua_State *L )
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>(L, 1);
		LuaHelpers::Push( L, p->GetProfileIndex( pn ) );
		return 1;
	}

	static int Finish( T* p, lua_State *L )
	{
		bool bRet = p->Finish();
		LuaHelpers::Push( L, bRet );
		return 1;
	}

	static int Cancel( T* p, lua_State *L )
	{
		p->Cancel( SM_GoToPrevScreen );
		return 1;
	}

	LunaScreenSelectProfile()
	{
		ADD_METHOD( SetProfileIndex );
		ADD_METHOD( GetProfileIndex );
		ADD_METHOD( Finish );
		ADD_METHOD( Cancel );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenSelectProfile, ScreenWithMenuElements )

/*
* (c) 2004 Chris Danford
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
