#include "global.h"
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
#include "song.h"
#include "GameManager.h"
#include "SongManager.h"
#include "RageException.h"
#include "RageLog.h"
#include "MsdFile.h"
#include "PlayerOptions.h"
#include "SongOptions.h"
#include "RageUtil.h"
#include "TitleSubstitution.h"
#include "Steps.h"
#include "GameState.h"
#include "BannerCache.h"
#include "RageFile.h"
#include "arch/arch.h"
#include "ThemeManager.h"
#include "ProfileManager.h"

/* Amount to increase meter ranges to make them difficult: */
const int DIFFICULT_METER_CHANGE = 2;

/* Maximum lower value of ranges when difficult: */
const int MAX_BOTTOM_RANGE = 10;

/* -1 is the default parameter of a few Course calls; leaving it out indicates
 * to use GAMESTATE->m_bDifficultCourses. */
static bool IsDifficult( int Difficult )
{
	if( Difficult == -1 )
		return GAMESTATE->m_bDifficultCourses;
	else
		return !!Difficult;
}

Course::Course()
{
	m_bIsAutogen = false;
	m_bRepeat = false;
	m_bRandomize = false;
	m_bDifficult = false;
	m_iLives = -1;
	m_iMeter = -1;
}

PlayMode Course::GetPlayMode() const
{
	if( m_bRepeat )
		return PLAY_MODE_ENDLESS;
	return m_iLives > 0? PLAY_MODE_ONI:PLAY_MODE_NONSTOP;
}

const int DifficultMeterRamp = 3;
float Course::GetMeter( int Difficult ) const
{
	if( m_iMeter != -1 )
		return float(m_iMeter + (IsDifficult(Difficult)? DifficultMeterRamp:0));

	/*LOG->Trace( "Course file '%s' contains a song '%s%s%s' that is not present",
			m_sPath.c_str(), sGroup.c_str(), sGroup.size()? SLASH:"", sSong.c_str());*/
	vector<Info> ci;
	GetCourseInfo( GAMESTATE->GetCurrentStyleDef()->m_StepsType, ci, Difficult );

	if( ci.size() == 0 )
		return 0;

	/* Take the average meter. */
	float fTotalMeter = 0;
	for( unsigned c = 0; c < ci.size(); ++c )
	{
		if( ci[c].Mystery )
		{
			switch( GetDifficulty(ci[c]) )
			{
			case DIFFICULTY_INVALID:
			{
				int iMeterLow, iMeterHigh;
				GetMeterRange(ci[c], iMeterLow, iMeterHigh );
				fTotalMeter += (iMeterLow + iMeterHigh) / 2.0f;
				break;
			}
			case DIFFICULTY_BEGINNER:	fTotalMeter += 1; break;
			case DIFFICULTY_EASY:		fTotalMeter += 2; break;
			case DIFFICULTY_MEDIUM:		fTotalMeter += 5; break;
			case DIFFICULTY_HARD:		fTotalMeter += 7; break;
			case DIFFICULTY_CHALLENGE:	fTotalMeter += 9; break;
			}
		}
		else
			fTotalMeter += ci[c].pNotes->GetMeter();
	}
//	LOG->Trace("Course '%s': %f", m_sName.c_str(), fTotalMeter/ci.size() );
	return fTotalMeter / ci.size();
}

