#include "global.h"
#include "stdlib.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenHowToPlay

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScreenHowToPlay.h"
#include "ThemeManager.h"
#include "GameState.h"
#include "GameDef.h"
#include "RageLog.h"
#include "RageDisplay.h"
#include "SongManager.h"
#include "NoteFieldPositioning.h"
#include "GameManager.h"
#include "NotesLoaderSM.h"

#define SECONDS_TO_SHOW						THEME->GetMetricF("ScreenHowToPlay","SecondsToShow")
#define STEPFILE							THEME->GetMetric ("ScreenHowToPlay","Stepfile")
#define NUM_PERFECTS						THEME->GetMetricI("ScreenHowToPlay","NumPerfects")
#define NUM_MISSES							THEME->GetMetricI("ScreenHowToPlay","NumMisses")
//
#define USELIFEBAR							THEME->GetMetricB("ScreenHowToPlay","UseLifeMeterBar")
#define LIFEBARONCOMMAND					THEME->GetMetric ("ScreenHowToPlay","LifeMeterBarOnCommand")
//
#define USECHARACTER						THEME->GetMetricB("ScreenHowToPlay","UseCharacter")
#define CHARACTERONCOMMAND					THEME->GetMetric ("ScreenHowToPlay","CharacterOnCommand")
//
#define USEPAD								THEME->GetMetricB("ScreenHowToPlay","UsePad")
#define PADONCOMMAND						THEME->GetMetric ("ScreenHowToPlay","PadOnCommand")
//
#define USEPLAYER							THEME->GetMetricB("ScreenHowToPlay","UseNotefield")
#define PLAYERX								THEME->GetMetricF("ScreenHowToPlay","PlayerX")

