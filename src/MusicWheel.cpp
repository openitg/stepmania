#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: MusicWheel

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "MusicWheel.h"
#include "RageUtil.h"
#include "SongManager.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "RageSoundManager.h"
#include "ScreenManager.h"	// for sending SM_PlayMusicSample
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include <math.h>
#include "ThemeManager.h"


// WheelItem stuff
#define ICON_X			THEME->GetMetricF("WheelItemDisplay","IconX")
#define SONG_NAME_X		THEME->GetMetricF("WheelItemDisplay","SongNameX")
#define SECTION_NAME_X	THEME->GetMetricF("WheelItemDisplay","SectionNameX")
#define SECTION_ZOOM	THEME->GetMetricF("WheelItemDisplay","SectionZoom")
#define ROULETTE_X		THEME->GetMetricF("WheelItemDisplay","RouletteX")
#define ROULETTE_ZOOM	THEME->GetMetricF("WheelItemDisplay","RouletteZoom")
#define COURSE_X		THEME->GetMetricF("WheelItemDisplay","CourseX")
#define COURSE_ZOOM		THEME->GetMetricF("WheelItemDisplay","CourseZoom")
#define GRADE_X( p )	THEME->GetMetricF("WheelItemDisplay",ssprintf("GradeP%dX",p+1))
#define DEFAULT_SCROLL_DIRECTION		THEME->GetMetricI("Notes","DefaultScrollDirection")

// MusicWheel stuff
#define FADE_SECONDS				THEME->GetMetricF("MusicWheel","FadeSeconds")
#define SWITCH_SECONDS				THEME->GetMetricF("MusicWheel","SwitchSeconds")
#define ROULETTE_SWITCH_SECONDS		THEME->GetMetricF("MusicWheel","RouletteSwitchSeconds")
#define FAST_SPIN_SWITCH_SECONDS	0.05 // THEME->GetMetricF("MusicWheel","RouletteSwitchSeconds")
#define ROULETTE_SLOW_DOWN_SWITCHES	THEME->GetMetricI("MusicWheel","RouletteSlowDownSwitches")
#define LOCKED_INITIAL_VELOCITY		THEME->GetMetricF("MusicWheel","LockedInitialVelocity")
#define SCROLL_BAR_X				THEME->GetMetricF("MusicWheel","ScrollBarX")
#define SCROLL_BAR_HEIGHT			THEME->GetMetricI("MusicWheel","ScrollBarHeight")
#define ITEM_SPACING_Y				THEME->GetMetricF("MusicWheel","ItemSpacingY")
#define NUM_SECTION_COLORS			THEME->GetMetricI("MusicWheel","NumSectionColors")
#define SECTION_COLORS( i )			THEME->GetMetricC("MusicWheel",ssprintf("SectionColor%d",i+1))

float g_fItemSpacingY;	// cache

inline RageColor GetNextSectionColor() {
	static int i=0;
	i = i % NUM_SECTION_COLORS;
	return SECTION_COLORS(i++);
}


WheelItemData::WheelItemData( WheelItemType wit, Song* pSong, const CString &sSectionName, Course* pCourse, const RageColor color )
{
	m_WheelItemType = wit;
	m_pSong = pSong;
	m_sSectionName = sSectionName;
	m_pCourse = pCourse;
	m_color = color;
	m_IconType = MusicStatusDisplay::none;
}


WheelItemDisplay::WheelItemDisplay()
{
	data = NULL;

	m_fPercentGray = 0;
	m_MusicStatusDisplay.SetXY( ICON_X, 0 );
	
	m_TextBanner.SetHorizAlign( align_left );
	m_TextBanner.SetXY( SONG_NAME_X, 0 );

	m_sprSongBar.Load( THEME->GetPathTo("Graphics","select music song bar") );
	m_sprSongBar.SetXY( 0, 0 );

	m_sprSectionBar.Load( THEME->GetPathTo("Graphics","select music section bar") );
	m_sprSectionBar.SetXY( 0, 0 );

	m_textSectionName.LoadFromFont( THEME->GetPathTo("Fonts","musicwheel section") );
	m_textSectionName.TurnShadowOff();
	m_textSectionName.SetVertAlign( align_middle );
	m_textSectionName.SetXY( SECTION_NAME_X, 0 );
	m_textSectionName.SetZoom( SECTION_ZOOM );


	m_textRoulette.LoadFromFont( THEME->GetPathTo("Fonts","musicwheel roulette") );
	m_textRoulette.TurnShadowOff();
	m_textRoulette.TurnRainbowOn();
	m_textRoulette.SetZoom( ROULETTE_ZOOM );
	m_textRoulette.SetXY( ROULETTE_X, 0 );

	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		m_GradeDisplay[p].SetZoom( 1.0f );
		m_GradeDisplay[p].SetXY( GRADE_X(p), 0 );
	}

	m_textCourse.LoadFromFont( THEME->GetPathTo("Fonts","musicwheel course") );
	m_textCourse.TurnShadowOff();
	m_textCourse.SetZoom( COURSE_ZOOM );
	m_textCourse.SetHorizAlign( align_left );
	m_textCourse.SetXY( COURSE_X, 0 );
}


void WheelItemDisplay::LoadFromWheelItemData( WheelItemData* pWID )
{
	ASSERT( pWID != NULL );
	
	
	
	data = pWID;
	/*
	// copy all data items
	this->m_WheelItemType	= pWID->m_WheelItemType;
	this->m_sSectionName	= pWID->m_sSectionName;
	this->m_pCourse			= pWID->m_pCourse;
	this->m_pSong			= pWID->m_pSong;
	this->m_color			= pWID->m_color;
	this->m_IconType		= pWID->m_IconType; */


	// init type specific stuff
	switch( pWID->m_WheelItemType )
	{
	case TYPE_SECTION:
	case TYPE_COURSE:
		{
			CString sDisplayName;
			BitmapText *bt;
			if(pWID->m_WheelItemType == TYPE_SECTION)
			{
				sDisplayName = SONGMAN->ShortenGroupName(data->m_sSectionName);
				bt = &m_textSectionName;
			}
			else
			{
				sDisplayName = data->m_pCourse->m_sName;
				bt = &m_textCourse;
			}
			bt->SetZoom( 1 );
			bt->SetText( sDisplayName );
			bt->SetDiffuse( data->m_color );
			bt->TurnRainbowOff();

			float fSourcePixelWidth = (float)bt->GetWidestLineWidthInSourcePixels();
			float fMaxTextWidth = 200;
			if( fSourcePixelWidth > fMaxTextWidth  )
				bt->SetZoomX( fMaxTextWidth / fSourcePixelWidth );
		}
		break;
	case TYPE_SONG:
		{
			m_TextBanner.LoadFromSong( data->m_pSong );
			m_TextBanner.SetDiffuse( data->m_color );
			m_MusicStatusDisplay.SetType( data->m_IconType );
			RefreshGrades();
		}
		break;
	case TYPE_ROULETTE:
		m_textRoulette.SetText( "ROULETTE" );
		break;

	case TYPE_RANDOM:
		m_textRoulette.SetText( "RANDOM" );
		break;

	default:
		ASSERT( 0 );	// invalid type
	}
}

