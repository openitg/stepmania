#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: SongManager

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

//#include <d3dxmath.h>
#include "SongManager.h"
#include "IniFile.h"
#include "RageLog.h"

#include "PrefsManager.h"

SongManager*	SONGMAN = NULL;	// global and accessable from anywhere in our program


const CString g_sStatisticsFileName = "statistics.ini";

D3DXCOLOR GROUP_COLORS[] = { 
	D3DXCOLOR( 0.9f, 0.0f, 0.2f, 1 ),	// red
	D3DXCOLOR( 0.7f, 0.0f, 0.5f, 1 ),	// pink
	D3DXCOLOR( 0.4f, 0.2f, 0.6f, 1 ),	// purple
	D3DXCOLOR( 0.0f, 0.4f, 0.8f, 1 ),	// sky blue
	D3DXCOLOR( 0.0f, 0.6f, 0.6f, 1 ),	// sea green
	D3DXCOLOR( 0.0f, 0.6f, 0.2f, 1 ),	// green
	D3DXCOLOR( 0.8f, 0.6f, 0.0f, 1 ),	// orange
};
const int NUM_GROUP_COLORS = sizeof(GROUP_COLORS) / sizeof(D3DXCOLOR);


SongManager::SongManager()
{
	m_pCurSong = NULL;
	for( int p=0; p<NUM_PLAYERS; p++ )
		m_pCurNotes[p] = NULL;
	m_pCurCourse = NULL;

	InitSongArrayFromDisk();
	ReadStatisticsFromDisk();

	InitCoursesFromDisk();
}


SongManager::~SongManager()
{
	SaveStatisticsToDisk();
	FreeSongArray();
}


Song* SongManager::GetCurrentSong()
{
	return m_pCurSong;
}

Notes* SongManager::GetCurrentNotes( PlayerNumber p )
{
	return m_pCurNotes[p];
}

void SongManager::SetCurrentSong( Song* pSong )
{
	m_pCurSong = pSong;
}

void SongManager::SetCurrentNotes( PlayerNumber p, Notes* pNotes )
{
	m_pCurNotes[p] = pNotes;
}

GameplayStatistics SongManager::GetLatestGameplayStatistics( PlayerNumber p )
{
	ASSERT( m_aGameplayStatistics[p].GetSize() > 0 );
	return m_aGameplayStatistics[p][ m_aGameplayStatistics[p].GetSize()-1 ];
}

void SongManager::InitSongArrayFromDisk()
{
	LoadStepManiaSongDir( "Songs" );

	for( int i=0; i<PREFSMAN->m_asSongFolders.GetSize(); i++ )
        LoadStepManiaSongDir( PREFSMAN->m_asSongFolders[i] );
	
	// compute group names
	CArray<Song*, Song*> arraySongs;
	arraySongs.Copy( m_pSongs );
	SortSongPointerArrayByGroup( arraySongs );

	for( i=0; i<m_pSongs.GetSize(); i++ )
	{
		Song* pSong = m_pSongs[i];
		const CString sGroupName = m_pSongs[i]->m_sGroupName;

		if( m_arrayGroupNames.GetSize() == 0  ||  m_arrayGroupNames[m_arrayGroupNames.GetSize()-1] != sGroupName )
			m_arrayGroupNames.Add( sGroupName );
	}

	LOG->WriteLine( "Found %d Songs.", m_pSongs.GetSize() );
}