ScreenHowToPlay::ScreenHowToPlay() : ScreenAttract("ScreenHowToPlay")
{
	m_iPerfects = 0;
	m_iNumPerfects = NUM_PERFECTS;

	// initialize these because they might not be used.
	m_pPlayer = NULL;
	m_pLifeMeterBar = NULL;
	m_pmCharacter = NULL;
	m_pmDancePad = NULL;

	m_In.Load( THEME->GetPathToB("ScreenHowToPlay in") );
	m_In.StartTransitioning();

	m_Out.Load( THEME->GetPathToB("ScreenHowToPlay out") );

	m_Overlay.LoadFromAniDir( THEME->GetPathToB("ScreenHowToPlay overlay") );
	this->AddChild( &m_Overlay );

	if( (USEPAD && DoesFileExist("Characters" SLASH "DancePad-DDR.txt")) )
	{
		m_pmDancePad = new Model;
		m_pmDancePad->LoadMilkshapeAscii("Characters" SLASH "DancePad-DDR.txt");
		m_pmDancePad->SetRotationX( 35 );
		m_pmDancePad->Command( PADONCOMMAND );
	}
	
	// Display random character
	if( (USECHARACTER && GAMESTATE->m_pCharacters.size()) )
	{
		Character* rndchar = GAMESTATE->GetRandomCharacter();

		m_pmCharacter = new Model;
		m_pmCharacter->LoadMilkshapeAscii( rndchar->GetModelPath() );
//		m_pmCharacter->LoadMilkshapeAsciiBones("howtoplay", rndchar->GetHowToPlayAnimationPath() );
		m_pmCharacter->LoadMilkshapeAsciiBones( "Step-LEFT","Characters" SLASH "BeginnerHelper_step-left.bones.txt" );
		m_pmCharacter->LoadMilkshapeAsciiBones( "Step-DOWN","Characters" SLASH "BeginnerHelper_step-down.bones.txt" );
		m_pmCharacter->LoadMilkshapeAsciiBones( "Step-UP","Characters" SLASH "BeginnerHelper_step-up.bones.txt" );
		m_pmCharacter->LoadMilkshapeAsciiBones( "Step-RIGHT","Characters" SLASH "BeginnerHelper_step-right.bones.txt" );
		m_pmCharacter->LoadMilkshapeAsciiBones( "Step-JUMPLR","Characters" SLASH "BeginnerHelper_step-jumplr.bones.txt" );
		m_pmCharacter->LoadMilkshapeAsciiBones( "rest",rndchar->GetRestAnimationPath() );
		m_pmCharacter->SetDefaultAnimation( "rest" );
		m_pmCharacter->PlayAnimation( "rest" );
		m_pmCharacter->m_bRevertToDefaultAnimation = true;		// Stay bouncing after a step has finished animating.
		
		m_pmCharacter->SetRotationX( 40 );
		m_pmCharacter->Command( CHARACTERONCOMMAND );
	}
	
	// silly to use the lifebar without a player, since the player updates the lifebar
	if( USELIFEBAR )
	{
		m_pLifeMeterBar = new LifeMeterBar;
		m_pLifeMeterBar->Load( PLAYER_1 );
		m_pLifeMeterBar->Command( LIFEBARONCOMMAND );
		m_pLifeMeterBar->FillForHowToPlay( NUM_PERFECTS, NUM_MISSES );
	}

	switch(GAMESTATE->m_CurGame) // which style should we use to demonstrate?
	{
		case GAME_DANCE:	GAMESTATE->m_CurStyle = STYLE_DANCE_SINGLE;		break;
		case GAME_PUMP:		GAMESTATE->m_CurStyle = STYLE_PUMP_SINGLE;		break;
		case GAME_EZ2:		GAMESTATE->m_CurStyle = STYLE_EZ2_SINGLE;		break;
		case GAME_PARA:		GAMESTATE->m_CurStyle = STYLE_PARA_SINGLE;		break;
		case GAME_DS3DDX:	GAMESTATE->m_CurStyle = STYLE_DS3DDX_SINGLE;	break;
		case GAME_BM:		GAMESTATE->m_CurStyle = STYLE_BM_SINGLE;		break;
		case GAME_MANIAX:	GAMESTATE->m_CurStyle = STYLE_MANIAX_SINGLE;	break;
		default: ASSERT(0); // we should cover all gametypes....
	}

	SMLoader smfile;
	smfile.LoadFromSMFile( THEME->GetCurThemeDir() + STEPFILE, m_Song, false );
	ASSERT( m_Song.m_apNotes.size() == 1 );
	m_Song.m_apNotes[0]->GetNoteData(&m_NoteData);

	GAMESTATE->m_pCurSong = &m_Song;
	GAMESTATE->m_bPastHereWeGo = true;
	GAMESTATE->m_PlayerController[PLAYER_1] = PC_AUTOPLAY;

	if( USEPLAYER )
	{
		m_pPlayer = new Player;
		m_pPlayer->Load( PLAYER_1, &m_NoteData, m_pLifeMeterBar, NULL, NULL, NULL, NULL, NULL );
		m_pPlayer->SetX( PLAYERX );
		this->AddChild( m_pPlayer );
	}

	// deferred until after the player, so the notes go under it
	if( m_pLifeMeterBar )
		this->AddChild( m_pLifeMeterBar );

	// Don't show judgement
	GAMESTATE->m_PlayerOptions[PLAYER_1].m_fBlind = 1;
	GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
	GAMESTATE->m_bDemonstrationOrJukebox = true;

	m_fFakeSecondsIntoSong = 0;
	this->ClearMessageQueue();
	this->PostScreenMessage( SM_BeginFadingOut, SECONDS_TO_SHOW );

	this->MoveToTail( &m_Overlay );
	this->MoveToTail( &m_In );
	this->MoveToTail( &m_Out );
}

ScreenHowToPlay::~ScreenHowToPlay()
{
	SAFE_DELETE(m_pLifeMeterBar);
	SAFE_DELETE(m_pmCharacter);
	SAFE_DELETE(m_pmDancePad);
	SAFE_DELETE(m_pPlayer);
}

