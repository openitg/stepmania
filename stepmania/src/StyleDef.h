#pragma once
/*
-----------------------------------------------------------------------------
 Class: StyleDef

 Desc: A data structure that holds the definition for one of a Game's styles.
	Styles define a set of columns for each player, and information about those
	columns, like what Instruments are used play those columns and what track 
	to use to populate the column's notes.
	A "track" is the term used to descibe a particular vertical sting of note 
	in NoteData.
	A "column" is the term used to describe the vertical string of notes that 
	a player sees on the screen while they're playing.  Column notes are 
	picked from a track, but columns and tracks don't have a 1-to-1 
	correspondance.  For example, dance-versus has 8 columns but only 4 tracks
	because two players place from the same set of 4 tracks.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Notes.h"
#include "NoteData.h"
#include "StyleInput.h"
#include "GameInput.h"
#include "Game.h"


const int MAX_COLS_PER_PLAYER = MAX_NOTE_TRACKS;


class GameDef;

class StyleDef
{
public:
	Game		m_Game;			// Game this style is active under
	char		m_szName[60];
	NotesType	m_NotesType;	// the notes format that this style reads.  
								// For example, the "dance versus" reads the Notes with the tag "dance-single".
	enum StyleType
	{
		ONE_PLAYER_ONE_CREDIT,	// e.g. single
		TWO_PLAYERS_TWO_CREDITS,	// e.g. versus
		ONE_PLAYER_TWO_CREDITS,	// e.g. double
	};
	StyleType	m_StyleType;		// Defines how many players are allowed to play this Style.
	int			m_iCenterX[NUM_PLAYERS];	// center of the player
	int			m_iColsPerPlayer;	// number of total tracks this style expects (e.g. 4 for versus, but 8 for double)
	struct ColumnInfo 
	{ 
		int				track;		// take note data from this track
		GameController	controller;	// use this instrument to hit a note on this track
		GameButton		button;		// use this button to hit a note on this track
		float			fXOffset;	// x position of the column relative to player center
	};
	ColumnInfo	m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];	// maps each players' column to a track in the NoteData
	int			m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];

	GameInput StyleInputToGameInput( const StyleInput StyleI );
	StyleInput GameInputToStyleInput( const GameInput &GameI );

	void GetTransformedNoteDataForStyle( PlayerNumber p, NoteData* pOriginal, NoteData* pNoteDataOut );
};

