#include "stdafx.h"

#include "NotesLoaderDWI.h"
#include "NotesLoader.h"
#include "RageLog.h"
#include "RageException.h"
#include "MsdFile.h"
#include "RageUtil.h"
#include "NoteData.h"

#include <map>
using namespace std;

void DWILoader::DWIcharToNote( char c, GameController i, DanceNote &note1Out, DanceNote &note2Out )
{
	switch( c )
		{
		case '0':	note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
		case '1':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_LEFT;	break;
		case '2':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_NONE;			break;
		case '3':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case '4':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_NONE;			break;
		case '5':	note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
		case '6':	note1Out = DANCE_NOTE_PAD1_RIGHT;	note2Out = DANCE_NOTE_NONE;			break;
		case '7':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_LEFT;	break;
		case '8':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_NONE;			break;
		case '9':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'A':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_DOWN;	break;
		case 'B':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'C':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_NONE;			break;
		case 'D':	note1Out = DANCE_NOTE_PAD1_UPRIGHT;	note2Out = DANCE_NOTE_NONE;			break;
		case 'E':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_UPLEFT;	break;
		case 'F':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_DOWN;	break;
		case 'G':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_UP;		break;
		case 'H':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'I':	note1Out = DANCE_NOTE_PAD1_LEFT;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'J':	note1Out = DANCE_NOTE_PAD1_DOWN;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'K':	note1Out = DANCE_NOTE_PAD1_UP;		note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		case 'L':	note1Out = DANCE_NOTE_PAD1_UPRIGHT;	note2Out = DANCE_NOTE_PAD1_RIGHT;	break;
		case 'M':	note1Out = DANCE_NOTE_PAD1_UPLEFT;	note2Out = DANCE_NOTE_PAD1_UPRIGHT;	break;
		default:	
			LOG->Warn( "Encountered invalid DWI note characer '%c'", c );
			note1Out = DANCE_NOTE_NONE;			note2Out = DANCE_NOTE_NONE;			break;
	}

	switch( i )
	{
	case GAME_CONTROLLER_1:
		break;
	case GAME_CONTROLLER_2:
		if( note1Out != DANCE_NOTE_NONE )
			note1Out += 6;
		if( note2Out != DANCE_NOTE_NONE )
			note2Out += 6;
		break;
	default:
		ASSERT( false );
	}
}



