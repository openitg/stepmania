#include "global.h"
#include "ScreenOptionsMasterPrefs.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "AnnouncerManager.h"
#include "NoteSkinManager.h"
#include "PlayerOptions.h"
#include "SongOptions.h"
#include "RageDisplay.h"
#include "RageUtil.h"
#include "GameManager.h"
#include "GameState.h"
#include "InputMapper.h"
#include "StepMania.h"
#include "Game.h"
#include "Foreach.h"
#include "GameConstantsAndTypes.h"

static void GetDefaultModifiers( PlayerOptions &po, SongOptions &so )
{
	po.FromString( PREFSMAN->m_sDefaultModifiers );
	so.FromString( PREFSMAN->m_sDefaultModifiers );
}

static void SetDefaultModifiers( const PlayerOptions &po, const SongOptions &so )
{
	CStringArray as;
	if( po.GetString() != "" )
		as.push_back( po.GetString() );
	if( so.GetString() != "" )
		as.push_back( so.GetString() );

	PREFSMAN->m_sDefaultModifiers = join(", ",as);
}

template<class T>
static void MoveMap( int &sel, T &opt, bool ToSel, const T *mapping, unsigned cnt )
{
	if( ToSel )
	{
		/* opt -> sel.  Find the closest entry in mapping. */
		T best_dist = T();
		bool have_best = false;

		for( unsigned i = 0; i < cnt; ++i )
		{
			const T val = mapping[i];
			T dist = opt < val? (T)(val-opt):(T)(opt-val);
			if( have_best && best_dist < dist )
				continue;

			have_best = true;
			best_dist = dist;

			sel = i;
		}
	} else {
		/* sel -> opt */
		opt = mapping[sel];
	}
}


/* "sel" is the selection in the menu. */
template<class T>
static void MoveData( int &sel, T &opt, bool ToSel )
{
	if( ToSel )	(int&) sel = opt;
	else		opt = (T) sel;
}

template<>
static void MoveData( int &sel, bool &opt, bool ToSel )
{
	if( ToSel )	sel = opt;
	else		opt = !!sel;
}

#define MOVE( name, opt ) \
	static void name( int &sel, bool ToSel, const ConfOption *pConfOption ) \
	{ \
		MoveData( sel, opt, ToSel ); \
	}


static void GameChoices( CStringArray &out )
{
	vector<const Game*> aGames;
	GAMEMAN->GetEnabledGames( aGames );
	FOREACH( const Game*, aGames, g )
	{
		CString sGameName = (*g)->m_szName;
		sGameName.MakeUpper();
		out.push_back( sGameName );
	}
}

static void GameSel( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	CStringArray choices;
	pConfOption->MakeOptionsList( choices );

	if( ToSel )
	{
		const CString sCurGameName = GAMESTATE->m_pCurGame->m_szName;

		sel = 0;
		for(unsigned i = 0; i < choices.size(); ++i)
			if( !stricmp(choices[i], sCurGameName) )
				sel = i;
	} else {
		vector<const Game*> aGames;
		GAMEMAN->GetEnabledGames( aGames );
		ChangeCurrentGame( aGames[sel] );
	}
}

static void LanguageChoices( CStringArray &out )
{
	THEME->GetLanguages( out );
}

static void Language( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	CStringArray choices;
	pConfOption->MakeOptionsList( choices );

	if( ToSel )
	{
		sel = 0;
		for( unsigned i=1; i<choices.size(); i++ )
			if( !stricmp(choices[i], THEME->GetCurLanguage()) )
				sel = i;
	} else {
		const CString sNewLanguage = choices[sel];
		
		if( THEME->GetCurLanguage() != sNewLanguage )
			THEME->SwitchThemeAndLanguage( THEME->GetCurThemeName(), sNewLanguage );
	}
}

static void ThemeChoices( CStringArray &out )
{
	THEME->GetThemeNames( out );
}

static void Theme( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	CStringArray choices;
	pConfOption->MakeOptionsList( choices );

	if( ToSel )
	{
		sel = 0;
		for( unsigned i=1; i<choices.size(); i++ )
			if( !stricmp(choices[i], THEME->GetCurThemeName()) )
				sel = i;
	} else {
		const CString sNewTheme = choices[sel];
		if( THEME->GetCurThemeName() != sNewTheme )
			THEME->SwitchThemeAndLanguage( sNewTheme, THEME->GetCurLanguage() );
	}
}

