#include "global.h"

#include "NotesLoaderSM.h"
#include "GameManager.h"
#include "RageException.h"
#include "MsdFile.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "SongManager.h"
#include "RageFileManager.h"

#define MAX_EDIT_SIZE_BYTES  20*1024	// 20 KB

void SMLoader::LoadFromSMTokens( 
	CString sNotesType, 
	CString sDescription,
	CString sDifficulty,
	CString sMeter,
	CString sRadarValues,
	CString sNoteData,
	CString sAttackData,
	Steps &out
)
{
	TrimLeft(sNotesType); 
	TrimRight(sNotesType); 
	TrimLeft(sDescription); 
	TrimRight(sDescription); 
	TrimLeft(sDifficulty); 
	TrimRight(sDifficulty); 


//	LOG->Trace( "Steps::LoadFromSMTokens()" );

	out.m_StepsType = GameManager::StringToNotesType(sNotesType);
	out.SetDescription(sDescription);
	out.SetDifficulty(StringToDifficulty( sDifficulty ));

	// HACK:  We used to store SMANIAC as DIFFICULTY_HARD with special description.
	// Now, it has its own DIFFICULTY_CHALLENGE
	if( sDescription.CompareNoCase("smaniac") == 0 ) 
		out.SetDifficulty( DIFFICULTY_CHALLENGE );
	// HACK:  We used to store CHALLENGE as DIFFICULTY_HARD with special description.
	// Now, it has its own DIFFICULTY_CHALLENGE
	if( sDescription.CompareNoCase("challenge") == 0 ) 
		out.SetDifficulty( DIFFICULTY_CHALLENGE );

	out.SetMeter(atoi(sMeter));
	CStringArray saValues;
	split( sRadarValues, ",", saValues, true );
	if( saValues.size() == NUM_RADAR_CATEGORIES )
		for( int r=0; r<NUM_RADAR_CATEGORIES; r++ )
			out.SetRadarValue(r, (float)atof(saValues[r]));
    
	out.SetSMNoteData(sNoteData, sAttackData);

	out.TidyUpData();
}

void SMLoader::GetApplicableFiles( CString sPath, CStringArray &out )
{
	GetDirListing( sPath + CString("*.sm"), out );
}

bool SMLoader::LoadTimingFromFile( const CString &fn, TimingData &out )
{
	MsdFile msd;
	if( !msd.ReadFile( fn ) )
	{
		LOG->Warn( "Couldn't load %s, \"%s\"", fn.c_str(), msd.GetError().c_str() );
		return false;
	}

	out.m_sFile = fn;
	LoadTimingFromSMFile( msd, out );
	return true;
}

void SMLoader::LoadTimingFromSMFile( const MsdFile &msd, TimingData &out )
{
	out.m_fBeat0OffsetInSeconds = 0;
	out.m_BPMSegments.clear();
	out.m_StopSegments.clear();

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		const MsdFile::value_t &sParams = msd.GetValue(i);
		const CString sValueName = sParams[0];

		if( 0==stricmp(sValueName,"OFFSET") )
			out.m_fBeat0OffsetInSeconds = (float)atof( sParams[1] );
		else if( 0==stricmp(sValueName,"STOPS") || 0==stricmp(sValueName,"FREEZES") )
		{
			CStringArray arrayFreezeExpressions;
			split( sParams[1], ",", arrayFreezeExpressions );

			for( unsigned f=0; f<arrayFreezeExpressions.size(); f++ )
			{
				CStringArray arrayFreezeValues;
				split( arrayFreezeExpressions[f], "=", arrayFreezeValues );
				/* XXX: Once we have a way to display warnings that the user actually
				 * cares about (unlike most warnings), this should be one of them. */
				if( arrayFreezeValues.size() != 2 )
				{
					LOG->Warn( "Invalid #%s value \"%s\" (must have exactly one '='), ignored",
						sValueName.c_str(), arrayFreezeExpressions[f].c_str() );
					continue;
				}

				const float fFreezeBeat = (float)atof( arrayFreezeValues[0] );
				const float fFreezeSeconds = (float)atof( arrayFreezeValues[1] );
				
				StopSegment new_seg;
				new_seg.m_fStartBeat = fFreezeBeat;
				new_seg.m_fStopSeconds = fFreezeSeconds;

//				LOG->Trace( "Adding a freeze segment: beat: %f, seconds = %f", new_seg.m_fStartBeat, new_seg.m_fStopSeconds );

				out.AddStopSegment( new_seg );
			}
		}

		else if( 0==stricmp(sValueName,"BPMS") )
		{
			CStringArray arrayBPMChangeExpressions;
			split( sParams[1], ",", arrayBPMChangeExpressions );

			for( unsigned b=0; b<arrayBPMChangeExpressions.size(); b++ )
			{
				CStringArray arrayBPMChangeValues;
				split( arrayBPMChangeExpressions[b], "=", arrayBPMChangeValues );
				/* XXX: Once we have a way to display warnings that the user actually
				 * cares about (unlike most warnings), this should be one of them. */
				if(arrayBPMChangeValues.size() != 2)
				{
					LOG->Warn( "Invalid #%s value \"%s\" (must have exactly one '='), ignored",
						sValueName.c_str(), arrayBPMChangeExpressions[b].c_str() );
					continue;
				}

				const float fBeat = (float)atof( arrayBPMChangeValues[0] );
				const float fNewBPM = (float)atof( arrayBPMChangeValues[1] );
				
				BPMSegment new_seg;
				new_seg.m_fStartBeat = fBeat;
				new_seg.m_fBPM = fNewBPM;
				
				out.AddBPMSegment( new_seg );
			}
		}
	}
}

