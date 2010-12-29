#include "global.h"
#include "Profile.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "XmlFile.h"
#include "IniFile.h"
#include "GameManager.h"
#include "RageLog.h"
#include "song.h"
#include "SongManager.h"
#include "Steps.h"
#include "Course.h"
#include "ThemeManager.h"
#include "CryptManager.h"
#include "ProfileManager.h"
#include "RageFileManager.h"
#include "LuaManager.h"
#include "crypto/CryptRand.h"
#include "UnlockManager.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "Foreach.h"
#include "CatalogXml.h"
#include "Bookkeeper.h"
#include "Game.h"
#include "CharacterManager.h"
#include "Character.h"
#include "Json/Value.h"
#include "JsonUtil.h"

const RString STATS_XSL            = "Stats.xsl";
const RString COMMON_XSL           = "Common.xsl";
const RString STATS_JSON           = "Stats.json";
const RString EDITABLE_INI         = "Editable.ini";
const RString DONT_SHARE_SIG       = "DontShare.sig";
const RString PUBLIC_KEY_FILE      = "public.key";
const RString SCREENSHOTS_SUBDIR   = "Screenshots/";
const RString EDIT_STEPS_SUBDIR    = "Edits/";
const RString EDIT_COURSES_SUBDIR  = "EditCourses/";

#define GUID_SIZE_BYTES 8

#define MAX_EDITABLE_INI_SIZE_BYTES		2*1024		// 2KB
#define MAX_PLAYER_STATS_JSON_SIZE_BYTES	\
	100 /* Songs */				\
	* 3 /* Steps per Song */		\
	* 10 /* HighScores per Steps */		\
	* 1024 /* size in bytes of a HighScores XNode */

const unsigned int DEFAULT_WEIGHT_POUNDS	= 120;

#if defined(_MSC_VER)
#pragma warning (disable : 4706) // assignment within conditional expression
#endif


int Profile::HighScoresForASong::GetNumTimesPlayed() const
{
	int iCount = 0;
	FOREACHM_CONST( StepsID, HighScoreList, m_StepsHighScores, i )
	{
		iCount += i->second.GetNumTimesPlayed();
	}
	return iCount;
}

int Profile::HighScoresForACourse::GetNumTimesPlayed() const
{
	int iCount = 0;
	FOREACHM_CONST( TrailID, HighScoreList, m_TrailHighScores, i )
	{
		iCount += i->second.GetNumTimesPlayed();
	}
	return iCount;
}


void Profile::InitEditableData()
{
	m_sDisplayName = "";
	m_sCharacterID = "";
	m_sLastUsedHighScoreName = "";
	m_iWeightPounds = 0;
}

RString Profile::MakeGuid()
{
	// Does the RNG need to be inited and seeded every time?
	random_init();
	random_add_noise( "ai8049ujr3odusj" );
	
	RString s;
	for( unsigned i=0; i<GUID_SIZE_BYTES; i++ )
		s += ssprintf( "%02x", random_byte() );
	return s;
}

void Profile::InitGeneralData()
{
	m_sGuid = MakeGuid();

	m_SortOrder = SORT_INVALID;
	m_LastDifficulty = DIFFICULTY_INVALID;
	m_LastCourseDifficulty = DIFFICULTY_INVALID;
	m_lastSong.Unset();
	m_lastCourse.Unset();
	m_iTotalPlays = 0;
	m_iTotalPlaySeconds = 0;
	m_iTotalGameplaySeconds = 0;
	m_iCurrentCombo = 0;
	m_fTotalCaloriesBurned = 0;
	m_GoalType = (GoalType)0;
	m_iGoalCalories = 0;
	m_iGoalSeconds = 0;
	m_iTotalDancePoints = 0;
	m_iNumExtraStagesPassed = 0;
	m_iNumExtraStagesFailed = 0;
	m_iNumToasties = 0;
	m_UnlockedEntryIDs.clear();
	m_sLastPlayedMachineGuid = "";
	m_LastPlayedDate.Init();
	m_iTotalTapsAndHolds = 0;
	m_iTotalJumps = 0;
	m_iTotalHolds = 0;
	m_iTotalRolls = 0;
	m_iTotalMines = 0;
	m_iTotalHands = 0;

	m_iNumSongsPlayedByPlayMode.clear();
	m_iNumSongsPlayedByStyle.clear();
	m_iNumSongsPlayedByDifficulty.clear();
	m_iNumSongsPlayedByMeter.clear();
	m_iNumTotalSongsPlayed = 0;
	m_iNumStagesPassedByPlayMode.clear();
	m_iNumStagesPassedByGrade.clear();

	Lua *L = LUA->Get();
	lua_newtable( L );
	m_SavedLuaData.SetFromStack( L );
	LUA->Release( L );
}

void Profile::InitSongScores()
{
	m_SongHighScores.clear();
}

void Profile::InitCourseScores()
{
	m_CourseHighScores.clear();
}

void Profile::InitCategoryScores()
{
	m_CategoryHighScores.clear();
}

void Profile::InitScreenshotData()
{
	m_vScreenshots.clear();
}

void Profile::InitCalorieData()
{
	m_mapDayToCaloriesBurned.clear();
}

void Profile::InitRecentSongScores()
{
	m_vRecentStepsScores.clear();
}

void Profile::InitRecentCourseScores()
{
	m_vRecentCourseScores.clear();
}

RString Profile::GetDisplayNameOrHighScoreName() const
{
	if( !m_sDisplayName.empty() )
		return m_sDisplayName;
	else if( !m_sLastUsedHighScoreName.empty() )
		return m_sLastUsedHighScoreName;
	else
		return RString();
}

Character *Profile::GetCharacter() const
{
	vector<Character*> vpCharacters;
	CHARMAN->GetCharacters( vpCharacters );
	FOREACH_CONST( Character*, vpCharacters, c )
	{
		if( (*c)->m_sCharacterID.CompareNoCase(m_sCharacterID)==0 )
			return *c;
	}
	return CHARMAN->GetDefaultCharacter();
}

static RString FormatCalories( float fCals )
{
	return Commify((int)fCals) + " Cal";
}

int Profile::GetCalculatedWeightPounds() const
{
	if( m_iWeightPounds == 0 )	// weight not entered
		return DEFAULT_WEIGHT_POUNDS;
	else 
		return m_iWeightPounds;
}

RString Profile::GetDisplayTotalCaloriesBurned() const
{
	return FormatCalories( m_fTotalCaloriesBurned );
}

RString Profile::GetDisplayTotalCaloriesBurnedToday() const
{
	float fCals = GetCaloriesBurnedToday();
	return FormatCalories( fCals );
}

float Profile::GetCaloriesBurnedToday() const
{
	DateTime now = DateTime::GetNowDate();
	return GetCaloriesBurnedForDay(now);
}

int Profile::GetTotalNumSongsPassed() const
{
	int iTotal = 0;
	FOREACHM_CONST( PlayMode, int, m_iNumStagesPassedByPlayMode, iter )
		iTotal += iter->second;
	return iTotal;
}