void Course::LoadFromCRSFile( CString sPath )
{
	LOG->Trace( "Course::LoadFromCRSFile( '%s' )", sPath.c_str() );

	m_sPath = sPath;	// save path

	MsdFile msd;
	if( !msd.ReadFile(sPath) )
		RageException::Throw( "Error opening CRS file '%s'.", sPath.c_str() );

	const CString sFName = SetExtension( sPath, "" );

	CStringArray arrayPossibleBanners;
	GetDirListing( sFName + ".png", arrayPossibleBanners, false, true );
	GetDirListing( sFName + ".jpg", arrayPossibleBanners, false, true );
	GetDirListing( sFName + ".bmp", arrayPossibleBanners, false, true );
	GetDirListing( sFName + ".gif", arrayPossibleBanners, false, true );
	if( !arrayPossibleBanners.empty() )
	{
		m_sBannerPath = arrayPossibleBanners[0];

		/* Cache and load the course banner. */
		BANNERCACHE->CacheBanner( m_sBannerPath );
	}

	AttackArray attacks;
	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		CString sValueName = msd.GetParam(i, 0);
		const MsdFile::value_t &sParams = msd.GetValue(i);

		// handle the data
		if( 0 == stricmp(sValueName, "COURSE") )
		{
			m_sName = sParams[1];
			m_sTranslitName = m_sName;
		}
		else if( 0 == stricmp(sValueName, "REPEAT") )
		{
			CString str = sParams[1];
			str.MakeLower();
			if( str.Find("yes") != -1 )
				m_bRepeat = true;
		}

		else if( 0 == stricmp(sValueName, "LIVES") )
			m_iLives = atoi( sParams[1] );

		else if( 0 == stricmp(sValueName, "METER") )
			m_iMeter = atoi( sParams[1] );

		else if( 0 == stricmp(sValueName, "MODS") )
		{
			Attack attack;
			float end = -9999;
			for( unsigned j = 1; j < sParams.params.size(); ++j )
			{
				CStringArray sBits;
				split( sParams[j], "=", sBits, false );
				if( sBits.size() == 0 )
					continue;
				LOG->Trace("b '%s'", sBits[0].c_str());

				TrimLeft( sBits[0] );
				TrimRight( sBits[0] );
				if( !sBits[0].CompareNoCase("TIME") )
					attack.fStartSecond = (float) atof( sBits[1] );
				else if( !sBits[0].CompareNoCase("LEN") )
					attack.fSecsRemaining = (float) atof( sBits[1] );
				else if( !sBits[0].CompareNoCase("END") )
					end = (float) atof( sBits[1] );
				else if( !sBits[0].CompareNoCase("MODS") )
				{
					if( end != -9999 )
					{
						attack.fSecsRemaining = end - attack.fStartSecond;
						end = -9999;
					}
					attack.sModifier = sBits[1];
					attacks.push_back( attack );
				}
			}

				
		}
		else if( 0 == stricmp(sValueName, "SONG") )
		{
			CourseEntry new_entry;

			// infer entry::Type from the first param
			if( sParams[1].Left(strlen("BEST")) == "BEST" )
			{
				new_entry.type = COURSE_ENTRY_BEST;
				new_entry.players_index = atoi( sParams[1].Right(sParams[1].size()-strlen("BEST")) ) - 1;
				CLAMP( new_entry.players_index, 0, 500 );
			}
			else if( sParams[1].Left(strlen("WORST")) == "WORST" )
			{
				new_entry.type = COURSE_ENTRY_WORST;
				new_entry.players_index = atoi( sParams[1].Right(sParams[1].size()-strlen("WORST")) ) - 1;
				CLAMP( new_entry.players_index, 0, 500 );
			}
			else if( sParams[1] == "*" )
			{
				new_entry.mystery = true;
				new_entry.type = COURSE_ENTRY_RANDOM;
			}
			else if( sParams[1].Right(1) == "*" )
			{
				new_entry.mystery = true;
				new_entry.type = COURSE_ENTRY_RANDOM_WITHIN_GROUP;
				CString sSong = sParams[1];
				sSong.Replace( "\\", "/" );
				CStringArray bits;
				split( sSong, "/", bits );
				if( bits.size() == 2 )
					new_entry.group_name = bits[0];
				else
					LOG->Warn( "Course file '%s' contains a random_within_group entry '%s' that is invalid. "
								"Song should be in the format '<group>/*'.",
								m_sPath.c_str(), sSong.c_str());
				if( !SONGMAN->DoesGroupExist(new_entry.group_name) )
				{
					/* XXX: We need a place to put "user warnings".  This is too loud for info.txt--
				     * it obscures important warnings--and regular users never look there, anyway. */
					LOG->Trace( "Course file '%s' random_within_group entry '%s' specifies a group that doesn't exist. "
								"This entry will be ignored.",
								m_sPath.c_str(), sSong.c_str());
					continue;	// skip this #SONG
				}
			}
			else
			{
				new_entry.type = COURSE_ENTRY_FIXED;

				CString sSong = sParams[1];
				new_entry.pSong = SONGMAN->FindSong( sSong );

				if( new_entry.pSong == NULL )
				{
					/* XXX: We need a place to put "user warnings".  This is too loud for info.txt--
				     * it obscures important warnings--and regular users never look there, anyway. */
					LOG->Trace( "Course file '%s' contains a fixed song entry '%s' that does not exist. "
								"This entry will be ignored.",
								m_sPath.c_str(), sSong.c_str());
					continue;	// skip this #SONG
				}
			}

			new_entry.difficulty = StringToDifficulty( sParams[2] );
			if( new_entry.difficulty == DIFFICULTY_INVALID )
			{
				int retval = sscanf( sParams[2], "%d..%d", &new_entry.low_meter, &new_entry.high_meter );
				if( retval == 1 )
					new_entry.high_meter = new_entry.low_meter;
				else if( retval != 2 )
				{
					LOG->Warn("Course file '%s' contains an invalid difficulty setting: \"%s\", 3..6 used instead",
						m_sPath.c_str(), sParams[2].c_str());
					new_entry.low_meter = 3;
					new_entry.high_meter = 6;
				}
			}

			{
				/* If "showcourse" or "noshowcourse" is in the list, force new_entry.mystery 
				 * on or off. */
				CStringArray mods;
				split( sParams[3], ",", mods, true );
				for( int j = (int) mods.size()-1; j >= 0 ; --j )
				{
					if( !mods[j].CompareNoCase("showcourse") )
						new_entry.mystery = false;
					else if( !mods[j].CompareNoCase("noshowcourse") )
						new_entry.mystery = true;
					else 
						continue;
					mods.erase(mods.begin() + j);
				}
				new_entry.modifiers = join( ",", mods );
			}

			new_entry.attacks = attacks;
			attacks.clear();
			
			m_entries.push_back( new_entry );
		}

		else
			LOG->Trace( "Unexpected value named '%s'", sValueName.c_str() );
	}
	static TitleSubst tsub("courses");

	CString ignore;
	tsub.Subst(m_sName, ignore, ignore,
				ignore, ignore, ignore);
}