bool LoadFromBGChangesString( BackgroundChange &change, const CString &sBGChangeExpression )
{
	CStringArray aBGChangeValues;
	split( sBGChangeExpression, "=", aBGChangeValues );

	switch( aBGChangeValues.size() )
	{
	case 6:
		change.m_fRate = (float)atof( aBGChangeValues[2] );
		change.m_bFadeLast = atoi( aBGChangeValues[3] ) != 0;
		change.m_bRewindMovie = atoi( aBGChangeValues[4] ) != 0;
		change.m_bLoop = atoi( aBGChangeValues[5] ) != 0;
		// fall through
	case 2:
		change.m_fStartBeat = (float)atof( aBGChangeValues[0] );
		change.m_sBGName = aBGChangeValues[1];
		return true;
	default:
		LOG->Warn("Invalid #BGCHANGES value \"%s\" was ignored", sBGChangeExpression.c_str());
		return false;
	}
}

bool SMLoader::LoadFromSMFile( CString sPath, Song &out )
{
	LOG->Trace( "Song::LoadFromSMFile(%s)", sPath.c_str() );

	MsdFile msd;
	bool bResult = msd.ReadFile( sPath );
	if( !bResult )
		RageException::Throw( "Error opening file '%s'.", sPath.c_str() );

	out.m_Timing.m_sFile = sPath;
	LoadTimingFromSMFile( msd, out.m_Timing );

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t &sParams = msd.GetValue(i);
		const CString sValueName = sParams[0];

		// handle the data
		/* Don't use GetMainAndSubTitlesFromFullTitle; that's only for heuristically
		 * splitting other formats that *don't* natively support #SUBTITLE. */
		if( 0==stricmp(sValueName,"TITLE") )
			out.m_sMainTitle = sParams[1];

		else if( 0==stricmp(sValueName,"SUBTITLE") )
			out.m_sSubTitle = sParams[1];

		else if( 0==stricmp(sValueName,"ARTIST") )
			out.m_sArtist = sParams[1];

		else if( 0==stricmp(sValueName,"TITLETRANSLIT") )
			out.m_sMainTitleTranslit = sParams[1];

		else if( 0==stricmp(sValueName,"SUBTITLETRANSLIT") )
			out.m_sSubTitleTranslit = sParams[1];

		else if( 0==stricmp(sValueName,"ARTISTTRANSLIT") )
			out.m_sArtistTranslit = sParams[1];

		else if( 0==stricmp(sValueName,"CREDIT") )
			out.m_sCredit = sParams[1];

		else if( 0==stricmp(sValueName,"BANNER") )
			out.m_sBannerFile = sParams[1];

		else if( 0==stricmp(sValueName,"BACKGROUND") )
			out.m_sBackgroundFile = sParams[1];

		/* Save "#LYRICS" for later, so we can add an internal lyrics tag. */
		else if( 0==stricmp(sValueName,"LYRICSPATH") )
			out.m_sLyricsFile = sParams[1];

		else if( 0==stricmp(sValueName,"CDTITLE") )
			out.m_sCDTitleFile = sParams[1];

		else if( 0==stricmp(sValueName,"MUSIC") )
			out.m_sMusicFile = sParams[1];

		else if( 0==stricmp(sValueName,"MUSICLENGTH") )
		{
			if(!FromCache)
				continue;
			out.m_fMusicLengthSeconds = (float)atof( sParams[1] );
		}

		else if( 0==stricmp(sValueName,"MUSICBYTES") )
			; /* ignore */

		/* We calculate these.  Some SMs in circulation have bogus values for
		 * these, so make sure we always calculate it ourself. */
		else if( 0==stricmp(sValueName,"FIRSTBEAT") )
		{
			if(!FromCache)
				continue;
			out.m_fFirstBeat = (float)atof( sParams[1] );
		}

		else if( 0==stricmp(sValueName,"LASTBEAT") )
		{
			if(!FromCache)
				LOG->Trace("Ignored #LASTBEAT (cache only)");
			out.m_fLastBeat = (float)atof( sParams[1] );
		}
		else if( 0==stricmp(sValueName,"SONGFILENAME") )
		{
			if( FromCache )
				out.m_sSongFileName = sParams[1];
		}
		else if( 0==stricmp(sValueName,"HASMUSIC") )
		{
			if( FromCache )
				out.m_bHasMusic = atoi( sParams[1] ) != 0;
		}
		else if( 0==stricmp(sValueName,"HASBANNER") )
		{
			if( FromCache )
				out.m_bHasBanner = atoi( sParams[1] ) != 0;
		}

		else if( 0==stricmp(sValueName,"SAMPLESTART") )
			out.m_fMusicSampleStartSeconds = HHMMSSToSeconds( sParams[1] );

		else if( 0==stricmp(sValueName,"SAMPLELENGTH") )
			out.m_fMusicSampleLengthSeconds = HHMMSSToSeconds( sParams[1] );

		else if( 0==stricmp(sValueName,"DISPLAYBPM") )
		{
			// #DISPLAYBPM:[xxx][xxx:xxx]|[*]; 
			if( sParams[1] == "*" )
				out.m_DisplayBPMType = Song::DISPLAY_RANDOM;
			else 
			{
				out.m_DisplayBPMType = Song::DISPLAY_SPECIFIED;
				out.m_fSpecifiedBPMMin = (float)atof( sParams[1] );
				if( sParams[2].empty() )
					out.m_fSpecifiedBPMMax = out.m_fSpecifiedBPMMin;
				else
					out.m_fSpecifiedBPMMax = (float)atof( sParams[2] );
			}
		}

		else if( 0==stricmp(sValueName,"SELECTABLE") )
		{
			if(!stricmp(sParams[1],"YES"))
				out.m_SelectionDisplay = out.SHOW_ALWAYS;
			else if(!stricmp(sParams[1],"NO"))
				out.m_SelectionDisplay = out.SHOW_NEVER;
			else if(!stricmp(sParams[1],"ROULETTE"))
				out.m_SelectionDisplay = out.SHOW_ROULETTE;
			else
				LOG->Warn( "The song file '%s' has an unknown #SELECTABLE value, '%s'; ignored.", sPath.c_str(), sParams[1].c_str());
		}

		else if( 0==stricmp(sValueName,"BGCHANGES") || 0==stricmp(sValueName,"ANIMATIONS") )
		{
			CStringArray aBGChangeExpressions;
			split( sParams[1], ",", aBGChangeExpressions );

			for( unsigned b=0; b<aBGChangeExpressions.size(); b++ )
			{
				BackgroundChange change;
				if( LoadFromBGChangesString( change, aBGChangeExpressions[b] ) )
					out.AddBackgroundChange( change );
			}
		}

		else if( 0==stricmp(sValueName,"FGCHANGES") )
		{
			CStringArray aFGChangeExpressions;
			split( sParams[1], ",", aFGChangeExpressions );

			for( unsigned b=0; b<aFGChangeExpressions.size(); b++ )
			{
				BackgroundChange change;
				if( LoadFromBGChangesString( change, aFGChangeExpressions[b] ) )
					out.AddForegroundChange( change );
			}
		}

		else if( 0==stricmp(sValueName,"NOTES") )
		{
			Steps* pNewNotes = new Steps;
			ASSERT( pNewNotes );
			out.m_apNotes.push_back( pNewNotes );

			if( iNumParams < 7 )
			{
				LOG->Trace( "The song file '%s' is has %d fields in a #NOTES tag, but should have at least %d.", sPath.c_str(), iNumParams, 7 );
				continue;
			}

			LoadFromSMTokens( 
				sParams[1], sParams[2], sParams[3], sParams[4], sParams[5], sParams[6], (iNumParams>=8)?sParams[7]:"",
				*pNewNotes);
		}
		else if( 0==stricmp(sValueName,"OFFSET") || 0==stricmp(sValueName,"BPMS") ||
				 0==stricmp(sValueName,"STOPS") || 0==stricmp(sValueName,"FREEZES") )
				 ;
		else
			LOG->Trace( "Unexpected value named '%s'", sValueName.c_str() );
	}

	return true;
}