void ScreenHowToPlay::Step( float fDelta )
{
#define ST_LEFT		0x08
#define ST_DOWN		0x04
#define ST_UP		0x02
#define ST_RIGHT	0x01
#define ST_JUMPLR	(ST_LEFT | ST_RIGHT)
#define ST_JUMPUD	(ST_UP | ST_DOWN)

	float rate = 1; //GAMESTATE->m_fCurBPS;

	int iStep = 0;
	int iNoteRow = BeatToNoteRowNotRounded( GAMESTATE->m_fSongBeat + 0.6f );
	int iNumTracks = m_NoteData.GetNumTracks();
	// if we want to miss from here on out, don't process steps.
	if((m_iPerfects < m_iNumPerfects) && (m_NoteData.IsThereATapAtRow( iNoteRow )))
	{
		for( int k=0; k<iNumTracks; k++ )
			if( m_NoteData.GetTapNote(k, iNoteRow ) == TAP_TAP )
				iStep += 1 << (iNumTracks - (k + 1));

		switch( iStep )
		{
		case ST_LEFT:	m_pmCharacter->PlayAnimation( "Step-LEFT", 1.8f ); break;
		case ST_RIGHT:	m_pmCharacter->PlayAnimation( "Step-RIGHT", 1.8f ); break;
		case ST_UP:		m_pmCharacter->PlayAnimation( "Step-UP", 1.8f ); break;
		case ST_DOWN:	m_pmCharacter->PlayAnimation( "Step-DOWN", 1.8f ); break;
		case ST_JUMPLR: m_pmCharacter->PlayAnimation( "Step-JUMPLR", 1.8f ); break;
		case ST_JUMPUD:
			// Until I can get an UP+DOWN jump animation, this will have to do.
			m_pmCharacter->PlayAnimation( "Step-JUMPLR", 1.8f );
			
			m_pmCharacter->StopTweening();
			m_pmCharacter->BeginTweening( GAMESTATE->m_fCurBPS /8, TWEEN_LINEAR );
			m_pmCharacter->SetRotationY( 90 );
			m_pmCharacter->BeginTweening( (1/(GAMESTATE->m_fCurBPS * 2) ) ); //sleep between jump-frames
			m_pmCharacter->BeginTweening( GAMESTATE->m_fCurBPS /6, TWEEN_LINEAR );
			m_pmCharacter->SetRotationY( 0 );
			break;
		}
	}

	// if we want to freeze, freeze.
	if( GAMESTATE->m_bFreeze )
		rate = 0;

	m_pmCharacter->Update( fDelta * rate );
}

void ScreenHowToPlay::Update( float fDelta )
{
	if(GAMESTATE->m_pCurSong != NULL)
	{
		GAMESTATE->UpdateSongPosition( m_fFakeSecondsIntoSong );
		m_fFakeSecondsIntoSong += fDelta;

		static int iLastNoteRowCounted = 0;
		int iCurNoteRow = BeatToNoteRowNotRounded( GAMESTATE->m_fSongBeat );

		if(( iCurNoteRow != iLastNoteRowCounted ) &&(m_NoteData.IsThereATapAtRow( iCurNoteRow )))
		{
			m_iPerfects++;
			iLastNoteRowCounted = iCurNoteRow;
		}

		// we want misses from beat 22.8 on out, so change to a HUMAN controller.
		// since we aren't taking input from the user, the steps are always missed.
		if(m_iPerfects > m_iNumPerfects)
			GAMESTATE->m_PlayerController[PLAYER_1] = PC_HUMAN;

		if ( m_pmCharacter )
		{
			Step( fDelta );
		}
	}

	if( m_pmDancePad )
		m_pmDancePad->Update( fDelta );

	ScreenAttract::Update( fDelta );
}

void ScreenHowToPlay::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToNextScreen:
		GAMESTATE->Reset();
		break;
	}
	ScreenAttract::HandleScreenMessage( SM );
}

void ScreenHowToPlay::DrawPrimitives()
{
	Screen::DrawPrimitives();

	if( m_pmDancePad || m_pmCharacter )
	{
		DISPLAY->SetLighting( true );
		DISPLAY->SetLightDirectional( 
			0, 
			RageColor(0.5,0.5,0.5,1), 
			RageColor(1,1,1,1),
			RageColor(0,0,0,1),
			RageVector3(0, 0, 1) );

		if( m_pmCharacter )
			m_pmCharacter->Draw();
		if( m_pmDancePad )
			m_pmDancePad->Draw();

		DISPLAY->SetLightOff( 0 );
		DISPLAY->SetLighting( false );
	}

	m_Overlay.DrawPrimitives();
	m_In.DrawPrimitives();
	m_Out.DrawPrimitives();
}
