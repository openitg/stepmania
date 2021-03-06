#include "global.h"
#include "PlayerOptions.h"
#include "RageUtil.h"
#include "GameState.h"
#include "NoteSkinManager.h"
#include "Song.h"
#include "Course.h"
#include "Steps.h"
#include "ThemeManager.h"
#include "Foreach.h"
#include "Style.h"
#include "CommonMetrics.h"
#include <float.h>

#define ONE( arr ) { for( unsigned Z = 0; Z < ARRAYLEN(arr); ++Z ) arr[Z]=1.0f; }

void PlayerOptions::Init()
{
	m_bSetScrollSpeed = false;
	m_fTimeSpacing = 0;		m_SpeedfTimeSpacing = 1.0f;
	m_fScrollSpeed = 1.0f;		m_SpeedfScrollSpeed = 1.0f;
	m_fScrollBPM = 200;		m_SpeedfScrollBPM = 1.0f;
	ZERO( m_fAccels );		ONE( m_SpeedfAccels );
	ZERO( m_fEffects );		ONE( m_SpeedfEffects );
	ZERO( m_fAppearances );		ONE( m_SpeedfAppearances );
	ZERO( m_fScrolls );		ONE( m_SpeedfScrolls );
	m_fDark = 0;			m_SpeedfDark = 1.0f;
	m_fBlind = 0;			m_SpeedfBlind = 1.0f;
	m_fCover = 0;			m_SpeedfCover = 1.0f;
	m_fRandAttack = 0;		m_SpeedfRandAttack = 1.0f;
	m_fSongAttack = 0;		m_SpeedfSongAttack = 1.0f;
	m_fPlayerAutoPlay = 0;		m_SpeedfPlayerAutoPlay = 1.0f;
	m_bSetTiltOrSkew = false;
	m_fPerspectiveTilt = 0;		m_SpeedfPerspectiveTilt = 1.0f;
	m_fSkew = 0;			m_SpeedfSkew = 1.0f;
	m_fPassmark = 0;		m_SpeedfPassmark = 1.0f;
	m_fRandomSpeed = 0;		m_SpeedfRandomSpeed = 1.0f;
	ZERO( m_bTurns );
	ZERO( m_bTransforms );
	m_bMuteOnError = false;
	m_FailType = FAIL_IMMEDIATE;
	m_ScoreDisplay = SCORING_ADD;
	m_sNoteSkin = "";
}

