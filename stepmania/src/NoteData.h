#ifndef NOTEDATA_H
#define NOTEDATA_H
/*
-----------------------------------------------------------------------------
 File: NoteData.h

 Desc: Holds data about the notes that the player is supposed to hit.  NoteData
	is organized by:
	track - corresponds to different columns of notes on the screen
	row/index - corresponds to subdivisions of beats

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "NoteTypes.h"
#include <map>
#include <set>
#include "Attack.h"


class NoteData
{
	/* Keep this aligned, so that they all have the same size. */
	vector<TapNote> m_TapNotes[MAX_NOTE_TRACKS];
	int m_iNumTracks;

	vector<HoldNote>	m_HoldNotes;

	map<TapNote,Attack>	m_AttackMap;

	/* Pad m_TapNotes so it includes the row "rows". */
	void PadTapNotes(int rows);

	void LoadOverlapped( const NoteData* pOriginal, int iNewNumTracks );

public:

	/* Set up to hold the data in From; same number of tracks, same
	 * divisor.  Doesn't allocate or copy anything. */
	void Config( const NoteData &From );

	NoteData();
	~NoteData();
	void Init();
	
	int GetNumTracks() const;
	void SetNumTracks( int iNewNumTracks );

	// TODO: Think of better accessors
	const map<TapNote,Attack>& GetAttackMap() const { return m_AttackMap; }
	map<TapNote,Attack>& GetAttackMap() { return m_AttackMap; }

	/* Return the note at the given track and row.  Row may be out of
	 * range; pretend the song goes on with TAP_EMPTYs indefinitely. */
	inline TapNote GetTapNote(unsigned track, int row) const
	{
		if(row < 0 || row >= (int) m_TapNotes[track].size()) return TapNote(TAP_EMPTY);
		return m_TapNotes[track][row];
	}
	void ReserveRows( int row );

	/* GetTapNote is called a lot.  This one doesn't do any bounds checking,
	 * which is much faster.  Be sure that 0 <= row < GetMaxRow(). */
	inline TapNote GetTapNoteX(unsigned track, int row) const
	{
		return m_TapNotes[track][row];
	}
	void MoveTapNoteTrack(int dest, int src);
	void SetTapNote(int track, int row, TapNote t);
	
	void ClearRange( int iNoteIndexBegin, int iNoteIndexEnd );
	void ClearAll();
	void CopyRange( const NoteData* pFrom, int iFromIndexBegin, int iFromIndexEnd, int iToIndexBegin = 0 );
	void CopyAll( const NoteData* pFrom );

	bool IsRowEmpty( int index ) const;
	bool IsRangeEmpty( int track, int iIndexBegin, int iIndexEnd ) const;
	int GetNumTapNonEmptyTracks( int index ) const;
	void GetTapNonEmptyTracks( int index, set<int>& addTo ) const;
	int GetFirstNonEmptyTrack( int index ) const;
	int GetNumTracksWithTap( int index ) const;
	int GetNumTracksWithTapOrHoldHead( int index ) const;
	int GetFirstTrackWithTap( int index ) const;
	int GetFirstTrackWithTapOrHoldHead( int index ) const;

	inline bool IsThereATapAtRow( int index ) const
	{
		return GetFirstTrackWithTap( index ) != -1;
	}
	inline bool IsThereATapOrHoldHeadAtRow( int index ) const
	{
		return GetFirstTrackWithTapOrHoldHead( index ) != -1;
	}
	void GetTracksHeldAtRow( int row, set<int>& addTo );
	int GetNumTracksHeldAtRow( int row );

	//
	// used in edit/record
	//
	void AddHoldNote( HoldNote newNote );	// add note hold note merging overlapping HoldNotes and destroying TapNotes underneath
	void RemoveHoldNote( int index );
	HoldNote &GetHoldNote( int index ) { ASSERT( index < (int) m_HoldNotes.size() ); return m_HoldNotes[index]; }
	const HoldNote &GetHoldNote( int index ) const { ASSERT( index < (int) m_HoldNotes.size() ); return m_HoldNotes[index]; }
	int GetMatchingHoldNote( const HoldNote &hn ) const;

	void SetTapAttackNote( int track, int row, Attack attack );
	void PruneUnusedAttacksFromMap();	// slow
	const Attack& GetAttackAt( int track, int row );
	// remove Attacks with SetTapNote(TAP_EMPTY)

	//
	// statistics
	//
	/* Return the highest beat/row that might contain notes.  (Use GetLastBeat if you need
	 * accuracy.) */
	float GetMaxBeat() const { return NoteRowToBeat(GetMaxRow()); }
	int GetMaxRow() const { return int(m_TapNotes[0].size()); }

	float GetFirstBeat() const;	// return the beat number of the first note
	int GetFirstRow() const;
	float GetLastBeat() const;	// return the beat number of the last note
	int GetLastRow() const;
	int GetNumTapNotes( const float fStartBeat = 0, const float fEndBeat = -1 ) const;
	int GetNumMines( const float fStartBeat = 0, const float fEndBeat = -1 ) const;
	int GetNumHands( const float fStartBeat = 0, const float fEndBeat = -1 ) const;
	int GetNumRowsWithTap( const float fStartBeat = 0, const float fEndBeat = -1 ) const;
	int GetNumRowsWithTapOrHoldHead( const float fStartBeat = 0, const float fEndBeat = -1 ) const;
	int GetNumN( int MinTaps, const float fStartBeat = 0, const float fEndBeat = -1 ) const;
	int GetNumDoubles( const float fStartBeat = 0, const float fEndBeat = -1 ) const { return GetNumN( 2, fStartBeat, fEndBeat ); }
	/* optimization: for the default of start to end, use the second (faster) */
	int GetNumHoldNotes( const float fStartBeat, const float fEndBeat = -1 ) const;
	int GetNumHoldNotes() const { return m_HoldNotes.size(); }
	int RowNeedsHands( int row ) const;

	// Transformations
	void LoadTransformed( const NoteData* pOriginal, int iNewNumTracks, const int iOriginalTrackToTakeFrom[] );	// -1 for iOriginalTracksToTakeFrom means no track
	void LoadTransformedSlidingWindow( const NoteData* pOriginal, int iNewNumTracks );	// used by autogen

	// Convert between HoldNote representation and '2' and '3' markers in TapNotes
	void Convert2sAnd3sToHoldNotes();
	void ConvertHoldNotesTo2sAnd3s();
	void To2sAnd3s( const NoteData &out );
	void From2sAnd3s( const NoteData &out );

	void Convert4sToHoldNotes();
	void ConvertHoldNotesTo4s();
	void To4s( const NoteData &out );
	void From4s( const NoteData &out );

	void EliminateAllButOneTap( int row ); 
};


#endif
