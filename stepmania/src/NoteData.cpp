/*
 * NoteData is organized by:
 *  track - corresponds to different columns of notes on the screen
 *  row/index - corresponds to subdivisions of beats
 */

#include "global.h"
#include "NoteData.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "XmlFile.h"
#include "Foreach.h"
#include "RageUtil_AutoPtr.h"

REGISTER_CLASS_TRAITS( NoteData, new NoteData(*pCopy) )

void NoteData::Init()
{
	m_TapNotes = vector<TrackMap>();	// ensure that the memory is freed
}

void NoteData::SetNumTracks( int iNewNumTracks )
{
	ASSERT( iNewNumTracks > 0 );

	m_TapNotes.resize( iNewNumTracks );
}

bool NoteData::IsComposite() const
{
	for( int track = 0; track < GetNumTracks(); ++track )
	{
		FOREACHM_CONST( int, TapNote, m_TapNotes[track], tn )
			if( tn->second.pn != PLAYER_INVALID )
				return true;
	}
	return false;
}

/* Clear [rowBegin,rowEnd). */
void NoteData::ClearRangeForTrack( int rowBegin, int rowEnd, int iTrack )
{
	/* Optimization: if the range encloses everything, just clear the whole maps. */
	if( rowBegin == 0 && rowEnd == MAX_NOTE_ROW )
	{
		m_TapNotes[iTrack].clear();
		return;
	}

	/* If the range is empty, don't do anything.  Otherwise, an empty range will cause
	 * hold notes to be split when they shouldn't be. */
	if( rowBegin == rowEnd )
		return;

	iterator begin, end;
	GetTapNoteRangeInclusive( iTrack, rowBegin, rowEnd, begin, end );

	if( begin != end && begin->first < rowBegin && begin->first + begin->second.iDuration > rowEnd )
	{
		/* A hold note overlaps the whole range.  Truncate it, and add the remainder to
		 * the end. */
		TapNote tn1 = begin->second;
		TapNote tn2 = tn1;

		int iEndRow = begin->first + tn1.iDuration;
		int iRow = begin->first;

		tn1.iDuration = rowBegin - iRow;
		tn2.iDuration = iEndRow - rowEnd;

		SetTapNote( iTrack, iRow, tn1 );
		SetTapNote( iTrack, rowEnd, tn2 );

		/* We may have invalidated our iterators. */
		GetTapNoteRangeInclusive( iTrack, rowBegin, rowEnd, begin, end );
	}
	else if( begin != end && begin->first < rowBegin )
	{
		/* A hold note overlaps the beginning of the range.  Truncate it. */
		TapNote &tn1 = begin->second;
		int iRow = begin->first;
		tn1.iDuration = rowBegin - iRow;

		++begin;
	}

	if( begin != end )
	{
		iterator prev = end;
		--prev;
		TapNote tn = begin->second;
		int iRow = prev->first;
		if( tn.type == TapNote::hold_head && iRow + tn.iDuration > rowEnd )
		{
			/* A hold note overlaps the end of the range.  Separate it. */
			SetTapNote( iTrack, iRow, TAP_EMPTY );

			int iAdd = rowEnd - iRow;
			tn.iDuration -= iAdd;
			iRow += iAdd;
			SetTapNote( iTrack, iRow, tn );
			end = prev;
		}

		/* We may have invalidated our iterators. */
		GetTapNoteRangeInclusive( iTrack, rowBegin, rowEnd, begin, end );
	}

	m_TapNotes[iTrack].erase( begin, end );
}

void NoteData::ClearRange( int rowBegin, int rowEnd )
{
	for( int t=0; t < GetNumTracks(); ++t )
		ClearRangeForTrack( rowBegin, rowEnd, t );
}

void NoteData::ClearAll()
{
	for( int t=0; t<GetNumTracks(); t++ )
		m_TapNotes[t].clear();
}

/* Copy [rowFromBegin,rowFromEnd) from pFrom to this.  (Note that this does *not* overlay;
 * all data in the range is overwritten.) */