void Course::Save()
{
	ASSERT( !m_bIsAutogen );

	FILE* fp = fopen( m_sPath, "w" );
	if( fp == NULL )
	{
		LOG->Warn( "Could not write course file '%s'.", m_sPath.c_str() );
		return;
	}

	fprintf( fp, "#COURSE:%s;\n", m_sName.c_str() );
	if( m_bRepeat )
		fprintf( fp, "#REPEAT:YES;\n" );
	if( m_iLives != -1 )
		fprintf( fp, "#LIVES:%i;\n", m_iLives );
	if( m_iMeter != -1 )
		fprintf( fp, "#METER:%i;\n", m_iMeter );

	for( unsigned i=0; i<m_entries.size(); i++ )
	{
		const CourseEntry& entry = m_entries[i];

		for( unsigned j = 0; j < entry.attacks.size(); ++j )
		{
			if( j == 0 )
				fprintf( fp, "#MODS:\n" );

			const Attack &a = entry.attacks[j];
			fprintf( fp, "  TIME=%.2f:LEN=%.2f:MODS=%s",
				a.fStartSecond, a.fSecsRemaining, a.sModifier.c_str() );

			if( j+1 < entry.attacks.size() )
				fprintf( fp, ":" );
			else
				fprintf( fp, ";" );
			fprintf( fp, "\n" );
		}

		switch( entry.type )
		{
		case COURSE_ENTRY_FIXED:
			{
				// strip off everything but the group name and song dir
				CStringArray as;
				split( entry.pSong->GetSongDir(), SLASH, as );
				ASSERT( !as.empty() );
				CString sGroup = as[ as.size()-2 ];
				CString sSong = as[ as.size()-1 ];
				fprintf( fp, "#SONG:" + sGroup + SLASH + sSong );
			}
			break;
		case COURSE_ENTRY_RANDOM:
			fprintf( fp, "#SONG:*" );
			break;
		case COURSE_ENTRY_RANDOM_WITHIN_GROUP:
			fprintf( fp, "#SONG:%s/*", entry.group_name.c_str() );
			break;
		case COURSE_ENTRY_BEST:
			fprintf( fp, "#SONG:BEST%d", entry.players_index+1 );
			break;
		case COURSE_ENTRY_WORST:
			fprintf( fp, "#SONG:WORST%d", entry.players_index+1 );
			break;
		default:
			ASSERT(0);
		}

		fprintf( fp, ":" );
		if( entry.difficulty != DIFFICULTY_INVALID )
			fprintf( fp, "%s", DifficultyToString(entry.difficulty).c_str() );
		else if( entry.low_meter != -1  &&  entry.high_meter != -1 )
			fprintf( fp, "%d..%d", entry.low_meter, entry.high_meter );
		fprintf( fp, ":%s", entry.modifiers.c_str() );

		bool default_mystery = (entry.type == COURSE_ENTRY_RANDOM || entry.type == COURSE_ENTRY_RANDOM_WITHIN_GROUP);
		if( default_mystery != entry.mystery )
		{
			if( entry.modifiers != "" )
				fprintf( fp, "," );
			fprintf( fp, entry.mystery? "noshowcourse":"showcourse" );
		}

		fprintf( fp, ";\n" );
	}

	fclose( fp );
}