int Profile::GetTotalStepsWithTopGrade( StepsType st, Difficulty d, Grade g ) const
{
	int iCount = 0;

	FOREACH_CONST( Song*, SONGMAN->GetAllSongs(), pSong )
	{
		if( (*pSong)->m_SelectionDisplay == ShowSong_Never )
			continue;	// skip

		FOREACH_CONST( Steps*, (*pSong)->GetAllSteps(), pSteps )
		{
			if( (*pSteps)->m_StepsType != st )
				continue;	// skip

			if( (*pSteps)->GetDifficulty() != d )
				continue;	// skip

			const HighScoreList &hsl = GetStepsHighScoreList( *pSong, *pSteps );
			if( hsl.vHighScores.empty() )
				continue;	// skip

			if( hsl.vHighScores[0].GetGrade() == g )
				iCount++;
		}
	}

	return iCount;
}

int Profile::GetTotalTrailsWithTopGrade( StepsType st, CourseDifficulty d, Grade g ) const
{
	int iCount = 0;

	// add course high scores
	vector<Course*> vCourses;
	SONGMAN->GetAllCourses( vCourses, false );
	FOREACH_CONST( Course*, vCourses, pCourse )
	{
		// Don't count any course that has any entries that change over time.
		if( !(*pCourse)->AllSongsAreFixed() )
			continue;

		vector<Trail*> vTrails;
		Trail* pTrail = (*pCourse)->GetTrail( st, d );
		if( pTrail == NULL )
			continue;

		const HighScoreList &hsl = GetCourseHighScoreList( *pCourse, pTrail );
		if( hsl.vHighScores.empty() )
			continue;	// skip

		if( hsl.vHighScores[0].GetGrade() == g )
			iCount++;
	}

	return iCount;
}

float Profile::GetSongsPossible( StepsType st, Difficulty dc ) const
{
	int iTotalSteps = 0;

	// add steps high scores
	const vector<Song*> vSongs = SONGMAN->GetAllSongs();
	for( unsigned i=0; i<vSongs.size(); i++ )
	{
		Song* pSong = vSongs[i];
		
		if( pSong->m_SelectionDisplay == ShowSong_Never )
			continue;	// skip

		vector<Steps*> vSteps = pSong->GetAllSteps();
		for( unsigned j=0; j<vSteps.size(); j++ )
		{
			Steps* pSteps = vSteps[j];
			
			if( pSteps->m_StepsType != st )
				continue;	// skip

			if( pSteps->GetDifficulty() != dc )
				continue;	// skip

			iTotalSteps++;
		}
	}

	return (float) iTotalSteps;
}

float Profile::GetSongsActual( StepsType st, Difficulty dc ) const
{
	CHECKPOINT_M( ssprintf("Profile::GetSongsActual(%d,%d)",st,dc) );
	
	float fTotalPercents = 0;
	
	// add steps high scores
	FOREACHM_CONST( SongID, HighScoresForASong, m_SongHighScores, i )
	{
		const SongID &id = i->first;
		Song* pSong = id.ToSong();
		
		CHECKPOINT_M( ssprintf("Profile::GetSongsActual: %p", pSong) );
		
		// If the Song isn't loaded on the current machine, then we can't 
		// get radar values to compute dance points.
		if( pSong == NULL )
			continue;
		
		if( pSong->m_SelectionDisplay == ShowSong_Never )
			continue;	// skip
		
		CHECKPOINT_M( ssprintf("Profile::GetSongsActual: song %s", pSong->GetSongDir().c_str()) );
		const HighScoresForASong &hsfas = i->second;
		
		FOREACHM_CONST( StepsID, HighScoreList, hsfas.m_StepsHighScores, j )
		{
			const StepsID &id = j->first;
			Steps* pSteps = id.ToSteps( pSong, true );
			CHECKPOINT_M( ssprintf("Profile::GetSongsActual: song %p, steps %p", pSong, pSteps) );
			
			// If the Steps isn't loaded on the current machine, then we can't 
			// get radar values to compute dance points.
			if( pSteps == NULL )
				continue;
			
			if( pSteps->m_StepsType != st )
				continue;
			
			CHECKPOINT_M( ssprintf("Profile::GetSongsActual: n %s = %p", id.ToString().c_str(), pSteps) );
			if( pSteps->GetDifficulty() != dc )
				continue;	// skip
			CHECKPOINT;
			
			const HighScoreList& hsl = j->second;			
			fTotalPercents += hsl.GetTopScore().GetPercentDP();
		}
		CHECKPOINT;
	}

	return fTotalPercents;
}

float Profile::GetSongsPercentComplete( StepsType st, Difficulty dc ) const
{
	return GetSongsActual(st,dc) / GetSongsPossible(st,dc);
}

static void GetHighScoreCourses( vector<Course*> &vpCoursesOut )
{
	vpCoursesOut.clear();

	vector<Course*> vpCourses;
	SONGMAN->GetAllCourses( vpCourses, false );
	FOREACH_CONST( Course*, vpCourses, c )
	{
		// Don't count any course that has any entries that change over time.
		if( !(*c)->AllSongsAreFixed() )
			continue;

		vpCoursesOut.push_back( *c );
	}
}

float Profile::GetCoursesPossible( StepsType st, CourseDifficulty cd ) const
{
	int iTotalTrails = 0;

	vector<Course*> vpCourses;
	GetHighScoreCourses( vpCourses );
	FOREACH_CONST( Course*, vpCourses, c )
	{
		Trail* pTrail = (*c)->GetTrail(st,cd);
		if( pTrail == NULL )
			continue;

		iTotalTrails++;
	}
	
	return (float) iTotalTrails;
}
	
float Profile::GetCoursesActual( StepsType st, CourseDifficulty cd ) const
{
	float fTotalPercents = 0;
	
	vector<Course*> vpCourses;
	GetHighScoreCourses( vpCourses );
	FOREACH_CONST( Course*, vpCourses, c )
	{
		Trail *pTrail = (*c)->GetTrail( st, cd );
		if( pTrail == NULL )
			continue;

		const HighScoreList& hsl = GetCourseHighScoreList( *c, pTrail );
		fTotalPercents += hsl.GetTopScore().GetPercentDP();
	}

	return fTotalPercents;
}

float Profile::GetCoursesPercentComplete( StepsType st, CourseDifficulty cd ) const
{
	return GetCoursesActual(st,cd) / GetCoursesPossible(st,cd);
}

float Profile::GetSongsAndCoursesPercentCompleteAllDifficulties( StepsType st ) const
{
	float fActual = 0;
	float fPossible = 0;
	FOREACH_Difficulty( d )
	{
		fActual += GetSongsActual(st,d);
		fPossible += GetSongsPossible(st,d);
	}
	FOREACH_CourseDifficulty( d )
	{
		fActual += GetCoursesActual(st,d);
		fPossible += GetCoursesPossible(st,d);
	}
	return fActual / fPossible;
}

int Profile::GetSongNumTimesPlayed( const Song* pSong ) const
{
	SongID songID;
	songID.FromSong( pSong );
	return GetSongNumTimesPlayed( songID );
}

int Profile::GetSongNumTimesPlayed( const SongID& songID ) const
{
	const HighScoresForASong *hsSong = GetHighScoresForASong( songID );
	if( hsSong == NULL )
		return 0;

	int iTotalNumTimesPlayed = 0;
	FOREACHM_CONST( StepsID, HighScoreList, hsSong->m_StepsHighScores, j )
	{
		const HighScoreList &hsSteps = j->second;

		iTotalNumTimesPlayed += hsSteps.GetNumTimesPlayed();
	}
	return iTotalNumTimesPlayed;
}