void NoteData::CopyRange( const NoteData& from, int rowFromBegin, int rowFromEnd, int rowToBegin )
{
	ASSERT( from.GetNumTracks() == GetNumTracks() );

	if( rowFromBegin > rowFromEnd )
		return; /* empty range */

	const int rowToEnd = (rowFromEnd-rowFromBegin) + rowToBegin;
	const int iMoveBy = rowToBegin-rowFromBegin;

	/* Clear the region. */
	ClearRange( rowToBegin, rowToEnd );

	for( int t=0; t<GetNumTracks(); t++ )
	{
		const_iterator begin, end;
		from.GetTapNoteRangeInclusive( t, rowFromBegin, rowFromEnd, begin, end );
		for( ; begin != end; ++begin )
		{
			TapNote head = begin->second;
			if( head.type == TapNote::empty )
				continue;

			if( head.type == TapNote::hold_head )
			{
				int iStartRow = begin->first + iMoveBy;
				int iEndRow = iStartRow + head.iDuration;

				iStartRow = clamp( iStartRow, rowToBegin, rowToEnd );
				iEndRow = clamp( iEndRow, rowToBegin, rowToEnd );

				this->AddHoldNote( t, iStartRow, iEndRow, head );
			}
			else
			{
				int iTo = begin->first + iMoveBy;
				if( iTo >= rowToBegin && iTo <= rowToEnd )
					this->SetTapNote( t, iTo, head );
			}
		}
	}
}

void NoteData::CopyAll( const NoteData& from )
{
	*this = from;
}

bool NoteData::IsRowEmpty( int row ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
		if( GetTapNote(t, row).type != TapNote::empty )
			return false;
	return true;
}

bool NoteData::IsRangeEmpty( int track, int rowBegin, int rowEnd ) const
{
	ASSERT( track < GetNumTracks() );

	FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, track, r, rowBegin, rowEnd )
		if( GetTapNote(track,r).type != TapNote::empty )
			return false;
	return true;
}

int NoteData::GetNumTapNonEmptyTracks( int row ) const
{
	int iNum = 0;
	for( int t=0; t<GetNumTracks(); t++ )
		if( GetTapNote(t, row).type != TapNote::empty )
			iNum++;
	return iNum;
}

void NoteData::GetTapNonEmptyTracks( int row, set<int>& addTo ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
		if( GetTapNote(t, row).type != TapNote::empty )
			addTo.insert(t);
}