static void AnnouncerChoices( CStringArray &out )
{
	ANNOUNCER->GetAnnouncerNames( out );
	out.insert( out.begin(), "OFF" );
}

static void Announcer( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	CStringArray choices;
	pConfOption->MakeOptionsList( choices );

	if( ToSel )
	{
		sel = 0;
		for( unsigned i=1; i<choices.size(); i++ )
			if( !stricmp(choices[i], ANNOUNCER->GetCurAnnouncerName()) )
				sel = i;
	} else {
		const CString sNewAnnouncer = sel? choices[sel]:CString("");
		ANNOUNCER->SwitchAnnouncer( sNewAnnouncer );
	}
}

static void DefaultNoteSkinChoices( CStringArray &out )
{
	NOTESKIN->GetNoteSkinNames( out );
	for( unsigned i = 0; i < out.size(); ++i )
		out[i].MakeUpper();
}

static void DefaultNoteSkin( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	CStringArray choices;
	pConfOption->MakeOptionsList( choices );

	if( ToSel )
	{
		PlayerOptions po;
		po.FromString( PREFSMAN->m_sDefaultModifiers );
		sel = 0;
		for( unsigned i=0; i < choices.size(); i++ )
			if( !stricmp(choices[i], po.m_sNoteSkin) )
				sel = i;
	} else {
		PlayerOptions po;
		SongOptions so;
		GetDefaultModifiers( po, so );
		po.m_sNoteSkin = choices[sel];
		SetDefaultModifiers( po, so );
	}
}

static void MovePref( int &iSel, bool bToSel, const ConfOption *pConfOption )
{
	IPreference *pPref = PREFSMAN->GetPreferenceByName( pConfOption->name );
	ASSERT_M( pPref != NULL, pConfOption->name );

	if( bToSel )
	{
		const CString sVal = pPref->ToString();
		iSel = atoi( sVal );
	}
	else
	{
		const CString sVal = ToString(iSel);
		pPref->FromString( sVal );
	}
}

/* Appearance options */
MOVE( Instructions,			PREFSMAN->m_bShowInstructions );
MOVE( Caution,				PREFSMAN->m_bShowCaution );
MOVE( OniScoreDisplay,		PREFSMAN->m_bDancePointsForOni );
MOVE( SongGroup,			PREFSMAN->m_bShowSelectGroup );
MOVE( WheelSections,		PREFSMAN->m_MusicWheelUsesSections );
MOVE( CourseSort,			PREFSMAN->m_iCourseSortOrder );
MOVE( RandomAtEnd,			PREFSMAN->m_bMoveRandomToEnd );
MOVE( Translations,			PREFSMAN->m_bShowNativeLanguage );
MOVE( Lyrics,				PREFSMAN->m_bShowLyrics );

/* Misc. options */
MOVE( AutogenSteps,			PREFSMAN->m_bAutogenSteps );
MOVE( AutogenGroupCourses,	PREFSMAN->m_bAutogenGroupCourses );

/* Background options */
MOVE( DancingCharacters,	PREFSMAN->m_ShowDancingCharacters );
MOVE( BeginnerHelper,		PREFSMAN->m_bShowBeginnerHelper );

