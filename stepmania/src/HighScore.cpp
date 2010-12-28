#include "global.h"
#include "HighScore.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"
#include "ThemeManager.h"
#include "XmlFile.h"
#include "Foreach.h"
#include "Json/Value.h"
#include "JsonUtil.h"

#define EMPTY_NAME			THEME->GetMetric ("HighScore","EmptyName")


struct HighScoreImpl
{
	RString	sName;	// name that shows in the machine's ranking screen
	Grade grade;
	int iScore;
	float fPercentDP;
	float fSurviveSeconds;
	RString	sModifiers;
	DateTime dateTime;		// return value of time() when screenshot was taken
	RString sPlayerGuid;	// who made this high score
	RString sMachineGuid;	// where this high score was made
	int iProductID;
	int iTapNoteScores[NUM_TapNoteScore];
	int iHoldNoteScores[NUM_HoldNoteScore];
	RadarValues radarValues;
	float fLifeRemainingSeconds;

	HighScoreImpl();
	XNode *CreateNode() const;
	void LoadFromNode( const XNode *pNode );

	void Serialize( Json::Value &root ) const;
	void Deserialize( const Json::Value &root );

	bool operator==( const HighScoreImpl& other ) const;
	bool operator!=( const HighScoreImpl& other ) const { return !(*this == other); }
};

bool HighScoreImpl::operator==( const HighScoreImpl& other ) const 
{
#define COMPARE(x)	if( x!=other.x )	return false;
	COMPARE( sName );
	COMPARE( grade );
	COMPARE( iScore );
	COMPARE( fPercentDP );
	COMPARE( fSurviveSeconds );
	COMPARE( sModifiers );
	COMPARE( dateTime );
	COMPARE( sPlayerGuid );
	COMPARE( sMachineGuid );
	COMPARE( iProductID );
	FOREACH_TapNoteScore( tns )
		COMPARE( iTapNoteScores[tns] );
	FOREACH_HoldNoteScore( hns )
		COMPARE( iHoldNoteScores[hns] );
	COMPARE( radarValues );
	COMPARE( fLifeRemainingSeconds );
#undef COMPARE

	return true;
}

HighScoreImpl::HighScoreImpl()
{
	sName = "";
	grade = Grade_NoData;
	iScore = 0;
	fPercentDP = 0;
	fSurviveSeconds = 0;
	sModifiers = "";
	dateTime.Init();
	sPlayerGuid = "";
	sMachineGuid = "";
	iProductID = 0;
	ZERO( iTapNoteScores );
	ZERO( iHoldNoteScores );
	radarValues.MakeUnknown();
	fLifeRemainingSeconds = 0;
}

XNode *HighScoreImpl::CreateNode() const
{
	XNode *pNode = new XNode;
	pNode->m_sName = "HighScore";

	// TRICKY:  Don't write "name to fill in" markers.
	pNode->AppendChild( "Name", IsRankingToFillIn(sName) ? RString("") : sName );
	pNode->AppendChild( "Grade",			GradeToString(grade) );
	pNode->AppendChild( "Score",			iScore );
	pNode->AppendChild( "PercentDP",		fPercentDP );
	pNode->AppendChild( "SurviveSeconds",	fSurviveSeconds );
	pNode->AppendChild( "Modifiers",		sModifiers );
	pNode->AppendChild( "DateTime",			dateTime );
	pNode->AppendChild( "PlayerGuid",		sPlayerGuid );
	pNode->AppendChild( "MachineGuid",		sMachineGuid );
	pNode->AppendChild( "ProductID",		iProductID );
	XNode* pTapNoteScores = pNode->AppendChild( "TapNoteScores" );
	FOREACH_TapNoteScore( tns )
		if( tns != TNS_None )	// HACK: don't save meaningless "none" count
			pTapNoteScores->AppendChild( TapNoteScoreToString(tns), iTapNoteScores[tns] );
	XNode* pHoldNoteScores = pNode->AppendChild( "HoldNoteScores" );
	FOREACH_HoldNoteScore( hns )
		if( hns != HNS_None )	// HACK: don't save meaningless "none" count
			pHoldNoteScores->AppendChild( HoldNoteScoreToString(hns), iHoldNoteScores[hns] );
	pNode->AppendChild( radarValues.CreateNode() );
	pNode->AppendChild( "LifeRemainingSeconds",		fLifeRemainingSeconds );

	return pNode;
}