void WheelItemDisplay::RefreshGrades()
{
	// Refresh Grades
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled( (PlayerNumber)p ) )
		{
			m_GradeDisplay[p].SetDiffuse( RageColor(1,1,1,0) );
			continue;
		}

		if( data->m_pSong )	// this is a song display
		{
			if( data->m_pSong == GAMESTATE->m_pCurSong )
			{
				Notes* pNotes = GAMESTATE->m_pCurNotes[p];
				m_GradeDisplay[p].SetGrade( (PlayerNumber)p, pNotes ? pNotes->m_TopGrade : GRADE_NO_DATA );
			}
			else
			{
				const Difficulty dc = GAMESTATE->m_PreferredDifficulty[p];
				const Grade grade = data->m_pSong->GetGradeForDifficulty( GAMESTATE->GetCurrentStyleDef(), p, dc );
				m_GradeDisplay[p].SetGrade( (PlayerNumber)p, grade );
			}
		}
		else	// this is a section display
		{
			m_GradeDisplay[p].SetGrade( (PlayerNumber)p, GRADE_NO_DATA );
		}
	}

}


void WheelItemDisplay::Update( float fDeltaTime )
{
	Actor::Update( fDeltaTime );

	switch( data->m_WheelItemType )
	{
	case TYPE_SECTION:
		m_sprSectionBar.Update( fDeltaTime );
		m_textSectionName.Update( fDeltaTime );
		break;
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
		m_sprSectionBar.Update( fDeltaTime );
		m_textRoulette.Update( fDeltaTime );
		break;
	case TYPE_SONG:
		{
			m_sprSongBar.Update( fDeltaTime );
			m_MusicStatusDisplay.Update( fDeltaTime );
			m_TextBanner.Update( fDeltaTime );
			for( int p=0; p<NUM_PLAYERS; p++ )
				m_GradeDisplay[p].Update( fDeltaTime );
		}
		break;
	case TYPE_COURSE:
		m_sprSongBar.Update( fDeltaTime );
		m_textCourse.Update( fDeltaTime );
		break;
	default:
		ASSERT(0);
	}
}

void WheelItemDisplay::DrawPrimitives()
{
	Sprite *bar = NULL;
	switch( data->m_WheelItemType )
	{
	case TYPE_SECTION: 
	case TYPE_ROULETTE:
	case TYPE_RANDOM: bar = &m_sprSectionBar; break;
	case TYPE_SONG:		
	case TYPE_COURSE: bar = &m_sprSongBar; break;
	default: ASSERT(0);
	}
	
	bar->Draw();

	switch( data->m_WheelItemType )
	{
	case TYPE_SECTION:
		m_textSectionName.Draw();
		break;
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
		m_textRoulette.Draw();
		break;
	case TYPE_SONG:		
		m_TextBanner.Draw();
		m_MusicStatusDisplay.Draw();
		int p;
		for( p=0; p<NUM_PLAYERS; p++ )
			m_GradeDisplay[p].Draw();
		break;
	case TYPE_COURSE:
		m_textCourse.Draw();
		break;
	default:
		ASSERT(0);
	}

	if( m_fPercentGray > 0 )
	{
		bar->SetGlow( RageColor(0,0,0,m_fPercentGray) );
		bar->SetDiffuse( RageColor(0,0,0,0) );
		bar->Draw();
		bar->SetDiffuse( RageColor(0,0,0,1) );
		bar->SetGlow( RageColor(0,0,0,0) );
	}
}