bool DWILoader::LoadFromDWITokens( 
	CString sMode, 
	CString sDescription,
	CString sNumFeet,
	CString sStepData1, 
	CString sStepData2,
	Notes &out, Notes &out2)
{
	LOG->Trace( "Notes::LoadFromDWITokens()" );

	out.m_NotesType = NOTES_TYPE_INVALID;
	out2.m_NotesType = NOTES_TYPE_INVALID;

	sStepData1.Replace( "\n", "" );
	sStepData1.Replace( " ", "" );
	sStepData2.Replace( "\n", "" );
	sStepData2.Replace( " ", "" );

	if(		 sMode == "SINGLE" )	out.m_NotesType = NOTES_TYPE_DANCE_SINGLE;
	else if( sMode == "DOUBLE" )	out.m_NotesType = NOTES_TYPE_DANCE_DOUBLE;
	else if( sMode == "COUPLE" )	out.m_NotesType = NOTES_TYPE_DANCE_COUPLE_1;
	else if( sMode == "SOLO" )		out.m_NotesType = NOTES_TYPE_DANCE_SOLO;
	else	
	{
		ASSERT(0);	// Unrecognized DWI notes format
		out.m_NotesType = NOTES_TYPE_DANCE_SINGLE;
	}


	std::map<int,int> mapDanceNoteToNoteDataColumn;
	switch( out.m_NotesType )
	{
	case NOTES_TYPE_DANCE_SINGLE:
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
		break;
	case NOTES_TYPE_DANCE_DOUBLE:
	case NOTES_TYPE_DANCE_COUPLE_1:
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 1;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 2;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 3;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_LEFT] = 4;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_DOWN] = 5;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_UP] = 6;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD2_RIGHT] = 7;
		break;
	case NOTES_TYPE_DANCE_SOLO:
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_LEFT] = 0;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPLEFT] = 1;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_DOWN] = 2;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UP] = 3;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_UPRIGHT] = 4;
		mapDanceNoteToNoteDataColumn[DANCE_NOTE_PAD1_RIGHT] = 5;
		break;
	default:
		ASSERT(0);
	}

	out.m_sDescription = sDescription;

	out.m_iMeter = atoi( sNumFeet );

	//m_DifficultyClass = DifficultyClassFromDescriptionAndMeter( m_sDescription, m_iMeter );

	NoteData* pNoteData = new NoteData;
	ASSERT( pNoteData );
	pNoteData->m_iNumTracks = mapDanceNoteToNoteDataColumn.size();

	for( int pad=0; pad<2; pad++ )		// foreach pad
	{
		CString sStepData;
		switch( pad )
		{
		case 0:
			sStepData = sStepData1;
			break;
		case 1:
			if( sStepData2 == "" )	// no data
				continue;	// skip
			sStepData = sStepData2;
			break;
		default:
			ASSERT( false );
		}

		double fCurrentBeat = 0;
		double fCurrentIncrementer = 1.0/8 * BEATS_PER_MEASURE;

		for( int i=0; i<sStepData.GetLength(); )
		{
			char c = sStepData[i++];

			switch( c )
			{
			// begins a series
			case '(':
				fCurrentIncrementer = 1.0/16 * BEATS_PER_MEASURE;
				break;
			case '[':
				fCurrentIncrementer = 1.0/24 * BEATS_PER_MEASURE;
				break;
			case '{':
				fCurrentIncrementer = 1.0/64 * BEATS_PER_MEASURE;
				break;
			case '<':
				fCurrentIncrementer = 1.0/192 * BEATS_PER_MEASURE;
				break;

			// ends a series
			case ')':
			case ']':
			case '}':
			case '>':
				fCurrentIncrementer = 1.0/8 * BEATS_PER_MEASURE;
				break;

			case ' ':
				break;	// do nothing!
			
			case '!':		// hold start
				{
					// rewind and get the last step we inserted
					double fLastStepBeat = fCurrentBeat - fCurrentIncrementer;
					int iIndex = BeatToNoteRow( (float)fLastStepBeat );

					char holdChar = sStepData[i++];
					
					DanceNote note1, note2;
					DWIcharToNote( holdChar, (GameController)pad, note1, note2 );

					if( note1 != DANCE_NOTE_NONE )
					{
						int iCol1 = mapDanceNoteToNoteDataColumn[note1];
						pNoteData->m_TapNotes[iCol1][iIndex] = '2';
					}
					if( note2 != DANCE_NOTE_NONE )
					{
						int iCol2 = mapDanceNoteToNoteDataColumn[note2];
						pNoteData->m_TapNotes[iCol2][iIndex] = '2';
					}
				}
				break;
			default:	// this is a note character
				{
					int iIndex = BeatToNoteRow( (float)fCurrentBeat );

					DanceNote note1, note2;
					DWIcharToNote( c, (GameController)pad, note1, note2 );

					if( note1 != DANCE_NOTE_NONE )
					{
						int iCol1 = mapDanceNoteToNoteDataColumn[note1];
						pNoteData->m_TapNotes[iCol1][iIndex] = '1';
					}
					if( note2 != DANCE_NOTE_NONE )
					{
						int iCol2 = mapDanceNoteToNoteDataColumn[note2];
						pNoteData->m_TapNotes[iCol2][iIndex] = '1';
					}

					fCurrentBeat += fCurrentIncrementer;
				}
				break;
			}
		}
	}

	// this will expand the HoldNote begin markers we wrote into actual HoldNotes
	pNoteData->Convert2sAnd3sToHoldNotes();

	ASSERT( pNoteData->m_iNumTracks > 0 );

	if(out.m_NotesType == NOTES_TYPE_DANCE_COUPLE_1) {
		int iTransformNewToOld[MAX_NOTE_TRACKS];
		/* Couples.  Set up a second note pattern for the 2p side. */
		out2 = out;
		out2.m_NotesType = NOTES_TYPE_DANCE_COUPLE_2;

		for( int i = 0; i < MAX_NOTE_TRACKS; ++i)
			iTransformNewToOld[i] = -1;

		iTransformNewToOld[0] = 4;
		iTransformNewToOld[1] = 5;
		iTransformNewToOld[2] = 6;
		iTransformNewToOld[3] = 7;

		NoteData* pNoteData2 = new NoteData;
		pNoteData2->m_iNumTracks = 4;

		pNoteData2->LoadTransformed( pNoteData, 4, iTransformNewToOld );
		out2.SetNoteData(pNoteData2);

		delete pNoteData2;
		out2.TidyUpData();

		pNoteData->m_iNumTracks = 4;
	}
	out.SetNoteData(pNoteData);

	delete pNoteData;
	out.TidyUpData();

	return true;
}