void SongManager::LoadStepManiaSongDir( CString sDir )
{
	// trim off the trailing slash if any
	sDir.TrimRight( "/\\" );

	// Find all group directories in "Songs" folder
	CStringArray arrayGroupDirs;
	GetDirListing( sDir+"\\*.*", arrayGroupDirs, true );
	SortCStringArray( arrayGroupDirs );
	
	for( int i=0; i< arrayGroupDirs.GetSize(); i++ )	// for each dir in /Songs/
	{
		CString sGroupDirName = arrayGroupDirs[i];

		if( 0 == stricmp( sGroupDirName, "cvs" ) )	// the directory called "CVS"
			continue;		// ignore it

		// Check to see if they put a song directly inside of the group folder
		CStringArray arrayFiles;
		GetDirListing( ssprintf("%s\\%s\\*.mp3", sDir, sGroupDirName), arrayFiles );
		GetDirListing( ssprintf("%s\\%s\\*.ogg", sDir, sGroupDirName), arrayFiles );
		GetDirListing( ssprintf("%s\\%s\\*.wav", sDir, sGroupDirName), arrayFiles );
		if( arrayFiles.GetSize() > 0 )
			throw RageException( 
				ssprintf( "The song folder '%s' must be placed inside of a group folder.\n\n"
					"All song folders must be placed below a group folder.  For example, 'Songs\\DDR 4th Mix\\B4U'.  See the StepMania readme for more info.",
					ssprintf("%s\\%s", sDir, sGroupDirName ) )
			);
		
		// Look for a group banner in this group folder
		CStringArray arrayGroupBanners;
		GetDirListing( ssprintf("%s\\%s\\*.png", sDir, sGroupDirName), arrayGroupBanners );
		GetDirListing( ssprintf("%s\\%s\\*.jpg", sDir, sGroupDirName), arrayGroupBanners );
		GetDirListing( ssprintf("%s\\%s\\*.gif", sDir, sGroupDirName), arrayGroupBanners );
		GetDirListing( ssprintf("%s\\%s\\*.bmp", sDir, sGroupDirName), arrayGroupBanners );
		if( arrayGroupBanners.GetSize() > 0 )
		{
			m_mapGroupToBannerPath[sGroupDirName] = ssprintf("%s\\%s\\%s", sDir, sGroupDirName, arrayGroupBanners[0] );
			LOG->WriteLine( ssprintf("Group banner for '%s' is '%s'.", sGroupDirName, m_mapGroupToBannerPath[sGroupDirName]) );
		}

		// Find all Song folders in this group directory
		CStringArray arraySongDirs;
		GetDirListing( ssprintf("%s\\%s\\*.*", sDir, sGroupDirName), arraySongDirs, true );
		SortCStringArray( arraySongDirs );

		for( int j=0; j< arraySongDirs.GetSize(); j++ )	// for each song dir
		{
			CString sSongDirName = arraySongDirs[j];

			if( 0 == stricmp( sSongDirName, "cvs" ) )	// the directory called "CVS"
				continue;		// ignore it

			// this is a song directory.  Load a new song!
			Song* pNewSong = new Song;
			pNewSong->LoadFromSongDir( ssprintf("%s\\%s\\%s", sDir, sGroupDirName, sSongDirName) );
			m_pSongs.Add( pNewSong );
		}
	}
}


/*
void SongManager::LoadDWISongDir( CString DWIHome )
{
	// trim off the trailing slash if any
	DWIHome.TrimRight( "/\\" );

	// this has to be fixed, DWI doesn't put files
	// in it's DWI folder.  It puts them in Songs/<MIX>/<SONGNAME>
	// so what we have to do, is go to the DWI directory (which will
	// be changeable by the user so they don't have to copy everything
	// over and have two copies of everything
	// Find all directories in "DWIs" folder
	CStringArray arrayDirs;
	CStringArray MixDirs;
	// now we've got the listing of the mix directories
	// and we need to use THOSE directories to find our
	// dwis
	GetDirListing( ssprintf("%s\\Songs\\*.*", DWIHome ), MixDirs, true );
	SortCStringArray( MixDirs );
	
	for( int i=0; i< MixDirs.GetSize(); i++ )	// for each dir in /Songs/
	{
		// the dir name will most likely be something like
		// Dance Dance Revolution 4th Mix, etc.
		CString sDirName = MixDirs[i];
		sDirName.MakeLower();
		if( sDirName == "cvs" )	// ignore the directory called "CVS"
			continue;
		GetDirListing( ssprintf("%s\\Songs\\%s\\*.*", DWIHome, MixDirs[i]), arrayDirs,  true);
		SortCStringArray(arrayDirs, true);

		for( int b = 0; b < arrayDirs.GetSize(); b++)
		{
			// Find all DWIs in this directory
			CStringArray arrayDWIFiles;
			GetDirListing( ssprintf("%s\\Songs\\%s\\%s\\*.dwi", DWIHome, MixDirs[i], arrayDirs[b]), arrayDWIFiles, false);
			SortCStringArray( arrayDWIFiles );

			for( int j=0; j< arrayDWIFiles.GetSize(); j++ )	// for each DWI file
			{
				CString sDWIFileName = arrayDWIFiles[j];
				sDWIFileName.MakeLower();

				// load DWIs from the sub dirs
				Song* pNewSong = new Song;
				pNewSong->LoadFromDWIFile( ssprintf("%s\\Songs\\%s\\%s\\%s", DWIHome, MixDirs[i], arrayDirs[b], sDWIFileName) );
				m_pSongs.Add( pNewSong );
			}
		}
	}
}
*/