static void BGBrightness( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const float mapping[] = { 0.0f,0.1f,0.2f,0.3f,0.4f,0.5f,0.6f,0.7f,0.8f,0.9f,1.0f };
	MoveMap( sel, PREFSMAN->m_fBGBrightness.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

static void NumBackgrounds( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 5,10,15,20 };
	MoveMap( sel, PREFSMAN->m_iNumBackgrounds.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

/* Input options */
MOVE( AutoMapOnJoyChange,	PREFSMAN->m_bAutoMapOnJoyChange );
MOVE( AutoPlay,				PREFSMAN->m_bAutoPlay );
MOVE( BackDelayed,			PREFSMAN->m_bDelayedBack );
MOVE( OptionsNavigation,	PREFSMAN->m_bArcadeOptionsNavigation );

static void WheelSpeed( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 5, 10, 15, 25 };
	MoveMap( sel, PREFSMAN->m_iMusicWheelSwitchSpeed, ToSel, mapping, ARRAYSIZE(mapping) );
}

/* Gameplay options */
MOVE( SoloSingles,			PREFSMAN->m_bSoloSingle );
MOVE( EasterEggs,			PREFSMAN->m_bEasterEggs );
MOVE( MarvelousTiming,		PREFSMAN->m_iMarvelousTiming );
MOVE( AllowExtraStage,		PREFSMAN->m_bAllowExtraStage );
MOVE( PickExtraStage,		PREFSMAN->m_bPickExtraStage );
MOVE( UnlockSystem,			PREFSMAN->m_bUseUnlockSystem );

static void CoinModeM( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const CoinMode mapping[] = { COIN_HOME, COIN_PAY, COIN_FREE };
	MoveMap( sel, PREFSMAN->m_CoinMode, ToSel, mapping, ARRAYSIZE(mapping) );
}

static void CoinModeNoHome( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const CoinMode mapping[] = { COIN_PAY, COIN_FREE };
	MoveMap( sel, PREFSMAN->m_CoinMode, ToSel, mapping, ARRAYSIZE(mapping) );
}

static void CoinsPerCredit( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 1,2,3,4,5,6,7,8 };
	MoveMap( sel, PREFSMAN->m_iCoinsPerCredit, ToSel, mapping, ARRAYSIZE(mapping) );
}

static void PremiumM( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const Premium mapping[] = { PREMIUM_NONE, PREMIUM_DOUBLES, PREMIUM_JOINT };
	MoveMap( sel, PREFSMAN->m_Premium, ToSel, mapping, ARRAYSIZE(mapping) );
}

static void SongsPerPlay( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 1,2,3,4,5,6,7 };
	MoveMap( sel, PREFSMAN->m_iNumArcadeStages, ToSel, mapping, ARRAYSIZE(mapping) );
}
MOVE( EventMode,			PREFSMAN->m_bEventMode );

/* Machine options */
MOVE( ScoringType,			PREFSMAN->m_iScoringType );

