#include "global.h"
#include "ModeChoice.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "GameManager.h"
#include "GameState.h"
#include "RageDisplay.h"
#include "AnnouncerManager.h"
#include "ProfileManager.h"
#include "StepMania.h"
#include "ScreenManager.h"
#include "SongManager.h"
#include "PrefsManager.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "MemoryCardManager.h"
#include "song.h"

void ModeChoice::Init()
{
	m_sName = "";
	m_bInvalid = true;
	m_iIndex = -1;
	m_game = GAME_INVALID;
	m_style = STYLE_INVALID;
	m_pm = PLAY_MODE_INVALID;
	m_dc = DIFFICULTY_INVALID;
	m_sModifiers = "";
	m_sAnnouncer = "";
	m_sScreen = "";
	m_pSong = NULL;
	m_pSteps = NULL;
	m_pCharacter = NULL;
	m_CourseDifficulty = COURSE_DIFFICULTY_INVALID;
}

bool CompareSongOptions( const SongOptions &so1, const SongOptions &so2 );

bool ModeChoice::DescribesCurrentModeForAllPlayers() const
{
	for( int pn=0; pn<NUM_PLAYERS; pn++ )
		if( !DescribesCurrentMode( (PlayerNumber) pn) )
			return false;

	return true;
}

bool ModeChoice::DescribesCurrentMode( PlayerNumber pn ) const
{
	if( m_game != GAME_INVALID && m_game != GAMESTATE->m_CurGame )
		return false;
	if( m_pm != PLAY_MODE_INVALID && GAMESTATE->m_PlayMode != m_pm )
		return false;
	if( m_style != STYLE_INVALID && GAMESTATE->m_CurStyle != m_style )
		return false;
	// HACK: don't compare m_dc if m_pSteps is set.  This causes problems 
	// in ScreenSelectOptionsMaster::ImportOptions if m_PreferredDifficulty 
	// doesn't match the difficulty of m_pCurSteps.
	if( m_pSteps == NULL  &&  m_dc != DIFFICULTY_INVALID )
	{
		// Why is this checking for all players?
		for( int pn=0; pn<NUM_PLAYERS; pn++ )
			if( GAMESTATE->IsHumanPlayer(pn) && GAMESTATE->m_PreferredDifficulty[pn] != m_dc )
				return false;
	}
		
	if( m_sAnnouncer != "" && m_sAnnouncer != ANNOUNCER->GetCurAnnouncerName() )
		return false;

	if( m_sModifiers != "" )
	{
		/* Apply modifiers. */
		PlayerOptions po = GAMESTATE->m_PlayerOptions[pn];
		SongOptions so = GAMESTATE->m_SongOptions;
		po.FromString( m_sModifiers );
		so.FromString( m_sModifiers );

		if( po != GAMESTATE->m_PlayerOptions[pn] )
			return false;
		if( so != GAMESTATE->m_SongOptions )
			return false;
	}

	if( m_pSong && GAMESTATE->m_pCurSong != m_pSong )
		return false;
	if( m_pSteps && GAMESTATE->m_pCurNotes[pn] != m_pSteps )
		return false;
	if( m_pCharacter && GAMESTATE->m_pCurCharacters[pn] != m_pCharacter )
		return false;
	if( m_CourseDifficulty != COURSE_DIFFICULTY_INVALID && GAMESTATE->m_CourseDifficulty[pn] != m_CourseDifficulty )
		return false;

	return true;
}