void SongManager::FreeSongArray()
{
	// Memory is being corrupt somewhere, and this is causing a crash.  Bad news.  I'll fix it later.  Let the OS free it for now.
	for( int i=0; i<m_pSongs.GetSize(); i++ )
		SAFE_DELETE( m_pSongs[i] );
	m_pSongs.RemoveAll();

	m_mapGroupToBannerPath.RemoveAll();
}


void SongManager::ReloadSongArray()
{
	InitSongArrayFromDisk();
	FreeSongArray();
}



void SongManager::ReadStatisticsFromDisk()
{
	IniFile ini;
	ini.SetPath( g_sStatisticsFileName );
	if( !ini.ReadFile() ) {
		LOG->WriteLine( "WARNING: Could not read config file '%s'.", g_sStatisticsFileName );
		return;		// load nothing
	}


	// load song statistics
	CMapStringToString* pKey = ini.GetKeyPointer( "Statistics" );
	if( pKey )
	{
		for( POSITION pos = pKey->GetStartPosition(); pos != NULL; )
		{
			CString name_string, value_string;

			pKey->GetNextAssoc( pos, name_string, value_string );

			// Each value has the format "SongName::StepsName=TimesPlayed::TopGrade::TopScore::MaxCombo".

			char szSongDir[256];
			char szNotesName[256];
			int iRetVal;
			int i;

			// Parse for Song name and Notes name
			iRetVal = sscanf( name_string, "%[^:]::%[^\n]", szSongDir, szNotesName );
			if( iRetVal != 2 )
				continue;	// this line doesn't match what is expected
	
			
			// Search for the corresponding Song pointer.
			Song* pSong = NULL;
			for( i=0; i<m_pSongs.GetSize(); i++ )
			{
				if( m_pSongs[i]->m_sSongDir == szSongDir )	// match!
				{
					pSong = m_pSongs[i];
					break;
				}
			}
			if( pSong == NULL )	// didn't find a match
				continue;	// skip this entry


			// Search for the corresponding Notes pointer.
			Notes* pNotes = NULL;
			for( i=0; i<pSong->m_arrayNotes.GetSize(); i++ )
			{
				if( pSong->m_arrayNotes[i]->m_sDescription == szNotesName )	// match!
				{
					pNotes = pSong->m_arrayNotes[i];
					break;
				}
			}
			if( pNotes == NULL )	// didn't find a match
				continue;	// skip this entry


			// Parse the Notes statistics.
			char szGradeLetters[10];	// longest possible string is "AAA"

			iRetVal = sscanf( 
				value_string, 
				"%d::%[^:]::%d::%d", 
				&pNotes->m_iNumTimesPlayed,
				szGradeLetters,
				&pNotes->m_iTopScore,
				&pNotes->m_iMaxCombo
			);
			if( iRetVal != 4 )
				continue;

			pNotes->m_TopGrade = StringToGrade( szGradeLetters );
		}
	}
}

void SongManager::SaveStatisticsToDisk()
{
	IniFile ini;
	ini.SetPath( g_sStatisticsFileName );

	// save song statistics
	for( int i=0; i<m_pSongs.GetSize(); i++ )		// for each Song
	{
		Song* pSong = m_pSongs[i];

		for( int j=0; j<pSong->m_arrayNotes.GetSize(); j++ )		// for each Notes
		{
			Notes* pNotes = pSong->m_arrayNotes[j];

			if( pNotes->m_TopGrade == GRADE_NO_DATA )
				continue;		// skip

			// Each value has the format "SongName::NotesName=TimesPlayed::TopGrade::TopScore::MaxCombo".

			CString sName = ssprintf( "%s::%s", pSong->m_sSongDir, pNotes->m_sDescription );
			CString sValue = ssprintf( 
				"%d::%s::%d::%d",
				pNotes->m_iNumTimesPlayed,
				GradeToString( pNotes->m_TopGrade ),
				pNotes->m_iTopScore, 
				pNotes->m_iMaxCombo
			);

			ini.SetValue( "Statistics", sName, sValue );
		}
	}

	ini.WriteFile();
}


