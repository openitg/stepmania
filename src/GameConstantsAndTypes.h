#pragma once
/*
-----------------------------------------------------------------------------
 File: GameConstantsAndTypes.h

 Desc: These things don't change very often.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

/////////////////////////////
// Screen Dimensions
/////////////////////////////
#define	SCREEN_WIDTH	(640)
#define	SCREEN_HEIGHT	(480)

#define	SCREEN_LEFT		(0)
#define	SCREEN_RIGHT	(SCREEN_WIDTH)
#define	SCREEN_TOP		(0)
#define	SCREEN_BOTTOM	(SCREEN_HEIGHT)

#define	CENTER_X		(SCREEN_LEFT + (SCREEN_RIGHT - SCREEN_LEFT)/2.0f)
#define	CENTER_Y		(SCREEN_TOP + (SCREEN_BOTTOM - SCREEN_TOP)/2.0f)


/////////////////////////
// Note definitions
/////////////////////////
typedef unsigned char TapNote;

enum 
{
	TRACK_1 = 0,
	TRACK_2,
	TRACK_3,
	TRACK_4,
	TRACK_5,
	TRACK_6,
	TRACK_7,
	TRACK_8,
	TRACK_9,
	TRACK_10,
	TRACK_11,
	TRACK_12,
	TRACK_13,
	TRACK_14,
	TRACK_15,
	TRACK_16,
	MAX_NOTE_TRACKS		// leave this at the end
};

const int MAX_MEASURES		= 200;	// this should be long enough to hold 10:00 minute songs (
const int BEATS_PER_MEASURE = 4;
const int MAX_BEATS			= MAX_MEASURES * BEATS_PER_MEASURE;

const int ELEMENTS_PER_BEAT	= 12;	// It is important that this number is evenly divisible by 2, 3, and 4.
const int ELEMENTS_PER_MEASURE = ELEMENTS_PER_BEAT * BEATS_PER_MEASURE;
const int MAX_TAP_NOTE_ROWS = MAX_BEATS*ELEMENTS_PER_BEAT;

const int MAX_HOLD_NOTE_ELEMENTS = 200;

enum NoteType 
{ 
	NOTE_4TH,	// quarter notes
	NOTE_8TH,	// eighth notes
	NOTE_12TH,	// triplets
	NOTE_16TH,	// sixteenth notes
	NUM_NOTE_TYPES
};

inline D3DXCOLOR NoteTypeToColor( NoteType nt )
{
	switch( nt )
	{
	case NOTE_4TH:	return D3DXCOLOR(1,0,0,1);	// red
	case NOTE_8TH:	return D3DXCOLOR(0,0,1,1);	// blue
	case NOTE_12TH:	return D3DXCOLOR(1,0,1,1);	// purple
	case NOTE_16TH:	return D3DXCOLOR(1,1,0,1);	// yellow
	default:		ASSERT( false );	return D3DXCOLOR(1,1,1,1);
	}		
};

inline float NoteTypeToBeat( NoteType nt )
{
	switch( nt )
	{
	case NOTE_4TH:	return 1.0f;	// quarter notes
	case NOTE_8TH:	return 1.0f/2;	// eighth notes
	case NOTE_12TH:	return 1.0f/3;	// triplets
	case NOTE_16TH:	return 1.0f/4;	// sixteenth notes
	default:	ASSERT( false );	return 0;
	}
};

inline bool IsNoteOfType( int iNoteIndex, NoteType t )
{ 
	switch( t )
	{
	case NOTE_4TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/4) == 0;
	case NOTE_8TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/8) == 0;
	case NOTE_12TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/12) == 0;
	case NOTE_16TH:	return iNoteIndex % (ELEMENTS_PER_MEASURE/16) == 0;
	default:	ASSERT( false );	return false;
	}
};

inline D3DXCOLOR GetNoteColorFromIndex( int iStepIndex )
{ 
	for( int t=0; t<NUM_NOTE_TYPES; t++ )
	{
		if( IsNoteOfType( iStepIndex, (NoteType)t ) )
			return NoteTypeToColor( (NoteType)t );
	}
	return D3DXCOLOR(0.5f,0.5f,0.5f,1);
};

enum RadarCatrgory	// starting from 12-o'clock rotating clockwise
{
	RADAR_STREAM = 0,
	RADAR_VOLTAGE,
	RADAR_AIR,
	RADAR_CHAOS,
	RADAR_FREEZE,
	NUM_RADAR_VALUES	// leave this at the end
};

enum DifficultyClass 
{ 
	CLASS_EASY,		// corresponds to Basic
	CLASS_MEDIUM,	// corresponds to Trick, Another, Standard
	CLASS_HARD,		// corresponds to Maniac, SSR, Heavy
	NUM_DIFFICULTY_CLASSES
};

inline D3DXCOLOR DifficultyClassToColor( DifficultyClass dc )
{
	switch( dc )
	{
	case CLASS_EASY:	return D3DXCOLOR(1,1,0,1);	// yellow
	case CLASS_MEDIUM:	return D3DXCOLOR(1,0,0,1);	// red
	case CLASS_HARD:	return D3DXCOLOR(0,1,0,1);	// green
	default:	ASSERT(0);	return D3DXCOLOR();	// invalid DifficultyClass
	}
}

enum NotesType
{
	NOTES_TYPE_DANCE_SINGLE = 0,
	NOTES_TYPE_DANCE_DOUBLE,
	NOTES_TYPE_DANCE_COUPLE,
	NOTES_TYPE_DANCE_SOLO,
	NOTES_TYPE_PUMP_SINGLE,
	NOTES_TYPE_PUMP_VERSUS,
	NUM_NOTES_TYPES		// leave this at the end
};

inline int NotesTypeToNumColumns( NotesType nt )
{
	switch( nt )
	{
	case NOTES_TYPE_DANCE_SINGLE:	return 4;
	case NOTES_TYPE_DANCE_DOUBLE:	return 8;
	case NOTES_TYPE_DANCE_COUPLE:	return 8;
	case NOTES_TYPE_DANCE_SOLO:		return 6;
	case NOTES_TYPE_PUMP_SINGLE:	return 5;
	default:	ASSERT(0);		return -1;	// invalid NotesType
	}
}

inline NotesType StringToNotesType( CString sNotesType )
{
	if     ( sNotesType == "dance-single" )	return NOTES_TYPE_DANCE_SINGLE;
	else if( sNotesType == "dance-double" )	return NOTES_TYPE_DANCE_DOUBLE;
	else if( sNotesType == "dance-couple" )	return NOTES_TYPE_DANCE_COUPLE;
	else if( sNotesType == "dance-solo" )	return NOTES_TYPE_DANCE_SOLO;
	else if( sNotesType == "pump-single" )	return NOTES_TYPE_PUMP_SINGLE;
	else	ASSERT(0);	return NOTES_TYPE_DANCE_SINGLE;	// invalid NotesType
}

inline CString NotesTypeToString( NotesType nt )
{
	switch( nt )
	{
	case NOTES_TYPE_DANCE_SINGLE:	return "dance-single";
	case NOTES_TYPE_DANCE_DOUBLE:	return "dance-double";
	case NOTES_TYPE_DANCE_COUPLE:	return "dance-couple";
	case NOTES_TYPE_DANCE_SOLO:		return "dance-solo";
	case NOTES_TYPE_PUMP_SINGLE:	return "pump-single";
	default:	ASSERT(0);		return "";	// invalid NotesType
	}
}


//////////////////////////
// Play mode stuff
//////////////////////////
enum PlayMode
{
	PLAY_MODE_ARCADE,
	PLAY_MODE_NONSTOP,
	PLAY_MODE_ENDLESS,
	NUM_PLAY_MODES
};

enum PlayerNumber {
	PLAYER_1 = 0,
	PLAYER_2,
	NUM_PLAYERS,	// leave this at the end
	PLAYER_NONE
};

inline D3DXCOLOR PlayerToColor( PlayerNumber p ) 
{
	switch( p )
	{
		case PLAYER_1:	return D3DXCOLOR(0.4f,1.0f,0.8f,1);	// sea green
		case PLAYER_2:	return D3DXCOLOR(1.0f,0.5f,0.2f,1);	// orange
		default:	ASSERT( false ); return D3DXCOLOR(1,1,1,1);
	}
};
inline D3DXCOLOR PlayerToColor( int p ) { return PlayerToColor( (PlayerNumber)p ); }


enum SongSortOrder { 
	SORT_GROUP, 
	SORT_TITLE, 
	SORT_BPM, 
	SORT_MOST_PLAYED, 
	NUM_SORT_ORDERS 
};


///////////////////////////////
// Game/Style definition stuff
///////////////////////////////
enum Game
{
	GAME_DANCE,
	GAME_PUMP,
	NUM_GAMES	// leave this at the end
};

enum Style
{
	STYLE_DANCE_SINGLE,
	STYLE_DANCE_VERSUS,
	STYLE_DANCE_DOUBLE,
	STYLE_DANCE_COUPLE,
	STYLE_DANCE_SOLO,
	STYLE_DANCE_SOLO_VERSUS,
	STYLE_PUMP_SINGLE,
	STYLE_PUMP_VERSUS,
	NUM_STYLES	// leave this at the end
};

inline Game StyleToGame( Style s )
{
	switch( s )
	{
	case STYLE_DANCE_SINGLE:
	case STYLE_DANCE_VERSUS:
	case STYLE_DANCE_DOUBLE:
	case STYLE_DANCE_COUPLE:
	case STYLE_DANCE_SOLO:
	case STYLE_DANCE_SOLO_VERSUS:
		return GAME_DANCE;
	case STYLE_PUMP_SINGLE:
	case STYLE_PUMP_VERSUS:
		return GAME_PUMP;
	default:
		ASSERT(0);	// invalid Style
		return GAME_DANCE;
	}
}

///////////////////////////
// Options stuff
///////////////////////////

struct PlayerOptions
{
	PlayerOptions() {
		m_fArrowScrollSpeed = 1.0f;
		m_EffectType = EFFECT_NONE;
		m_AppearanceType = APPEARANCE_VISIBLE;
		m_TurnType = TURN_NONE;
		m_bLittle = false;
		m_bReverseScroll = false;
		m_ColorType = COLOR_ARCADE;
		m_bHoldNotes = true;
		m_DrainType = DRAIN_NORMAL;
	};

	float m_fArrowScrollSpeed;
	enum EffectType	{ EFFECT_NONE=0, EFFECT_BOOST, EFFECT_WAVE, EFFECT_DRUNK, EFFECT_DIZZY, EFFECT_SPACE };
	EffectType m_EffectType;
	enum AppearanceType	{ APPEARANCE_VISIBLE=0, APPEARANCE_HIDDEN, APPEARANCE_SUDDEN, APPEARANCE_STEALTH };
	AppearanceType m_AppearanceType;
	enum TurnType { TURN_NONE=0, TURN_MIRROR, TURN_LEFT, TURN_RIGHT, TURN_SHUFFLE };
	TurnType m_TurnType;
	bool m_bLittle;
	bool m_bReverseScroll;
	enum ColorType { COLOR_ARCADE=0, COLOR_NOTE, COLOR_FLAT, COLOR_PLAIN };
	ColorType m_ColorType;
	bool m_bHoldNotes;
	enum DrainType { DRAIN_NORMAL=0, DRAIN_NO_RECOVER, DRAIN_SUDDEN_DEATH };
	DrainType m_DrainType;
};

struct SongOptions
{
	SongOptions() {
		m_FailType = FAIL_ARCADE;
		m_AssistType = ASSIST_NONE;
		m_fMusicRate = 1.0f;
	};

	enum FailType { FAIL_ARCADE=0, FAIL_END_OF_SONG, FAIL_OFF };
	FailType m_FailType;
	enum AssistType { ASSIST_NONE=0, ASSIST_TICK };
	AssistType m_AssistType;
	float m_fMusicRate;
};


struct ScoreSummary {
	ScoreSummary() { perfect=great=good=boo=miss=ok=ng=max_combo=0; score=0; };
	int perfect, great, good, boo, miss, ok, ng, max_combo;
	float score;
};