/*
 * Get the profile default modifiers.  Return true if set, in which case sModifiersOut
 * will be set.  Return false if no modifier string is set, in which case the theme
 * defaults should be used.  Note that the null string means "no modifiers are active",
 * which is distinct from no modifier string being set at all.
 *
 * In practice, we get the default modifiers from the theme the first time a game
 * is played, and from the profile every time thereafter.
 */
bool Profile::GetDefaultModifiers( const Game* pGameType, RString &sModifiersOut ) const
{
	map<RString,RString>::const_iterator it;
	it = m_DefaultModifiersByGame.find( pGameType->m_szName );
	if( it == m_DefaultModifiersByGame.end() )
		return false;
	sModifiersOut = it->second;
	return true;
}

void Profile::SetDefaultModifiers( const Game* pGameType, const RString &sModifiers )
{
	if( sModifiers == "" )
		m_DefaultModifiersByGame.erase( pGameType->m_szName );
	else
		m_DefaultModifiersByGame[pGameType->m_szName] = sModifiers;
}

bool Profile::IsCodeUnlocked( RString sUnlockEntryID ) const
{
	return m_UnlockedEntryIDs.find( sUnlockEntryID ) != m_UnlockedEntryIDs.end();
}


Song *Profile::GetMostPopularSong() const
{
	int iMaxNumTimesPlayed = 0;
	SongID id;
	FOREACHM_CONST( SongID, HighScoresForASong, m_SongHighScores, i )
	{
		int iNumTimesPlayed = i->second.GetNumTimesPlayed();
		if( iNumTimesPlayed > iMaxNumTimesPlayed )
		{
			id = i->first;
			iMaxNumTimesPlayed = iNumTimesPlayed;
		}
	}

	return id.ToSong();
}

Course *Profile::GetMostPopularCourse() const
{
	int iMaxNumTimesPlayed = 0;
	CourseID id;
	FOREACHM_CONST( CourseID, HighScoresForACourse, m_CourseHighScores, i )
	{
		int iNumTimesPlayed = i->second.GetNumTimesPlayed();
		if( iNumTimesPlayed > iMaxNumTimesPlayed )
		{
			id = i->first;
			iMaxNumTimesPlayed = iNumTimesPlayed;
		}
	}

	return id.ToCourse();
}

//
// Steps high scores
//
void Profile::AddStepsHighScore( const Song* pSong, const Steps* pSteps, HighScore hs, int &iIndexOut )
{
	GetStepsHighScoreList(pSong,pSteps).AddHighScore( hs, iIndexOut, IsMachine() );
}

const HighScoreList& Profile::GetStepsHighScoreList( const Song* pSong, const Steps* pSteps ) const
{
	return ((Profile*)this)->GetStepsHighScoreList(pSong,pSteps);
}

HighScoreList& Profile::GetStepsHighScoreList( const Song* pSong, const Steps* pSteps )
{
	SongID songID;
	songID.FromSong( pSong );
	
	StepsID stepsID;
	stepsID.FromSteps( pSteps );
	
	HighScoresForASong &hsSong = m_SongHighScores[songID];	// operator[] inserts into map
	HighScoreList &hsSteps = hsSong.m_StepsHighScores[stepsID];	// operator[] inserts into map

	return hsSteps;
}

int Profile::GetStepsNumTimesPlayed( const Song* pSong, const Steps* pSteps ) const
{
	return GetStepsHighScoreList(pSong,pSteps).GetNumTimesPlayed();
}

DateTime Profile::GetSongLastPlayedDateTime( const Song* pSong ) const
{
	SongID id;
	id.FromSong( pSong );
	std::map<SongID,HighScoresForASong>::const_iterator iter = m_SongHighScores.find( id );

	// don't call this unless has been played once
	ASSERT( iter != m_SongHighScores.end() );
	ASSERT( !iter->second.m_StepsHighScores.empty() );

	DateTime dtLatest;	// starts out zeroed
	FOREACHM_CONST( StepsID, HighScoreList, iter->second.m_StepsHighScores, i )
	{
		const HighScoreList &hsl = i->second;
		if( hsl.GetNumTimesPlayed() == 0 )
			continue;
		if( dtLatest < hsl.GetLastPlayed() )
			dtLatest = hsl.GetLastPlayed();
	}
	return dtLatest;
}

bool Profile::HasPassedSteps( const Song* pSong, const Steps* pSteps ) const
{
	const HighScoreList &hsl = GetStepsHighScoreList( pSong, pSteps );
	Grade grade = hsl.GetTopScore().GetGrade();
	switch( grade )
	{
	case Grade_Failed:
	case Grade_NoData:
		return false;
	default:
		return true;
	}
}

bool Profile::HasPassedAnyStepsInSong( const Song* pSong ) const
{
	FOREACH_CONST( Steps*, pSong->GetAllSteps(), steps )
	{
		if( HasPassedSteps( pSong, *steps ) )
			return true;
	}
	return false;
}

void Profile::IncrementStepsPlayCount( const Song* pSong, const Steps* pSteps )
{
	DateTime now = DateTime::GetNowDate();
	GetStepsHighScoreList(pSong,pSteps).IncrementPlayCount( now );
}

void Profile::GetGrades( const Song* pSong, StepsType st, int iCounts[NUM_Grade] ) const
{
	SongID songID;
	songID.FromSong( pSong );

	
	memset( iCounts, 0, sizeof(int)*NUM_Grade );
	const HighScoresForASong *hsSong = GetHighScoresForASong( songID );
	if( hsSong == NULL )
		return;

	FOREACH_Grade(g)
	{
		FOREACHM_CONST( StepsID, HighScoreList, hsSong->m_StepsHighScores, it )
		{
			const StepsID &id = it->first;
			if( !id.MatchesStepsType(st) )
				continue;

			const HighScoreList &hsSteps = it->second;
			if( hsSteps.GetTopScore().GetGrade() == g )
				iCounts[g]++;
		}
	}
}

//
// Course high scores
//
void Profile::AddCourseHighScore( const Course* pCourse, const Trail* pTrail, HighScore hs, int &iIndexOut )
{
	GetCourseHighScoreList(pCourse,pTrail).AddHighScore( hs, iIndexOut, IsMachine() );
}

const HighScoreList& Profile::GetCourseHighScoreList( const Course* pCourse, const Trail* pTrail ) const
{
	return ((Profile *)this)->GetCourseHighScoreList( pCourse, pTrail );
}

HighScoreList& Profile::GetCourseHighScoreList( const Course* pCourse, const Trail* pTrail )
{
	CourseID courseID;
	courseID.FromCourse( pCourse );

	TrailID trailID;
	trailID.FromTrail( pTrail );

	HighScoresForACourse &hsCourse = m_CourseHighScores[courseID];	// operator[] inserts into map
	HighScoreList &hsl = hsCourse.m_TrailHighScores[trailID];	// operator[] inserts into map
	return hsl;
}

int Profile::GetCourseNumTimesPlayed( const Course* pCourse ) const
{
	CourseID courseID;
	courseID.FromCourse( pCourse );

	return GetCourseNumTimesPlayed( courseID );
}