void Course::AutogenEndlessFromGroup( CString sGroupName, vector<Song*> &apSongsInGroup )
{
	m_bIsAutogen = true;
	m_bRepeat = true;
	m_bRandomize = true;
	m_iLives = -1;
	m_iMeter = -1;

	m_sName = SONGMAN->ShortenGroupName( sGroupName );	
	m_sBannerPath = SONGMAN->GetGroupBannerPath( sGroupName );

	// We want multiple songs, so we can try to prevent repeats during
	// gameplay. (We might still get a repeat at the repeat boundary,
	// but that'd be rare.) -glenn
	CourseEntry e;
	e.type = COURSE_ENTRY_RANDOM_WITHIN_GROUP;
	e.group_name = sGroupName;
	e.difficulty = DIFFICULTY_MEDIUM;
	e.mystery = true;

	vector<Song*> vSongs;
	SONGMAN->GetSongs( vSongs, e.group_name );
	for( unsigned i = 0; i < vSongs.size(); ++i)
		m_entries.push_back( e );
}

void Course::AutogenNonstopFromGroup( CString sGroupName, vector<Song*> &apSongsInGroup )
{
	AutogenEndlessFromGroup( sGroupName, apSongsInGroup );

	m_bRepeat = false;

	m_sName += " Random";	

	// resize to 4
	while( m_entries.size() < 4 )
		m_entries.push_back( m_entries[0] );
	while( m_entries.size() > 4 )
		m_entries.pop_back();
}

void Course::MemCardData::AddHighScore( HighScore hs, int &iIndexOut )
{
	int i;
	for( i=0; i<(int)vHighScores.size(); i++ )
	{
		if( hs >= vHighScores[i] )
			break;
	}
	if( i < NUM_RANKING_LINES )
	{
		vHighScores.insert( vHighScores.begin()+i, hs );
		iIndexOut = i;
		if( vHighScores.size() > unsigned(NUM_RANKING_LINES) )
			vHighScores.erase( vHighScores.begin()+NUM_RANKING_LINES, vHighScores.end() );
	}
}

/*
 * Difficult courses do the following:
 *
 * For entries with a meter range, bump it up by DIFFICULT_METER_CHANGE;
 * eg. 3..6 -> 5..8, with a minimum no higher than MAX_BOTTOM_RANGE.
 *
 * For entries with a difficulty class, use notes one class harder, if they
 * exist. This way, if a static song entry points to a difficulty, we'll always
 * play that song, even if we're on difficult and harder notes don't exist.  (The
 * exception is a static song entry with a meter range, but that's not very useful.)
 */
bool Course::HasDifficult( StepsType nt ) const
{
	/* Check to see if any songs would change if difficult. */

	vector<Info> Normal, Hard;
	GetCourseInfo( nt, Normal, false );
	GetCourseInfo( nt, Hard, true );

	if( Normal.size() != Hard.size() )
		return true; /* it changed */

	for( unsigned i=0; i<Normal.size(); i++ )
	{
		if( Normal[i].CourseIndex != Hard[i].CourseIndex )
			return true; /* it changed */

		if( Normal[i].Mystery )
		{
			const CourseEntry &e = m_entries[ Normal[i].CourseIndex ];
			
			/* Difficulties under CHALLENGE change by getting harder. */
			if( e.difficulty < DIFFICULTY_CHALLENGE )
				return true;

			/* Meters under MAX_BOTTOM_RANGE..MAX_BOTTOM_RANGE change by getting harder. */
			if( e.difficulty != DIFFICULTY_INVALID &&
				e.low_meter < MAX_BOTTOM_RANGE &&
				e.high_meter < MAX_BOTTOM_RANGE )
				return true;
			continue;
		}
		
		if( Normal[i].pSong != Hard[i].pSong || Normal[i].pNotes != Hard[i].pNotes )
			return true;
	}
	return false;
}

bool Course::IsPlayableIn( StepsType nt ) const
{
	vector<Info> ci;
	GetCourseInfo( nt, ci );
	return ci.size() > 0;
}


static vector<Song*> GetFilteredBestSongs( StepsType nt )
{
	vector<Song*> vSongsByMostPlayed = SONGMAN->GetBestSongs();
	// filter out songs that don't have both medium and hard steps and long ver sons
	for( int j=vSongsByMostPlayed.size()-1; j>=0; j-- )
	{
		Song* pSong = vSongsByMostPlayed[j];
		if( SONGMAN->GetNumStagesForSong(pSong) > 1 ||
			!pSong->GetStepsByDifficulty(nt, DIFFICULTY_MEDIUM, PREFSMAN->m_bAutogenMissingTypes)  ||
			!pSong->GetStepsByDifficulty(nt, DIFFICULTY_HARD, PREFSMAN->m_bAutogenMissingTypes) )
			vSongsByMostPlayed.erase( vSongsByMostPlayed.begin()+j );
	}

	return vSongsByMostPlayed;
}