MusicWheel::MusicWheel() 
{ 
	LOG->Trace( "MusicWheel::MusicWheel()" );


	if(DEFAULT_SCROLL_DIRECTION && GAMESTATE->m_pCurSong == NULL) /* check the song is null... incase they have just come back from a song and changed their PlayerOptions */
	{
		for(int i=0; i<NUM_PLAYERS; i++)
			GAMESTATE->m_PlayerOptions[i].m_bReverseScroll = true;
	}


	// update theme metric cache
	g_fItemSpacingY = ITEM_SPACING_Y;


	// for debugging
	if( GAMESTATE->m_CurStyle == STYLE_NONE )
		GAMESTATE->m_CurStyle = STYLE_DANCE_SINGLE;

	m_sprSelectionOverlay.Load( THEME->GetPathTo("Graphics","select music song highlight") );
	m_sprSelectionOverlay.SetXY( 0, 0 );
	m_sprSelectionOverlay.SetDiffuse( RageColor(1,1,1,1) );
	m_sprSelectionOverlay.SetEffectGlowing( 1.0f, RageColor(1,1,1,0.4f), RageColor(1,1,1,1) );
	AddChild( &m_sprSelectionOverlay );

	m_ScrollBar.SetX( SCROLL_BAR_X ); 
	m_ScrollBar.SetBarHeight( SCROLL_BAR_HEIGHT ); 
	this->AddChild( &m_ScrollBar );
	
	m_soundChangeMusic.Load(	THEME->GetPathTo("Sounds","select music change music") );
	m_soundChangeSort.Load(		THEME->GetPathTo("Sounds","select music change sort") );
	m_soundExpand.Load(			THEME->GetPathTo("Sounds","select music section expand") );
	m_soundStart.Load(			THEME->GetPathTo("Sounds","menu start") );
	m_soundLocked.Load(			THEME->GetPathTo("Sounds","select music wheel locked") );


	// init m_mapGroupNameToBannerColor

	vector<Song*> arraySongs = SONGMAN->m_pSongs;
	SortSongPointerArrayByGroup( arraySongs );
	
	m_iSelection = 0;

	m_WheelState = STATE_SELECTING_MUSIC;
	m_fTimeLeftInState = 0;
	m_fPositionOffsetFromSelection = 0;

	m_iSwitchesLeftInSpinDown = 0;
	
	if( GAMESTATE->IsExtraStage()  ||  GAMESTATE->IsExtraStage2() )
	{
		// make the preferred group the group of the last song played.
		if( GAMESTATE->m_sPreferredGroup == "ALL MUSIC" )
			GAMESTATE->m_sPreferredGroup = GAMESTATE->m_pCurSong->m_sGroupName;

		Song* pSong;
		Notes* pNotes;
		PlayerOptions po;
		SongOptions so;
		SONGMAN->GetExtraStageInfo(
			GAMESTATE->IsExtraStage2(),
			GAMESTATE->m_sPreferredGroup,
			GAMESTATE->GetCurrentStyleDef(),
			pSong,
			pNotes,
			po,
			so );
		GAMESTATE->m_pCurSong = pSong;
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			if( GAMESTATE->IsPlayerEnabled(p) )
			{
				GAMESTATE->m_pCurNotes[p] = pNotes;
				GAMESTATE->m_PlayerOptions[p] = po;
			}
		}
		GAMESTATE->m_SongOptions = so;
	}

	/* Build all of the wheel item data.  Do tihs after selecting
	 * the extra stage, so it knows to always display it. */
	for( int so=0; so<NUM_SORT_ORDERS; so++ )
		BuildWheelItemDatas( m_WheelItemDatas[so], SongSortOrder(so) );
	BuildWheelItemDatas( m_WheelItemDatas[SORT_ROULETTE], SongSortOrder(SORT_ROULETTE) );

	// If there is no currently selected song, select one.
	if( GAMESTATE->m_pCurSong == NULL )
	{
		CStringArray asGroupNames;
		SONGMAN->GetGroupNames( asGroupNames );
		if( !asGroupNames.empty() )
		{
			/* XXX: Do groups get added if they're empty?
			 * This will select songs we can't use; we want the first song
			 * that's actually visible.  Should we select out of wheel data? 
			 * -glenn */
			vector<Song*> arraySongs;
			SONGMAN->GetSongsInGroup( asGroupNames[0], arraySongs );
			if( !arraySongs.empty() ) // still nothing selected
				GAMESTATE->m_pCurSong = arraySongs[0];	// select the first song
		}
	}


	// Select the the previously selected song (if any)
	bool selected = SelectSong(GAMESTATE->m_pCurSong);
	// Select the the previously selected course (if any)
	if(!selected) selected = SelectCourse(GAMESTATE->m_pCurCourse);
	if(!selected) SetOpenGroup("");

	// rebuild the WheelItems that appear on screen
	RebuildWheelItemDisplays();
}

MusicWheel::~MusicWheel()
{
}

bool MusicWheel::SelectSong( const Song *p )
{
	if(p == NULL)
		return false;

	unsigned i;
	vector<WheelItemData> &from = m_WheelItemDatas[GAMESTATE->m_SongSortOrder];
	for( i=0; i<from.size(); i++ )
	{
		if( from[i].m_pSong == p )
		{
			// make its group the currently expanded group
			SetOpenGroup(from[i].m_sSectionName);
			break;
		}
	}

	if(i == from.size())
		return false;

	for( i=0; i<m_CurWheelItemData.size(); i++ )
	{
		if( m_CurWheelItemData[i]->m_pSong == p )
			m_iSelection = i;		// select it
	}
	return true;
}

bool MusicWheel::SelectCourse( const Course *p )
{
	if(p == NULL)
		return false;

	unsigned i;
	vector<WheelItemData> &from = m_WheelItemDatas[GAMESTATE->m_SongSortOrder];
	for( i=0; i<from.size(); i++ )
	{
		if( from[i].m_pCourse == p )
		{
			// make its group the currently expanded group
			SetOpenGroup(from[i].m_sSectionName);
			break;
		}
	}

	if(i == from.size())
		return false;

	for( i=0; i<m_CurWheelItemData.size(); i++ )
	{
		if( m_CurWheelItemData[i]->m_pCourse == p )
			m_iSelection = i;		// select it
	}

	return true;
}

void MusicWheel::GetSongList(vector<Song*> &arraySongs, bool bRoulette )
{
	// copy only songs that have at least one Notes for the current GameMode
	for( unsigned i=0; i<SONGMAN->m_pSongs.size(); i++ )
	{
		Song* pSong = SONGMAN->m_pSongs[i];

		/* If we're on an extra stage, and this song is selected, ignore
			* #SELECTABLE. */
		if( pSong != GAMESTATE->m_pCurSong || 
			(!GAMESTATE->IsExtraStage() && !GAMESTATE->IsExtraStage2()) ) {
			/* Hide songs that asked to be hidden via #SELECTABLE. */
			if( !bRoulette && !pSong->NormallyDisplayed() )
				continue;
			if( bRoulette && !pSong->RouletteDisplayed() )
				continue;
		}

		vector<Notes*> arraySteps;
		pSong->GetNotesThatMatch( GAMESTATE->GetCurrentStyleDef()->m_NotesType, arraySteps );

		if( !arraySteps.empty() )
			arraySongs.push_back( pSong );
	}
}