int Profile::GetCourseNumTimesPlayed( const CourseID &courseID ) const
{
	const HighScoresForACourse *hsCourse = GetHighScoresForACourse( courseID );
	if( hsCourse == NULL )
		return 0;

	int iTotalNumTimesPlayed = 0;
	FOREACHM_CONST( TrailID, HighScoreList, hsCourse->m_TrailHighScores, j )
	{
		const HighScoreList &hsl = j->second;
		iTotalNumTimesPlayed += hsl.GetNumTimesPlayed();
	}
	return iTotalNumTimesPlayed;
}

DateTime Profile::GetCourseLastPlayedDateTime( const Course* pCourse ) const
{
	CourseID id;
	id.FromCourse( pCourse );
	std::map<CourseID,HighScoresForACourse>::const_iterator iter = m_CourseHighScores.find( id );

	// don't call this unless has been played once
	ASSERT( iter != m_CourseHighScores.end() );
	ASSERT( !iter->second.m_TrailHighScores.empty() );

	DateTime dtLatest;	// starts out zeroed
	FOREACHM_CONST( TrailID, HighScoreList, iter->second.m_TrailHighScores, i )
	{
		const HighScoreList &hsl = i->second;
		if( hsl.GetNumTimesPlayed() == 0 )
			continue;
		if( dtLatest < hsl.GetLastPlayed() )
			dtLatest = hsl.GetLastPlayed();
	}
	return dtLatest;
}

void Profile::IncrementCoursePlayCount( const Course* pCourse, const Trail* pTrail )
{
	DateTime now = DateTime::GetNowDate();
	GetCourseHighScoreList(pCourse,pTrail).IncrementPlayCount( now );
}

//
// Category high scores
//
void Profile::AddCategoryHighScore( StepsType st, RankingCategory rc, HighScore hs, int &iIndexOut )
{
	m_CategoryHighScores[st].m_v[rc].AddHighScore( hs, iIndexOut, IsMachine() );
}

const HighScoreList& Profile::GetCategoryHighScoreList( StepsType st, RankingCategory rc ) const
{
	return ((Profile *)this)->m_CategoryHighScores[st].m_v[rc];
}

HighScoreList& Profile::GetCategoryHighScoreList( StepsType st, RankingCategory rc )
{
	return m_CategoryHighScores[st].m_v[rc];
}

int Profile::GetCategoryNumTimesPlayed( StepsType st ) const
{
	int iNumTimesPlayed = 0;
	map<StepsType,RankingCategoryToHighScoreList>::const_iterator iter1 = m_CategoryHighScores.find(st);
	if( iter1 == m_CategoryHighScores.end() )
		return 0;
	FOREACHM_CONST( RankingCategory, HighScoreList, iter1->second.m_v, iter2 )
		iNumTimesPlayed += iter2->second.GetNumTimesPlayed();
	return iNumTimesPlayed;
}

void Profile::IncrementCategoryPlayCount( StepsType st, RankingCategory rc )
{
	DateTime now = DateTime::GetNowDate();
	m_CategoryHighScores[st].m_v[rc].IncrementPlayCount( now );
}


//
// Loading and saving
//

#define WARN_PARSER	LOG->Warn("Error parsing file at %s:%d",__FILE__,__LINE__);
#define WARN_AND_RETURN { WARN_PARSER; return; }
#define WARN_AND_CONTINUE { WARN_PARSER; continue; }
#define WARN_AND_BREAK { WARN_PARSER; break; }
#define WARN_M(m)	LOG->Warn("Error parsing file at %s:%d: %s",__FILE__,__LINE__, (const char*) (m) );
#define WARN_AND_RETURN_M(m) { WARN_M(m); return; }
#define WARN_AND_CONTINUE_M(m) { WARN_M(m); continue; }
#define WARN_AND_BREAK_M(m) { WARN_M(m); break; }