void Course::GetCourseInfo( StepsType nt, vector<Course::Info> &ci, int Difficult ) const
{
	vector<CourseEntry> entries = m_entries;

	/* Different seed for each course, but the same for the whole round: */
	RandomGen rnd( GAMESTATE->m_iRoundSeed + GetHashForString(m_sName) );

	if( m_bRandomize )
	{
		/* Always randomize the same way per round.  Otherwise, the displayed course
		 * will change every time it's viewed, and the displayed order will have no
		 * bearing on what you'll actually play. */
		random_shuffle( entries.begin(), entries.end(), rnd );
	}

	/* This can take some time, so don't fill it out unless we need it. */
	bool bMostPlayedSet = false;
	vector<Song*> vSongsByMostPlayed;
	
	vector<Song*> AllSongsShuffled = SONGMAN->GetAllSongs();
	random_shuffle( AllSongsShuffled.begin(), AllSongsShuffled.end(), rnd );
	int CurSong = 0; /* Current offset into AllSongsShuffled */

	ci.clear(); 

	for( unsigned i=0; i<entries.size(); i++ )
	{
		const CourseEntry &e = entries[i];

		Song* pSong = NULL;	// fill this in
		Steps* pNotes = NULL;	// fill this in

		/* This applies difficult mode for meter ranges.  (If it's a difficulty
		 * class, we'll do it below.) */
		int low_meter, high_meter;
		GetMeterRange( i, low_meter, high_meter, Difficult );

		switch( e.type )
		{
		case COURSE_ENTRY_FIXED:
			pSong = e.pSong;
			if( pSong )
			{
				if( e.difficulty != DIFFICULTY_INVALID )
					pNotes = pSong->GetStepsByDifficulty( nt, e.difficulty, PREFSMAN->m_bAutogenMissingTypes );
				else if( e.low_meter != -1  &&  e.high_meter != -1 )
					pNotes = pSong->GetStepsByMeter( nt, low_meter, high_meter, PREFSMAN->m_bAutogenMissingTypes );
				else
					pNotes = pSong->GetStepsByDifficulty( nt, DIFFICULTY_MEDIUM, PREFSMAN->m_bAutogenMissingTypes );
			}
			break;
		case COURSE_ENTRY_RANDOM:
		case COURSE_ENTRY_RANDOM_WITHIN_GROUP:
			{
				// find a song with the notes we want
				for( unsigned j=0; j<AllSongsShuffled.size(); j++ )
				{
					/* See if the first song matches what we want. */
					ASSERT( unsigned(CurSong) < AllSongsShuffled.size() );
					pSong = AllSongsShuffled[CurSong];
					ASSERT( pSong );
					CurSong = (CurSong+1) % AllSongsShuffled.size();

					if(e.type == COURSE_ENTRY_RANDOM_WITHIN_GROUP &&
					   pSong->m_sGroupName.CompareNoCase(e.group_name))
					   continue; /* wrong group */

					if( e.difficulty == DIFFICULTY_INVALID )
						pNotes = pSong->GetStepsByMeter( nt, low_meter, high_meter, PREFSMAN->m_bAutogenMissingTypes );
					else
						pNotes = pSong->GetStepsByDifficulty( nt, e.difficulty, PREFSMAN->m_bAutogenMissingTypes );

					if( pNotes )	// found a match
						break;		// stop searching

					pSong = NULL;
					pNotes = NULL;
				}
			}
			break;
		case COURSE_ENTRY_BEST:
		case COURSE_ENTRY_WORST:
			{
				if( !bMostPlayedSet )
				{
					bMostPlayedSet = true;
					vSongsByMostPlayed = GetFilteredBestSongs( nt );
				}

				if( e.players_index >= (int)vSongsByMostPlayed.size() )
					break;

				switch( e.type )
				{
				case COURSE_ENTRY_BEST:
					pSong = vSongsByMostPlayed[e.players_index];
					break;
				case COURSE_ENTRY_WORST:
					pSong = vSongsByMostPlayed[vSongsByMostPlayed.size()-1-e.players_index];
					break;
				default:
					ASSERT(0);
				}

				if( e.difficulty == DIFFICULTY_INVALID )
					pNotes = pSong->GetStepsByMeter( nt, low_meter, high_meter, PREFSMAN->m_bAutogenMissingTypes );
				else
					pNotes = pSong->GetStepsByDifficulty( nt, e.difficulty, PREFSMAN->m_bAutogenMissingTypes );

				if( pNotes == NULL )
					pNotes = pSong->GetClosestNotes( nt, DIFFICULTY_MEDIUM );
			}
			break;
		default:
			ASSERT(0);
		}

		if( !pSong || !pNotes )
			continue;	// this song entry isn't playable.  Skip.

		/* If e.difficulty == DIFFICULTY_INVALID, then we already increased difficulty
		 * based on meter. */
		if( IsDifficult(Difficult) && e.difficulty != DIFFICULTY_INVALID )
		{
			/* See if we can find a NoteData that's one notch more difficult than
			 * the one we found above. */
			Difficulty dc = pNotes->GetDifficulty();
			if(dc < DIFFICULTY_CHALLENGE)
			{
				dc  = Difficulty(dc + 1);
				Steps* pNewNotes = pSong->GetStepsByDifficulty( nt, dc, PREFSMAN->m_bAutogenMissingTypes );
				if(pNewNotes)
					pNotes = pNewNotes;
			}
		}

		Info cinfo;
		cinfo.pSong = pSong;
		cinfo.pNotes = pNotes;
		cinfo.Modifiers = e.modifiers;
		cinfo.Attacks = e.attacks;
		cinfo.Random = ( e.type == COURSE_ENTRY_RANDOM || e.type == COURSE_ENTRY_RANDOM_WITHIN_GROUP );
		cinfo.Mystery = e.mystery;
		cinfo.CourseIndex = i;
		cinfo.Difficult = IsDifficult(Difficult);
		ci.push_back( cinfo ); 
	}
}