void ModeChoice::Load( int iIndex, CString sChoice )
{
	m_iIndex = iIndex;

	m_bInvalid = false;

	CString sSteps;

	CStringArray asCommands;
	split( sChoice, ";", asCommands );
	for( unsigned i=0; i<asCommands.size(); i++ )
	{
		CString sCommand = asCommands[i];

		CStringArray asBits;
		split( sCommand, ",", asBits );
		
		CString sName = asBits[0];
		asBits.erase(asBits.begin(), asBits.begin()+1);
		CString sValue = join( ",", asBits );

		sName.MakeLower();
		// sValue.MakeLower();

		if( sName == "game" )
		{
			Game game = GAMEMAN->StringToGameType( sValue );
			if( game != GAME_INVALID )
				m_game = game;
			else
				m_bInvalid |= true;
		}


		if( sName == "style" )
		{
			Style style = GAMEMAN->GameAndStringToStyle( GAMESTATE->m_CurGame, sValue );
			if( style != STYLE_INVALID )
				m_style = style;
			else
				m_bInvalid |= true;
		}

		if( sName == "playmode" )
		{
			PlayMode pm = StringToPlayMode( sValue );
			if( pm != PLAY_MODE_INVALID )
				m_pm = pm;
			else
				m_bInvalid |= true;
		}

		if( sName == "difficulty" )
		{
			Difficulty dc = StringToDifficulty( sValue );
			if( dc != DIFFICULTY_INVALID )
				m_dc = dc;
			else
				m_bInvalid |= true;
		}

		if( sName == "announcer" )
			m_sAnnouncer = sValue;

		if( sName == "name" )
			m_sName = sValue;

		if( sName == "mod" )
		{
			if( m_sModifiers != "" )
				m_sModifiers += ",";
			m_sModifiers += sValue;
		}
		
		if( sName == "song" )
		{
			m_pSong = SONGMAN->FindSong( sValue );
			if( m_pSong == NULL )
				m_bInvalid |= true;
		}

		if( sName == "steps" )
		{
			/* Save the name of the steps, and set this later, since we want to process
			 * any "song" and "style" commands first. */
			sSteps = sValue;
		}

		if( sName == "screen" )
			m_sScreen = sValue;
	}

	if( !m_bInvalid && sSteps != "" )
	{
		if( m_pSong == NULL || m_style == STYLE_INVALID )
			RageException::Throw( "Must set Song and Style to set Steps" );

		m_pSteps = m_pSong->GetStepsByDescription( GAMEMAN->GetStyleDefForStyle(m_style)->m_StepsType, sSteps );
		if( m_pSteps == NULL )
			m_bInvalid |= true;
	}
}

int GetSidesRequiredToPlayStyle( Style style )
{
	switch( GAMEMAN->GetStyleDefForStyle(style)->m_StyleType )
	{
	case StyleDef::ONE_PLAYER_ONE_CREDIT:
		return 1;
	case StyleDef::TWO_PLAYERS_TWO_CREDITS:
		return 2;
	case StyleDef::ONE_PLAYER_TWO_CREDITS:
		return (PREFSMAN->m_Premium == PrefsManager::DOUBLES_PREMIUM) ? 1 : 2;
	default:
		ASSERT(0);
		return 1;
	}
}

bool ModeChoice::IsPlayable( CString *why ) const
{
	if( m_bInvalid )
		return false;

	if ( m_style != STYLE_INVALID )
	{
		int iNumSidesJoined = GAMESTATE->GetNumSidesJoined();
		int iNumSidesRequired = GetSidesRequiredToPlayStyle(m_style);
		
		if( iNumSidesRequired != iNumSidesJoined )
			return false;
	}

	/* Don't allow a PlayMode that's incompatible with our current Style (if set),
	 * and vice versa. */
	const PlayMode &rPlayMode = (m_pm != PLAY_MODE_INVALID) ? m_pm : GAMESTATE->m_PlayMode;
	if( rPlayMode == PLAY_MODE_RAVE || rPlayMode == PLAY_MODE_BATTLE )
	{
		// Can't play rave if there isn't enough room for two players.
		// This is correct for dance (ie, no rave for solo and doubles),
		// and should be okay for pump .. not sure about other game types.
		const Style &rStyle = m_style != STYLE_INVALID? m_style: GAMESTATE->m_CurStyle;
		if( rStyle != STYLE_INVALID &&
			GAMEMAN->GetStyleDefForStyle(rStyle)->m_iColsPerPlayer >= 6 )
			return false;
	}

	if( !m_sScreen.CompareNoCase("ScreenEditCoursesMenu") )
	{
		vector<Course*> vCourses;
		SONGMAN->GetAllCourses( vCourses, false );

		if( vCourses.size() == 0 )
		{
			if( why )
				*why = "No courses are installed";
			return false;
		}
	}

	if( !m_sScreen.CompareNoCase("ScreenJukeboxMenu") ||
		!m_sScreen.CompareNoCase("ScreenEditMenu") ||
		!m_sScreen.CompareNoCase("ScreenEditCoursesMenu") )
	{
		if( SONGMAN->GetNumSongs() == 0 )
		{
			if( why )
				*why = "No songs are installed";
			return false;
		}
	}

	return true;
}