ProfileLoadResult Profile::LoadAllFromDir( RString sDir, bool bRequireSignature )
{
	CHECKPOINT;

	LOG->Trace( "Profile::LoadAllFromDir( %s )", sDir.c_str() );

	ASSERT( sDir.Right(1) == "/" );

	InitAll();

	// Not critical if this fails
	LoadEditableDataFromDir( sDir );
	
	// Check for the existance of stats.xml
	RString fn = sDir + STATS_JSON;
	if( !IsAFile(fn) )
		return ProfileLoadResult_FailedNoProfile;

	//
	// Don't unreasonably large stats.xml files.
	//
	if( !IsMachine() )	// only check stats coming from the player
	{
		int iBytes = FILEMAN->GetFileSizeInBytes( fn );
		if( iBytes > MAX_PLAYER_STATS_JSON_SIZE_BYTES )
		{
			LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
			return ProfileLoadResult_FailedTampered;
		}
	}

	if( bRequireSignature )
	{ 
		RString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
		RString sDontShareFile = sDir + DONT_SHARE_SIG;

		LOG->Trace( "Verifying don't share signature" );
		// verify the stats.xml signature with the "don't share" file
		if( !CryptManager::VerifyFileWithFile(sStatsXmlSigFile, sDontShareFile) )
		{
			LOG->Warn( "The don't share check for '%s' failed.  Data will be ignored.", sStatsXmlSigFile.c_str() );
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace( "Done." );

		// verify stats.xml
		LOG->Trace( "Verifying stats.xml signature" );
		if( !CryptManager::VerifyFileWithFile(fn, sStatsXmlSigFile) )
		{
			LOG->Warn( "The signature check for '%s' failed.  Data will be ignored.", fn.c_str() );
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace( "Done." );
	}

	LOG->Trace( "Loading %s", fn.c_str() );
	Json::Value root;
	if( !JsonUtil::LoadFromFileShowErrors(root, fn) )
		return ProfileLoadResult_FailedTampered;
	LOG->Trace( "Done." );

	return LoadStatsJson( root );
}

RString DateTimeToString( const DateTime &dt )
{
	return dt.GetString();
}
DateTime StringToDateTime( const RString &s )
{
	DateTime dt;
	dt.FromString(s);
	return dt;
}

ProfileLoadResult Profile::LoadStatsJson( const Json::Value &root, bool bIgnoreEditable )
{
	/* These are loaded from Editable, so we usually want to ignore them
	 * here. */
	RString sName = m_sDisplayName;
	RString sCharacterID = m_sCharacterID;
	RString sLastUsedHighScoreName = m_sLastUsedHighScoreName;
	int iWeightPounds = m_iWeightPounds;

	LoadGeneral( root["General"] );
	JsonUtil::DeserializeObjectToObjectMapAsArray( m_SongHighScores, "Song", "HighScoresForASong", root["SongScores"] );
	JsonUtil::DeserializeObjectToObjectMapAsArray( m_CourseHighScores, "Course", "HighScoresForACourse", root["CourseScores"] );
	JsonUtil::DeserializeStringToObjectMap( m_CategoryHighScores, StringToStepsType, root["CategoryScores"] );
	JsonUtil::DeserializeArrayObjects( m_vScreenshots, root["ScreenshotData"] );
	JsonUtil::DeserializeStringToValueMap( m_mapDayToCaloriesBurned, StringToDateTime, root["CalorieData"] );
	JsonUtil::DeserializeArrayObjects( m_vRecentStepsScores, root["RecentSongScores"] );
	JsonUtil::DeserializeArrayObjects( m_vRecentCourseScores, root["RecentCourseScores"] );
		
	if( bIgnoreEditable )
	{
		m_sDisplayName = sName;
		m_sCharacterID = sCharacterID;
		m_sLastUsedHighScoreName = sLastUsedHighScoreName;
		m_iWeightPounds = iWeightPounds;
	}

	return ProfileLoadResult_Success;
}

bool Profile::SaveAllToDir( RString sDir, bool bSignData ) const
{
	m_sLastPlayedMachineGuid = PROFILEMAN->GetMachineProfile()->m_sGuid;
	m_LastPlayedDate = DateTime::GetNowDate();

	// Save editable.ini
	SaveEditableDataToDir( sDir );

	bool bSaved = SaveStatsJsonToDir( sDir, bSignData );
	
	SaveStatsWebPageToDir( sDir );

	// Empty directories if none exist.
	FILEMAN->CreateDir( sDir + EDIT_STEPS_SUBDIR );
	FILEMAN->CreateDir( sDir + EDIT_COURSES_SUBDIR );
	FILEMAN->CreateDir( sDir + SCREENSHOTS_SUBDIR );

	FlushDirCache();

	return bSaved;
}


void Profile::RankingCategoryToHighScoreList::Serialize( Json::Value &root ) const
{
	JsonUtil::SerializeStringToObjectMap( m_v, RankingCategoryToString, root );
}

void Profile::RankingCategoryToHighScoreList::Deserialize( const Json::Value &root )
{
	JsonUtil::DeserializeStringToObjectMap( m_v, StringToRankingCategory, root );
}

bool Profile::SaveStatsJsonToDir( RString sDir, bool bSignData ) const
{
	Json::Value root;
	SaveGeneral( root["General"] );
	JsonUtil::SerializeObjectToObjectMapAsArray( m_SongHighScores, "Song", "HighScoresForASong", root["SongScores"] );
	JsonUtil::SerializeObjectToObjectMapAsArray( m_CourseHighScores, "Course", "HighScoresForACourse", root["CourseScores"] );
	JsonUtil::SerializeStringToObjectMap( m_CategoryHighScores, StepsTypeToString, root["CategoryScores"] );
	JsonUtil::SerializeArrayObjects( m_vScreenshots, root["ScreenshotData"] );
	JsonUtil::SerializeStringToValueMap( m_mapDayToCaloriesBurned, DateTimeToString, root["CalorieData"] );
	JsonUtil::SerializeArrayObjects( m_vRecentStepsScores, root["RecentSongScores"] );
	JsonUtil::SerializeArrayObjects( m_vRecentCourseScores, root["RecentCourseScores"] );

	RString fn = sDir + STATS_JSON;
	bool bSaved = JsonUtil::WriteFile( root, fn, false );
	
	// Update file cache, or else IsAFile in CryptManager won't see this new file.
	FILEMAN->FlushDirCache( sDir );
	
	if( bSaved && bSignData )
	{
		RString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
		CryptManager::SignFileToFile(fn, sStatsXmlSigFile);

		// Update file cache, or else IsAFile in CryptManager won't see sStatsXmlSigFile.
		FILEMAN->FlushDirCache( sDir );

		// Save the "don't share" file
		RString sDontShareFile = sDir + DONT_SHARE_SIG;
		CryptManager::SignFileToFile(sStatsXmlSigFile, sDontShareFile);
	}

	return bSaved;
}

void Profile::SaveEditableDataToDir( RString sDir ) const
{
	IniFile ini;

	ini.SetValue( "Editable", "DisplayName",			m_sDisplayName );
	ini.SetValue( "Editable", "CharacterID",			m_sCharacterID );
	ini.SetValue( "Editable", "LastUsedHighScoreName",		m_sLastUsedHighScoreName );
	ini.SetValue( "Editable", "WeightPounds",			m_iWeightPounds );

	ini.WriteFile( sDir + EDITABLE_INI );
}

const RString MeterToString(int i)
{
	return ssprintf("%d",i);
}

int StringToMeter(const RString &s)
{
	return atoi(s);
}

void Profile::SaveGeneral( Json::Value &root ) const
{
	// These are write-only elements that are normally never read again.
	// This data is required by other apps (like internet ranking), but is 
	// redundant to the game app.
	root["DisplayName"] =			GetDisplayNameOrHighScoreName();
	root["CharacterID"] = 			m_sCharacterID;
	root["LastUsedHighScoreName"] = 	m_sLastUsedHighScoreName;
	root["WeightPounds"] = 			m_iWeightPounds;
	root["IsMachine"] = 			IsMachine();
	root["IsWeightSet"] = 			m_iWeightPounds != 0;

	root["Guid"] = 				m_sGuid;
	root["SortOrder"] = 			SortOrderToString(m_SortOrder);
	root["LastDifficulty"] = 		DifficultyToString(m_LastDifficulty);
	root["LastCourseDifficulty"] = 		CourseDifficultyToString(m_LastCourseDifficulty);
	m_lastSong.Serialize( root["LastSong"] );
	m_lastCourse.Serialize( root["LastCourse"] );
	root["TotalPlays"] = 			m_iTotalPlays;
	root["TotalPlaySeconds"] = 		m_iTotalPlaySeconds;
	root["TotalGameplaySeconds"] = 		m_iTotalGameplaySeconds;
	root["CurrentCombo"] = 			m_iCurrentCombo;
	root["TotalCaloriesBurned"] = 		m_fTotalCaloriesBurned;
	root["GoalType"] = 			m_GoalType;
	root["GoalCalories"] = 			m_iGoalCalories;
	root["GoalSeconds"] = 			m_iGoalSeconds;
	root["LastPlayedMachineGuid"] = 	m_sLastPlayedMachineGuid;
	root["LastPlayedDate"] = 		m_LastPlayedDate.GetString();
	root["TotalDancePoints"] = 		m_iTotalDancePoints;
	root["NumExtraStagesPassed"] = 		m_iNumExtraStagesPassed;
	root["NumExtraStagesFailed"] = 		m_iNumExtraStagesFailed;
	root["NumToasties"] = 			m_iNumToasties;
	root["TotalTapsAndHolds"] = 		m_iTotalTapsAndHolds;
	root["TotalJumps"] = 			m_iTotalJumps;
	root["TotalHolds"] = 			m_iTotalHolds;
	root["TotalRolls"] = 			m_iTotalRolls;
	root["TotalMines"] = 			m_iTotalMines;
	root["TotalHands"] = 			m_iTotalHands;
	root["SavedLuaData"] = 			m_SavedLuaData.Serialize();
	root["NumTotalSongsPlayed"] =		m_iNumTotalSongsPlayed;

	JsonUtil::SerializeValueToValueMap( m_DefaultModifiersByGame, root["DefaultModifiersByGame"] );
	JsonUtil::SerializeArrayValues( m_UnlockedEntryIDs, root["UnlockEntryIDs"] );
	JsonUtil::SerializeStringToValueMap( m_iNumSongsPlayedByPlayMode, PlayModeToString, root["NumSongsPlayedByPlayMode"] );
	JsonUtil::SerializeObjectToValueMapAsArray(m_iNumSongsPlayedByStyle, "Style", "NumPlays", root["NumSongsPlayedByStyle"] );
	JsonUtil::SerializeStringToValueMap( m_iNumSongsPlayedByDifficulty, DifficultyToString, root["NumSongsPlayedByDifficulty"] );
	JsonUtil::SerializeStringToValueMap( m_iNumSongsPlayedByMeter, MeterToString, root["NumSongsPlayedByMeter"] );
	JsonUtil::SerializeStringToValueMap( m_iNumStagesPassedByPlayMode, PlayModeToString, root["NumStagesPassedByPlayMode"] );
	JsonUtil::SerializeStringToValueMap( m_iNumStagesPassedByGrade, GradeToString, root["NumStagesPassedByGrade"] );
}

ProfileLoadResult Profile::LoadEditableDataFromDir( RString sDir )
{
	RString fn = sDir + EDITABLE_INI;

	//
	// Don't load unreasonably large editable.xml files.
	//
	int iBytes = FILEMAN->GetFileSizeInBytes( fn );
	if( iBytes > MAX_EDITABLE_INI_SIZE_BYTES )
	{
		LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
		return ProfileLoadResult_FailedTampered;
	}

	if( !IsAFile(fn) )
		return ProfileLoadResult_FailedNoProfile;

	IniFile ini;
	ini.ReadFile( fn );

	ini.GetValue( "Editable", "DisplayName",			m_sDisplayName );
	ini.GetValue( "Editable", "CharacterID",			m_sCharacterID );
	ini.GetValue( "Editable", "LastUsedHighScoreName",		m_sLastUsedHighScoreName );
	ini.GetValue( "Editable", "WeightPounds",			m_iWeightPounds );

	// This is data that the user can change, so we have to validate it.
	wstring wstr = RStringToWstring(m_sDisplayName);
	if( wstr.size() > PROFILE_MAX_DISPLAY_NAME_LENGTH )
		wstr = wstr.substr(0, PROFILE_MAX_DISPLAY_NAME_LENGTH);
	m_sDisplayName = WStringToRString(wstr);
	// TODO: strip invalid chars?
	if( m_iWeightPounds != 0 )
		CLAMP( m_iWeightPounds, 20, 1000 );

	return ProfileLoadResult_Success;
}

void Profile::LoadGeneral( const Json::Value &root )
{
	root["DisplayName"].TryGet(		m_sDisplayName );
	root["CharacterID"].TryGet(		m_sCharacterID );
	root["LastUsedHighScoreName"].TryGet(	m_sLastUsedHighScoreName );
	root["WeightPounds"].TryGet(		m_iWeightPounds );
	root["Guid"].TryGet(			m_sGuid );
	m_SortOrder = StringToSortOrder( root["SortOrder"].asString() );
	m_LastDifficulty = StringToDifficulty( root["LastDifficulty"].asString() );
	m_LastCourseDifficulty = StringToCourseDifficulty( root["LastCourseDifficulty"].asString() );
	m_lastSong.Deserialize( root["LastSong"] );
	m_lastCourse.Deserialize( root["LastCourse"] );
	root["TotalPlays"].TryGet(		m_iTotalPlays );
	root["TotalPlaySeconds"].TryGet(	m_iTotalPlaySeconds );
	root["TotalGameplaySeconds"].TryGet(	m_iTotalGameplaySeconds );
	root["CurrentCombo"].TryGet(		m_iCurrentCombo );
	root["TotalCaloriesBurned"].TryGet(	m_fTotalCaloriesBurned );
	root["GoalType"].TryGet(		(int&)m_GoalType );
	root["GoalCalories"].TryGet(		m_iGoalCalories );
	root["GoalSeconds"].TryGet(		m_iGoalSeconds );
	root["LastPlayedMachineGuid"].TryGet(	m_sLastPlayedMachineGuid );
	m_LastPlayedDate.FromString( root["LastPlayedDate"].asString() );
	root["TotalDancePoints"].TryGet(	m_iTotalDancePoints );
	root["NumExtraStagesPassed"].TryGet(	m_iNumExtraStagesPassed );
	root["NumExtraStagesFailed"].TryGet(	m_iNumExtraStagesFailed );
	root["NumToasties"].TryGet(		m_iNumToasties );
	root["TotalTapsAndHolds"].TryGet(	m_iTotalTapsAndHolds );
	root["TotalJumps"].TryGet(		m_iTotalJumps );
	root["TotalHolds"].TryGet(		m_iTotalHolds );
	root["TotalRolls"].TryGet(		m_iTotalRolls );
	root["TotalMines"].TryGet(		m_iTotalMines );
	root["TotalHands"].TryGet(		m_iTotalHands );
	root["NumTotalSongsPlayed"].TryGet(	m_iNumTotalSongsPlayed );

	{
		m_SavedLuaData.LoadFromString( root["SavedLuaData"].asString() );
		if( m_SavedLuaData.GetLuaType() != LUA_TTABLE )
		{
			LOG->Warn( "Profile data did not evaluate to a table" );
			Lua *L = LUA->Get();
			lua_newtable( L );
			m_SavedLuaData.SetFromStack( L );
			LUA->Release( L );
		}
	}

	JsonUtil::DeserializeValueToValueMap( m_DefaultModifiersByGame, root["DefaultModifiersByGame"] );
	JsonUtil::DeserializeArrayValuesIntoSet<set<RString>, RString>( m_UnlockedEntryIDs, root["UnlockedEntryID"] );
	JsonUtil::DeserializeStringToValueMap( m_iNumSongsPlayedByPlayMode, StringToPlayMode, root["NumSongsPlayedByPlayMode"] );
	JsonUtil::DeserializeObjectToValueMapAsArray( m_iNumSongsPlayedByStyle, "Style", "NumPlays", root["NumSongsPlayedByStyle"] );
	JsonUtil::DeserializeStringToValueMap( m_iNumSongsPlayedByDifficulty, StringToDifficulty, root["NumSongsPlayedByDifficulty"] );
	JsonUtil::DeserializeStringToValueMap( m_iNumSongsPlayedByMeter, StringToMeter, root["NumSongsPlayedByMeter"] );
	JsonUtil::DeserializeStringToValueMap( m_iNumStagesPassedByPlayMode, StringToPlayMode, root["NumStagesPassedByPlayMode"] );
	JsonUtil::DeserializeStringToValueMap( m_iNumStagesPassedByGrade, StringToGrade, root["NumStagesPassedByGrade"] );
}

void Profile::AddStepTotals( int iTotalTapsAndHolds, int iTotalJumps, int iTotalHolds, int iTotalRolls, int iTotalMines, int iTotalHands, float fCaloriesBurned )
{
	m_iTotalTapsAndHolds += iTotalTapsAndHolds;
	m_iTotalJumps += iTotalJumps;
	m_iTotalHolds += iTotalHolds;
	m_iTotalRolls += iTotalRolls;
	m_iTotalMines += iTotalMines;
	m_iTotalHands += iTotalHands;

	m_fTotalCaloriesBurned += fCaloriesBurned;

	DateTime date = DateTime::GetNowDate();
	m_mapDayToCaloriesBurned[date] = GetCaloriesBurnedForDay(date) + fCaloriesBurned;	// GetCaloriesBurnedForDay returns 0 if date not already in map
}

void Profile::HighScoresForASong::Serialize( Json::Value &root ) const
{
	JsonUtil::SerializeObjectToObjectMapAsArray( m_StepsHighScores, "Steps", "HighScoreList", root );
}

bool Profile::HighScoresForASong::Deserialize( const Json::Value &root )
{
	JsonUtil::DeserializeObjectToObjectMapAsArray( m_StepsHighScores, "Steps", "HighScoreList", root );
	return true;
}

void Profile::HighScoresForACourse::Serialize( Json::Value &root ) const
{
	JsonUtil::SerializeObjectToObjectMapAsArray( m_TrailHighScores, "Trail", "HighScoreList", root );
}

bool Profile::HighScoresForACourse::Deserialize( const Json::Value &root )
{
	JsonUtil::DeserializeObjectToObjectMapAsArray( m_TrailHighScores, "Trail", "HighScoreList", root );
	return true;
}

void Profile::SaveStatsWebPageToDir( RString sDir ) const
{
	ASSERT( PROFILEMAN );

	FileCopy( THEME->GetPathO("Profile",STATS_XSL), sDir+STATS_XSL );
	FileCopy( THEME->GetPathO("Profile",CATALOG_XSL), sDir+CATALOG_XSL );
	FileCopy( THEME->GetPathO("Profile",COMMON_XSL), sDir+COMMON_XSL );
	FileCopy( CATALOG_XML_FILE, sDir+CATALOG_XML );
}

void Profile::SaveMachinePublicKeyToDir( RString sDir ) const
{
	if( PREFSMAN->m_bSignProfileData && IsAFile(CRYPTMAN->GetPublicKeyFileName()) )
		FileCopy( CRYPTMAN->GetPublicKeyFileName(), sDir+PUBLIC_KEY_FILE );
}

void Profile::AddScreenshot( const Screenshot &screenshot )
{
	m_vScreenshots.push_back( screenshot );
}

float Profile::GetCaloriesBurnedForDay( DateTime day ) const
{
	day.StripTime();
	map<DateTime,float>::const_iterator i = m_mapDayToCaloriesBurned.find( day );
	if( i == m_mapDayToCaloriesBurned.end() )
		return 0;
	else
		return i->second;
}

XNode* Profile::HighScoreForASongAndSteps::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->m_sName = "HighScoreForASongAndSteps";

	pNode->AppendChild( songID.CreateNode() );
	pNode->AppendChild( stepsID.CreateNode() );
	pNode->AppendChild( hs.CreateNode() );

	return pNode;
}

void Profile::HighScoreForASongAndSteps::LoadFromNode( const XNode* pNode )
{
	Unset();

	ASSERT( pNode->m_sName == "HighScoreForASongAndSteps" );
	const XNode* p;
	if( (p = pNode->GetChild("Song")) )
		songID.LoadFromNode( p );
	if( (p = pNode->GetChild("Steps")) )
		stepsID.LoadFromNode( p );
	if( (p = pNode->GetChild("HighScore")) )
		hs.LoadFromNode( p );
}

void Profile::HighScoreForASongAndSteps::Serialize( Json::Value &root ) const
{
	songID.Serialize( root["Song"] );
	stepsID.Serialize( root["Steps"] );
	hs.Serialize( root["HighScore"] );
}

void Profile::HighScoreForASongAndSteps::Deserialize( const Json::Value &root )
{
	songID.Deserialize( root["Song"] );
	stepsID.Deserialize( root["Steps"] );
	hs.Deserialize( root["HighScore"] );
}

void Profile::AddStepsRecentScore( const Song* pSong, const Steps* pSteps, HighScore hs )
{
	HighScoreForASongAndSteps h;
	h.songID.FromSong( pSong );
	h.stepsID.FromSteps( pSteps );
	h.hs = hs;
	m_vRecentStepsScores.push_back( h );

	int iMaxRecentScoresToSave = IsMachine() ? PREFSMAN->m_iMaxRecentScoresForMachine : PREFSMAN->m_iMaxRecentScoresForPlayer;
	int iNumToErase = m_vRecentStepsScores.size() - iMaxRecentScoresToSave;
	if( iNumToErase > 0 )
		m_vRecentStepsScores.erase( m_vRecentStepsScores.begin(), m_vRecentStepsScores.begin() +  iNumToErase );
}


XNode* Profile::HighScoreForACourseAndTrail::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->m_sName = "HighScoreForACourseAndTrail";

	pNode->AppendChild( courseID.CreateNode() );
	pNode->AppendChild( trailID.CreateNode() );
	pNode->AppendChild( hs.CreateNode() );

	return pNode;
}