void MusicWheel::BuildWheelItemDatas( vector<WheelItemData> &arrayWheelItemDatas, SongSortOrder so )
{
	unsigned i;

	if(so == SongSortOrder(SORT_ROULETTE) && GAMESTATE->m_PlayMode != PLAY_MODE_ARCADE)
		return; /* only used in arcade */

	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_ARCADE:
		{
			///////////////////////////////////
			// Make an array of Song*, then sort them
			///////////////////////////////////
			vector<Song*> arraySongs;
			
			GetSongList(arraySongs, so == SORT_ROULETTE);

			// sort the songs
			switch( so )
			{
			case SORT_GROUP:
			case SORT_ROULETTE:
				SortSongPointerArrayByGroup( arraySongs );
				break;
			case SORT_TITLE:
				SortSongPointerArrayByTitle( arraySongs );
				break;
			case SORT_BPM:
				SortSongPointerArrayByBPM( arraySongs );
				break;
		//	case SORT_ARTIST:
		//		SortSongPointerArrayByArtist( arraySongs );
		//		break;
			case SORT_MOST_PLAYED:
				SortSongPointerArrayByMostPlayed( arraySongs );
				if( arraySongs.size() > 30 )
					arraySongs.erase(arraySongs.begin()+30, arraySongs.end());
				break;
			default:
				ASSERT(0);	// unhandled SortOrder
			}



			///////////////////////////////////
			// Build an array of WheelItemDatas from the sorted list of Song*'s
			///////////////////////////////////
			arrayWheelItemDatas.clear();	// clear out the previous wheel items 

			bool bUseSections = false;
			switch( so )
			{
			case SORT_MOST_PLAYED:	bUseSections = false;	break;
			case SORT_BPM:			bUseSections = false;	break;
			case SORT_GROUP:		bUseSections = GAMESTATE->m_sPreferredGroup == "ALL MUSIC";	break;
			case SORT_TITLE:		bUseSections = true;	break;
			case SORT_ROULETTE:		bUseSections = false;	break;
			default:		ASSERT( false );
			}

			if( !PREFSMAN->m_bMusicWheelUsesSections )
				bUseSections = false;

			if( bUseSections )
			{
				// make WheelItemDatas with sections
				CString sLastSection = "";
				RageColor colorSection;
				for( unsigned i=0; i< arraySongs.size(); i++ )
				{
					Song* pSong = arraySongs[i];
					CString sThisSection = GetSectionNameFromSongAndSort( pSong, so );
					int iSectionColorIndex = 0;

					if( GAMESTATE->m_sPreferredGroup != "ALL MUSIC"  &&  pSong->m_sGroupName != GAMESTATE->m_sPreferredGroup )
							continue;
					if( sThisSection != sLastSection)	// new section, make a section item
					{
						colorSection = (so==SORT_GROUP) ? SONGMAN->GetGroupColor(pSong->m_sGroupName) : SECTION_COLORS(iSectionColorIndex);
						iSectionColorIndex = (iSectionColorIndex+1) % NUM_SECTION_COLORS;
						arrayWheelItemDatas.push_back( WheelItemData(TYPE_SECTION, NULL, sThisSection, NULL, colorSection) );
						sLastSection = sThisSection;
					}

					arrayWheelItemDatas.push_back( WheelItemData( TYPE_SONG, pSong, sThisSection, NULL, SONGMAN->GetSongColor(pSong)) );
				}
			}
			else
			{
				for( unsigned i=0; i<arraySongs.size(); i++ )
				{
					Song* pSong = arraySongs[i];
					if( GAMESTATE->m_sPreferredGroup != "ALL MUSIC"  &&  pSong->m_sGroupName != GAMESTATE->m_sPreferredGroup )
						continue;	// skip
					arrayWheelItemDatas.push_back( WheelItemData(TYPE_SONG, pSong, "", NULL, SONGMAN->GetSongColor(pSong)) );
				}
			}
		}


		if( so != SORT_ROULETTE )
		{
			arrayWheelItemDatas.push_back( WheelItemData(TYPE_ROULETTE, NULL, "", NULL, RageColor(1,0,0,1)) );
			arrayWheelItemDatas.push_back( WheelItemData(TYPE_RANDOM, NULL, "", NULL, RageColor(1,0,0,1)) );
		}

		// HACK:  Add extra stage item if it isn't already present on the music wheel
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
		{
			Song* pSong;
			Notes* pNotes;
			PlayerOptions po;
			SongOptions so;
			SONGMAN->GetExtraStageInfo( GAMESTATE->IsExtraStage2(), GAMESTATE->m_pCurSong->m_sGroupName, GAMESTATE->GetCurrentStyleDef(), pSong, pNotes, po, so );
			
			bool bFoundExtraSong = false;

			for( unsigned i=0; i<arrayWheelItemDatas.size(); i++ )
			{
				if( arrayWheelItemDatas[i].m_pSong == pSong )
				{
					bFoundExtraSong = true;
					break;
				}
			}
			
			if( !bFoundExtraSong )
				arrayWheelItemDatas.push_back( WheelItemData(TYPE_SONG, pSong, "", NULL, GAMESTATE->GetStageColor()) );
		}

		break;
	case PLAY_MODE_ONI:
	case PLAY_MODE_ENDLESS:
		{
			unsigned i;

			vector<Course*> apCourses;
			switch( GAMESTATE->m_PlayMode )
			{
			case PLAY_MODE_ONI:
				for( i=0; i<SONGMAN->m_aOniCourses.size(); i++ )
					apCourses.push_back( &SONGMAN->m_aOniCourses[i] );
				SortCoursePointerArrayByDifficulty( apCourses );
				break;
			case PLAY_MODE_ENDLESS:
				for( i=0; i<SONGMAN->m_aEndlessCourses.size(); i++ )
					apCourses.push_back( &SONGMAN->m_aEndlessCourses[i] );
				break;
			}

			for( unsigned c=0; c<apCourses.size(); c++ )	// foreach course
			{
				Course* pCourse = apCourses[c];

				// check that this course has at least one song playable in the current style
				vector<Song*> apSongs;
				vector<Notes*> apNotes;
				CStringArray asModifiers;
				pCourse->GetSongAndNotesForCurrentStyle( apSongs, apNotes, asModifiers, false );

				if( !apNotes.empty() )
					arrayWheelItemDatas.push_back( WheelItemData(TYPE_COURSE, NULL, "", pCourse, pCourse->GetColor()) );
			}
		}
		break;
	default:
		ASSERT(0);	// invalid PlayMode
	}

	// init crowns
	for( i=0; i<arrayWheelItemDatas.size(); i++ )
	{
		Song* pSong = arrayWheelItemDatas[i].m_pSong;
		if( pSong == NULL )
			continue;

		bool bIsEasy = pSong->IsEasy( GAMESTATE->GetCurrentStyleDef()->m_NotesType ); 
		WheelItemData& WID = arrayWheelItemDatas[i];
		WID.m_IconType = bIsEasy ? MusicStatusDisplay::easy : MusicStatusDisplay::none;
	}

	if( so == SORT_MOST_PLAYED )
	{
		// init crown icons 
		for( i=0; i< min(3u,arrayWheelItemDatas.size()); i++ )
		{
			WheelItemData& WID = arrayWheelItemDatas[i];
			WID.m_IconType = MusicStatusDisplay::IconType(MusicStatusDisplay::crown1 + i);
		}
	}

	if( arrayWheelItemDatas.empty() )
	{
		arrayWheelItemDatas.push_back( WheelItemData(TYPE_SECTION, NULL, "- EMPTY -", NULL, RageColor(1,0,0,1)) );
	}
}

float MusicWheel::GetBannerY( float fPosOffsetsFromMiddle )
{
	return roundf( fPosOffsetsFromMiddle*g_fItemSpacingY );
}

float MusicWheel::GetBannerX( float fPosOffsetsFromMiddle )
{	
	float fX = (1-cosf((fPosOffsetsFromMiddle)/3))*95.0f;
	
	return roundf( fX );
}