bool NoteData::GetTapFirstNonEmptyTrack( int row, int &iNonEmptyTrackOut ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		if( GetTapNote( t, row ).type != TapNote::empty )
		{
			iNonEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

bool NoteData::GetTapFirstEmptyTrack( int row, int &iEmptyTrackOut ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		if( GetTapNote( t, row ).type == TapNote::empty )
		{
			iEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

bool NoteData::GetTapLastEmptyTrack( int row, int &iEmptyTrackOut ) const
{
	for( int t=GetNumTracks()-1; t>=0; t-- )
	{
		if( GetTapNote( t, row ).type == TapNote::empty )
		{
			iEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

int NoteData::GetNumTracksWithTap( int row ) const
{
	int iNum = 0;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNote( t, row );
		if( tn.type == TapNote::tap || tn.type == TapNote::lift )
			iNum++;
	}
	return iNum;
}

int NoteData::GetNumTracksWithTapOrHoldHead( int row ) const
{
	int iNum = 0;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNote( t, row );
		if( tn.type == TapNote::tap || tn.type == TapNote::lift || tn.type == TapNote::hold_head )
			iNum++;
	}
	return iNum;
}

int NoteData::GetFirstTrackWithTap( int row ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNote( t, row );
		if( tn.type == TapNote::tap || tn.type == TapNote::lift )
			return t;
	}
	return -1;
}

int NoteData::GetFirstTrackWithTapOrHoldHead( int row ) const
{
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNote( t, row );
		if( tn.type == TapNote::tap || tn.type == TapNote::lift || tn.type == TapNote::hold_head )
			return t;
	}
	return -1;
}

int NoteData::GetLastTrackWithTapOrHoldHead( int row ) const
{
	for( int t=GetNumTracks()-1; t>=0; t-- )
	{
		const TapNote &tn = GetTapNote( t, row );
		if( tn.type == TapNote::tap || tn.type == TapNote::lift || tn.type == TapNote::hold_head )
			return t;
	}
	return -1;
}

void NoteData::AddHoldNote( int iTrack, int iStartRow, int iEndRow, TapNote tn )
{
	ASSERT( iStartRow>=0 && iEndRow>=0 );
	ASSERT_M( iEndRow >= iStartRow, ssprintf("%d %d",iEndRow,iStartRow) );
	
	/* Include adjacent (non-overlapping) hold notes, since we need to merge with them. */
	iterator begin, end;
	GetTapNoteRangeInclusive( iTrack, iStartRow, iEndRow, begin, end, true );

	/* Look for other hold notes that overlap and merge them into add. */
	for( iterator it = begin; it != end; ++it )
	{
		int iOtherRow = it->first;
		const TapNote &tnOther = it->second;
		if( tnOther.type == TapNote::hold_head )
		{
			iStartRow = min( iStartRow, iOtherRow );
			iEndRow = max( iEndRow, iOtherRow + tnOther.iDuration );
		}
	}

	tn.iDuration = iEndRow - iStartRow;

	/* Remove everything in the range. */
	while( begin != end )
	{
		iterator next = begin;
		++next;

		RemoveTapNote( iTrack, begin );
		
		begin = next;
	}

	/* Additionally, if there's a tap note lying at the end of our range, remove it,
	 * too. */
	SetTapNote( iTrack, iEndRow, TAP_EMPTY );

	// add a tap note at the start of this hold
	SetTapNote( iTrack, iStartRow, tn );
}

/* Determine if a hold note lies on the given spot.  Return true if so.  If
 * pHeadRow is non-NULL, return the row of the head. */
bool NoteData::IsHoldHeadOrBodyAtRow( int iTrack, int iRow, int *pHeadRow ) const
{
	const TapNote &tn = GetTapNote( iTrack, iRow );
	if( tn.type == TapNote::hold_head )
	{
		if( pHeadRow != NULL )
			*pHeadRow = iRow;
		return true;
	}

	return IsHoldNoteAtRow( iTrack, iRow, pHeadRow );
}

/* Determine if a hold note lies on the given spot.  Return true if so.  If
 * pHeadRow is non-NULL, return the row of the head.  (Note that this returns
 * false if a hold head lies on iRow itself.) */
/* XXX: rename this to IsHoldBodyAtRow */
bool NoteData::IsHoldNoteAtRow( int iTrack, int iRow, int *pHeadRow ) const
{
	int iDummy;
	if( pHeadRow == NULL )
		pHeadRow = &iDummy;

	/* Starting at iRow, search upwards.  If we find a TapNote::hold_head, we're within
	 * a hold.  If we find a tap, mine or attack, we're not--those never lie within hold
	 * notes.  Ignore autoKeysound. */
	FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE_REVERSE( *this, iTrack, r, 0, iRow )
	{
		const TapNote &tn = GetTapNote( iTrack, r );
		switch( tn.type )
		{
		case TapNote::hold_head:
			if( tn.iDuration + r < iRow )
				return false;
			*pHeadRow = r;
			return true;

		case TapNote::tap:
		case TapNote::mine:
		case TapNote::attack:
		case TapNote::lift:
		case TapNote::fake:
			return false;

		case TapNote::empty:
		case TapNote::autoKeysound:
			/* ignore */
			continue;
		DEFAULT_FAIL( tn.type );
		}
	}

	return false;
}

bool NoteData::IsEmpty() const
{ 
	for( int t=0; t < GetNumTracks(); t++ )
	{
		int iRow = -1;
		if( !GetNextTapNoteRowForTrack( t, iRow ) )
			continue;

		return false;
	}

	return true;
}

int NoteData::GetFirstRow() const
{ 
	int iEarliestRowFoundSoFar = -1;
	
	for( int t=0; t < GetNumTracks(); t++ )
	{
		int iRow = -1;
		if( !GetNextTapNoteRowForTrack( t, iRow ) )
			continue;

		if( iEarliestRowFoundSoFar == -1 )
			iEarliestRowFoundSoFar = iRow;
		else
			iEarliestRowFoundSoFar = min( iEarliestRowFoundSoFar, iRow );
	}

	if( iEarliestRowFoundSoFar == -1 )	// there are no notes
		return 0;

	return iEarliestRowFoundSoFar;
}

int NoteData::GetLastRow() const
{ 
	int iOldestRowFoundSoFar = 0;
	
	for( int t=0; t < GetNumTracks(); t++ )
	{
		int iRow = MAX_NOTE_ROW;
		if( !GetPrevTapNoteRowForTrack( t, iRow ) )
			continue;

		/* XXX: We might have a hold note near the end with autoplay sounds
		 * after it.  Do something else with autoplay sounds ... */
		const TapNote &tn = GetTapNote( t, iRow );
		if( tn.type == TapNote::hold_head )
			iRow += tn.iDuration;

		iOldestRowFoundSoFar = max( iOldestRowFoundSoFar, iRow );
	}

	return iOldestRowFoundSoFar;
}

int NoteData::GetNumTapNotes( int iStartIndex, int iEndIndex ) const
{
	int iNumNotes = 0;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, t, r, iStartIndex, iEndIndex )
		{
			const TapNote &tn = GetTapNote(t, r);
			if( tn.type != TapNote::empty  &&  tn.type != TapNote::mine  &&  tn.type != TapNote::fake )
				iNumNotes++;
		}
	}
	
	return iNumNotes;
}

int NoteData::GetNumTapNotesInRow( int iRow ) const
{
	// Optimization opportunity: There's no need to instantiate row iterators if we're only checking one row.
	return GetNumTapNotes( iRow, iRow+1 );
}

int NoteData::GetNumRowsWithTap( int iStartIndex, int iEndIndex ) const
{
	int iNumNotes = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
		if( IsThereATapAtRow(r) )
			iNumNotes++;
	
	return iNumNotes;
}

int NoteData::GetNumMines( int iStartIndex, int iEndIndex ) const
{
	int iNumMines = 0;

	for( int t=0; t<GetNumTracks(); t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( *this, t, r, iStartIndex, iEndIndex )
			if( GetTapNote(t, r).type == TapNote::mine )
				iNumMines++;
	}
	
	return iNumMines;
}

int NoteData::GetNumRowsWithTapOrHoldHead( int iStartIndex, int iEndIndex ) const
{
	int iNumNotes = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
		if( IsThereATapOrHoldHeadAtRow(r) )
			iNumNotes++;
	
	return iNumNotes;
}

bool NoteData::RowNeedsAtLeastSimultaneousPresses( int iMinSimultaneousPresses, const int row ) const
{
	int iNumNotesThisIndex = 0;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const TapNote &tn = GetTapNote(t, row);
		switch( tn.type )
		{
		case TapNote::mine:
		case TapNote::empty:
		case TapNote::fake:
			continue;	// skip these types - they don't count
		}
		++iNumNotesThisIndex;
	}

	/* We must have at least one tap or hold head at this row to count it. */
	if( !iNumNotesThisIndex )
		return false;

	if( iNumNotesThisIndex < iMinSimultaneousPresses )
	{
		/* We have at least one, but not enough.  Count holds.  Do count adjacent holds. */
		for( int t=0; t<GetNumTracks(); ++t )
		{
			if( IsHoldNoteAtRow(t, row) )
				++iNumNotesThisIndex;
		}
	}

	return iNumNotesThisIndex >= iMinSimultaneousPresses;
}