RageColor Course::GetColor() const
{
	switch (PREFSMAN->m_iCourseSortOrder)
	{
	case PrefsManager::COURSE_SORT_SONGS:	
		if( m_entries.size() >= 7 )
			return RageColor(1,0,0,1);	// red
		else if( m_entries.size() >= 4 )
			return RageColor(1,1,0,1);	// yellow
		else
			return RageColor(0,1,0,1);	// green
		// never should get here
		break;

	case PrefsManager::COURSE_SORT_METER:
		if ( !IsFixed() )
			return RageColor(0,0,1,1);  // blue
		if (GetMeter() > 8.5)
			return RageColor(1,0,0,1);  // red
		if (GetMeter() >= 7)
			return RageColor(1,0.5f,0,1); // orange
		if (GetMeter() >= 5)
			return RageColor(1,1,0,1);  // yellow
		return RageColor(0,1,0,1); // green

	case PrefsManager::COURSE_SORT_METER_SUM:
		if ( !IsFixed() )
			return RageColor(0,0,1,1);  // blue
		if (SortOrder_TotalDifficulty >= 40)
			return RageColor(1,0,0,1);  // red
		if (SortOrder_TotalDifficulty >= 30)
			return RageColor(1,0.5f,0,1); // orange
		if (SortOrder_TotalDifficulty >= 20)
			return RageColor(1,1,0,1);  // yellow
		return RageColor(0,1,0,1); // green

	case PrefsManager::COURSE_SORT_RANK:
		if (SortOrder_Ranking == 3)
			return RageColor(0,0,1,1);  // blue
		if (SortOrder_Ranking == 2)
			return RageColor(1,0.5f,0,1); // orange
		if (SortOrder_Ranking == 1)
			return RageColor(0,1,0,1); // green
		return RageColor(1,1,0,1); // yellow, never should get here
	default:
		ASSERT(0);
		return RageColor(1,1,1,1);  // white, never should reach here
	}
}

bool Course::IsFixed() const
{
	for(unsigned i = 0; i < m_entries.size(); i++)
	{
		if ( m_entries[i].type == COURSE_ENTRY_FIXED )
			continue;

		return false;
	}

	return true;
}

Difficulty Course::GetDifficulty( const Info &stage ) const
{
	Difficulty dc = m_entries[stage.CourseIndex].difficulty;

	if( stage.Difficult && dc < DIFFICULTY_CHALLENGE )
		dc  = Difficulty(dc + 1);

	return dc;
}

void Course::GetMeterRange( int stage, int& iMeterLowOut, int& iMeterHighOut, int Difficult ) const
{
	iMeterLowOut = m_entries[stage].low_meter;
	iMeterHighOut = m_entries[stage].high_meter;

	if( m_entries[stage].difficulty == DIFFICULTY_INVALID && IsDifficult(Difficult) )
	{
		iMeterHighOut += DIFFICULT_METER_CHANGE;
		iMeterLowOut += DIFFICULT_METER_CHANGE;
		iMeterLowOut = min( iMeterLowOut, MAX_BOTTOM_RANGE );
	}
}


void Course::GetMeterRange( const Info &stage, int& iMeterLowOut, int& iMeterHighOut ) const
{
	GetMeterRange( stage.CourseIndex, iMeterLowOut, iMeterHighOut, stage.Difficult );
}