void ModeChoice::ApplyToAllPlayers() const
{
	for( int pn=0; pn<NUM_PLAYERS; pn++ )
		if( GAMESTATE->IsHumanPlayer(pn) )
			Apply((PlayerNumber) pn);

	if( m_sScreen != "" )
		SCREENMAN->SetNewScreen( m_sScreen );
}

void ModeChoice::Apply( PlayerNumber pn ) const
{
	if( !GAMESTATE->IsHumanPlayer(pn) )
		return;

	const PlayMode OldPlayMode = GAMESTATE->m_PlayMode;

	if( m_game != GAME_INVALID )
		GAMESTATE->m_CurGame = m_game;
	if( m_pm != PLAY_MODE_INVALID )
		GAMESTATE->m_PlayMode = m_pm;
	if( m_style != STYLE_INVALID )
	{
		GAMESTATE->m_CurStyle = m_style;

		// If only one side is joined and we picked a style
		// that requires both sides, join the other side.
		switch( GAMEMAN->GetStyleDefForStyle(m_style)->m_StyleType )
		{
		case StyleDef::ONE_PLAYER_ONE_CREDIT:
			break;
		case StyleDef::TWO_PLAYERS_TWO_CREDITS:
		case StyleDef::ONE_PLAYER_TWO_CREDITS:
			int p;
			for( p=0; p<NUM_PLAYERS; p++ )
				GAMESTATE->m_bSideIsJoined[p] = true;
			break;
		default:
			ASSERT(0);
		}
	}
	if( m_dc != DIFFICULTY_INVALID  &&  pn != PLAYER_INVALID )
		GAMESTATE->m_PreferredDifficulty[pn] = m_dc;
	if( m_sAnnouncer != "" )
		ANNOUNCER->SwitchAnnouncer( m_sAnnouncer );
	if( m_sModifiers != "" )
		GAMESTATE->ApplyModifiers( pn, m_sModifiers );
	if( m_pSong )
		GAMESTATE->m_pCurSong = m_pSong;
	if( m_pSteps )
		GAMESTATE->m_pCurNotes[pn] = m_pSteps;
	if( m_pCharacter )
		GAMESTATE->m_pCurCharacters[pn] = m_pCharacter;
	if( m_CourseDifficulty != COURSE_DIFFICULTY_INVALID )
	{
		GAMESTATE->m_CourseDifficulty[pn] = m_CourseDifficulty;
		if( PREFSMAN->m_bLockCourseDifficulties )
			for( int p = 0; p < NUM_PLAYERS; ++p )
				GAMESTATE->m_CourseDifficulty[p] = GAMESTATE->m_CourseDifficulty[pn];
	}

	// HACK:  Set life type to BATTERY just once here so it happens once and 
	// we don't override the user's changes if they back out.
	if( GAMESTATE->m_PlayMode == PLAY_MODE_ONI && GAMESTATE->m_PlayMode != OldPlayMode )
		GAMESTATE->m_SongOptions.m_LifeType = SongOptions::LIFE_BATTERY;


	//
	// We know what players are joined at the time we set the Style
	//
	if( m_style != STYLE_INVALID )
	{
		GAMESTATE->PlayersFinalized();
	}
	
}

bool ModeChoice::IsZero() const
{
	if( m_game != GAME_INVALID ||
		m_pm != PLAY_MODE_INVALID ||
		m_style != STYLE_INVALID ||
		m_dc != DIFFICULTY_INVALID ||
		m_sAnnouncer != "" ||
		m_sModifiers != "" ||
		m_pSong != NULL || 
		m_pSteps != NULL || 
		m_pCharacter != NULL || 
		m_CourseDifficulty != COURSE_DIFFICULTY_INVALID )
		return false;

	return true;
}