int NoteData::GetNumRowsWithSimultaneousPresses( int iMinSimultaneousPresses, int iStartIndex, int iEndIndex ) const
{
	/* Count the number of times you have to use your hands.  This includes
	 * three taps at the same time, a tap while two hold notes are being held,
	 * etc.  Only count rows that have at least one tap note (hold heads count).
	 * Otherwise, every row of hold notes counts, so three simultaneous hold
	 * notes will count as hundreds of "hands". */
	int iNum = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
	{
		if( !RowNeedsAtLeastSimultaneousPresses(iMinSimultaneousPresses,r) )
			continue;

		iNum++;
	}

	return iNum;
}

int NoteData::GetNumRowsWithSimultaneousTaps( int iMinTaps, int iStartIndex, int iEndIndex ) const
{
	int iNum = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( *this, r, iStartIndex, iEndIndex )
	{
		int iNumNotesThisIndex = 0;
		for( int t=0; t<GetNumTracks(); t++ )
		{
			const TapNote &tn = GetTapNote(t, r);
			if( tn.type != TapNote::mine  &&  tn.type != TapNote::empty  &&  tn.type != TapNote::fake )	// mines don't count
				iNumNotesThisIndex++;
		}
		if( iNumNotesThisIndex >= iMinTaps )
			iNum++;
	}
	
	return iNum;
}