void HighScoreImpl::LoadFromNode( const XNode *pNode )
{
	ASSERT( pNode->m_sName == "HighScore" );

	RString s;

	pNode->GetChildValue( "Name", sName );
	pNode->GetChildValue( "Grade", s );
	grade = StringToGrade( s );
	pNode->GetChildValue( "Score",			iScore );
	pNode->GetChildValue( "PercentDP",		fPercentDP );
	pNode->GetChildValue( "SurviveSeconds", fSurviveSeconds );
	pNode->GetChildValue( "Modifiers",		sModifiers );
	pNode->GetChildValue( "DateTime",		dateTime );
	pNode->GetChildValue( "PlayerGuid",		sPlayerGuid );
	pNode->GetChildValue( "MachineGuid",	sMachineGuid );
	pNode->GetChildValue( "ProductID",		iProductID );
	const XNode* pTapNoteScores = pNode->GetChild( "TapNoteScores" );
	if( pTapNoteScores )
		FOREACH_TapNoteScore( tns )
			pTapNoteScores->GetChildValue( TapNoteScoreToString(tns), iTapNoteScores[tns] );
	const XNode* pHoldNoteScores = pNode->GetChild( "HoldNoteScores" );
	if( pHoldNoteScores )
		FOREACH_HoldNoteScore( hns )
			pHoldNoteScores->GetChildValue( HoldNoteScoreToString(hns), iHoldNoteScores[hns] );
	const XNode* pRadarValues = pNode->GetChild( "RadarValues" );
	if( pRadarValues )
		radarValues.LoadFromNode( pRadarValues );
	pNode->GetChildValue( "LifeRemainingSeconds",		fLifeRemainingSeconds );

	/* Validate input. */
	grade = clamp( grade, Grade_Tier01, Grade_Failed );
}

void HighScoreImpl::Serialize( Json::Value &root ) const
{
	// Tricky: Don't write "name to fill in" markers.
	root["Name"] =		IsRankingToFillIn(sName) ? RString() : sName;
	root["Grade"] =		GradeToString(grade);
	root["Score"] =		iScore;
	root["PercentDP"] =	fPercentDP;
	root["SurviveSeconds"] =fSurviveSeconds;
	root["Modifiers"] =	sModifiers;
	root["DateTime"] =	dateTime.GetString();
	root["PlayerGuid"] =	sPlayerGuid;
	root["MachineGuid"] =	sMachineGuid;
	root["ProductID"] =	iProductID;
	root["LifeRemainingSeconds"] =	fLifeRemainingSeconds;
	{
		Json::Value &v = root["TapNoteScores"];
		FOREACH_TapNoteScore( tns )
			if( tns != TNS_None ) // don't save meaningless "none" count
				v[ TapNoteScoreToString(tns) ] = iTapNoteScores[tns];
	}
	{
		Json::Value &v = root["HoldNoteScores"];
		FOREACH_HoldNoteScore( hns )
			if( hns != HNS_None ) // don't save meaningless "none" count
				v[ HoldNoteScoreToString(hns) ] = iHoldNoteScores[hns];
	}
	radarValues.Serialize( root["RadarValues"] );
}

void HighScoreImpl::Deserialize( const Json::Value &root )
{
	sName = root["Name"].asString();
	grade = StringToGrade( root["Grade"].asString() );
	iScore = root["Score"].asInt();
	fPercentDP = (float)root["PercentDP"].asDouble();
	fSurviveSeconds = (float)root["SurviveSeconds"].asDouble();
	sModifiers = root["Modifiers"].asString();
	dateTime.FromString( root["DateTime"].asString() );
	sPlayerGuid = root["PlayerGuid"].asString();
	sMachineGuid = root["MachineGuid"].asString();
	iProductID = root["ProductID"].asInt();
	fLifeRemainingSeconds = (float)root["LifeRemainingSeconds"].asDouble();
	{
		const Json::Value &v = root["TapNoteScores"];
		FOREACH_TapNoteScore( tns )
			v[ TapNoteScoreToString(tns) ].TryGet( iTapNoteScores[tns] );
	}
	{
		const Json::Value &v = root["HoldNoteScores"];
		FOREACH_HoldNoteScore( hns )
			v[ HoldNoteScoreToString(hns) ].TryGet( iHoldNoteScores[hns] );
	}
	radarValues.Deserialize( root["RadarValues"] );

	/* Validate input. */
	grade = clamp( grade, Grade_Tier01, Grade_Failed );
}

REGISTER_CLASS_TRAITS( HighScoreImpl, new HighScoreImpl(*pCopy) )

HighScore::HighScore()
{
	m_Impl = new HighScoreImpl;
}