bool DWILoader::LoadFromDWIFile( CString sPath, Song &out )
{
	LOG->Trace( "Song::LoadFromDWIFile(%s)", sPath );
	

	MsdFile msd;
	bool bResult = msd.ReadFile( sPath );
	if( !bResult )
		throw RageException( "Error opening file '%s'.", sPath );

	for( int i=0; i<msd.m_iNumValues; i++ )
	{
		int iNumParams = msd.m_iNumParams[i];
		CString* sParams = msd.m_sValuesAndParams[i];
		CString sValueName = sParams[0];

		// handle the data
		if( 0==stricmp(sValueName,"FILE") )
			out.m_sMusicFile = sParams[1];

		else if( 0==stricmp(sValueName,"TITLE") )
			out.GetMainAndSubTitlesFromFullTitle( sParams[1], out.m_sMainTitle, out.m_sSubTitle );

		else if( 0==stricmp(sValueName,"ARTIST") )
			out.m_sArtist = sParams[1];

		else if( 0==stricmp(sValueName,"CDTITLE") )
			out.m_sCDTitleFile = sParams[1];

		else if( 0==stricmp(sValueName,"BPM") )
			out.AddBPMSegment( BPMSegment(0, (float)atof(sParams[1])) );		

		else if( 0==stricmp(sValueName,"GAP") )
			// the units of GAP is 1/1000 second
			out.m_fBeat0OffsetInSeconds = -atoi( sParams[1] ) / 1000.0f;

		else if( 0==stricmp(sValueName,"SAMPLESTART") )
			out.m_fMusicSampleStartSeconds = TimeToSeconds( sParams[1] );

		else if( 0==stricmp(sValueName,"SAMPLELENGTH") )
			out.m_fMusicSampleLengthSeconds = TimeToSeconds(  sParams[1] );

		else if( 0==stricmp(sValueName,"FREEZE") )
		{
			CStringArray arrayFreezeExpressions;
			split( sParams[1], ",", arrayFreezeExpressions );

			for( int f=0; f<arrayFreezeExpressions.GetSize(); f++ )
			{
				CStringArray arrayFreezeValues;
				split( arrayFreezeExpressions[f], "=", arrayFreezeValues );
				float fIndex = atoi( arrayFreezeValues[0] ) * ROWS_PER_BEAT / 4.0f;
				float fFreezeBeat = NoteRowToBeat( fIndex );
				float fFreezeSeconds = (float)atof( arrayFreezeValues[1] ) / 1000.0f;
				
				out.AddStopSegment( StopSegment(fFreezeBeat, fFreezeSeconds) );
				LOG->Trace( "Adding a freeze segment: beat: %f, seconds = %f", fFreezeBeat, fFreezeSeconds );
			}
		}

		else if( 0==stricmp(sValueName,"CHANGEBPM")  || 0==stricmp(sValueName,"BPMCHANGE") )
		{
			CStringArray arrayBPMChangeExpressions;
			split( sParams[1], ",", arrayBPMChangeExpressions );

			for( int b=0; b<arrayBPMChangeExpressions.GetSize(); b++ )
			{
				CStringArray arrayBPMChangeValues;
				split( arrayBPMChangeExpressions[b], "=", arrayBPMChangeValues );
				float fIndex = atoi( arrayBPMChangeValues[0] ) * ROWS_PER_BEAT / 4.0f;
				float fBeat = NoteRowToBeat( fIndex );
				float fNewBPM = (float)atof( arrayBPMChangeValues[1] );
				
				out.AddBPMSegment( BPMSegment(fBeat, fNewBPM) );
			}
		}

		else if( 0==stricmp(sValueName,"SINGLE")  || 
			     0==stricmp(sValueName,"DOUBLE")  ||
				 0==stricmp(sValueName,"COUPLE")  || 
				 0==stricmp(sValueName,"SOLO"))
		{
			Notes* pNewNotes = new Notes;
			Notes* pNewNotes2 = new Notes;
			LoadFromDWITokens( 
				sParams[0], 
				sParams[1], 
				sParams[2], 
				sParams[3], 
				(iNumParams==5) ? sParams[4] : "",
				*pNewNotes,
				*pNewNotes2
				);
			/* Add either note pattern that actually loaded. */
			if(pNewNotes->m_NotesType != NOTES_TYPE_INVALID)
				out.m_apNotes.Add( pNewNotes );
			else
				delete pNewNotes;
			if(pNewNotes2->m_NotesType != NOTES_TYPE_INVALID)
				out.m_apNotes.Add( pNewNotes2 );
			else
				delete pNewNotes2;
		}
		else
			// do nothing.  We don't care about this value name
			;
	}

	return true;
}