int NoteData::GetNumHoldNotes( int iStartIndex, int iEndIndex ) const
{
	int iNumHolds = 0;
	for( int t=0; t<GetNumTracks(); ++t )
	{
		const_iterator begin, end;
		GetTapNoteRangeExclusive( t, iStartIndex, iEndIndex, begin, end );
		for( ; begin != end; ++begin )
		{
			if( begin->second.type != TapNote::hold_head ||
				begin->second.subType != TapNote::hold_head_hold )
				continue;
			iNumHolds++;
		}
	}
	return iNumHolds;
}

int NoteData::GetNumRolls( int iStartIndex, int iEndIndex ) const
{
	int iNumRolls = 0;
	for( int t=0; t<GetNumTracks(); ++t )
	{
		const_iterator begin, end;
		GetTapNoteRangeExclusive( t, iStartIndex, iEndIndex, begin, end );
		for( ; begin != end; ++begin )
		{
			if( begin->second.type != TapNote::hold_head ||
				begin->second.subType != TapNote::hold_head_roll )
				continue;
			iNumRolls++;
		}
	}
	return iNumRolls;
}

// -1 for iOriginalTracksToTakeFrom means no track
void NoteData::LoadTransformed( const NoteData& in, int iNewNumTracks, const int iOriginalTrackToTakeFrom[] )
{
	// reset all notes
	Init();
	
	SetNumTracks( iNewNumTracks );

	// copy tracks
	for( int t=0; t<GetNumTracks(); t++ )
	{
		const int iOriginalTrack = iOriginalTrackToTakeFrom[t];
		ASSERT_M( iOriginalTrack < in.GetNumTracks(), ssprintf("from %i >= %i (to %i)", 
			iOriginalTrack, in.GetNumTracks(), iOriginalTrackToTakeFrom[t]));

		if( iOriginalTrack == -1 )
			continue;
		m_TapNotes[t] = in.m_TapNotes[iOriginalTrack];
	}
}

void NoteData::MoveTapNoteTrack( int dest, int src )
{
	if(dest == src) return;
	m_TapNotes[dest] = m_TapNotes[src];
	m_TapNotes[src].clear();
}

void NoteData::SetTapNote( int track, int row, const TapNote& t )
{
	DEBUG_ASSERT( track>=0 && track<GetNumTracks() );
	
	if( row < 0 )
		return;

	// There's no point in inserting empty notes into the map.
	// Any blank space in the map is defined to be empty.
	// If we're trying to insert an empty at a spot where
	// another note already exists, then we're really deleting
	// from the map.
	if( t == TAP_EMPTY )
	{
		TrackMap &trackMap = m_TapNotes[track];
		// remove the element at this position (if any).  This will return either 0 or 1.
		trackMap.erase( row );
	}
	else
	{
		m_TapNotes[track][row] = t;
	}
}

void NoteData::GetTracksHeldAtRow( int row, set<int>& addTo )
{
	for( int t=0; t<GetNumTracks(); ++t )
		if( IsHoldNoteAtRow( t, row ) )
			addTo.insert( t );
}

int NoteData::GetNumTracksHeldAtRow( int row )
{
	static set<int> viTracks;
	viTracks.clear();
	GetTracksHeldAtRow( row, viTracks );
	return viTracks.size();
}

