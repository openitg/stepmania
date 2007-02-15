#include "global.h"
#include "CodeDetector.h"
#include "PlayerOptions.h"
#include "GameState.h"
#include "InputQueue.h"
#include "InputMapper.h"
#include "ThemeManager.h"
#include "RageLog.h"
#include "Game.h"
#include "RageUtil.h"
#include "PlayerState.h"
#include "InputEventPlus.h"

const char *CodeNames[] = {
	"Easier1",
	"Easier2",
	"Harder1",
	"Harder2",
	"NextSort1",
	"NextSort2",
	"NextSort3",
	"NextSort4",
	"ModeMenu1",
	"ModeMenu2",
	"Mirror",
	"Left",
	"Right",
	"Shuffle",
	"SuperShuffle",
	"NextTransform",
	"NextScrollSpeed",
	"PreviousScrollSpeed",
	"NextAccel",
	"NextEffect",
	"NextAppearance",
	"NextTurn",
	"Reverse",
	"HoldNotes",
	"Mines",
	"Dark",
	"Hidden",
	"RandomVanish",
	"CancelAll",
	"NextTheme",
	"NextTheme2",
	"NextAnnouncer",
	"NextAnnouncer2",
	"NextBannerGroup",
	"NextBannerGroup2",
	"SaveScreenshot1",
	"SaveScreenshot2",
	"CancelAllPlayerOptions",
	"BackInEventMode",
};
XToString( Code );

static CodeItem g_CodeItems[NUM_Code];


bool CodeItem::EnteredCode( GameController controller ) const
{
	if( controller == GameController_Invalid )
		return false;
	if( buttons.size() == 0 )
		return false;

	switch( m_Type )
	{
	case sequence:
		return INPUTQUEUE->MatchesSequence( controller, &buttons[0], buttons.size(), fMaxSecondsBack );
	case hold_and_press:
		{
			RageTimer OldestTimeAllowed = RageTimer() - 0.05f;
			InputEventPlus iep;
			if( !INPUTQUEUE->WasPressedRecently(controller, buttons[buttons.size()-1], OldestTimeAllowed, &iep) )
				return false;

			// Check that all but the last were being held when the last button was pressed.
			for( unsigned i=0; i<buttons.size()-1; i++ )
			{
				GameInput gi( controller, buttons[i] );
				if( !INPUTMAPPER->IsBeingPressed(gi, MultiPlayer_Invalid, &iep.InputList) )
					return false;
			}
			return true;
		}
		break;
	case tap:
		return INPUTQUEUE->AllWerePressedRecently( controller, &buttons[0], buttons.size(), fMaxSecondsBack );
	default:
		ASSERT(0);
		return false;
	}
}

bool CodeItem::Load( RString sButtonsNames )
{
	buttons.clear();

	vector<RString> asButtonNames;

	bool bHasAPlus = sButtonsNames.find( '+' ) != string::npos;
	bool bHasADash = sButtonsNames.find( '-' ) != string::npos;

	if( bHasAPlus )
	{
		m_Type = tap;
		split( sButtonsNames, "+", asButtonNames, false );
	}
	else if( bHasADash )
	{
		m_Type = hold_and_press;
		split( sButtonsNames, "-", asButtonNames, false );
	}
	else
	{
		m_Type = sequence;
		split( sButtonsNames, ",", asButtonNames, false );
	}

	if( asButtonNames.size() < 1 )
	{
		if( sButtonsNames != "" )
			LOG->Trace( "The code '%s' is less than 2 buttons, so it will be ignored.", sButtonsNames.c_str() );
		return false;
	}

	for( unsigned i=0; i<asButtonNames.size(); i++ )	// for each button in this code
	{
		const RString sButtonName = asButtonNames[i];

		// Search for the corresponding GameButton
		const GameButton gb = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex( sButtonName );
		if( gb == GameButton_Invalid )
		{
			LOG->Trace( "The code '%s' contains an unrecognized button '%s'.", sButtonsNames.c_str(), sButtonName.c_str() );
			buttons.clear();
			return false;
		}

		buttons.push_back( gb );
	}

	switch( m_Type )
	{
	case sequence:
		if( buttons.size() == 1 )
			fMaxSecondsBack = -1;
		else
			fMaxSecondsBack = (buttons.size()-1)*0.6f;
		break;
	case hold_and_press:
		fMaxSecondsBack = -1.f;	// not applicable
		break;
	case tap:
		fMaxSecondsBack = 0.05f;	// simultaneous
		break;
	default:
		ASSERT(0);
	}

	// if we make it here, we found all the buttons in the code
	return true;
}

bool CodeDetector::EnteredCode( GameController controller, Code code )
{
	return g_CodeItems[code].EnteredCode( controller );
}


void CodeDetector::RefreshCacheItems( RString sClass )
{
	if( sClass == "" )
		sClass = "CodeDetector";
	FOREACH_Code( c )
	{
		CodeItem& item = g_CodeItems[c];
		const RString sCodeName = CodeToString(c);
		const RString sButtonsNames = THEME->GetMetric(sClass,sCodeName);

		item.Load( sButtonsNames );
	}
}