void Profile::HighScoreForACourseAndTrail::LoadFromNode( const XNode* pNode )
{
	Unset();

	ASSERT( pNode->m_sName == "HighScoreForACourseAndTrail" );
	const XNode* p;
	if( (p = pNode->GetChild("Course")) )
		courseID.LoadFromNode( p );
	if( (p = pNode->GetChild("Trail")) )
		trailID.LoadFromNode( p );
	if( (p = pNode->GetChild("HighScore")) )
		hs.LoadFromNode( p );
}

void Profile::HighScoreForACourseAndTrail::Serialize( Json::Value &root ) const
{
	courseID.Serialize( root["Course"] );
	trailID.Serialize( root["Trail"] );
	hs.Serialize( root["HighScore"] );
}

void Profile::HighScoreForACourseAndTrail::Deserialize( const Json::Value &root )
{
	courseID.Deserialize( root["Course"] );
	trailID.Deserialize( root["Trail"] );
	hs.Deserialize( root["HighScore"] );
}

void Profile::AddCourseRecentScore( const Course* pCourse, const Trail* pTrail, HighScore hs )
{
	HighScoreForACourseAndTrail h;
	h.courseID.FromCourse( pCourse );
	h.trailID.FromTrail( pTrail );
	h.hs = hs;
	m_vRecentCourseScores.push_back( h );
	
	int iMaxRecentScoresToSave = IsMachine() ? PREFSMAN->m_iMaxRecentScoresForMachine : PREFSMAN->m_iMaxRecentScoresForPlayer;
	int iNumToErase = m_vRecentCourseScores.size() - iMaxRecentScoresToSave;
	if( iNumToErase > 0 )
		m_vRecentCourseScores.erase( m_vRecentCourseScores.begin(), m_vRecentCourseScores.begin() +  iNumToErase );
}