bool Course::GetTotalSeconds( float& fSecondsOut ) const
{
	vector<Info> ci;
	GetCourseInfo( STEPS_TYPE_DANCE_SINGLE, ci );

	fSecondsOut = 0;
	for( unsigned i=0; i<ci.size(); i++ )
	{
		if( ci[i].Mystery )
			return false;
		fSecondsOut += ci[i].pSong->m_fMusicLengthSeconds;
	}
	return true;
}


//
// Sorting stuff
//
static bool CompareCoursePointersByName(const Course* pCourse1, const Course* pCourse2)
{
	// HACK:  strcmp and other string comparators appear to eat whitespace.
	// For example, the string "Players Best 13-16" is sorted between 
	// "Players Best  1-4" and "Players Best  5-8".  Replace the string "  "
	// with " 0" for comparison only.

	// XXX: That doesn't happen to me, and it shouldn't (strcmp is strictly
	// a byte sort, though CompareNoCase doesn't use strcmp).  Are you sure
	// you didn't have only one space before? -glenn
	CString sName1 = pCourse1->m_sName;
	CString sName2 = pCourse2->m_sName;
	sName1.Replace( "  " , " 0" );
	sName2.Replace( "  " , " 0" );
	return sName1.CompareNoCase( sName2 ) == -1;
}

static bool CompareCoursePointersByAutogen(const Course* pCourse1, const Course* pCourse2)
{
	int b1 = pCourse1->m_bIsAutogen;
	int b2 = pCourse2->m_bIsAutogen;
	if( b1 < b2 )
		return true;
	else if( b1 > b2 )
		return false;
	else
		return CompareCoursePointersByName(pCourse1,pCourse2);
}

static bool CompareCoursePointersByDifficulty(const Course* pCourse1, const Course* pCourse2)
{
	int iNum1 = pCourse1->GetEstimatedNumStages();
	int iNum2 = pCourse2->GetEstimatedNumStages();
	if( iNum1 < iNum2 )
		return true;
	else if( iNum1 > iNum2 )
		return false;
	else // iNum1 == iNum2
		return CompareCoursePointersByAutogen( pCourse1, pCourse2 );
}

static bool CompareCoursePointersByAvgDifficulty(const Course* pCourse1, const Course* pCourse2)
{
	float fNum1 = pCourse1->GetMeter( false );
	float fNum2 = pCourse2->GetMeter( false );

	if( fNum1 < fNum2 )
		return true;
	else if( fNum1 > fNum2 )
		return false;
	else // fNum1 == fNum2
		return ( pCourse1->m_sName < pCourse2->m_sName );
}

static bool CompareCoursePointersByTotalDifficulty(const Course* pCourse1, const Course* pCourse2)
{
	int iNum1 = pCourse1->SortOrder_TotalDifficulty;
	int iNum2 = pCourse2->SortOrder_TotalDifficulty;

	if( iNum1 == iNum2 )
		return CompareCoursePointersByAutogen( pCourse1, pCourse2 );
	return iNum1 < iNum2;
}

bool Course::CourseHasBestOrWorst() const
{
	for(unsigned i = 0; i < m_entries.size(); i++)
	{
		switch( m_entries[i].type )
		{
		case COURSE_ENTRY_BEST:
		case COURSE_ENTRY_WORST:
			return true;
		}
	}

	return false;
}

static bool MovePlayersBestToEnd( const Course* pCourse1, const Course* pCourse2 )
{
	bool C1HasBest = pCourse1->CourseHasBestOrWorst();
	bool C2HasBest = pCourse2->CourseHasBestOrWorst();
	if( !C1HasBest && !C2HasBest )
		return false;
	if( C1HasBest && !C2HasBest )
		return false;
	if( !C1HasBest && C2HasBest )
		return true;

	return pCourse1->m_sName < pCourse2->m_sName;
}

static bool CompareRandom( const Course* pCourse1, const Course* pCourse2 )
{
	return ( pCourse1->IsFixed() && !pCourse2->IsFixed() );
}

static bool CompareCoursePointersByRanking(const Course* pCourse1, const Course* pCourse2)
{
	int iNum1 = pCourse1->SortOrder_Ranking;
	int iNum2 = pCourse2->SortOrder_Ranking;

	if( iNum1 == iNum2 )
		return CompareCoursePointersByAutogen( pCourse1, pCourse2 );
	return iNum1 < iNum2;
}

void SortCoursePointerArrayByDifficulty( vector<Course*> &apCourses )
{
	sort( apCourses.begin(), apCourses.end(), CompareCoursePointersByDifficulty );
}

void SortCoursePointerArrayByRanking( vector<Course*> &apCourses )
{
	for(unsigned i=0; i<apCourses.size(); i++)
		apCourses[i]->UpdateCourseStats();
	sort( apCourses.begin(), apCourses.end(), CompareCoursePointersByRanking );
}