static void JudgeDifficulty( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const float mapping[] = { 1.50f,1.33f,1.16f,1.00f,0.84f,0.66f,0.50f,0.33f,0.20f };
	MoveMap( sel, PREFSMAN->m_fJudgeWindowScale.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

void LifeDifficulty( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const float mapping[] = { 1.60f,1.40f,1.20f,1.00f,0.80f,0.60f,0.40f };
	MoveMap( sel, PREFSMAN->m_fLifeDifficultyScale.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

static void ShowSongOptions( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const PrefsManager::Maybe mapping[] = { PrefsManager::NO,PrefsManager::YES,PrefsManager::ASK };
	MoveMap( sel, PREFSMAN->m_ShowSongOptions, ToSel, mapping, ARRAYSIZE(mapping) );
}

static void ShowNameEntry( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const PrefsManager::GetRankingName mapping[] = { PrefsManager::RANKING_OFF, PrefsManager::RANKING_ON, PrefsManager::RANKING_LIST };
	MoveMap( sel, PREFSMAN->m_iGetRankingName, ToSel, mapping, ARRAYSIZE(mapping) );
}

static void DefaultFailType( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	if( ToSel )
	{
		SongOptions so;
		so.FromString( PREFSMAN->m_sDefaultModifiers );
		sel = so.m_FailType;
	} else {
		PlayerOptions po;
		SongOptions so;
		GetDefaultModifiers( po, so );

		switch( sel )
		{
		case 0:	so.m_FailType = SongOptions::FAIL_IMMEDIATE;			break;
		case 1:	so.m_FailType = SongOptions::FAIL_COMBO_OF_30_MISSES;	break;
		case 2:	so.m_FailType = SongOptions::FAIL_END_OF_SONG;			break;
		case 3:	so.m_FailType = SongOptions::FAIL_OFF;					break;
		default:
			ASSERT(0);
		}

		SetDefaultModifiers( po, so );
	}
}

MOVE( ProgressiveLifebar,	PREFSMAN->m_iProgressiveLifebar );
MOVE( ProgressiveStageLifebar,		PREFSMAN->m_iProgressiveStageLifebar );
MOVE( ProgressiveNonstopLifebar,	PREFSMAN->m_iProgressiveNonstopLifebar );

/* Graphic options */
MOVE( CelShadeModels,		PREFSMAN->m_bCelShadeModels );
MOVE( SmoothLines,			PREFSMAN->m_bSmoothLines );

struct res_t
{
	int w, h;
	res_t(): w(0), h(0) { }
	res_t( int w_, int h_ ): w(w_), h(h_) { }
	res_t operator-( const res_t &rhs ) const
	{
		return res_t( w-rhs.w, h-rhs.h );
	}

	bool operator<( const res_t &rhs ) const
	{
		if( w != rhs.w )
			return w < rhs.w;
		return h < rhs.h;
	}
};

static void DisplayResolution( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const res_t mapping[] =
	{
		res_t(320, 240),
		res_t(400, 300),
		res_t(512, 384),
		res_t(640, 480),
		res_t(800, 600),
		res_t(1024, 768),
		res_t(1280, 960),
		res_t(1280, 1024)
	};
	res_t sel_res( PREFSMAN->m_iDisplayWidth, PREFSMAN->m_iDisplayHeight );
	MoveMap( sel, sel_res, ToSel, mapping, ARRAYSIZE(mapping) );
	if( !ToSel )
	{
		PREFSMAN->m_iDisplayWidth = sel_res.w;
		PREFSMAN->m_iDisplayHeight = sel_res.h;
	}
}

static void DisplayColor( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 16,32 };
	MoveMap( sel, PREFSMAN->m_iDisplayColorDepth.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

static void TextureResolution( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 256,512,1024,2048 };
	MoveMap( sel, PREFSMAN->m_iMaxTextureResolution.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

static void TextureColor( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 16,32 };
	MoveMap( sel, PREFSMAN->m_iTextureColorDepth.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

static void MovieColor( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { 16,32 };
	MoveMap( sel, PREFSMAN->m_iMovieColorDepth.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

static void RefreshRate( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const int mapping[] = { (int) REFRESH_DEFAULT,60,70,72,75,80,85,90,100,120,150 };
	MoveMap( sel, PREFSMAN->m_iRefreshRate.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

static void AspectRatio( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const float mapping[] = { 3/4.f,1,4/3.0f,16/10.0f,16/9.f, 8/3.f };
	MoveMap( sel, PREFSMAN->m_fAspectRatio.Value(), ToSel, mapping, ARRAYSIZE(mapping) );
}

/* Sound options */
MOVE( ResamplingQuality,	PREFSMAN->m_iSoundResampleQuality );
MOVE( AttractSoundFrequency,PREFSMAN->m_iAttractSoundFrequency );

static void SoundVolume( int &sel, bool ToSel, const ConfOption *pConfOption )
{
	const float mapping[] = { 0.0f,0.1f,0.2f,0.3f,0.4f,0.5f,0.6f,0.7f,0.8f,0.9f,1.0f };
	MoveMap( sel, PREFSMAN->m_fSoundVolume, ToSel, mapping, ARRAYSIZE(mapping) );
}

static vector<ConfOption> g_ConfOptions;
static void InitializeConfOptions()
{
	if( !g_ConfOptions.empty() )
		return;

#define ADD(x) g_ConfOptions.push_back( x )
	/* Select game */
	ADD( ConfOption( "Game",					GameSel, GameChoices ) );
	g_ConfOptions.back().m_iEffects = OPT_RESET_GAME;

	/* Appearance options */
	ADD( ConfOption( "Language",				Language, LanguageChoices ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_THEME;

	ADD( ConfOption( "Theme",					Theme, ThemeChoices ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_THEME;

	ADD( ConfOption( "Announcer",				Announcer, AnnouncerChoices ) );
	ADD( ConfOption( "Default\nNoteSkin",		DefaultNoteSkin, DefaultNoteSkinChoices ) );
	ADD( ConfOption( "Instructions",			Instructions,		"SKIP","SHOW") );
	ADD( ConfOption( "Caution",					Caution,			"SKIP","SHOW") );
	ADD( ConfOption( "Oni Score\nDisplay",		OniScoreDisplay,	"PERCENT","DANCE POINTS") );
	ADD( ConfOption( "Song\nGroup",				SongGroup,			"ALL MUSIC","CHOOSE") );
	ADD( ConfOption( "Wheel\nSections",			WheelSections,		"NEVER","ALWAYS", "ABC ONLY") );
	ADD( ConfOption( "Course\nSort",			CourseSort,			"# SONGS","AVG FEET","TOTAL FEET","RANKING") );
	ADD( ConfOption( "Random\nAt End",			RandomAtEnd,		"NO","YES") );
	ADD( ConfOption( "Translations",			Translations,		"ROMANIZATION","NATIVE LANGUAGE") );
	ADD( ConfOption( "Lyrics",					Lyrics,				"HIDE","SHOW") );

	/* Misc options */
	ADD( ConfOption( "Autogen\nSteps",			AutogenSteps, "OFF","ON" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_SONG;

	ADD( ConfOption( "Autogen\nGroup Courses",	AutogenGroupCourses, "OFF","ON" ) );
	ADD( ConfOption( "FastLoad",				MovePref,			"OFF","ON" ) );

	/* Background options */
	ADD( ConfOption( "BackgroundMode",			MovePref,			"OFF","ANIMATIONS","VISUALIZATIONS","RANDOM MOVIES" ) );
	ADD( ConfOption( "Brightness",				BGBrightness,		"0%","10%","20%","30%","40%","50%","60%","70%","80%","90%","100%" ) );
	ADD( ConfOption( "ShowDanger",				MovePref,			"HIDE","SHOW" ) );
	ADD( ConfOption( "Dancing\nCharacters",		DancingCharacters,	"DEFAULT TO OFF","DEFAULT TO RANDOM","SELECT" ) );
	ADD( ConfOption( "Beginner\nHelper",		BeginnerHelper,		"OFF","ON" ) );
	ADD( ConfOption( "Random\nBackgrounds",		NumBackgrounds,		"5","10","15","20" ) );

	/* Input options */
	ADD( ConfOption( "Auto Map\nOn Joy Change",	AutoMapOnJoyChange,	"OFF","ON (recommended)" ) );
	ADD( ConfOption( "OnlyDedicatedMenuButtons",MovePref,			"USE GAMEPLAY BUTTONS","ONLY DEDICATED BUTTONS" ) );
	ADD( ConfOption( "AutoPlay",				AutoPlay,			"OFF","ON" ) );
	ADD( ConfOption( "Back\nDelayed",			BackDelayed,		"INSTANT","HOLD" ) );
	ADD( ConfOption( "Options\nNavigation",		OptionsNavigation,	"SM STYLE","ARCADE STYLE" ) );
	ADD( ConfOption( "Wheel\nSpeed",			WheelSpeed,			"SLOW","NORMAL","FAST","REALLY FAST" ) );

	/* Gameplay options */
	ADD( ConfOption( "Solo\nSingles",			SoloSingles,		"OFF","ON" ) );
	ADD( ConfOption( "HiddenSongs",				MovePref,			"OFF","ON" ) );
	ADD( ConfOption( "Easter\nEggs",			EasterEggs,			"OFF","ON" ) );
	ADD( ConfOption( "Marvelous\nTiming",		MarvelousTiming,	"NEVER","COURSES ONLY","ALWAYS" ) );
	ADD( ConfOption( "Allow Extra\nStage",		AllowExtraStage,	"OFF","ON" ) );
	ADD( ConfOption( "Pick Extra\nStage",		PickExtraStage,		"OFF","ON" ) );
	ADD( ConfOption( "Unlock\nSystem",			UnlockSystem,		"OFF","ON" ) );

	/* Machine options */
	ADD( ConfOption( "MenuTimer",				MovePref,			"OFF","ON" ) );
	ADD( ConfOption( "CoinMode",				CoinModeM,			"HOME","PAY","FREE PLAY" ) );
	ADD( ConfOption( "CoinModeNoHome",			CoinModeNoHome,		"PAY","FREE PLAY" ) );
	ADD( ConfOption( "Songs Per\nPlay",			SongsPerPlay,		"1","2","3","4","5","6","7" ) );
	ADD( ConfOption( "Event\nMode",				EventMode,			"OFF","ON" ) );
	ADD( ConfOption( "Scoring\nType",			ScoringType,		"MAX2","5TH" ) );
	ADD( ConfOption( "Judge\nDifficulty",		JudgeDifficulty,	"1","2","3","4","5","6","7","8","JUSTICE" ) );
	ADD( ConfOption( "Life\nDifficulty",		LifeDifficulty,		"1","2","3","4","5","6","7" ) );
	ADD( ConfOption( "Progressive\nLifebar",	ProgressiveLifebar,	"OFF","1","2","3","4","5","6","7","8") );
	ADD( ConfOption( "Progressive\nStage Lifebar",ProgressiveStageLifebar,	"OFF","1","2","3","4","5","6","7","8","INSANITY") );
	ADD( ConfOption( "Progressive\nNonstop Lifebar",ProgressiveNonstopLifebar,"OFF","1","2","3","4","5","6","7","8","INSANITY") );
	ADD( ConfOption( "Default\nFail Type",		DefaultFailType,	"IMMEDIATE","COMBO OF 30 MISSES","END OF SONG","OFF" ) );	
	ADD( ConfOption( "DefaultFailTypeNoOff",	DefaultFailType,	"IMMEDIATE","COMBO OF 30 MISSES","END OF SONG" ) );	
	ADD( ConfOption( "Coins Per\nCredit",		CoinsPerCredit,		"1","2","3","4","5","6","7","8" ) );
	ADD( ConfOption( "Premium",					PremiumM,			"OFF","DOUBLE FOR 1 CREDIT","JOINT PREMIUM" ) );
	ADD( ConfOption( "Show Song\nOptions",		ShowSongOptions,	"HIDE","SHOW","ASK" ) );
	ADD( ConfOption( "Show Name\nEntry",		ShowNameEntry,		"OFF", "ON", "RANKING SONGS" ) );

	/* Graphic options */
	ADD( ConfOption( "Windowed",				MovePref,			"FULLSCREEN", "WINDOWED" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "DisplayResolution",		DisplayResolution,	"320","400","512","640","800","1024","1280x960","1280x1024" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "DisplayColor",			DisplayColor,		"16BIT","32BIT" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "Texture\nResolution",		TextureResolution,	"256","512","1024","2048" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "Texture\nColor",			TextureColor,		"16BIT","32BIT" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "Movie\nColor",			MovieColor,			"16BIT","32BIT" ) );
	ADD( ConfOption( "DelayedTextureDelete",	MovePref,			"OFF","ON" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "CelShade\nModels",		CelShadeModels,		"OFF","ON" ) );
	ADD( ConfOption( "Smooth\nLines",			SmoothLines,		"OFF","ON" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "Refresh\nRate",			RefreshRate,		"DEFAULT","60","70","72","75","80","85","90","100","120","150" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "Aspect\nRatio",			AspectRatio,		"3:4","1:1","4:3","16:10","16:9","8:3" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_ASPECT_RATIO;
	ADD( ConfOption( "Vsync",					MovePref,			"NO", "YES" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD( ConfOption( "ShowStats",				MovePref,			"OFF","ON" ) );
	ADD( ConfOption( "ShowBanners",				MovePref,			"OFF","ON" ) );

	/* Sound options */
	ADD( ConfOption( "Resampling\nQuality",		ResamplingQuality,	"FAST","NORMAL","HIGH QUALITY" ) );
	ADD( ConfOption( "Attract\nSound Frequency",AttractSoundFrequency,	"NEVER","ALWAYS","2 TIMES","3 TIMES","4 TIMES","5 TIMES" ) );
	ADD( ConfOption( "Sound\nVolume",			SoundVolume,		"SILENT","10%","20%","30%","40%","50%","60%","70%","80%","90%","100%" ) );
	g_ConfOptions.back().m_iEffects = OPT_APPLY_SOUND;
}

/* Get a mask of effects to apply if the given option changes. */
int ConfOption::GetEffects() const
{
	return m_iEffects | OPT_SAVE_PREFERENCES;
}

ConfOption *ConfOption::Find( CString name )
{
	InitializeConfOptions();
	for( unsigned i = 0; i < g_ConfOptions.size(); ++i )
	{
		ConfOption *opt = &g_ConfOptions[i];

		CString match(opt->name);
		match.Replace("\n", "");
		match.Replace("-", "");
		match.Replace(" ", "");

		if( match.CompareNoCase(name) )
			continue;

		return opt;
	}

	return NULL;
}

void ConfOption::UpdateAvailableOptions()
{
	if( MakeOptionsListCB != NULL )
	{
		names.clear();
		MakeOptionsListCB( names );
	}
}

void ConfOption::MakeOptionsList( CStringArray &out ) const
{
	out = names;
}

/*
 * (c) 2003-2004 Glenn Maynard
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