void PlayerOptions::Approach( const PlayerOptions& other, float fDeltaSeconds )
{
#define APPROACH( opt ) \
	fapproach( m_ ## opt, other.m_ ## opt, fDeltaSeconds * other.m_Speed ## opt );
#define DO_COPY( x ) \
	x = other.x;

	APPROACH( fTimeSpacing );
	APPROACH( fScrollSpeed );
	fapproach( m_fScrollBPM, other.m_fScrollBPM, fDeltaSeconds * other.m_SpeedfScrollBPM*150 );
	for( int i=0; i<NUM_ACCELS; i++ )
		APPROACH( fAccels[i] );
	for( int i=0; i<NUM_EFFECTS; i++ )
		APPROACH( fEffects[i] );
	for( int i=0; i<NUM_APPEARANCES; i++ )
		APPROACH( fAppearances[i] );
	for( int i=0; i<NUM_SCROLLS; i++ )
		APPROACH( fScrolls[i] );
	APPROACH( fDark );
	APPROACH( fBlind );
	APPROACH( fCover );
	APPROACH( fRandAttack );
	APPROACH( fSongAttack );
	APPROACH( fPlayerAutoPlay );
	APPROACH( fPerspectiveTilt );
	APPROACH( fSkew );
	APPROACH( fPassmark );
	APPROACH( fRandomSpeed );

	DO_COPY( m_bSetScrollSpeed );
	DO_COPY( m_bSetTiltOrSkew );
	for( int i=0; i<NUM_TURNS; i++ )
		DO_COPY( m_bTurns[i] );
	for( int i=0; i<NUM_TRANSFORMS; i++ )
		DO_COPY( m_bTransforms[i] );
	DO_COPY( m_bMuteOnError );
	DO_COPY( m_ScoreDisplay );
	DO_COPY( m_FailType );
	DO_COPY( m_sNoteSkin );
#undef APPROACH
#undef DO_COPY
}

static void AddPart( vector<RString> &AddTo, float level, RString name )
{
	if( level == 0 )
		return;

	const RString LevelStr = (level == 1)? RString(""): ssprintf( "%ld%% ", lrintf(level*100) );

	AddTo.push_back( LevelStr + name );
}

RString PlayerOptions::GetString( bool bForceNoteSkin ) const
{
	vector<RString> v;
	GetMods( v, bForceNoteSkin );
	return join( ", ", v );
}

void PlayerOptions::GetMods( vector<RString> &AddTo, bool bForceNoteSkin ) const
{
	RString sReturn;

	if( !m_fTimeSpacing )
	{
		if( m_bSetScrollSpeed || m_fScrollSpeed != 1 )
		{
			/* -> 1.00 */
			RString s = ssprintf( "%2.2f", m_fScrollSpeed );
			if( s[s.size()-1] == '0' )
			{
				/* -> 1.0 */
				s.erase( s.size()-1 );	// delete last char
				if( s[s.size()-1] == '0' )
				{
					/* -> 1 */
					s.erase( s.size()-2 );	// delete last 2 chars
				}
			}
			AddTo.push_back( s + "x" );
		}
	}
	else
	{
		RString s = ssprintf( "C%.0f", m_fScrollBPM );
		AddTo.push_back( s );
	}

	AddPart( AddTo, m_fAccels[ACCEL_BOOST],		"Boost" );
	AddPart( AddTo, m_fAccels[ACCEL_BRAKE],		"Brake" );
	AddPart( AddTo, m_fAccels[ACCEL_WAVE],		"Wave" );
	AddPart( AddTo, m_fAccels[ACCEL_EXPAND],	"Expand" );
	AddPart( AddTo, m_fAccels[ACCEL_BOOMERANG],	"Boomerang" );

	AddPart( AddTo, m_fEffects[EFFECT_DRUNK],	"Drunk" );
	AddPart( AddTo, m_fEffects[EFFECT_DIZZY],	"Dizzy" );
	AddPart( AddTo, m_fEffects[EFFECT_CONFUSION],	"Confusion" );
	AddPart( AddTo, m_fEffects[EFFECT_MINI],	"Mini" );
	AddPart( AddTo, m_fEffects[EFFECT_TINY],	"Tiny" );
	AddPart( AddTo, m_fEffects[EFFECT_FLIP],	"Flip" );
	AddPart( AddTo, m_fEffects[EFFECT_INVERT],	"Invert" );
	AddPart( AddTo, m_fEffects[EFFECT_TORNADO],	"Tornado" );
	AddPart( AddTo, m_fEffects[EFFECT_TIPSY],	"Tipsy" );
	AddPart( AddTo, m_fEffects[EFFECT_BUMPY],	"Bumpy" );
	AddPart( AddTo, m_fEffects[EFFECT_BEAT],	"Beat" );

	AddPart( AddTo, m_fAppearances[APPEARANCE_HIDDEN],		"Hidden" );
	AddPart( AddTo, m_fAppearances[APPEARANCE_HIDDEN_OFFSET],	"HiddenOffset" );
	AddPart( AddTo, m_fAppearances[APPEARANCE_SUDDEN],		"Sudden" );
	AddPart( AddTo, m_fAppearances[APPEARANCE_SUDDEN_OFFSET],	"SuddenOffset" );
	AddPart( AddTo, m_fAppearances[APPEARANCE_STEALTH],		"Stealth" );
	AddPart( AddTo, m_fAppearances[APPEARANCE_BLINK],		"Blink" );
	AddPart( AddTo, m_fAppearances[APPEARANCE_RANDOMVANISH],	"RandomVanish" );

	AddPart( AddTo, m_fScrolls[SCROLL_REVERSE],	"Reverse" );
	AddPart( AddTo, m_fScrolls[SCROLL_SPLIT],	"Split" );
	AddPart( AddTo, m_fScrolls[SCROLL_ALTERNATE],	"Alternate" );
	AddPart( AddTo, m_fScrolls[SCROLL_CROSS],	"Cross" );
	AddPart( AddTo, m_fScrolls[SCROLL_CENTERED],	"Centered" );

	AddPart( AddTo, m_fDark,	"Dark" );

	AddPart( AddTo, m_fBlind,	"Blind" );
	AddPart( AddTo, m_fCover,	"Cover" );

	AddPart( AddTo, m_fRandAttack,		"RandomAttacks" );
	AddPart( AddTo, m_fSongAttack,		"SongAttacks" );
	AddPart( AddTo, m_fPlayerAutoPlay,	"PlayerAutoPlay" );

	AddPart( AddTo, m_fPassmark,	"Passmark" );

	AddPart( AddTo, m_fRandomSpeed,	"RandomSpeed" );

	if( m_bTurns[TURN_MIRROR] )		AddTo.push_back( "Mirror" );
	if( m_bTurns[TURN_LEFT] )		AddTo.push_back( "Left" );
	if( m_bTurns[TURN_RIGHT] )		AddTo.push_back( "Right" );
	if( m_bTurns[TURN_SHUFFLE] )		AddTo.push_back( "Shuffle" );
	if( m_bTurns[TURN_SUPER_SHUFFLE] )	AddTo.push_back( "SuperShuffle" );

	if( m_bTransforms[TRANSFORM_NOHOLDS] )	AddTo.push_back( "NoHolds" );
	if( m_bTransforms[TRANSFORM_NOROLLS] )	AddTo.push_back( "NoRolls" );
	if( m_bTransforms[TRANSFORM_NOMINES] )	AddTo.push_back( "NoMines" );
	if( m_bTransforms[TRANSFORM_LITTLE] )	AddTo.push_back( "Little" );
	if( m_bTransforms[TRANSFORM_WIDE] )	AddTo.push_back( "Wide" );
	if( m_bTransforms[TRANSFORM_BIG] )	AddTo.push_back( "Big" );
	if( m_bTransforms[TRANSFORM_QUICK] )	AddTo.push_back( "Quick" );
	if( m_bTransforms[TRANSFORM_BMRIZE] )	AddTo.push_back( "BMRize" );
	if( m_bTransforms[TRANSFORM_SKIPPY] )	AddTo.push_back( "Skippy" );
	if( m_bTransforms[TRANSFORM_MINES] )	AddTo.push_back( "Mines" );
	if( m_bTransforms[TRANSFORM_ATTACKMINES] ) AddTo.push_back( "AttackMines" );
	if( m_bTransforms[TRANSFORM_ECHO] )	AddTo.push_back( "Echo" );
	if( m_bTransforms[TRANSFORM_STOMP] )	AddTo.push_back( "Stomp" );
	if( m_bTransforms[TRANSFORM_PLANTED] )	AddTo.push_back( "Planted" );
	if( m_bTransforms[TRANSFORM_FLOORED] )	AddTo.push_back( "Floored" );
	if( m_bTransforms[TRANSFORM_TWISTER] )	AddTo.push_back( "Twister" );
	if( m_bTransforms[TRANSFORM_HOLDROLLS] ) AddTo.push_back( "HoldsToRolls" );
	if( m_bTransforms[TRANSFORM_NOJUMPS] )	AddTo.push_back( "NoJumps" );
	if( m_bTransforms[TRANSFORM_NOHANDS] )	AddTo.push_back( "NoHands" );
	if( m_bTransforms[TRANSFORM_NOQUADS] )	AddTo.push_back( "NoQuads" );
	if( m_bTransforms[TRANSFORM_NOSTRETCH] )AddTo.push_back( "NoStretch" );
	if( m_bMuteOnError )			AddTo.push_back( "MuteOnError" );

	switch( m_FailType )
	{
	case FAIL_IMMEDIATE:							break;
	case FAIL_IMMEDIATE_CONTINUE:		AddTo.push_back("FailImmediateContinue");	break;
	case FAIL_AT_END:			AddTo.push_back("FailAtEnd");	break;
	case FAIL_OFF:				AddTo.push_back("FailOff");	break;
	default:		ASSERT(0);
	}

	if( m_fSkew==0 && m_fPerspectiveTilt==0 )		{ if( m_bSetTiltOrSkew ) AddTo.push_back( "Overhead" ); }
	else if( m_fSkew == 0 )
	{
		if( m_fPerspectiveTilt > 0 )
			AddPart( AddTo, m_fPerspectiveTilt, "Distant" );
		else
			AddPart( AddTo, -m_fPerspectiveTilt, "Hallway" );
	}
	else if( fabsf(m_fSkew-m_fPerspectiveTilt) < 0.0001f )
	{
		AddPart( AddTo, m_fSkew, "Space" );
	}
	else if( fabsf(m_fSkew+m_fPerspectiveTilt) < 0.0001f )
	{
		AddPart( AddTo, m_fSkew, "Incoming" );
	}

	// Don't display a string if using the default NoteSkin unless we force it.
	if( bForceNoteSkin || (!m_sNoteSkin.empty() && m_sNoteSkin != CommonMetrics::DEFAULT_NOTESKIN_NAME.GetValue()) )
	{
		RString s = m_sNoteSkin;
		Capitalize( s );
		AddTo.push_back( s );
	}
}

/* Options are added to the current settings; call Init() beforehand if
 * you don't want this. */
void PlayerOptions::FromString( const RString &sMultipleMods )
{
	RString sTemp = sMultipleMods;
	vector<RString> vs;
	split( sTemp, ",", vs, true );
	RString sThrowAway;
	FOREACH( RString, vs, s )
	{
		FromOneModString( *s, sThrowAway );
	}
}

bool PlayerOptions::FromOneModString( const RString &sOneMod, RString &sErrorOut )
{
	ASSERT( NOTESKIN );

	RString sBit = sOneMod;
	sBit.MakeLower();
	Trim( sBit );

	/* "drunk"
	 * "no drunk"
	 * "150% drunk"
	 * "*2 100% drunk": approach at 2x normal speed */

	float level = 1;
	float speed = 1;
	vector<RString> asParts;
	split( sBit, " ", asParts, true );

	FOREACH_CONST( RString, asParts, s )
	{
		if( *s == "no" )
		{
			level = 0;
		}
		else if( isdigit((*s)[0]) || (*s)[0] == '-' )
		{
			/* If the last character is a *, they probably said "123*" when
			 * they meant "*123". */
			if( s->Right(1) == "*" )
			{
				/* XXX We know what they want, is there any reason not to handle it? */
				/* Yes.  We should be strict in handling the format. -Chris */
				sErrorOut = ssprintf("Invalid player options \"%s\"; did you mean '*%d'?", s->c_str(), atoi(*s) );
				return false;
			}
			else
			{
				level = StringToFloat( *s ) / 100.0f;
			}
		}
		else if( *s[0]=='*' )
		{
			sscanf( *s, "*%f", &speed );
			if( !isfinite(speed) )
				speed = 1.0f;
		}
	}

	
	sBit = asParts.back();

#define SET_FLOAT( opt ) { m_ ## opt = level; m_Speed ## opt = speed; }
	const bool on = (level > 0.5f);

	static Regex mult("^([0-9]+(\\.[0-9]+)?)x$");
	vector<RString> matches;
	if( mult.Compare(sBit, matches) )
	{
		StringConversion::FromString( matches[0], level );
		SET_FLOAT( fScrollSpeed )
		SET_FLOAT( fTimeSpacing )
		m_fTimeSpacing = 0;
	}
	else if( sscanf( sBit, "c%f", &level ) == 1 )
	{
		if( !isfinite(level) || level <= 0.0f )
			level = 200.0f; // Just pick some value.
		SET_FLOAT( fScrollBPM )
		SET_FLOAT( fTimeSpacing )
		m_fTimeSpacing = 1;
	}
	else if( sBit == "clearall" )				Init();
	else if( sBit == "boost" )				SET_FLOAT( fAccels[ACCEL_BOOST] )
	else if( sBit == "brake" || sBit == "land" )		SET_FLOAT( fAccels[ACCEL_BRAKE] )
	else if( sBit == "wave" )				SET_FLOAT( fAccels[ACCEL_WAVE] )
	else if( sBit == "expand" || sBit == "dwiwave" )	SET_FLOAT( fAccels[ACCEL_EXPAND] )
	else if( sBit == "boomerang" )				SET_FLOAT( fAccels[ACCEL_BOOMERANG] )
	else if( sBit == "drunk" )				SET_FLOAT( fEffects[EFFECT_DRUNK] )
	else if( sBit == "dizzy" )				SET_FLOAT( fEffects[EFFECT_DIZZY] )
	else if( sBit == "confusion" )				SET_FLOAT( fEffects[EFFECT_CONFUSION] )
	else if( sBit == "mini" )				SET_FLOAT( fEffects[EFFECT_MINI] )
	else if( sBit == "tiny" )				SET_FLOAT( fEffects[EFFECT_TINY] )
	else if( sBit == "flip" )				SET_FLOAT( fEffects[EFFECT_FLIP] )
	else if( sBit == "invert" )				SET_FLOAT( fEffects[EFFECT_INVERT] )
	else if( sBit == "tornado" )				SET_FLOAT( fEffects[EFFECT_TORNADO] )
	else if( sBit == "tipsy" )				SET_FLOAT( fEffects[EFFECT_TIPSY] )
	else if( sBit == "bumpy" )				SET_FLOAT( fEffects[EFFECT_BUMPY] )
	else if( sBit == "beat" )				SET_FLOAT( fEffects[EFFECT_BEAT] )
	else if( sBit == "hidden" )				SET_FLOAT( fAppearances[APPEARANCE_HIDDEN] )
	else if( sBit == "hiddenoffset" )			SET_FLOAT( fAppearances[APPEARANCE_HIDDEN_OFFSET] )
	else if( sBit == "sudden" )				SET_FLOAT( fAppearances[APPEARANCE_SUDDEN] )
	else if( sBit == "suddenoffset" )			SET_FLOAT( fAppearances[APPEARANCE_SUDDEN_OFFSET] )
	else if( sBit == "stealth" )				SET_FLOAT( fAppearances[APPEARANCE_STEALTH] )
	else if( sBit == "blink" )				SET_FLOAT( fAppearances[APPEARANCE_BLINK] )
	else if( sBit == "randomvanish" )			SET_FLOAT( fAppearances[APPEARANCE_RANDOMVANISH] )
	else if( sBit == "turn" && !on )			ZERO( m_bTurns ); /* "no turn" */
	else if( sBit == "mirror" )				m_bTurns[TURN_MIRROR] = on;
	else if( sBit == "left" )				m_bTurns[TURN_LEFT] = on;
	else if( sBit == "right" )				m_bTurns[TURN_RIGHT] = on;
	else if( sBit == "shuffle" )				m_bTurns[TURN_SHUFFLE] = on;
	else if( sBit == "supershuffle" )			m_bTurns[TURN_SUPER_SHUFFLE] = on;
	else if( sBit == "little" )				m_bTransforms[TRANSFORM_LITTLE] = on;
	else if( sBit == "wide" )				m_bTransforms[TRANSFORM_WIDE] = on;
	else if( sBit == "big" )				m_bTransforms[TRANSFORM_BIG] = on;
	else if( sBit == "quick" )				m_bTransforms[TRANSFORM_QUICK] = on;
	else if( sBit == "bmrize" )				m_bTransforms[TRANSFORM_BMRIZE] = on;
	else if( sBit == "skippy" )				m_bTransforms[TRANSFORM_SKIPPY] = on;
	else if( sBit == "mines" )				m_bTransforms[TRANSFORM_MINES] = on;
	else if( sBit == "attackmines" )			m_bTransforms[TRANSFORM_ATTACKMINES] = on;
	else if( sBit == "echo" )				m_bTransforms[TRANSFORM_ECHO] = on;
	else if( sBit == "stomp" )				m_bTransforms[TRANSFORM_STOMP] = on;
	else if( sBit == "planted" )				m_bTransforms[TRANSFORM_PLANTED] = on;
	else if( sBit == "floored" )				m_bTransforms[TRANSFORM_FLOORED] = on;
	else if( sBit == "twister" )				m_bTransforms[TRANSFORM_TWISTER] = on;
	else if( sBit == "holdrolls" )				m_bTransforms[TRANSFORM_HOLDROLLS] = on;
	else if( sBit == "nojumps" )				m_bTransforms[TRANSFORM_NOJUMPS] = on;
	else if( sBit == "nohands" )				m_bTransforms[TRANSFORM_NOHANDS] = on;
	else if( sBit == "noquads" )				m_bTransforms[TRANSFORM_NOQUADS] = on;
	else if( sBit == "reverse" )				SET_FLOAT( fScrolls[SCROLL_REVERSE] )
	else if( sBit == "split" )				SET_FLOAT( fScrolls[SCROLL_SPLIT] )
	else if( sBit == "alternate" )				SET_FLOAT( fScrolls[SCROLL_ALTERNATE] )
	else if( sBit == "cross" )				SET_FLOAT( fScrolls[SCROLL_CROSS] )
	else if( sBit == "centered" )				SET_FLOAT( fScrolls[SCROLL_CENTERED] )
	else if( sBit == "noholds" )				m_bTransforms[TRANSFORM_NOHOLDS] = on;
	else if( sBit == "norolls" )				m_bTransforms[TRANSFORM_NOROLLS] = on;
	else if( sBit == "nomines" )				m_bTransforms[TRANSFORM_NOMINES] = on;
	else if( sBit == "nostretch" )				m_bTransforms[TRANSFORM_NOSTRETCH] = on;
	else if( sBit == "dark" )				SET_FLOAT( fDark )
	else if( sBit == "blind" )				SET_FLOAT( fBlind )
	else if( sBit == "cover" )				SET_FLOAT( fCover )
	else if( sBit == "randomattacks" )			SET_FLOAT( fRandAttack )
	else if( sBit == "songattacks" )			SET_FLOAT( fSongAttack )
	else if( sBit == "playerautoplay" )			SET_FLOAT( fPlayerAutoPlay )
	else if( sBit == "passmark" )				SET_FLOAT( fPassmark )
	else if( sBit == "overhead" )				{ m_bSetTiltOrSkew = true; m_fSkew = 0;		m_fPerspectiveTilt = 0;		m_SpeedfSkew = m_SpeedfPerspectiveTilt = speed; }
	else if( sBit == "incoming" )				{ m_bSetTiltOrSkew = true; m_fSkew = level;	m_fPerspectiveTilt = -level;	m_SpeedfSkew = m_SpeedfPerspectiveTilt = speed; }
	else if( sBit == "space" )				{ m_bSetTiltOrSkew = true; m_fSkew = level;	m_fPerspectiveTilt = +level;	m_SpeedfSkew = m_SpeedfPerspectiveTilt = speed; }
	else if( sBit == "hallway" )				{ m_bSetTiltOrSkew = true; m_fSkew = 0;		m_fPerspectiveTilt = -level;	m_SpeedfSkew = m_SpeedfPerspectiveTilt = speed; }
	else if( sBit == "distant" )				{ m_bSetTiltOrSkew = true; m_fSkew = 0;		m_fPerspectiveTilt = +level;	m_SpeedfSkew = m_SpeedfPerspectiveTilt = speed; }
	else if( NOTESKIN && NOTESKIN->DoesNoteSkinExist(sBit) )	m_sNoteSkin = sBit;
	else if( sBit == "noteskin" && !on ) /* "no noteskin" */	m_sNoteSkin = CommonMetrics::DEFAULT_NOTESKIN_NAME;
	else if( sBit == "randomspeed" ) 			SET_FLOAT( fRandomSpeed )
	else if( sBit == "failarcade" || 
		 sBit == "failimmediate" )			m_FailType = FAIL_IMMEDIATE;
	else if( sBit == "failendofsong" ||
		 sBit == "failimmediatecontinue" )		m_FailType = FAIL_IMMEDIATE_CONTINUE;
	else if( sBit == "failatend" )				m_FailType = FAIL_AT_END;
	else if( sBit == "failoff" )				m_FailType = FAIL_OFF;
	else if( sBit == "faildefault" )
	{
		PlayerOptions po;
		GAMESTATE->GetDefaultPlayerOptions( po );
		m_FailType = po.m_FailType;
	}
	else if( sBit == "addscore" )				m_ScoreDisplay = SCORING_ADD;
	else if( sBit == "subtractscore" )			m_ScoreDisplay = SCORING_SUBTRACT;
	else if( sBit == "averagescore" )			m_ScoreDisplay = SCORING_AVERAGE;
	else if( sBit == "muteonerror" )			m_bMuteOnError = on;
	else if( sBit == "random" )				ChooseRandomModifiers();
	else
	{
		return false;
	}

	return true;
}

void NextFloat( float fValues[], int size )
{
	int index = -1;
	for( int i=0; i<size; i++ )
	{
		if( fValues[i] == 1 )
		{
			index = i;
			break;
		}
	}

	for( int i=0; i<size; i++ )
		fValues[i] = 0;

	if( index == size-1 )	// if true, then the last float in the list was selected
		;	// leave all off
	else
		fValues[index+1] = 1;
}

void NextBool( bool bValues[], int size )
{
	int index = -1;
	for( int i=0; i<size; i++ )
	{
		if( bValues[i] )
		{
			index = i;
			break;
		}
	}

	for( int i=0; i<size; i++ )
		bValues[i] = false;

	if( index == size-1 )	// if true, then the last float in the list was selected
		;	// leave all off
	else
		bValues[index+1] = 1;
}

void PlayerOptions::NextAccel()
{
	NextFloat( m_fAccels, NUM_ACCELS );
}

void PlayerOptions::NextEffect()
{
	NextFloat( m_fEffects, NUM_EFFECTS );
}

void PlayerOptions::NextAppearance()
{
	NextFloat( m_fAppearances, NUM_APPEARANCES );
}

void PlayerOptions::NextTurn()
{
	NextBool( m_bTurns, NUM_TURNS );
}

void PlayerOptions::NextTransform()
{
	NextBool( m_bTransforms, NUM_TRANSFORMS );
}

void PlayerOptions::NextScroll()
{
	NextFloat( m_fScrolls, NUM_SCROLLS );
}

void PlayerOptions::NextPerspective()
{
	switch( (int)m_fPerspectiveTilt )
	{
	case -1:		m_fPerspectiveTilt =  0;	break;
	case  0:		m_fPerspectiveTilt = +1;	break;
	case +1: default:	m_fPerspectiveTilt = -1;	break;
	}
}

void PlayerOptions::ChooseRandomModifiers()
{
	if( RandomFloat(0,1)>0.8f )
		m_fScrollSpeed = 1.5f;
	if( RandomFloat(0,1)>0.8f )
		m_fScrolls[SCROLL_REVERSE] = 1;
	if( RandomFloat(0,1)>0.9f )
		m_fDark = 1;
	float f;
	f = RandomFloat(0,1);
	if( f>0.66f )
		m_fAccels[RandomInt(NUM_ACCELS)] = 1;
	else if( f>0.33f )
		m_fEffects[RandomInt(NUM_EFFECTS)] = 1;
	f = RandomFloat(0,1);
	if( f>0.95f )
		m_fAppearances[APPEARANCE_HIDDEN] = 1;
	else if( f>0.9f )
		m_fAppearances[APPEARANCE_SUDDEN] = 1;
}

PlayerOptions::Accel PlayerOptions::GetFirstAccel()
{
	for( int i=0; i<NUM_ACCELS; i++ )
		if( m_fAccels[i] == 1.f )
			return (Accel)i;
	return (Accel)-1;
}

PlayerOptions::Effect PlayerOptions::GetFirstEffect()
{
	for( int i=0; i<NUM_EFFECTS; i++ )
		if( m_fEffects[i] == 1.f )
			return (Effect)i;
	return (Effect)-1;
}

PlayerOptions::Appearance PlayerOptions::GetFirstAppearance()
{
	for( int i=0; i<NUM_APPEARANCES; i++ )
		if( m_fAppearances[i] == 1.f )
			return (Appearance)i;
	return (Appearance)-1;
}

PlayerOptions::Scroll PlayerOptions::GetFirstScroll()
{
	for( int i=0; i<NUM_SCROLLS; i++ )
		if( m_fScrolls[i] == 1.f )
			return (Scroll)i;
	return (Scroll)-1;
}

void PlayerOptions::SetOneAccel( Accel a )
{
	ZERO( m_fAccels );
	m_fAccels[a] = 1;
}

void PlayerOptions::SetOneEffect( Effect e )
{
	ZERO( m_fEffects );
	m_fEffects[e] = 1;
}

void PlayerOptions::SetOneAppearance( Appearance a )
{
	ZERO( m_fAppearances );
	m_fAppearances[a] = 1;
}

void PlayerOptions::SetOneScroll( Scroll s )
{
	ZERO( m_fScrolls );
	m_fScrolls[s] = 1;
}

void PlayerOptions::ToggleOneTurn( Turn t )
{
	bool bWasOn = m_bTurns[t];
	ZERO( m_bTurns );
	m_bTurns[t] = !bWasOn;
}

float PlayerOptions::GetReversePercentForColumn( int iCol ) const
{
	float f = 0;
	int iNumCols = GAMESTATE->GetCurrentStyle()->m_iColsPerPlayer;

	f += m_fScrolls[SCROLL_REVERSE];
	
	if( iCol >= iNumCols/2 )
		f += m_fScrolls[SCROLL_SPLIT];

	if( (iCol%2)==1 )
		f += m_fScrolls[SCROLL_ALTERNATE];
	
	int iFirstCrossCol = iNumCols/4;
	int iLastCrossCol = iNumCols-1-iFirstCrossCol;
	if( iCol>=iFirstCrossCol && iCol<=iLastCrossCol )
		f += m_fScrolls[SCROLL_CROSS];
	
	if( f > 2 )
		f = fmodf( f, 2 );
	if( f > 1 )
		f = SCALE( f, 1.f, 2.f, 1.f, 0.f );
	return f;
}

bool PlayerOptions::operator==( const PlayerOptions &other ) const
{
#define COMPARE(x) { if( x != other.x ) return false; }
	COMPARE(m_fTimeSpacing);
	COMPARE(m_fScrollSpeed);
	COMPARE(m_fScrollBPM);
	COMPARE(m_fRandomSpeed);
	COMPARE(m_ScoreDisplay);
	COMPARE(m_FailType);
	COMPARE(m_bMuteOnError);
	COMPARE(m_fDark);
	COMPARE(m_fBlind);
	COMPARE(m_fCover);
	COMPARE(m_fRandAttack);
	COMPARE(m_fSongAttack);
	COMPARE(m_fPlayerAutoPlay);
	COMPARE(m_fPerspectiveTilt);
	COMPARE(m_fSkew);
	COMPARE(m_sNoteSkin);
	for( int i = 0; i < PlayerOptions::NUM_ACCELS; ++i )
		COMPARE(m_fAccels[i]);
	for( int i = 0; i < PlayerOptions::NUM_EFFECTS; ++i )
		COMPARE(m_fEffects[i]);
	for( int i = 0; i < PlayerOptions::NUM_APPEARANCES; ++i )
		COMPARE(m_fAppearances[i]);
	for( int i = 0; i < PlayerOptions::NUM_SCROLLS; ++i )
		COMPARE(m_fScrolls[i]);
	for( int i = 0; i < PlayerOptions::NUM_TURNS; ++i )
		COMPARE(m_bTurns[i]);
	for( int i = 0; i < PlayerOptions::NUM_TRANSFORMS; ++i )
		COMPARE(m_bTransforms[i]);
#undef COMPARE
	return true;
}

bool PlayerOptions::IsEasierForSongAndSteps( Song* pSong, Steps* pSteps, PlayerNumber pn ) const
{
	if( m_fTimeSpacing && pSong->HasSignificantBpmChangesOrStops() )
		return true;
	const RadarValues &rv = pSteps->GetRadarValues( pn );
	if( m_bTransforms[TRANSFORM_NOHOLDS] && rv[RadarCategory_Holds]>0 )
		return true;
	if( m_bTransforms[TRANSFORM_NOROLLS] && rv[RadarCategory_Rolls]>0 )
		return true;
	if( m_bTransforms[TRANSFORM_NOMINES] && rv[RadarCategory_Mines]>0 )
		return true;
	if( m_bTransforms[TRANSFORM_NOHANDS] && rv[RadarCategory_Hands]>0 )
		return true;
	if( m_bTransforms[TRANSFORM_NOQUADS] && rv[RadarCategory_Hands]>0 )
		return true;
	if( m_bTransforms[TRANSFORM_NOJUMPS] && rv[RadarCategory_Jumps]>0 )
		return true;
	if( m_bTransforms[TRANSFORM_NOSTRETCH] )
		return true;

	// Inserted holds can be really easy on some songs, and scores will be 
	// highly hold-weighted, and very little tap score weighted.
	if( m_bTransforms[TRANSFORM_LITTLE] )	return true;
	if( m_bTransforms[TRANSFORM_PLANTED] )	return true;
	if( m_bTransforms[TRANSFORM_FLOORED] )	return true;
	if( m_bTransforms[TRANSFORM_TWISTER] )	return true;

	// This makes songs with sparse notes easier.
	if( m_bTransforms[TRANSFORM_ECHO] )	return true;
	
	if( m_fCover )	return true;
	if( m_fPlayerAutoPlay )	return true;
	return false;
}

bool PlayerOptions::IsEasierForCourseAndTrail( Course* pCourse, Trail* pTrail ) const
{
	ASSERT( pCourse );
	ASSERT( pTrail );

	FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
	{
		if( e->pSong && IsEasierForSongAndSteps(e->pSong, e->pSteps, PLAYER_1) )
			return true;
	}
	return false;
}

void PlayerOptions::GetLocalizedMods( vector<RString> &AddTo ) const
{
	vector<RString> vMods;
	GetMods( vMods );
	FOREACH_CONST( RString, vMods, s )
	{
		const RString& sOneMod = *s;

		ASSERT( !sOneMod.empty() );

		vector<RString> asTokens;
		split( sOneMod, " ", asTokens );

		if( asTokens.empty() )
			continue;

		// Strip the approach speed token, if any
		if( asTokens[0][0] == '*' )
			asTokens.erase( asTokens.begin() );

		// capitalize NoteSkin names
		asTokens.back() = Capitalize( asTokens.back() );

		/* Theme the mod name (the last string).  Allow this to not exist, since
		 * characters might use modifiers that don't exist in the theme. */
		asTokens.back() = CommonMetrics::LocalizeOptionItem( asTokens.back(), true );

		RString sLocalizedMod = join( " ", asTokens );
		AddTo.push_back( sLocalizedMod );
	}
}

bool PlayerOptions::ContainsTransformOrTurn() const
{
	for( int i=0; i<NUM_TRANSFORMS; i++ )
	{
		if( m_bTransforms[i] )
			return true;
	}
	for( int i=0; i<NUM_TURNS; i++ )
	{
		if( m_bTurns[i] )
			return true;
	}
	return false;
}

RString PlayerOptions::GetSavedPrefsString() const
{
	PlayerOptions po_prefs;
#define SAVE(x) po_prefs.x = this->x;
	SAVE( m_fTimeSpacing );
	SAVE( m_fScrollSpeed );
	SAVE( m_fScrollBPM );
	SAVE( m_fScrolls[SCROLL_REVERSE] );
	SAVE( m_fPerspectiveTilt );
	SAVE( m_bTransforms[TRANSFORM_NOHOLDS] );
	SAVE( m_bTransforms[TRANSFORM_NOROLLS] );
	SAVE( m_bTransforms[TRANSFORM_NOMINES] );
	SAVE( m_bTransforms[TRANSFORM_NOJUMPS] );
	SAVE( m_bTransforms[TRANSFORM_NOHANDS] );
	SAVE( m_bTransforms[TRANSFORM_NOQUADS] );
	SAVE( m_bTransforms[TRANSFORM_NOSTRETCH] );
	SAVE( m_ScoreDisplay );
	SAVE( m_bMuteOnError );
	SAVE( m_sNoteSkin );
#undef SAVE
	return po_prefs.GetString();
}

void PlayerOptions::ResetPrefs( ResetPrefsType type )
{
	PlayerOptions defaults;
#define CPY(x) this->x = defaults.x;
	switch( type )
	{
	DEFAULT_FAIL( type );
	case saved_prefs:
		CPY( m_fTimeSpacing );
		CPY( m_fScrollSpeed );
		CPY( m_fScrollBPM );
		break;
	case saved_prefs_invalid_for_course:
		break;
	}
	CPY( m_fPerspectiveTilt );
	CPY( m_bTransforms[TRANSFORM_NOHOLDS] );
	CPY( m_bTransforms[TRANSFORM_NOROLLS] );
	CPY( m_bTransforms[TRANSFORM_NOMINES] );
	CPY( m_bTransforms[TRANSFORM_NOJUMPS] );
	CPY( m_bTransforms[TRANSFORM_NOHANDS] );
	CPY( m_bTransforms[TRANSFORM_NOQUADS] );
	CPY( m_bTransforms[TRANSFORM_NOSTRETCH] );
	// Don't clear this.
	// CPY( m_sNoteSkin );
#undef CPY
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
