#include "global.h"

#include "PercentageDisplay.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "PrefsManager.h"
#include "ActorUtil.h"
#include "RageLog.h"
#include "StageStats.h"
#include "PlayerState.h"
#include "XmlFile.h"
#include "Course.h"


REGISTER_ACTOR_CLASS( PercentageDisplay )

PercentageDisplay::PercentageDisplay()
{
	m_pPlayerState = NULL;
	m_pPlayerStageStats = NULL;

	m_Last = -1;
	m_LastMax = -1;
	m_iDancePointsDigits = 0;
	m_bUseRemainder = false;
	m_bApplyScoreDisplayOptions = false;
	m_bAutoRefresh = false;
	m_FormatPercentScore.SetFromExpression( "FormatPercentScore" );
}

void PercentageDisplay::LoadFromNode( const XNode* pNode )
{
	pNode->GetAttrValue( "DancePointsDigits", m_iDancePointsDigits );
	pNode->GetAttrValue( "ApplyScoreDisplayOptions", m_bApplyScoreDisplayOptions );
	pNode->GetAttrValue( "AutoRefresh", m_bAutoRefresh );
	{
		Lua *L = LUA->Get();
		if( pNode->PushAttrValue(L, "FormatPercentScore") )
			m_FormatPercentScore.SetFromStack( L );
		else
			lua_pop(L, 1);
		LUA->Release(L);
	}

	const XNode *pChild = pNode->GetChild( "Percent" );
	if( pChild == NULL )
		RageException::Throw( "%s: PercentageDisplay: missing the node \"Percent\"", ActorUtil::GetWhere(pNode).c_str() );
	m_textPercent.LoadFromNode( pChild );
	this->AddChild( &m_textPercent );

	pChild = pNode->GetChild( "PercentRemainder" );
	if( !ShowDancePointsNotPercentage()  &&  pChild != NULL )
	{
		m_bUseRemainder = true;
		m_textPercentRemainder.LoadFromNode( pChild );
		this->AddChild( &m_textPercentRemainder );
	}

	// only run the Init command after we load Fonts.
	ActorFrame::LoadFromNode( pNode );
}

void PercentageDisplay::Load( const PlayerState *pPlayerState, const PlayerStageStats *pPlayerStageStats )
{
	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;

	Refresh();
}

void PercentageDisplay::Load( const PlayerState *pPlayerState, const PlayerStageStats *pPlayerStageStats, const RString &sMetricsGroup, bool bAutoRefresh )
{
	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;
	m_bAutoRefresh = bAutoRefresh;

	m_iDancePointsDigits = THEME->GetMetricI( sMetricsGroup, "DancePointsDigits" );
	m_bUseRemainder = THEME->GetMetricB( sMetricsGroup, "PercentUseRemainder" );
	m_bApplyScoreDisplayOptions = THEME->GetMetricB( sMetricsGroup, "ApplyScoreDisplayOptions" );
	m_FormatPercentScore = THEME->GetMetricR( sMetricsGroup, "Format" );
	
	if( m_FormatPercentScore.IsNil() )
	{
		LOG->Trace( "Format is nil in [%s]. Defaulting to 'FormatPercentScore'.", sMetricsGroup.c_str() );
		m_FormatPercentScore.SetFromExpression( "FormatPercentScore" );
	}

	if( ShowDancePointsNotPercentage() )
		m_textPercent.SetName( "DancePoints" + PlayerNumberToString(m_pPlayerState->m_PlayerNumber) );
	else
		m_textPercent.SetName( "Percent" + PlayerNumberToString(m_pPlayerState->m_PlayerNumber) );

	m_textPercent.LoadFromFont( THEME->GetPathF(sMetricsGroup,"text") );
	ActorUtil::SetXY( m_textPercent, sMetricsGroup );
	ActorUtil::LoadAllCommands( m_textPercent, sMetricsGroup );
	this->AddChild( &m_textPercent );

	if( !ShowDancePointsNotPercentage() && m_bUseRemainder )
	{
		m_textPercentRemainder.SetName( "PercentRemainder" + PlayerNumberToString(m_pPlayerState->m_PlayerNumber) );
		m_textPercentRemainder.LoadFromFont( THEME->GetPathF(sMetricsGroup,"remainder") );
		ActorUtil::SetXY( m_textPercentRemainder, sMetricsGroup );
		ActorUtil::LoadAllCommands( m_textPercentRemainder, sMetricsGroup );
		ASSERT( m_textPercentRemainder.HasCommand("Off") );
		m_textPercentRemainder.SetText( "456" );
		this->AddChild( &m_textPercentRemainder );
	}

	Refresh();
}

