#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: Course

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Course.h"
#include "PrefsManager.h"
#include "Song.h"
#include "GameManager.h"
#include "SongManager.h"
#include "GameState.h"
#include "RageException.h"
#include "RageLog.h"
#include "MsdFile.h"
#include "PlayerOptions.h"
#include "SongOptions.h"
#include "RageUtil.h"

Course::Course()
{
	m_bRepeat = false;
	m_bRandomize = false;
	m_iLives = 4;
	m_iExtra = 0;
}

void Course::LoadFromCRSFile( CString sPath, vector<Song*> &apSongs )
{
	MsdFile msd;
	if( !msd.ReadFile(sPath) )
		RageException::Throw( "Error opening CRS file '%s'.", sPath.GetString() );

	CString sDir, sFName, sExt;
	splitrelpath( sPath, sDir, sFName, sExt );

	CStringArray arrayPossibleBanners;
	GetDirListing( "Courses\\" + sFName + ".png", arrayPossibleBanners, false, true );
	GetDirListing( "Courses\\" + sFName + ".jpg", arrayPossibleBanners, false, true );
	GetDirListing( "Courses\\" + sFName + ".bmp", arrayPossibleBanners, false, true );
	GetDirListing( "Courses\\" + sFName + ".gif", arrayPossibleBanners, false, true );
	if( !arrayPossibleBanners.empty() )
		m_sBannerPath = arrayPossibleBanners[0];

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		CString sValueName = msd.GetParam(i, 0);
		const MsdFile::value_t &sParams = msd.GetValue(i);

		// handle the data
		if( 0 == stricmp(sValueName, "COURSE") )
			m_sName = sParams[1];

		else if( 0 == stricmp(sValueName, "REPEAT") )
		{
			CString str = sParams[1];
			str.MakeLower();
			if( str.Find("yes") != -1 )
				m_bRepeat = true;
		}

		else if( 0 == stricmp(sValueName, "LIVES") )
			m_iLives = atoi( sParams[1] );

		else if( 0 == stricmp(sValueName, "EXTRA") )
			m_iExtra = atoi( sParams[1] );

		else if( 0 == stricmp(sValueName, "SONG") )
		{
			CString sSongDir = sParams[1];
			CString sNotesDescription = sParams[2];
			CString sModifiers = sParams[3];

			if(!sSongDir.GetLength()) {
			    /* Err. */
			    LOG->Trace( "Course file '%s' has an empty #SONG.  Ignored.", sPath.GetString(), sSongDir.GetString() );
			    continue;
			}

			/* GetSongDir() contains a path to the song, possibly a full path, eg:
			 * Songs\Group\SongName                   or 
			 * My Other Song Folder\Group\SongName    or
			 * c:\Corny J-pop\Group\SongName
			 *
			 * Most course group names are "Group\SongName", so we want to
			 * match against the last two elements. Let's also support
			 * "SongName" alone, since the group is only important when it's
			 * potentially ambiguous.
			 *
			 * Let's *not* support "Songs\Group\SongName" in course files.
			 * That's probably a common error, but that would result in
			 * course files floating around that only work for people who put
			 * songs in "Songs"; we don't want that.
			 */

			CStringArray split_SongDir;
			split( sSongDir, "\\", split_SongDir, true );

			if( split_SongDir.size() > 2 )
			{
			    LOG->Warn( "Course file \"%s\" path \"%s\" should contain "
						   "at most one backslash; ignored.",
				           (const char *) sPath, (const char *) sSongDir);
			    continue;
			}

			Song *pSong = NULL;
			// foreach song
			for( unsigned i = 0; pSong == NULL && i < apSongs.size(); i++ )
			{
				CStringArray splitted;
				split( apSongs[i]->GetSongDir(), "\\", splitted, true );
				bool matches = true;
				
				int split_no = splitted.size()-1;
				int SongDir_no = split_SongDir.size()-1;

				while( split_no >= 0 && SongDir_no >= 0 ) {
				    if( stricmp(splitted[split_no--], split_SongDir[SongDir_no--] ) )
						matches=false;
				}

				if(matches)
					pSong = apSongs[i];
			}
			if( pSong == NULL )	// we didn't find the Song
				continue;	// skip this song
			
			AddStage( pSong, sNotesDescription, sModifiers );
		}

		else
			LOG->Trace( "Unexpected value named '%s'", sValueName.GetString() );
	}
}