StepsType Profile::GetLastPlayedStepsType() const
{
	if( m_vRecentStepsScores.empty() )
		return STEPS_TYPE_INVALID;
	const HighScoreForASongAndSteps &h = m_vRecentStepsScores.back();
	return h.stepsID.GetStepsType();
}

const Profile::HighScoresForASong *Profile::GetHighScoresForASong( const SongID& songID ) const
{
	map<SongID,HighScoresForASong>::const_iterator it;
	it = m_SongHighScores.find( songID );
	if( it == m_SongHighScores.end() )
		return NULL;
	return &it->second;
}

const Profile::HighScoresForACourse *Profile::GetHighScoresForACourse( const CourseID& courseID ) const
{
	map<CourseID,HighScoresForACourse>::const_iterator it;
	it = m_CourseHighScores.find( courseID );
	if( it == m_CourseHighScores.end() )
		return NULL;
	return &it->second;
}

bool Profile::IsMachine() const
{
	// TODO: Think of a better way to handle this
	return this == PROFILEMAN->GetMachineProfile();
}

void Profile::MoveBackupToDir( RString sFromDir, RString sToDir )
{
	FILEMAN->Move( sFromDir+EDITABLE_INI,			sToDir+EDITABLE_INI );
	FILEMAN->Move( sFromDir+STATS_JSON,			sToDir+STATS_JSON );
	FILEMAN->Move( sFromDir+STATS_JSON+SIGNATURE_APPEND,	sToDir+STATS_JSON+SIGNATURE_APPEND );
	FILEMAN->Move( sFromDir+DONT_SHARE_SIG,			sToDir+DONT_SHARE_SIG );
}

