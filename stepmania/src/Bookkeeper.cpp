#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: Bookkeeper

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "Bookkeeper.h"
#include "RageUtil.h"
#include "arch/arch.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "IniFile.h"
#include "GameConstantsAndTypes.h"
#include "SongManager.h"
#include "RageFile.h"
#include <time.h>


Bookkeeper*	BOOKKEEPER = NULL;	// global and accessable from anywhere in our program

static const CString COINS_DAT = "Data/Coins.dat";

const int COINS_DAT_VERSION = 1;

tm AddDays( tm start, int iDaysToMove )
{
	start.tm_mday += iDaysToMove;
	time_t seconds = mktime( &start );
	ASSERT( seconds != (time_t)-1 );
	tm time = *localtime( &seconds );
	return time;
}

tm GetYesterday( tm start )
{
	return AddDays( start, -1 );
}

int GetDayOfWeek( tm time )
{
	int iDayOfWeek = time.tm_wday;
	ASSERT( iDayOfWeek < DAYS_IN_WEEK );
	return iDayOfWeek;
}

tm GetNextSunday( tm start )
{
	return AddDays( start, DAYS_IN_WEEK-GetDayOfWeek(start) );
}



Bookkeeper::Bookkeeper()
{
	int i, j;

	m_iLastSeenTime = time(NULL);
	for( i=0; i<DAYS_PER_YEAR; i++ )
		for( j=0; j<HOURS_PER_DAY; j++ )
			m_iCoinsByHourForYear[i][j] = 0;

	ReadFromDisk();

	UpdateLastSeenTime();
}

Bookkeeper::~Bookkeeper()
{
	WriteToDisk();
}

#define WARN_AND_RETURN { LOG->Warn("Error parsing at %s:%d",__FILE__,__LINE__); return; }

void Bookkeeper::ReadFromDisk()
{
    RageFile f;
	if( !f.Open(COINS_DAT, RageFile::READ) )
	{
		LOG->Trace( "Couldn't open file \"%s\": %s", COINS_DAT.c_str(), f.GetError().c_str() );
		return;
	}

	int iVer;
	if( !FileRead(f, iVer) )
		WARN_AND_RETURN;
	if( iVer != COINS_DAT_VERSION )
		WARN_AND_RETURN;

	if( !FileRead(f, m_iLastSeenTime) )
		WARN_AND_RETURN;

    for (int i=0; i<DAYS_PER_YEAR; ++i)
	{
        for (int j=0; j<HOURS_PER_DAY; ++j)
		{
			int iCoins;
			if( !FileRead(f, iCoins) )
				WARN_AND_RETURN;

            m_iCoinsByHourForYear[i][j] = iCoins;
		}
	}
}

void Bookkeeper::WriteToDisk()
{
	// write dat
    RageFile f;
	if( !f.Open(COINS_DAT, RageFile::WRITE) )
	{
		LOG->Warn( "Couldn't open file \"%s\" for writing: %s", COINS_DAT.c_str(), f.GetError().c_str() );
		return;
	}
    
	FileWrite(f, COINS_DAT_VERSION );
	
	FileWrite(f, m_iLastSeenTime);

	for (int i=0; i<DAYS_PER_YEAR; ++i)
        for (int j=0; j<HOURS_PER_DAY; ++j)
            FileWrite( f, m_iCoinsByHourForYear[i][j] );
}

void Bookkeeper::UpdateLastSeenTime()
{
	// clear all coin counts from (lOldTime,lNewTime]

	long lOldTime = m_iLastSeenTime;
	long lNewTime = time(NULL);

	if( lNewTime < lOldTime )
	{
		LOG->Warn( "The new time is older than the last seen time.  Is someone fiddling with the system clock?" );
		m_iLastSeenTime = lNewTime;
		return;
	}

    tm tOld = *localtime( &lOldTime );
    tm tNew = *localtime( &lNewTime );

	CLAMP( tOld.tm_year, tNew.tm_year-1, tNew.tm_year );

	while( 
		tOld.tm_year != tNew.tm_year ||
		tOld.tm_yday != tNew.tm_yday ||
		tOld.tm_hour != tNew.tm_hour )
	{
		tOld.tm_hour++;
		if( tOld.tm_hour == HOURS_PER_DAY )
		{
			tOld.tm_hour = 0;
			tOld.tm_yday++;
		}
		if( tOld.tm_yday == DAYS_PER_YEAR )
		{
			tOld.tm_yday = 0;
			tOld.tm_year++;
		}

		m_iCoinsByHourForYear[tOld.tm_yday][tOld.tm_hour] = 0;
	}

	m_iLastSeenTime = lNewTime;
}

void Bookkeeper::CoinInserted()
{
	UpdateLastSeenTime();

	long lTime = m_iLastSeenTime;
    tm *pTime = localtime( &lTime );

	m_iCoinsByHourForYear[pTime->tm_yday][pTime->tm_hour]++;
}

int Bookkeeper::GetCoinsForDay( int iDayOfYear )
{
	int iCoins = 0;
	for( int i=0; i<HOURS_PER_DAY; i++ )
		iCoins += m_iCoinsByHourForYear[iDayOfYear][i];
	return iCoins;
}


void Bookkeeper::GetCoinsLastDays( int coins[NUM_LAST_DAYS] )
{
	UpdateLastSeenTime();

	long lOldTime = m_iLastSeenTime;
    tm time = *localtime( &lOldTime );

	for( int i=0; i<NUM_LAST_DAYS; i++ )
	{
		coins[i] = GetCoinsForDay( time.tm_yday );
		time = GetYesterday( time );
	}
}


void Bookkeeper::GetCoinsLastWeeks( int coins[NUM_LAST_WEEKS] )
{
	UpdateLastSeenTime();

	long lOldTime = m_iLastSeenTime;
    tm time = *localtime( &lOldTime );

	time = GetNextSunday( time );
	time = GetYesterday( time );

	for( int w=0; w<NUM_LAST_WEEKS; w++ )
	{
		coins[w] = 0;

		for( int d=0; d<DAYS_IN_WEEK; d++ )
		{
			coins[w] += GetCoinsForDay( time.tm_yday );
			time = GetYesterday( time );
		}
	}
}

void Bookkeeper::GetCoinsByDayOfWeek( int coins[DAYS_IN_WEEK] )
{
	UpdateLastSeenTime();

	for( int i=0; i<DAYS_IN_WEEK; i++ )
		coins[i] = 0;

	long lOldTime = m_iLastSeenTime;
    tm time = *localtime( &lOldTime );

	for( int d=0; d<DAYS_PER_YEAR; d++ )
	{
		coins[GetDayOfWeek(time)] += GetCoinsForDay( time.tm_yday );
		time = GetYesterday( time );
	}
}

void Bookkeeper::GetCoinsByHour( int coins[HOURS_PER_DAY] )
{
	UpdateLastSeenTime();

	for( int h=0; h<HOURS_PER_DAY; h++ )
	{
		coins[h] = 0;

		for( int d=0; d<DAYS_PER_YEAR; d++ )
			coins[h] += m_iCoinsByHourForYear[d][h];
	}
}

CString HourToString( int iHourIndex )
{
	return ssprintf("%02d:00", iHourIndex);
}