bool SMLoader::LoadFromDir( CString sPath, Song &out )
{
	CStringArray aFileNames;
	GetApplicableFiles( sPath, aFileNames );

	if( aFileNames.size() > 1 )
		RageException::Throw( "There is more than one SM file in '%s'.  There should be only one!", sPath.c_str() );

	/* We should have exactly one; if we had none, we shouldn't have been
	 * called to begin with. */
	ASSERT( aFileNames.size() == 1 );

	return LoadFromSMFile( sPath + aFileNames[0], out );
}

bool SMLoader::LoadEdit( CString sEditFilePath, ProfileSlot slot )
{
	LOG->Trace( "Song::LoadEdit(%s)", sEditFilePath.c_str() );

	int iBytes = FILEMAN->GetFileSizeInBytes( sEditFilePath );
	if( iBytes > MAX_EDIT_SIZE_BYTES )
	{
		LOG->Warn( "The edit '%s' is unreasonably large.  It won't be loaded.", sEditFilePath.c_str() );
		return false;
	}

	MsdFile msd;
	bool bResult = msd.ReadFile( sEditFilePath );
	if( !bResult )
		RageException::Throw( "Error opening file '%s'.", sEditFilePath.c_str() );

	Song* pSong = NULL;

	for( unsigned i=0; i<msd.GetNumValues(); i++ )
	{
		int iNumParams = msd.GetNumParams(i);
		const MsdFile::value_t &sParams = msd.GetValue(i);
		const CString sValueName = sParams[0];

		// handle the data
		if( 0==stricmp(sValueName,"SONG") )
		{
			if( pSong )
			{
				LOG->Warn( "The edit file '%s' has more than one #SONG tag.", sEditFilePath.c_str() );
				return false;
			}

			CString sSongFullTitle = sParams[1];
			sSongFullTitle.Replace( '\\', '/' );

			pSong = SONGMAN->FindSong( sSongFullTitle );
			if( pSong == NULL )
			{
				LOG->Warn( "The edit file '%s' required a song '%s' that isn't present.", sEditFilePath.c_str(), sSongFullTitle.c_str() );
				return false;
			}

			if( pSong->GetNumStepsLoadedFromProfile(slot) >= MAX_EDITS_PER_SONG_PER_PROFILE )
			{
				LOG->Warn( "The song '%s' already has the maximum number of edits allowed for ProfileSlotP%d.", sSongFullTitle.c_str(), slot+1 );
				return false;
			}
		}

		else if( 0==stricmp(sValueName,"NOTES") )
		{
			if( pSong == NULL )
			{
				LOG->Warn( "The edit file '%s' has doesn't have a #SONG tag preceeding the first #NOTES tag.", sEditFilePath.c_str() );
				return false;
			}

			Steps* pNewNotes = new Steps;
			ASSERT( pNewNotes );
			pSong->m_apNotes.push_back( pNewNotes );

			if( iNumParams < 7 )
			{
				LOG->Trace( "The song file '%s' is has %d fields in a #NOTES tag, but should have at least %d.", sEditFilePath.c_str(), iNumParams, 7 );
				continue;
			}

			LoadFromSMTokens( 
				sParams[1], sParams[2], sParams[3], sParams[4], sParams[5], sParams[6], (iNumParams>=8)?sParams[7]:"",
				*pNewNotes);

			pNewNotes->SetLoadedFromProfile( slot );
			pNewNotes->SetDifficulty( DIFFICULTY_EDIT );

			return true;	// Only allow one Steps per edit file!
		}
		else
			LOG->Trace( "Unexpected value named '%s'", sValueName.c_str() );
	}

	return true;
	
}