// lua start
#include "LuaBinding.h"

class LunaProfile: public Luna<Profile>
{
public:
	LunaProfile() { LUA->Register( Register ); }

	static int GetDisplayName( T* p, lua_State *L )			{ lua_pushstring(L, p->m_sDisplayName ); return 1; }
	static int GetCharacter( T* p, lua_State *L )			{ p->GetCharacter()->PushSelf(L); return 1; }
	static int GetWeightPounds( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_iWeightPounds ); return 1; }
	static int SetWeightPounds( T* p, lua_State *L )		{ p->m_iWeightPounds = IArg(1); return 0; }
	static int GetGoalType( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_GoalType ); return 1; }
	static int SetGoalType( T* p, lua_State *L )			{ p->m_GoalType = (GoalType)IArg(1); return 0; }
	static int GetGoalCalories( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_iGoalCalories ); return 1; }
	static int SetGoalCalories( T* p, lua_State *L )		{ p->m_iGoalCalories = IArg(1); return 0; }
	static int GetGoalSeconds( T* p, lua_State *L )			{ lua_pushnumber(L, p->m_iGoalSeconds ); return 1; }
	static int SetGoalSeconds( T* p, lua_State *L )			{ p->m_iGoalSeconds = IArg(1); return 0; }
	static int GetCaloriesBurnedToday( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetCaloriesBurnedToday() ); return 1; }
	static int GetSaved( T* p, lua_State *L )			{ p->m_SavedLuaData.PushSelf(L); return 1; }
	static int GetTotalNumSongsPlayed( T* p, lua_State *L )	{ lua_pushnumber(L, p->m_iNumTotalSongsPlayed ); return 1; }
	static int IsCodeUnlocked( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsCodeUnlocked(SArg(1)) ); return 1; }
	static int GetSongsActual( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetSongsActual((StepsType)IArg(1),(Difficulty)IArg(2)) ); return 1; }
	static int GetCoursesActual( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetCoursesActual((StepsType)IArg(1),(CourseDifficulty)IArg(2)) ); return 1; }
	static int GetSongsPossible( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetSongsPossible((StepsType)IArg(1),(Difficulty)IArg(2)) ); return 1; }
	static int GetCoursesPossible( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetCoursesPossible((StepsType)IArg(1),(CourseDifficulty)IArg(2)) ); return 1; }
	static int GetSongsPercentComplete( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetSongsPercentComplete((StepsType)IArg(1),(Difficulty)IArg(2)) ); return 1; }
	static int GetCoursesPercentComplete( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetCoursesPercentComplete((StepsType)IArg(1),(CourseDifficulty)IArg(2)) ); return 1; }
	static int GetTotalStepsWithTopGrade( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetTotalStepsWithTopGrade((StepsType)IArg(1),(Difficulty)IArg(2),(Grade)IArg(3)) ); return 1; }
	static int GetTotalTrailsWithTopGrade( T* p, lua_State *L )	{ lua_pushnumber(L, p->GetTotalTrailsWithTopGrade((StepsType)IArg(1),(CourseDifficulty)IArg(2),(Grade)IArg(3)) ); return 1; }
	static int GetNumTotalSongsPlayed( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_iNumTotalSongsPlayed ); return 1; }
	static int GetLastPlayedStepsType( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetLastPlayedStepsType() ); return 1; }
	static int GetSongsAndCoursesPercentCompleteAllDifficulties( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetSongsAndCoursesPercentCompleteAllDifficulties((StepsType)IArg(1)) ); return 1; }
	static int GetTotalCaloriesBurned( T* p, lua_State *L )		{ lua_pushnumber(L, p->m_fTotalCaloriesBurned ); return 1; }
	static int GetDisplayTotalCaloriesBurned( T* p, lua_State *L )	{ lua_pushstring(L, p->GetDisplayTotalCaloriesBurned() ); return 1; }
	static int GetMostPopularSong( T* p, lua_State *L )
	{
		Song *p2 = p->GetMostPopularSong();
		if( p2 )
			p2->PushSelf(L);
		else
			lua_pushnil( L );
		return 1;
	}
	static int GetMostPopularCourse( T* p, lua_State *L )
	{
		Course *p2 = p->GetMostPopularCourse();
		if( p2 )
			p2->PushSelf(L);
		else
			lua_pushnil( L );
		return 1;
	}
	static int GetSongNumTimesPlayed( T* p, lua_State *L )
	{
		ASSERT( !lua_isnil(L,1) );
		Song *pS = Luna<Song>::check(L,1);
		lua_pushnumber( L, p->GetSongNumTimesPlayed(pS) );
		return 1;
	}
	static int HasPassedAnyStepsInSong( T* p, lua_State *L )
	{
		ASSERT( !lua_isnil(L,1) );
		Song *pS = Luna<Song>::check(L,1);
		lua_pushboolean( L, p->HasPassedAnyStepsInSong(pS) );
		return 1;
	}

	static void Register(lua_State *L)
	{
		ADD_METHOD( GetDisplayName );
		ADD_METHOD( GetCharacter );
		ADD_METHOD( GetWeightPounds );
		ADD_METHOD( SetWeightPounds );
		ADD_METHOD( GetGoalType );
		ADD_METHOD( SetGoalType );
		ADD_METHOD( GetGoalCalories );
		ADD_METHOD( SetGoalCalories );
		ADD_METHOD( GetGoalSeconds );
		ADD_METHOD( SetGoalSeconds );
		ADD_METHOD( GetCaloriesBurnedToday );
		ADD_METHOD( GetSaved );
		ADD_METHOD( GetTotalNumSongsPlayed );
		ADD_METHOD( IsCodeUnlocked );
		ADD_METHOD( GetSongsActual );
		ADD_METHOD( GetCoursesActual );
		ADD_METHOD( GetSongsPossible );
		ADD_METHOD( GetCoursesPossible );
		ADD_METHOD( GetSongsPercentComplete );
		ADD_METHOD( GetCoursesPercentComplete );
		ADD_METHOD( GetTotalStepsWithTopGrade );
		ADD_METHOD( GetTotalTrailsWithTopGrade );
		ADD_METHOD( GetNumTotalSongsPlayed );
		ADD_METHOD( GetLastPlayedStepsType );
		ADD_METHOD( GetSongsAndCoursesPercentCompleteAllDifficulties );
		ADD_METHOD( GetTotalCaloriesBurned );
		ADD_METHOD( GetDisplayTotalCaloriesBurned );
		ADD_METHOD( GetMostPopularSong );
		ADD_METHOD( GetMostPopularCourse );
		ADD_METHOD( GetSongNumTimesPlayed );
		ADD_METHOD( HasPassedAnyStepsInSong );

		Luna<T>::Register( L );
	}
};

LUA_REGISTER_CLASS( Profile )
// lua end


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