void MusicWheel::RebuildWheelItemDisplays()
{
	// rewind to first index that will be displayed;
	int iIndex = m_iSelection;
	if( m_iSelection > int(m_CurWheelItemData.size()-1) )
		m_iSelection = 0;
	
	iIndex -= NUM_WHEEL_ITEMS_TO_DRAW/2;
	while(iIndex < 0)
		iIndex += m_CurWheelItemData.size();

	// iIndex is now the index of the lowest WheelItem to draw
	for( int i=0; i<NUM_WHEEL_ITEMS_TO_DRAW; i++ )
	{
		WheelItemData     *data   = m_CurWheelItemData[iIndex];
		WheelItemDisplay& display = m_WheelItemDisplays[i];

		display.LoadFromWheelItemData( data );

		// increment iIndex
		iIndex++;
		if( iIndex > int(m_CurWheelItemData.size()-1) )
			iIndex = 0;
	}

}

void MusicWheel::NotesChanged( PlayerNumber pn )	// update grade graphics and top score
{
	for( int i=0; i<NUM_WHEEL_ITEMS_TO_DRAW; i++ )
	{
		WheelItemDisplay& display = m_WheelItemDisplays[i];
		display.RefreshGrades();
	}
}



void MusicWheel::DrawPrimitives()
{
	for( int i=0; i<NUM_WHEEL_ITEMS_TO_DRAW; i++ )
	{
		WheelItemDisplay& display = m_WheelItemDisplays[i];

		switch( m_WheelState )
		{
		case STATE_SELECTING_MUSIC:
		case STATE_ROULETTE_SPINNING:
		case STATE_ROULETTE_SLOWING_DOWN:
		case STATE_RANDOM_SPINNING:
		case STATE_LOCKED:
			{
				float fThisBannerPositionOffsetFromSelection = i - NUM_WHEEL_ITEMS_TO_DRAW/2 + m_fPositionOffsetFromSelection;

				float fY = GetBannerY( fThisBannerPositionOffsetFromSelection );
				if( fY < -SCREEN_HEIGHT/2  ||  fY > SCREEN_HEIGHT/2 )
					continue; // skip

				float fX = GetBannerX( fThisBannerPositionOffsetFromSelection );
				display.SetXY( fX, fY );
			}
			break;
		}

		if( m_WheelState == STATE_LOCKED  &&  i != NUM_WHEEL_ITEMS_TO_DRAW/2 )
			display.m_fPercentGray = 0.5f;
		else
			display.m_fPercentGray = 0;

		display.Draw();
	}

	ActorFrame::DrawPrimitives();
}

void MusicWheel::UpdateScrollbar()
{
	int total_num_items = m_CurWheelItemData.size();
	float item_at=m_iSelection - m_fPositionOffsetFromSelection;

	if(NUM_WHEEL_ITEMS_TO_DRAW > total_num_items) {
		m_ScrollBar.SetPercentage( 0, 1 );
	} else {
		float size = float(NUM_WHEEL_ITEMS_TO_DRAW) / total_num_items;
		float center = item_at / total_num_items;
		size *= 0.5f;

		m_ScrollBar.SetPercentage( center - size, center + size );
	}
}