void SortCoursePointerArrayByAvgDifficulty( vector<Course*> &apCourses )
{
	sort( apCourses.begin(), apCourses.end(), CompareCoursePointersByAvgDifficulty );
	stable_sort( apCourses.begin(), apCourses.end(), MovePlayersBestToEnd );
}

void SortCoursePointerArrayByTotalDifficulty( vector<Course*> &apCourses )
{
	for(unsigned i=0; i<apCourses.size(); i++)
		apCourses[i]->UpdateCourseStats();
	sort( apCourses.begin(), apCourses.end(), CompareCoursePointersByTotalDifficulty );
}

static bool CompareCoursePointersByType(const Course* pCourse1, const Course* pCourse2)
{
	return pCourse1->GetPlayMode() < pCourse2->GetPlayMode();
}

void MoveRandomToEnd( vector<Course*> &apCourses )
{
	stable_sort( apCourses.begin(), apCourses.end(), CompareRandom );
}

void SortCoursePointerArrayByType( vector<Course*> &apCourses )
{
	stable_sort( apCourses.begin(), apCourses.end(), CompareCoursePointersByType );
}

bool Course::HasBanner() const
{
	return m_sBannerPath != ""  &&  IsAFile(m_sBannerPath);
}

void Course::UpdateCourseStats()
{
	SortOrder_TotalDifficulty = 0;

	unsigned i;

	// courses with random/players best-worst songs should go at the end
	for(i = 0; i < m_entries.size(); i++)
	{
		if ( m_entries[i].type == COURSE_ENTRY_FIXED )
			continue;

		if ( SortOrder_Ranking == 2 )
			SortOrder_Ranking = 3;
		SortOrder_TotalDifficulty = 999999;     // large number
		return;
	}

	vector<Info> ci;
	GetCourseInfo( GAMESTATE->GetCurrentStyleDef()->m_StepsType, ci );

	for( i = 0; i < ci.size(); i++ )
		SortOrder_TotalDifficulty += ci[i].pNotes->GetMeter();

	// OPTIMIZATION: Ranking info isn't dependant on style, so
	// call it sparingly.  Its handled on startup and when
	// themes change..
	
	LOG->Trace("%s: Total feet: %d",
		this->m_sName.c_str(),
		SortOrder_TotalDifficulty );
}

void Course::Info::GetAttackArray( AttackArray &out ) const
{
	Attack a;
	a.fStartSecond = 0;
	a.fSecsRemaining = 10000; /* whole song */
	a.level = ATTACK_LEVEL_1;
	a.sModifier = Modifiers;

	out.push_back( a );

	out.insert( out.end(), Attacks.begin(), Attacks.end() );
}

bool Course::IsRanking() const
{
	CStringArray rankingsongs;
	
	split(THEME->GetMetric("ScreenRanking", "CoursesToShow"), ",", rankingsongs);

	for(unsigned i=0; i < rankingsongs.size(); i++)
		if (rankingsongs[i].CompareNoCase(m_sPath))
			return true;

	return false;
}

void Course::AddHighScore( StepsType st, PlayerNumber pn, MemCardData::HighScore hs, int &iPersonalIndexOut, int &iMachineIndexOut )
{
	hs.sName = RANKING_TO_FILL_IN_MARKER[pn];
	if( PROFILEMAN->IsUsingProfile(pn) )
		m_MemCardDatas[st][pn].AddHighScore( hs, iPersonalIndexOut );
	else
		iPersonalIndexOut = -1;
	m_MemCardDatas[st][MEMORY_CARD_MACHINE].AddHighScore( hs, iMachineIndexOut );
}

int Course::GetNumTimesPlayed( MemoryCard card ) const
{
	int iTotalNumTimesPlayed = 0;
	for( unsigned i = 0; i < NUM_STEPS_TYPES; ++i )
		iTotalNumTimesPlayed += GetNumTimesPlayed( (StepsType) i, card );
	return iTotalNumTimesPlayed;
}

static map<const Course*, int> course_sort_val;

bool CompareCoursePointersBySortVal(const Course *pSong1, const Course *pSong2)
{
	return course_sort_val[pSong1] < course_sort_val[pSong2];
}

void SortCoursePointerArrayByMostPlayed( vector<Course*> &arrayCoursePointers, MemoryCard card )
{
	for(unsigned i = 0; i < arrayCoursePointers.size(); ++i)
		course_sort_val[arrayCoursePointers[i]] = arrayCoursePointers[i]->GetNumTimesPlayed( card );
	stable_sort( arrayCoursePointers.begin(), arrayCoursePointers.end(), CompareCoursePointersBySortVal );
	reverse( arrayCoursePointers.begin(), arrayCoursePointers.end() );
	course_sort_val.clear();
}