bool NoteData::GetNextTapNoteRowForTrack( int track, int &rowInOut ) const
{
	const TrackMap &mapTrack = m_TapNotes[track];

	// lower_bound and upper_bound have the same effect here because duplicate 
	// keys aren't allowed.
	//
	// lower_bound "finds the first element whose key is not less than k" (>=);
	// upper_bound "finds the first element whose key greater than k".  They don't
	// have the same effect, but lower_bound(row+1) should equal upper_bound(row). -glenn
	TrackMap::const_iterator iter = mapTrack.lower_bound( rowInOut+1 );	// "find the first note for which row+1 < key == false"
	if( iter == mapTrack.end() )
		return false;

	ASSERT( iter->first > rowInOut );
	rowInOut = iter->first;
	return true;
}

bool NoteData::GetPrevTapNoteRowForTrack( int track, int &rowInOut ) const
{
	const TrackMap &mapTrack = m_TapNotes[track];

	/* Find the first note >= rowInOut. */
	TrackMap::const_iterator iter = mapTrack.lower_bound( rowInOut );

	/* If we're at the beginning, we can't move back any more. */
	if( iter == mapTrack.begin() )
		return false;

	/* Move back by one. */
	--iter;	
	ASSERT( iter->first < rowInOut );
	rowInOut = iter->first;
	return true;
}

/* Return an iterator range for [rowBegin,rowEnd).  This can be used to efficiently
 * iterate trackwise over a range of notes.  It's like FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE,
 * except it only requires two map searches (iterating is constant time), but the iterators will
 * become invalid if the notes they represent disappear, so you need to pay attention to
 * how you modify the data. */
void NoteData::GetTapNoteRange( int iTrack, int iStartRow, int iEndRow, TrackMap::iterator &begin, TrackMap::iterator &end )
{
	ASSERT_M( iTrack < GetNumTracks(), ssprintf("%i,%i", iTrack, GetNumTracks())  );
	TrackMap &mapTrack = m_TapNotes[iTrack];

	if( iStartRow > iEndRow )
	{
		begin = end = mapTrack.end();
		return;
	}

	if( iStartRow <= 0 )
		begin = mapTrack.begin(); /* optimization */
	else if( iStartRow >= MAX_NOTE_ROW )
		begin = mapTrack.end(); /* optimization */
	else
		begin = mapTrack.lower_bound( iStartRow );

	if( iEndRow <= 0 )
		end = mapTrack.begin(); /* optimization */
	else if( iEndRow >= MAX_NOTE_ROW )
		end = mapTrack.end(); /* optimization */
	else
		end = mapTrack.lower_bound( iEndRow );
}


/* Include hold notes that overlap the edges.  If a hold note completely surrounds the given
 * range, included it, too.  If bIncludeAdjacent is true, also include hold notes adjacent to,
 * but not overlapping, the edge. */
void NoteData::GetTapNoteRangeInclusive( int iTrack, int iStartRow, int iEndRow, iterator &begin, iterator &end, bool bIncludeAdjacent )
{
	GetTapNoteRange( iTrack, iStartRow, iEndRow, begin, end );

	if( begin != this->begin(iTrack) )
	{
		iterator prev = Decrement(begin);

		const TapNote &tn = prev->second;
		if( tn.type == TapNote::hold_head )
		{
			int iHoldStartRow = prev->first;
			int iHoldEndRow = iHoldStartRow + tn.iDuration;
			if( bIncludeAdjacent )
				++iHoldEndRow;
			if( iHoldEndRow > iStartRow )
			{
				/* The previous note is a hold. */
				begin = prev;
			}
		}
	}

	if( bIncludeAdjacent && end != this->end(iTrack) )
	{
		/* Include the next note if it's a hold and starts on iEndRow. */
		const TapNote &tn = end->second;
		int iHoldStartRow = end->first;
		if( tn.type == TapNote::hold_head && iHoldStartRow == iEndRow )
			++end;
	}
}

void NoteData::GetTapNoteRangeExclusive( int iTrack, int iStartRow, int iEndRow, iterator &begin, iterator &end )
{
	GetTapNoteRange( iTrack, iStartRow, iEndRow, begin, end );

	/* If end-1 is a hold_head, and extends beyond iEndRow, exclude it. */
	if( begin != end && end != this->begin(iTrack) )
	{
		iterator prev = end;
		--prev;
		if( prev->second.type == TapNote::hold_head )
		{
			int iStartRow = prev->first;
			const TapNote &tn = prev->second;
			if( iStartRow + tn.iDuration >= iEndRow )
				end = prev;
		}
	}
}