void MusicWheel::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	unsigned i;
	for( i=0; i<int(NUM_WHEEL_ITEMS_TO_DRAW); i++ )
	{
		WheelItemDisplay& display = m_WheelItemDisplays[i];

		display.Update( fDeltaTime );
	}

	UpdateScrollbar();

	if( m_Moving )
	{
		m_TimeBeforeMovingBegins -= fDeltaTime;
		m_TimeBeforeMovingBegins = max(m_TimeBeforeMovingBegins, 0);
		if(m_TimeBeforeMovingBegins == 0 && m_WheelState != STATE_LOCKED &&
			fabsf(m_fPositionOffsetFromSelection) < 0.5f)
		{
			if(m_WheelState == STATE_RANDOM_SPINNING && !m_iSwitchesLeftInSpinDown)
			{
//				m_fPositionOffsetFromSelection = max(m_fPositionOffsetFromSelection, 0.3f);
				m_Moving = 0;
				m_WheelState = STATE_LOCKED;
				m_soundStart.Play();
				m_fLockedWheelVelocity = 0;
				SCREENMAN->SendMessageToTopScreen( SM_SongChanged, 0 );
			}
			else {
				// spin as fast as possible
				if(m_Moving == 1) NextMusic();
				else PrevMusic();

				if(m_WheelState == STATE_RANDOM_SPINNING)
					m_iSwitchesLeftInSpinDown--;
			}
		}
	}

	// update wheel state
	m_fTimeLeftInState -= fDeltaTime;
	if( m_fTimeLeftInState <= 0 )	// time to go to a new state
	{
		switch( m_WheelState )
		{
		case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
			{
				Song* pPrevSelectedSong = m_CurWheelItemData[m_iSelection]->m_pSong;
				CString sPrevSelectedSection = m_CurWheelItemData[m_iSelection]->m_sSectionName;

				// change the sort order
				GAMESTATE->m_SongSortOrder = SongSortOrder( (GAMESTATE->m_SongSortOrder+1) % NUM_SORT_ORDERS );
				SCREENMAN->SendMessageToTopScreen( SM_SortOrderChanged, 0 );
				SetOpenGroup(GetSectionNameFromSongAndSort( pPrevSelectedSong, GAMESTATE->m_SongSortOrder ));

				//RebuildWheelItems();

				m_iSelection = 0;

				if( pPrevSelectedSong != NULL )		// the previous selected item was a song
					SelectSong(pPrevSelectedSong);
				else	// the previously selected item was a section
				{
					// find the previously selected song, and select it
					for( i=0; i<m_CurWheelItemData.size(); i++ )
					{
						if( m_CurWheelItemData[i]->m_sSectionName == sPrevSelectedSection )
						{
							m_iSelection = i;
							break;
						}
					}
				}

				// If changed sort to "BEST", put selection on most popular song
				if( GAMESTATE->m_SongSortOrder == SORT_MOST_PLAYED )
				{
					for( i=0; i<m_CurWheelItemData.size(); i++ )
					{
						if( m_CurWheelItemData[i]->m_pSong != NULL )
						{
							m_iSelection = i;
							break;
						}
					}
				}

				SCREENMAN->SendMessageToTopScreen( SM_SongChanged, 0 );
				RebuildWheelItemDisplays();
				TweenOnScreen(true);
				m_WheelState = STATE_FLYING_ON_AFTER_NEXT_SORT;
			}
			break;

		case STATE_FLYING_ON_AFTER_NEXT_SORT:
			m_WheelState = STATE_SELECTING_MUSIC;	// now, wait for input
			break;

		case STATE_TWEENING_ON_SCREEN:
			m_fTimeLeftInState = 0;
			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			{
//				if ( m_bUseRandomExtra )
//				{
//					SOUNDMAN->music->StopPlaying();
//					m_soundExpand.Play();
//					m_WheelState = STATE_ROULETTE_SPINNING;
//					m_SortOrder = SORT_GROUP;
//					m_MusicSortDisplay.SetDiffuse( RageColor(1,1,1,0) );
//					m_MusicSortDisplay.SetEffectNone();
//					BuildWheelItemDatas( m_WheelItemDatas[SORT_GROUP], SORT_GROUP, true );
//				}
//				else
				{
					m_WheelState = STATE_LOCKED;
					m_soundStart.Play();
					m_fLockedWheelVelocity = 0;
				}
			}
			else
			{
				m_WheelState = STATE_SELECTING_MUSIC;
			}
			break;
		case STATE_TWEENING_OFF_SCREEN:
			m_WheelState = STATE_WAITING_OFF_SCREEN;
			m_fTimeLeftInState = 0;
			break;
		case STATE_SELECTING_MUSIC:
			m_fTimeLeftInState = 0;
			break;
		case STATE_ROULETTE_SPINNING:
		case STATE_RANDOM_SPINNING:
			break;
		case STATE_WAITING_OFF_SCREEN:
			break;
		case STATE_LOCKED:
			break;
		case STATE_ROULETTE_SLOWING_DOWN:
			if( m_iSwitchesLeftInSpinDown == 0 )
			{
				m_WheelState = STATE_LOCKED;
				m_fTimeLeftInState = 0;
				m_soundStart.Play();
				m_fLockedWheelVelocity = 0;

				/* Send this again so the screen starts sample music. */
				SCREENMAN->SendMessageToTopScreen( SM_SongChanged, 0 );
			}
			else
			{
				m_iSwitchesLeftInSpinDown--;
				const float SwitchTimes[] = { 0.5f, 1.3f, 0.8f, 0.4f, 0.2f };
				ASSERT(m_iSwitchesLeftInSpinDown >= 0 && m_iSwitchesLeftInSpinDown <= 4);
				m_fTimeLeftInState = SwitchTimes[m_iSwitchesLeftInSpinDown];

				LOG->Trace( "m_iSwitchesLeftInSpinDown id %d, m_fTimeLeftInState is %f", m_iSwitchesLeftInSpinDown, m_fTimeLeftInState );

				if( m_iSwitchesLeftInSpinDown < 2 )
					randomf(0,1) >= 0.5f ? NextMusic() : PrevMusic();
				else
					NextMusic();
			}
			break;
		default:
			ASSERT(0);	// all state changes should be handled explitily
			break;
		}
	}


	if( m_WheelState == STATE_LOCKED )
	{
		/* Do this in at most .1 sec chunks, so we don't get weird if we
		 * stop for some reason (and so it behaves the same when being
		 * single stepped). */
		float tm = fDeltaTime;
		while(tm > 0)
		{
			float t = min(tm, 0.1f);
			tm -= t;

			m_fPositionOffsetFromSelection = clamp( m_fPositionOffsetFromSelection, -0.3f, +0.3f );

			float fSpringForce = - m_fPositionOffsetFromSelection * LOCKED_INITIAL_VELOCITY;
			m_fLockedWheelVelocity += fSpringForce;

			float fDrag = -m_fLockedWheelVelocity * t*4;
			m_fLockedWheelVelocity += fDrag;

			m_fPositionOffsetFromSelection  += m_fLockedWheelVelocity*t;

			if( fabsf(m_fPositionOffsetFromSelection) < 0.01f  &&  fabsf(m_fLockedWheelVelocity) < 0.01f )
			{
				m_fPositionOffsetFromSelection = 0;
				m_fLockedWheelVelocity = 0;
			}
		}
	}
	else
	{
		// "rotate" wheel toward selected song
		float fSpinSpeed;
		if( m_Moving && m_TimeBeforeMovingBegins == 0 )
			fSpinSpeed = m_SpinSpeed;
		else
			fSpinSpeed = 0.2f + fabsf(m_fPositionOffsetFromSelection)/SWITCH_SECONDS;

		if( m_fPositionOffsetFromSelection > 0 )
		{
			m_fPositionOffsetFromSelection -= fSpinSpeed*fDeltaTime;
			if( m_fPositionOffsetFromSelection < 0 )
				m_fPositionOffsetFromSelection = 0;
		}
		else if( m_fPositionOffsetFromSelection < 0 )
		{
			m_fPositionOffsetFromSelection += fSpinSpeed*fDeltaTime;
			if( m_fPositionOffsetFromSelection > 0 )
				m_fPositionOffsetFromSelection = 0;
		}
	}
}


void MusicWheel::PrevMusic()
{
	if( m_WheelState == STATE_LOCKED )
	{
		m_fLockedWheelVelocity = LOCKED_INITIAL_VELOCITY;
		m_soundLocked.Play();
		return;
	}

	if( fabsf(m_fPositionOffsetFromSelection) > 0.5f )	// wheel is busy spinning
		return;
	
	switch( m_WheelState )
	{
	case STATE_SELECTING_MUSIC:
	case STATE_ROULETTE_SPINNING:
	case STATE_ROULETTE_SLOWING_DOWN:
	case STATE_RANDOM_SPINNING:
		break;	// fall through
	default:
		return;	// don't fall through
	}

	// decrement m_iSelection
	m_iSelection--;
	if( m_iSelection < 0 )
		m_iSelection = m_CurWheelItemData.size()-1;

	RebuildWheelItemDisplays();

	m_fPositionOffsetFromSelection -= 1;

	m_soundChangeMusic.Play();

	SCREENMAN->SendMessageToTopScreen( SM_SongChanged, 0 );
}

void MusicWheel::NextMusic()
{
	if( m_WheelState == STATE_LOCKED )
	{
		m_fLockedWheelVelocity = -LOCKED_INITIAL_VELOCITY;
		m_soundLocked.Play();
		return;
	}

	if( fabsf(m_fPositionOffsetFromSelection) > 0.5f )	// wheel is very busy spinning
		return;
	
	switch( m_WheelState )
	{
	case STATE_SELECTING_MUSIC:
	case STATE_ROULETTE_SPINNING:
	case STATE_ROULETTE_SLOWING_DOWN:
	case STATE_RANDOM_SPINNING:
		break;
	default:
		LOG->Trace( "NextMusic() ignored" );
		return;	// don't continue
	}


	// increment m_iSelection
	m_iSelection++;
	if( m_iSelection > int(m_CurWheelItemData.size()-1) )
		m_iSelection = 0;

	RebuildWheelItemDisplays();

	m_fPositionOffsetFromSelection += 1;
	
	m_soundChangeMusic.Play();

	SCREENMAN->SendMessageToTopScreen( SM_SongChanged, 0 );
}