void HighScore::Unset()
{
	m_Impl = new HighScoreImpl;
}

RString	HighScore::GetName() const { return m_Impl->sName; }
Grade HighScore::GetGrade() const { return m_Impl->grade; }
int HighScore::GetScore() const { return m_Impl->iScore; }
float HighScore::GetPercentDP() const { return m_Impl->fPercentDP; }
float HighScore::GetSurviveSeconds() const { return m_Impl->fSurviveSeconds; }
float HighScore::GetSurvivalSeconds() const { return GetSurviveSeconds() + GetLifeRemainingSeconds(); }
RString HighScore::GetModifiers() const { return m_Impl->sModifiers; }
DateTime HighScore::GetDateTime() const { return m_Impl->dateTime; }
RString HighScore::GetPlayerGuid() const { return m_Impl->sPlayerGuid; }
RString HighScore::GetMachineGuid() const { return m_Impl->sMachineGuid; }
int HighScore::GetProductID() const { return m_Impl->iProductID; }
int HighScore::GetTapNoteScore( TapNoteScore tns ) const { return m_Impl->iTapNoteScores[tns]; }
int HighScore::GetHoldNoteScore( HoldNoteScore hns ) const { return m_Impl->iHoldNoteScores[hns]; }
const RadarValues &HighScore::GetRadarValues() const { return m_Impl->radarValues; }
float HighScore::GetLifeRemainingSeconds() const { return m_Impl->fLifeRemainingSeconds; }

void HighScore::SetName( const RString &sName ) { m_Impl->sName = sName; }
void HighScore::SetGrade( Grade g ) { m_Impl->grade = g; }
void HighScore::SetScore( int iScore ) { m_Impl->iScore = iScore; }
void HighScore::SetPercentDP( float f ) { m_Impl->fPercentDP = f; }
void HighScore::SetSurviveSeconds( float f ) { m_Impl->fSurviveSeconds = f; }
void HighScore::SetModifiers( RString s ) { m_Impl->sModifiers = s; }
void HighScore::SetDateTime( DateTime d ) { m_Impl->dateTime = d; }
void HighScore::SetPlayerGuid( RString s ) { m_Impl->sPlayerGuid = s; }
void HighScore::SetMachineGuid( RString s ) { m_Impl->sMachineGuid = s; }
void HighScore::SetProductID( int i ) { m_Impl->iProductID = i; }
void HighScore::SetTapNoteScore( TapNoteScore tns, int i ) { m_Impl->iTapNoteScores[tns] = i; }
void HighScore::SetHoldNoteScore( HoldNoteScore hns, int i ) { m_Impl->iHoldNoteScores[hns] = i; }
void HighScore::SetRadarValues( const RadarValues &rv ) { m_Impl->radarValues = rv; }
void HighScore::SetLifeRemainingSeconds( float f ) { m_Impl->fLifeRemainingSeconds = f; }

/* We normally don't give direct access to the members.  We need this one
 * for NameToFillIn; use a special accessor so it's easy to find where this
 * is used. */
RString *HighScore::GetNameMutable() { return &m_Impl->sName; }

bool HighScore::operator>=( const HighScore& other ) const
{
	/* Make sure we treat AAAA as higher than AAA, even though the score
 	 * is the same. */
	if( PREFSMAN->m_bPercentageScoring )
	{
		if( GetPercentDP() != other.GetPercentDP() )
			return GetPercentDP() >= other.GetPercentDP();
	}
	else
	{
		if( GetScore() != other.GetScore() )
			return GetScore() >= other.GetScore();
	}

	return GetGrade() >= other.GetGrade();
}

bool HighScore::operator==( const HighScore& other ) const 
{
	return *m_Impl == *other.m_Impl;
}

XNode* HighScore::CreateNode() const { return m_Impl->CreateNode(); }
void HighScore::LoadFromNode( const XNode* pNode ) { m_Impl->LoadFromNode( pNode ); }

void HighScore::Serialize( Json::Value &root ) const { m_Impl->Serialize(root); }
void HighScore::Deserialize( const Json::Value &root ) { m_Impl->Deserialize(root); }

RString HighScore::GetDisplayName() const
{
	if( GetName().empty() )
		return EMPTY_NAME;
	else
		return GetName();
}


void HighScoreList::Init()
{
	iNumTimesPlayed = 0;
	vHighScores.clear();
}