void NoteData::GetTapNoteRange( int iTrack, int iStartRow, int iEndRow, TrackMap::const_iterator &begin, TrackMap::const_iterator &end ) const
{
	TrackMap::iterator const_begin, const_end;
	const_cast<NoteData *>(this)->GetTapNoteRange( iTrack, iStartRow, iEndRow, const_begin, const_end );
	begin = const_begin;
	end = const_end;
}

void NoteData::GetTapNoteRangeInclusive( int iTrack, int iStartRow, int iEndRow, TrackMap::const_iterator &begin, TrackMap::const_iterator &end, bool bIncludeAdjacent ) const
{
	TrackMap::iterator const_begin, const_end;
	const_cast<NoteData *>(this)->GetTapNoteRangeInclusive( iTrack, iStartRow, iEndRow, const_begin, const_end, bIncludeAdjacent );
	begin = const_begin;
	end = const_end;
}

void NoteData::GetTapNoteRangeExclusive( int iTrack, int iStartRow, int iEndRow, TrackMap::const_iterator &begin, TrackMap::const_iterator &end ) const
{
	TrackMap::iterator const_begin, const_end;
	const_cast<NoteData *>(this)->GetTapNoteRange( iTrack, iStartRow, iEndRow, const_begin, const_end );
	begin = const_begin;
	end = const_end;
}



bool NoteData::GetNextTapNoteRowForAllTracks( int &rowInOut ) const
{
	int iClosestNextRow = MAX_NOTE_ROW;
	bool bAnyHaveNextNote = false;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		int iNewRowThisTrack = rowInOut;
		if( GetNextTapNoteRowForTrack( t, iNewRowThisTrack ) )
		{
			bAnyHaveNextNote = true;
			ASSERT( iNewRowThisTrack < MAX_NOTE_ROW );
			iClosestNextRow = min( iClosestNextRow, iNewRowThisTrack );
		}
	}

	if( bAnyHaveNextNote )
	{
		rowInOut = iClosestNextRow;
		return true;
	}
	else
	{
		return false;
	}
}

bool NoteData::GetPrevTapNoteRowForAllTracks( int &rowInOut ) const
{
	int iClosestPrevRow = 0;
	bool bAnyHavePrevNote = false;
	for( int t=0; t<GetNumTracks(); t++ )
	{
		int iNewRowThisTrack = rowInOut;
		if( GetPrevTapNoteRowForTrack( t, iNewRowThisTrack ) )
		{
			bAnyHavePrevNote = true;
			ASSERT( iNewRowThisTrack < MAX_NOTE_ROW );
			iClosestPrevRow = max( iClosestPrevRow, iNewRowThisTrack );
		}
	}

	if( bAnyHavePrevNote )
	{
		rowInOut = iClosestPrevRow;
		return true;
	}
	else
	{
		return false;
	}
}

XNode* NoteData::CreateNode() const
{
	XNode *p = new XNode( "NoteData" );
	
	all_tracks_const_iterator iter = GetTapNoteRangeAllTracks( 0, GetLastRow() );
	
	for( ; !iter.IsAtEnd(); ++iter )
	{
		XNode *p2 = iter->CreateNode();
		
		p2->AppendAttr( "Track", iter.Track() );
		p2->AppendAttr( "Row", iter.Row() );
		p->AppendChild( p2 );
	}
	return p;
}

void NoteData::LoadFromNode( const XNode* pNode )
{
	ASSERT(0);
}