bool MusicWheel::PrevSort()
{
	return NextSort();
}

bool MusicWheel::NextSort()
{
	switch( m_WheelState )
	{
	case STATE_SELECTING_MUSIC:
	case STATE_FLYING_ON_AFTER_NEXT_SORT:
		break;	// fall through
	default:
		return false;	// don't continue
	}

	m_soundChangeSort.Play();

	TweenOffScreen(true);

	m_WheelState = STATE_FLYING_OFF_BEFORE_NEXT_SORT;
	return true;
}
bool MusicWheel::Select()	// return true of a playable item was chosen
{
	LOG->Trace( "MusicWheel::Select()" );

	if( m_WheelState == STATE_ROULETTE_SLOWING_DOWN )
		return false;

	if( m_WheelState == STATE_ROULETTE_SPINNING )
	{
		m_WheelState = STATE_ROULETTE_SLOWING_DOWN;
		m_Moving = 0;
		m_iSwitchesLeftInSpinDown = ROULETTE_SLOW_DOWN_SWITCHES/2+1 + rand()%(ROULETTE_SLOW_DOWN_SWITCHES/2);
		m_fTimeLeftInState = 0.1f;
		return false;
	}

	switch( m_CurWheelItemData[m_iSelection]->m_WheelItemType )
	{
	case TYPE_SECTION:
		{
		CString sThisItemSectionName = m_CurWheelItemData[m_iSelection]->m_sSectionName;
		if( m_sExpandedSectionName == sThisItemSectionName )	// already expanded
			m_sExpandedSectionName = "";		// collapse it
		else				// already collapsed
			m_sExpandedSectionName = sThisItemSectionName;	// expand it

		SetOpenGroup(m_sExpandedSectionName);
	
		RebuildWheelItemDisplays();


		m_soundExpand.Play();


		m_iSelection = 0;	// reset in case we can't find the last selected song
		// find the section header and select it
		for( unsigned  i=0; i<m_CurWheelItemData.size(); i++ )
		{
			if( m_CurWheelItemData[i]->m_WheelItemType == TYPE_SECTION  
				&&  m_CurWheelItemData[i]->m_sSectionName == sThisItemSectionName )
			{
				m_iSelection = i;
				break;
			}
		}

		}
		return false;
	case TYPE_ROULETTE:
		StartRoulette();
		return false;
	case TYPE_RANDOM:
		{
			const RANDOM_SPINS = 5;

			/* Grab all of the currently-visible data. */
			vector<WheelItemData *> &newData = m_CurWheelItemData;
			newData.clear();

			int i;

			/* Add RANDOM_SPINS * 2 worth of random items. */
			int total =  int(m_WheelItemDatas[SORT_ROULETTE].size());
			for(i = 0; i < total; ++i)
				swap(m_WheelItemDatas[SORT_ROULETTE][i], m_WheelItemDatas[SORT_ROULETTE][rand() % total]);

			for(i = 0; i < RANDOM_SPINS * 2; ++i)
				newData.push_back( &m_WheelItemDatas[SORT_ROULETTE][i] );

			/* The cursor is on m_WheelItemDisplays[NUM_WHEEL_ITEMS_TO_DRAW/2].
			 * Fill from 0 to NUM_WHEEL_ITEMS_TO_DRAW, so all of the items
			 * that were on the screen stay there.  However, skip some from
			 * the beginning to make sure we don't actually *land* on one. */
			int fillstart = max(NUM_WHEEL_ITEMS_TO_DRAW/2 - RANDOM_SPINS + 1, 0);

			for( i=fillstart; i<NUM_WHEEL_ITEMS_TO_DRAW; i++ )
				newData.push_back(m_WheelItemDisplays[i].data);

			m_iSelection = newData.size() - NUM_WHEEL_ITEMS_TO_DRAW/2 - 1;
			
			m_Moving = -1;
			m_TimeBeforeMovingBegins = 0;
			m_SpinSpeed = 1.0f/ROULETTE_SWITCH_SECONDS;
			m_iSwitchesLeftInSpinDown = RANDOM_SPINS;
			m_WheelState = STATE_RANDOM_SPINNING;
			RebuildWheelItemDisplays();
		}

		return false;

	case TYPE_SONG:
	default:
		
		return true;
	}
}

void MusicWheel::StartRoulette() 
{
	m_WheelState = STATE_ROULETTE_SPINNING;
	m_Moving = 1;
	m_TimeBeforeMovingBegins = 0;
	m_SpinSpeed = 1.0f/ROULETTE_SWITCH_SECONDS;
	SetOpenGroup("", SongSortOrder(SORT_ROULETTE));
}

void MusicWheel::SetOpenGroup(CString group, SongSortOrder so)
{
	if(so == NUM_SORT_ORDERS)
		so = GAMESTATE->m_SongSortOrder;

	m_sExpandedSectionName = group;

	WheelItemData *old = NULL;
	if(!m_CurWheelItemData.empty())
		old = m_CurWheelItemData[m_iSelection];

	m_CurWheelItemData.clear();
	vector<WheelItemData> &from = m_WheelItemDatas[so];
	unsigned i;
	for(i = 0; i < from.size(); ++i)
	{
		if((from[i].m_WheelItemType == TYPE_SONG ||
			from[i].m_WheelItemType == TYPE_COURSE) &&
		     !from[i].m_sSectionName.empty() &&
			 from[i].m_sSectionName != group)
			 continue;
		m_CurWheelItemData.push_back(&from[i]);

	}

	m_iSelection = 0;
	for(i = 0; i < m_CurWheelItemData.size(); ++i)
	{
		if(m_CurWheelItemData[i] == old)
			m_iSelection=i;
	}

	RebuildWheelItemDisplays();
}

bool MusicWheel::IsRouletting() const
{
	return m_WheelState == STATE_ROULETTE_SPINNING || m_WheelState == STATE_ROULETTE_SLOWING_DOWN ||
		   m_WheelState == STATE_RANDOM_SPINNING;
}

int MusicWheel::IsMoving() const
{
	if(!m_Moving) return false;
	return m_TimeBeforeMovingBegins == 0;
}