bool CodeDetector::EnteredNextBannerGroup( GameController controller )
{
	return EnteredCode(controller,CODE_BW_NEXT_GROUP) || EnteredCode(controller,CODE_BW_NEXT_GROUP2);
}

bool CodeDetector::EnteredEasierDifficulty( GameController controller )
{
	return EnteredCode(controller,CODE_EASIER1) || EnteredCode(controller,CODE_EASIER2);
}

bool CodeDetector::EnteredHarderDifficulty( GameController controller )
{
	return EnteredCode(controller,CODE_HARDER1) || EnteredCode(controller,CODE_HARDER2);
}

bool CodeDetector::EnteredNextSort( GameController controller )
{
	return EnteredCode(controller,CODE_NEXT_SORT1) ||
		   EnteredCode(controller,CODE_NEXT_SORT2) ||
		   EnteredCode(controller,CODE_NEXT_SORT3) ||
		   EnteredCode(controller,CODE_NEXT_SORT4);
}

bool CodeDetector::EnteredModeMenu( GameController controller )
{
	return EnteredCode(controller,CODE_MODE_MENU1) || EnteredCode(controller,CODE_MODE_MENU2);
}

#define  TOGGLE(v,a,b)	if(v!=a) v=a; else v=b;
#define  FLOAT_TOGGLE(v)	if(v!=1.f) v=1.f; else v=0.f;
// XXX: Read the metrics file instead!
// Using this can give us unlisted scroll speeds on the Options screen.
#define  INCREMENT_SCROLL_SPEED(s)	(s==0.5f) ? s=0.75f : (s==0.75f) ? s=1.0f : (s==1.0f) ? s=1.5f : (s==1.5f) ? s=2.0f : (s==2.0f) ? s=3.0f : (s==3.0f) ? s=4.0f : (s==4.0f) ? s=5.0f : (s==5.0f) ? s=8.0f : s=0.5f;
#define  DECREMENT_SCROLL_SPEED(s)	(s==0.75f) ? s=0.5f : (s==1.0f) ? s=0.75f : (s==1.5f) ? s=1.0f : (s==2.0f) ? s=1.5f : (s==3.0f) ? s=2.0f : (s==4.0f) ? s=3.0f : (s==5.0f) ? s=4.0f : (s==8.0f) ? s=4.0f : s=8.0f;

bool CodeDetector::DetectAndAdjustMusicOptions( GameController controller )
{
	PlayerNumber pn = INPUTMAPPER->ControllerToPlayerNumber( controller );

	for( int c=CODE_MIRROR; c<=CODE_CANCEL_ALL; c++ )
	{
		Code code = (Code)c;
		
		PlayerOptions po = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.GetPreferred();

		if( EnteredCode(controller,code) )
		{
			switch( code )
			{
			case CODE_MIRROR:			po.ToggleOneTurn( PlayerOptions::TURN_MIRROR );		break;
			case CODE_LEFT:				po.ToggleOneTurn( PlayerOptions::TURN_LEFT );		break;
			case CODE_RIGHT:			po.ToggleOneTurn( PlayerOptions::TURN_RIGHT );		break;
			case CODE_SHUFFLE:			po.ToggleOneTurn( PlayerOptions::TURN_SHUFFLE );	break;
			case CODE_SUPER_SHUFFLE:		po.ToggleOneTurn( PlayerOptions::TURN_SUPER_SHUFFLE );	break;
			case CODE_NEXT_TRANSFORM:		po.NextTransform();					break;
			case CODE_NEXT_SCROLL_SPEED:		INCREMENT_SCROLL_SPEED( po.m_fScrollSpeed );		break;
			case CODE_PREVIOUS_SCROLL_SPEED:	DECREMENT_SCROLL_SPEED( po.m_fScrollSpeed );		break;
			case CODE_NEXT_ACCEL:			po.NextAccel();						break;
			case CODE_NEXT_EFFECT:			po.NextEffect();					break;
			case CODE_NEXT_APPEARANCE:		po.NextAppearance();					break;
			case CODE_NEXT_TURN:			po.NextTurn();						break;
			case CODE_REVERSE:			po.NextScroll();					break;
			case CODE_HOLDS:			TOGGLE( po.m_bTransforms[PlayerOptions::TRANSFORM_NOHOLDS], true, false );	break;
			case CODE_MINES:			TOGGLE( po.m_bTransforms[PlayerOptions::TRANSFORM_NOMINES], true, false );	break;
			case CODE_DARK:				FLOAT_TOGGLE( po.m_fDark );				break;
			case CODE_CANCEL_ALL:			GAMESTATE->GetDefaultPlayerOptions( po );		break;
			case CODE_HIDDEN:
				ZERO(po.m_fAppearances);
				po.m_fAppearances[PlayerOptions::APPEARANCE_HIDDEN] = 1;
				break;
			case CODE_RANDOMVANISH:
				ZERO(po.m_fAppearances);
				po.m_fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH] = 1;
				break;
			default:	break;
			}

			GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.Assign( ModsLevel_Preferred, po );


			return true;	// don't check any more
		}
	}

	return false;
}

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