CString SongManager::GetGroupBannerPath( CString sGroupName )
{
	CString sPath;

	if( m_mapGroupToBannerPath.Lookup( sGroupName, sPath ) )
		return sPath;
	else
		return "";
}

void SongManager::GetGroupNames( CStringArray &AddTo )
{
	AddTo.Copy( m_arrayGroupNames );
}

D3DXCOLOR SongManager::GetGroupColor( const CString &sGroupName )
{
	// search for the group index
	for( int i=0; i<m_arrayGroupNames.GetSize(); i++ )
	{
		if( m_arrayGroupNames[i] == sGroupName )
			break;
	}
	ASSERT( i != m_arrayGroupNames.GetSize() );	// this is not a valid group

	return GROUP_COLORS[i%NUM_GROUP_COLORS];
}

void SongManager::GetSongsInGroup( const CString sGroupName, CArray<Song*, Song*> &AddTo )
{
	for( int i=0; i<m_pSongs.GetSize(); i++ )
	{
		Song* pSong = m_pSongs[i];
		if( sGroupName == m_pSongs[i]->m_sGroupName )
			AddTo.Add( pSong );
	}
	SortSongPointerArrayByGroup( AddTo );
}

CString SongManager::ShortenGroupName( const CString &sOrigGroupName )
{
	CString sShortName = sOrigGroupName;
	sShortName.Replace( "Dance Dance Revolution", "DDR" );
	sShortName.Replace( "dance dance revolution", "DDR" );
	sShortName.Replace( "DANCE DANCE REVOLUTION", "DDR" );
	return sShortName;
}

void SongManager::InitCoursesFromDisk()
{
	//
	// Load courses from CRS files
	//
	CStringArray saCourseFiles;
	GetDirListing( "Courses\\*.crs", saCourseFiles );
	for( int i=0; i<saCourseFiles.GetSize(); i++ )
	{
		Course course;
		course.LoadFromCRSFile( "Courses\\" + saCourseFiles[i], m_pSongs );
		m_aCourses.Add( course );
	}


	//
	// Create default courses
	//
	CStringArray saGroupNames;
	this->GetGroupNames( saGroupNames );
	for( int g=0; g<saGroupNames.GetSize(); g++ )	// foreach Group
	{
		CString sGroupName = saGroupNames[g];
		CString sShortGroupName = this->ShortenGroupName( sGroupName );

		CArray<Song*, Song*> apSongs;
		this->GetSongsInGroup( sGroupName, apSongs );
		
		for( NotesType nt=NotesType(0); nt<NUM_NOTES_TYPES; nt=NotesType(nt+1) )	// foreach NotesType
		{

			for( DifficultyClass dc=CLASS_MEDIUM; dc<=CLASS_HARD; dc=DifficultyClass(dc+1) )	// foreach DifficultyClass
			{
				Course course;
				course.m_sName = sShortGroupName + " ";
				switch( dc )
				{
				case CLASS_EASY:	course.m_sName += "Easy";	break;
				case CLASS_MEDIUM:	course.m_sName += "Medium";	break;
				case CLASS_HARD:	course.m_sName += "Hard";	break;
				}
				course.m_NotesType = nt;


				for( int s=0; s<apSongs.GetSize(); s++ )
				{
					Song* pSong = apSongs[s];

					CArray<Notes*, Notes*> apNotes;
					pSong->GetNotesThatMatch( course.m_NotesType, apNotes );

					// search for first Notes matching this DifficultyClass
					for( int n=0; n<apNotes.GetSize(); n++ )
					{
						Notes* pNotes = apNotes[n];

						if( pNotes->m_DifficultyClass == dc )
						{
							course.AddStage( pSong, pNotes );
							break;
						}
					}
				}

				if( course.m_iStages > 0 )
					m_aCourses.Add( course );
			}
		}
	}
}

void SongManager::ReloadCourses()
{

}