void HighScoreList::AddHighScore( HighScore hs, int &iIndexOut, bool bIsMachine )
{
	int i;
	for( i=0; i<(int)vHighScores.size(); i++ )
	{
		if( hs >= vHighScores[i] )
			break;
	}
	const int iMaxScores = bIsMachine ? 
		PREFSMAN->m_iMaxHighScoresPerListForMachine : 
		PREFSMAN->m_iMaxHighScoresPerListForPlayer;
	if( i < iMaxScores )
	{
		vHighScores.insert( vHighScores.begin()+i, hs );
		iIndexOut = i;

		// Delete extra machine high scores in RemoveAllButOneOfEachNameAndClampSize
		// and not here so that we don't end up with less than iMaxScores after 
		// removing HighScores with duplicate names.
		//
		if( !bIsMachine )
			ClampSize( bIsMachine );
	}
}

void HighScoreList::IncrementPlayCount( DateTime _dtLastPlayed )
{
	dtLastPlayed = _dtLastPlayed;
	iNumTimesPlayed++;
}

const HighScore& HighScoreList::GetTopScore() const
{
	if( vHighScores.empty() )
	{
		static HighScore hs;
		hs = HighScore();
		return hs;
	}
	else
	{
		return vHighScores[0];
	}
}

XNode* HighScoreList::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->m_sName = "HighScoreList";

	pNode->AppendChild( "NumTimesPlayed", iNumTimesPlayed );
	pNode->AppendChild( "LastPlayed", dtLastPlayed );

	for( unsigned i=0; i<vHighScores.size(); i++ )
	{
		const HighScore &hs = vHighScores[i];
		pNode->AppendChild( hs.CreateNode() );
	}

	return pNode;
}

void HighScoreList::LoadFromNode( const XNode* pHighScoreList )
{
	Init();

	ASSERT( pHighScoreList->m_sName == "HighScoreList" );
	FOREACH_CONST_Child( pHighScoreList, p )
	{
		if( p->m_sName == "NumTimesPlayed" )
		{
			p->GetValue( iNumTimesPlayed );
		}
		else if( p->m_sName == "LastPlayed" )
		{
			p->GetValue( dtLastPlayed );
		}
		else if( p->m_sName == "HighScore" )
		{
			vHighScores.resize( vHighScores.size()+1 );
			vHighScores.back().LoadFromNode( p );
			
			// ignore all high scores that are 0
			if( vHighScores.back().GetScore() == 0 )
				vHighScores.pop_back();
		}
	}
}

void HighScoreList::Serialize( Json::Value &root ) const
{
	root["NumTimesPlayed"] = iNumTimesPlayed;
	root["LastPlayed"] = dtLastPlayed.GetString();
	JsonUtil::SerializeArrayObjects( vHighScores, root["HighScores"] );
}

void HighScoreList::Deserialize( const Json::Value &root )
{
	Init();

	root["NumTimesPlayed"].TryGet( iNumTimesPlayed );
	dtLastPlayed.FromString( root["LastPlayed"].asString() );
	JsonUtil::DeserializeArrayObjects( vHighScores, root["HighScores"] );
}


void HighScoreList::RemoveAllButOneOfEachName()
{
	FOREACH( HighScore, vHighScores, i )
	{
		for( vector<HighScore>::iterator j = i+1; j != vHighScores.end(); j++ )
		{
			if( i->GetName() == j->GetName() )
			{
				j--;
				vHighScores.erase( j+1 );
			}
		}
	}
}

void HighScoreList::ClampSize( bool bIsMachine )
{
	const int iMaxScores = bIsMachine ? 
		PREFSMAN->m_iMaxHighScoresPerListForMachine : 
		PREFSMAN->m_iMaxHighScoresPerListForPlayer;
	if( vHighScores.size() > unsigned(iMaxScores) )
		vHighScores.erase( vHighScores.begin()+iMaxScores, vHighScores.end() );
}

XNode* Screenshot::CreateNode() const
{
	XNode* pNode = new XNode;
	pNode->m_sName = "Screenshot";

	// TRICKY:  Don't write "name to fill in" markers.
	pNode->AppendChild( "FileName",		sFileName );
	pNode->AppendChild( "MD5",			sMD5 );
	pNode->AppendChild( highScore.CreateNode() );

	return pNode;
}

void Screenshot::LoadFromNode( const XNode* pNode ) 
{
	ASSERT( pNode->m_sName == "Screenshot" );

	pNode->GetChildValue( "FileName",	sFileName );
	pNode->GetChildValue( "MD5",		sMD5 );
	const XNode* pHighScore = pNode->GetChild( "HighScore" );
	if( pHighScore )
		highScore.LoadFromNode( pHighScore );
}

/*
 * (c) 2004 Chris Danford
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