void MusicWheel::TweenOnScreen(bool changing_sort)
{
	float factor = 1.0f;
	if(changing_sort) factor = 0.25;

	m_WheelState = STATE_TWEENING_ON_SCREEN;

	float fX = GetBannerX(0), fY = GetBannerY(0);
	
	m_sprSelectionOverlay.SetXY( fX+320, fY );

	if(changing_sort) {
		m_sprSelectionOverlay.BeginTweening( 0.04f * NUM_WHEEL_ITEMS_TO_DRAW/2 * factor );	// sleep
		m_sprSelectionOverlay.BeginTweening( 0.2f * factor, Actor::TWEEN_BIAS_BEGIN );
	} else {
		m_sprSelectionOverlay.BeginTweening( 0.05f * factor );	// sleep
		m_sprSelectionOverlay.BeginTweening( 0.4f * factor, Actor::TWEEN_BIAS_BEGIN );
	}
	m_sprSelectionOverlay.SetTweenX( fX );

	m_ScrollBar.SetX( SCROLL_BAR_X+30 );
	if(changing_sort)
		m_ScrollBar.BeginTweening( 0.2f * factor );	// sleep
	else
		m_ScrollBar.BeginTweening( 0.7f * factor );	// sleep
	m_ScrollBar.BeginTweening( 0.2f * factor , Actor::TWEEN_BIAS_BEGIN );
	m_ScrollBar.SetTweenX( SCROLL_BAR_X );	

	for( int i=0; i<NUM_WHEEL_ITEMS_TO_DRAW; i++ )
	{
		WheelItemDisplay& display = m_WheelItemDisplays[i];
		float fThisBannerPositionOffsetFromSelection = i - NUM_WHEEL_ITEMS_TO_DRAW/2 + m_fPositionOffsetFromSelection;

		float fX = GetBannerX(fThisBannerPositionOffsetFromSelection);
		float fY = GetBannerY(fThisBannerPositionOffsetFromSelection);
		display.SetXY( fX+320, fY );
		display.BeginTweening( 0.04f*i * factor );	// sleep
		display.BeginTweening( 0.2f * factor, Actor::TWEEN_BIAS_BEGIN );
		display.SetTweenX( fX );
	}

	m_fTimeLeftInState = TweenTime() + 0.100f;
}
						   
void MusicWheel::TweenOffScreen(bool changing_sort)
{
	float factor = 1.0f;
	if(changing_sort) factor = 0.25;

	m_WheelState = STATE_TWEENING_OFF_SCREEN;

	float fX, fY;
	fX = GetBannerX(0);
	fY = GetBannerY(0);
	m_sprSelectionOverlay.SetXY( fX, fY );

	if(changing_sort) {
		/* When changing sort, tween the overlay with the item in the center;
		 * having it separate looks messy when we're moving fast. */
		m_sprSelectionOverlay.BeginTweening( 0.04f * NUM_WHEEL_ITEMS_TO_DRAW/2 * factor );	// sleep
		m_sprSelectionOverlay.BeginTweening( 0.2f * factor, Actor::TWEEN_BIAS_END );
	} else {
		m_sprSelectionOverlay.BeginTweening( 0 );	// sleep
		m_sprSelectionOverlay.BeginTweening( 0.2f * factor, Actor::TWEEN_BIAS_END );
	}
	m_sprSelectionOverlay.SetTweenX( fX+320 );

	m_ScrollBar.BeginTweening( 0 );
	m_ScrollBar.BeginTweening( 0.2f * factor, Actor::TWEEN_BIAS_BEGIN );
	m_ScrollBar.SetTweenX( SCROLL_BAR_X+30 );	

	for( int i=0; i<NUM_WHEEL_ITEMS_TO_DRAW; i++ )
	{
		WheelItemDisplay& display = m_WheelItemDisplays[i];
		float fThisBannerPositionOffsetFromSelection = i - NUM_WHEEL_ITEMS_TO_DRAW/2 + m_fPositionOffsetFromSelection;

		float fX = GetBannerX(fThisBannerPositionOffsetFromSelection);
		float fY = GetBannerY(fThisBannerPositionOffsetFromSelection);
		display.SetXY( fX, fY );
		display.BeginTweening( 0.04f*i * factor );	// sleep
		display.BeginTweening( 0.2f * factor, Actor::TWEEN_BIAS_END );
		display.SetTweenX( fX+320 );
	}

	m_fTimeLeftInState = TweenTime() + 0.100f;
}

CString MusicWheel::GetSectionNameFromSongAndSort( Song* pSong, SongSortOrder so )
{
	if( pSong == NULL )
		return "";

	CString sTemp;

	switch( so )
	{
	case SORT_GROUP:	
		sTemp = pSong->m_sGroupName;
		return sTemp;
//		case SORT_ARTIST:	
//			sTemp = pSong->m_sArtist;
//			sTemp.MakeUpper();
//			sTemp =  (sTemp.GetLength() > 0) ? sTemp.Left(1) : "";
//			if( IsAnInt(sTemp) )
//				sTemp = "NUM";
//			return sTemp;
	case SORT_TITLE:
		sTemp = pSong->GetSortTitle();
		sTemp.MakeUpper();
		if(sTemp.empty()) return "";

		sTemp = sTemp[0];
		if( IsAnInt(sTemp) )
			sTemp = "NUM";
		else if(toupper(sTemp[0]) < 'A' || toupper(sTemp[0]) > 'Z')
			sTemp = "OTHER";
		return sTemp;
	case SORT_BPM:
	case SORT_MOST_PLAYED:
	default:
		return "";
	}
}

void MusicWheel::Move(int n)
{
	if(n == m_Moving)
		return;

	/* Give us one last chance to move.  This will ensure that we're at least
	 * 0.5f from the selection, so we have a chance to spin down smoothly. */
	Update(0);

	bool Stopping = (m_Moving != 0 && n == 0 && m_TimeBeforeMovingBegins == 0);

	m_TimeBeforeMovingBegins = TIME_BEFORE_SLOW_REPEATS;
	m_SpinSpeed = 1.0f/FAST_SPIN_SWITCH_SECONDS;
	m_Moving = n;

	if(Stopping)
	{
		/* Make sure the user always gets an SM_SongChanged when
		 * Moving() is 0, so the final banner, etc. always get set. */
		SCREENMAN->SendMessageToTopScreen( SM_SongChanged, 0 );
	} else {
		if(n == 1)
			NextMusic();
		else if(n == -1)
			PrevMusic();
	}
}