void PercentageDisplay::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	if( m_bAutoRefresh )
		Refresh();
}

void PercentageDisplay::Refresh()
{
	const int iActualDancePoints = m_pPlayerStageStats->m_iActualDancePoints;
	const int iCurPossibleDancePoints = m_pPlayerStageStats->m_iCurPossibleDancePoints;

	if( iActualDancePoints == m_Last && iCurPossibleDancePoints == m_LastMax )
		return;

	m_Last = iActualDancePoints;
	m_LastMax = iCurPossibleDancePoints;

	RString sNumToDisplay;

	if( ShowDancePointsNotPercentage() )
	{
		sNumToDisplay = ssprintf( "%*d", m_iDancePointsDigits, max( 0, iActualDancePoints ) );
	}
	else
	{
		float fPercentDancePoints = m_pPlayerStageStats->GetPercentDancePoints();
		float fCurMaxPercentDancePoints = m_pPlayerStageStats->GetCurMaxPercentDancePoints();
		
		if( m_bApplyScoreDisplayOptions )
		{
			switch( m_pPlayerState->m_PlayerOptions.GetCurrent().m_ScoreDisplay )
			{
			case PlayerOptions::SCORING_ADD:
				// nothing to do
				break;
			case PlayerOptions::SCORING_SUBTRACT:
				fPercentDancePoints = 1.0f - ( fCurMaxPercentDancePoints - fPercentDancePoints );
				break;
			case PlayerOptions::SCORING_AVERAGE:
				if( fCurMaxPercentDancePoints == 0.0f ) // don't divide by zero fats
					fPercentDancePoints = 0.0f;
				else
					fPercentDancePoints = fPercentDancePoints / fCurMaxPercentDancePoints;
				break;
			}
		}

		// clamp percentage - feedback is that negative numbers look weird here.
		CLAMP( fPercentDancePoints, 0.f, 1.f );

		if( m_bUseRemainder )
		{
			int iPercentWhole = int(fPercentDancePoints*100);
			int iPercentRemainder = int( (fPercentDancePoints*100 - int(fPercentDancePoints*100)) * 10 );
			sNumToDisplay = ssprintf( "%2d", iPercentWhole );
			m_textPercentRemainder.SetText( ssprintf(".%01d%%", iPercentRemainder) );
		}
		else
		{		
			Lua *L = LUA->Get();
			m_FormatPercentScore.PushSelf( L );
			ASSERT( !lua_isnil(L, -1) );
			LuaHelpers::Push( L, fPercentDancePoints );
			RString sError;
			if( !LuaHelpers::RunScriptOnStack(L, sError, 1, 1) ) // 1 arg, 1 result
				LOG->Warn( "Error running FormatPercentScore: %s", sError.c_str() );
			LuaHelpers::Pop( L, sNumToDisplay );
			LUA->Release(L);
			
			// HACK: Use the last frame in the numbers texture as '-'
			sNumToDisplay.Replace('-','x');
		}
	}

	m_textPercent.SetText( sNumToDisplay );
}

bool PercentageDisplay::ShowDancePointsNotPercentage() const
{
	// Use staight dance points in workout because the percentage denominator isn't accurate - we don't know when the players are going to stop.

	if( GAMESTATE->m_pCurCourse )
	{
		if( GAMESTATE->m_pCurCourse->m_fGoalSeconds > 0 )
			return true;
	}

	if( PREFSMAN->m_bDancePointsForOni )
		return true;

	return false;
}


#include "LuaBinding.h"

class LunaPercentageDisplay: public Luna<PercentageDisplay>
{
public:
	static int LoadFromStats( T* p, lua_State *L )
	{
		const PlayerState *pStageStats = Luna<PlayerState>::check( L, 1 );
		const PlayerStageStats *pPlayerStageStats = Luna<PlayerStageStats>::check( L, 2 );
		p->Load( pStageStats, pPlayerStageStats );
		return 0;
	}

	LunaPercentageDisplay()
	{
		ADD_METHOD( LoadFromStats );
	}
};

LUA_REGISTER_DERIVED_CLASS( PercentageDisplay, ActorFrame )
// lua end


/*
 * (c) 2001-2003 Chris Danford
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