void Course::CreateEndlessCourseFromGroupAndDifficulty( CString sGroupName, Difficulty dc, vector<Song*> &apSongsInGroup )
{
	m_bRepeat = true;
	m_bRandomize = true;
	m_iLives = -1;

	CStringArray asPossibleBannerPaths;
	GetDirListing( "Songs\\" + sGroupName + "\\banner.png", asPossibleBannerPaths, false, true );
	GetDirListing( "Songs\\" + sGroupName + "\\banner.jpg", asPossibleBannerPaths, false, true );
	GetDirListing( "Songs\\" + sGroupName + "\\banner.gif", asPossibleBannerPaths, false, true );
	GetDirListing( "Songs\\" + sGroupName + "\\banner.bmp", asPossibleBannerPaths, false, true );
	if( !asPossibleBannerPaths.empty() )
		m_sBannerPath = asPossibleBannerPaths[0];

	CString sShortGroupName = SONGMAN->ShortenGroupName( sGroupName );	

	m_sName = sShortGroupName + " ";
	switch( dc )
	{
	case DIFFICULTY_EASY:	m_sName += "Easy";		break;
	case DIFFICULTY_MEDIUM:	m_sName += "Medium";	break;
	case DIFFICULTY_HARD:	m_sName += "Hard";		break;
	}

	for( unsigned s=0; s<apSongsInGroup.size(); s++ )
	{
		Song* pSong = apSongsInGroup[s];
		AddStage( pSong, DifficultyToString(dc), "" );
	}
	Shuffle();
}

void Course::Shuffle()
{
	/* Shuffle the list. */
	for( int i = 0; i < GetNumStages(); ++i)
		swap(order[i], order[rand() % GetNumStages()]);
}

Notes* Course::GetNotesForStage( int iStage )
{
	Song* pSong = GetSong(iStage);
	CString sDescription = entries[iStage].description;
	unsigned i;
	
	for( i=0; i<pSong->m_apNotes.size(); i++ )
	{
		Notes* pNotes = pSong->m_apNotes[i];
		if( 0==stricmp(pNotes->GetDescription(), sDescription)  &&
			GAMESTATE->GetCurrentStyleDef()->MatchesNotesType(pNotes->m_NotesType) )
			return pNotes;
	}


	// Didn't find a matching description.  Try to match the Difficulty instead.
	Difficulty dc = Notes::DifficultyFromDescriptionAndMeter( sDescription, 5 );

	for( i=0; i<pSong->m_apNotes.size(); i++ )
	{
		Notes* pNotes = pSong->m_apNotes[i];
		if( pNotes->GetDifficulty() == dc  &&
			GAMESTATE->GetCurrentStyleDef()->MatchesNotesType(pNotes->m_NotesType) )
			return pNotes;
	}


	return NULL;
}

Song *Course::GetSong( int iStage ) const
{
	return entries[iStage].song;
}

CString Course::GetDescription( int iStage ) const
{
	return entries[iStage].description;
}

CString Course::GetModifiers( int iStage ) const
{
	return entries[iStage].modifiers;
}

void Course::AddStage( Song* pSong, CString sDescription, CString sModifiers )
{
	course_entry e;
	e.song = pSong;
	e.description = sDescription;
	e.modifiers = sModifiers;
	entries.push_back(e);

	order.push_back(order.size());
}

/* When bShuffled is true, returns courses in the song ordering list. */
void Course::GetSongAndNotesForCurrentStyle( 
	vector<Song*>& apSongsOut,
	vector<Notes*>& apNotesOut, 
	CStringArray& asModifiersOut,
	bool bShuffled )
{
	for( int i=0; i<GetNumStages(); i++ )
	{
		int num = bShuffled? order[i]:i;

		Song* pSong = GetSong(num);
		Notes* pNotes = GetNotesForStage( num );
		CString sModifiers = entries[num].modifiers;

		if( !pSong->HasMusic() )
			continue;	// skip
		if( pNotes == NULL )
			continue;	// skip

		apSongsOut.push_back( pSong );
		apNotesOut.push_back( pNotes );
		asModifiersOut.push_back( sModifiers );
	}
}

RageColor Course::GetColor()
{
	// This could be made smarter
	if( GetNumStages() >= 7 )
		return RageColor(1,0,0,1);	// red
	else if( GetNumStages() >= 4 )
		return RageColor(1,0.5f,0,1);	// orange
	else
		return RageColor(0,1,0,1);	// green
}

void Course::GetPlayerOptions( PlayerOptions* pPO_out )
{
	*pPO_out = PlayerOptions();
}

void Course::GetSongOptions( SongOptions* pSO_out )
{
	*pSO_out = SongOptions();
	pSO_out->m_LifeType = (m_iLives==-1) ? SongOptions::LIFE_BAR : SongOptions::LIFE_BATTERY;
	if( m_iLives != -1 )
		pSO_out->m_iBatteryLives = m_iLives;
}

int Course::GetNumStages() const
{
	return entries.size();
}

//
// Sorting stuff
//

static int CompareCoursePointersByDifficulty(const Course* pCourse1, const Course* pCourse2)
{
	return pCourse1->GetNumStages() < pCourse2->GetNumStages();
}

void SortCoursePointerArrayByDifficulty( vector<Course*> &apCourses )
{
	sort( apCourses.begin(), apCourses.end(), CompareCoursePointersByDifficulty );
}