template<typename ND, typename iter, typename TN>
void NoteData::_all_tracks_iterator<ND, iter, TN>::Find( bool bReverse )
{
	// If no notes can be found in the range, m_iTrack will stay -1 and IsAtEnd() will return true.
	m_iTrack = -1;
	if( bReverse )
	{
		int iMaxRow = INT_MIN;
		for( int iTrack = m_pNoteData->GetNumTracks() - 1; iTrack >= 0; --iTrack )
		{
			iter &i( m_vCurrentIters[iTrack] );
			const iter &end = m_vEndIters[iTrack];
			if( i != end  &&  i->first > iMaxRow )
			{
				iMaxRow = i->first;
				m_iTrack = iTrack;
			}
		}
	}
	else
	{

		int iMinRow = INT_MAX;
		for( int iTrack = 0; iTrack < m_pNoteData->GetNumTracks(); ++iTrack )
		{
			iter &i = m_vCurrentIters[iTrack];
			const iter &end = m_vEndIters[iTrack];
			if( i != end  &&  i->first < iMinRow )
			{
				iMinRow = i->first;
				m_iTrack = iTrack;
			}
		}
	}
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN>::_all_tracks_iterator( ND &nd, int iStartRow, int iEndRow, bool bReverse, bool bInclusive ) :
	m_pNoteData(&nd), m_iTrack(0), m_bReverse(bReverse)
{
	ASSERT( m_pNoteData->GetNumTracks() > 0 );

	for( int iTrack = 0; iTrack < m_pNoteData->GetNumTracks(); ++iTrack )
	{
		iter begin, end;
		if( bInclusive )
			m_pNoteData->GetTapNoteRangeInclusive( iTrack, iStartRow, iEndRow, begin, end );
		else
			m_pNoteData->GetTapNoteRange( iTrack, iStartRow, iEndRow, begin, end );

		m_vBeginIters.push_back( begin );
		m_vEndIters.push_back( end );

		iter cur;
		if( m_bReverse )
		{
			cur = end;
			if( cur != begin )
				cur--;
		}
		else
		{
			cur = begin;
		}
		m_vCurrentIters.push_back( cur );
	}

	Find( bReverse );
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN>::_all_tracks_iterator( const _all_tracks_iterator &other ) :
#define COPY_OTHER( x ) x( other.x )
	COPY_OTHER( m_pNoteData ),
	COPY_OTHER( m_vBeginIters ),
	COPY_OTHER( m_vCurrentIters ),
	COPY_OTHER( m_vEndIters ),
	COPY_OTHER( m_iTrack ),
	COPY_OTHER( m_bReverse )
#undef COPY_OTHER
{
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN> &NoteData::_all_tracks_iterator<ND, iter, TN>::operator++() // preincrement
{
	if( m_bReverse )
	{
		if( m_vCurrentIters[m_iTrack] == m_vBeginIters[m_iTrack] )
			m_vCurrentIters[m_iTrack] = m_vEndIters[m_iTrack];
		else
			--m_vCurrentIters[m_iTrack];
	}
	else
	{
		++m_vCurrentIters[m_iTrack];
	}
	Find( m_bReverse );
	return *this;
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN> NoteData::_all_tracks_iterator<ND, iter, TN>::operator++( int dummy ) // postincrement
{
	_all_tracks_iterator<ND, iter, TN> ret( *this );
	operator++();
	return ret;
}
/* XXX: This doesn't satisfy the requirements that ++iter; --iter; is a no-op so it cannot be bidirectional for now. */
#if 0
template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN> &NoteData::_all_tracks_iterator<ND, iter, TN>::operator--() // predecrement
{
	if( m_bReverse )
	{
		++m_vCurrentIters[m_iTrack];
	}
	else
	{
		if( m_vCurrentIters[m_iTrack] == m_vEndIters[m_iTrack] )
			m_vCurrentIters[m_iTrack] = m_vEndIters[m_iTrack];
		else
			--m_vCurrentIters[m_iTrack];
	}
	Find( !m_bReverse );
	return *this;
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN> NoteData::_all_tracks_iterator<ND, iter, TN>::operator--( int dummy ) // postdecrement
{
	_all_tracks_iterator<ND, iter, TN> ret( *this );
	operator--();
	return ret;
}
#endif

// Explicit instantiation.
template class NoteData::_all_tracks_iterator<NoteData, NoteData::iterator, TapNote>;
template class NoteData::_all_tracks_iterator<const NoteData, NoteData::const_iterator, const TapNote>;

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
