
/*
-----------------------------------------------------------------------------
 Class: Network Sync Manager

 Desc: See Header.

 Copyright (c) 2003-2004 by the person(s) listed below.  All rights reserved.
	Charles Lohr
	Joshua Allen
-----------------------------------------------------------------------------
*/

#include "global.h"
#include <cstring>
#include "NetworkSyncManager.h"
#include "ProfileManager.h"
#include "ezsockets.h"
#include "RageLog.h"
#include "StepMania.h"
#include "RageUtil.h"
#include "ThemeManager.h"
#include "ScreenSelectMusic.h"
#include "Screen.h"
#include "PlayerNumber.h"
#include "GameConstantsAndTypes.h"
#include "SongOptions.h"
#include "ScreenManager.h"
#include "Course.h"
#include "song.h"
#include "SongManager.h"
#include "GameState.h"


#define NEXT_SCREEN							THEME->GetMetric ("ScreenSelectMusic","NextScreen")
	//Used for advancing to gameplay screen

NetworkSyncManager::NetworkSyncManager()
{
    NetPlayerClient = new EzSockets;
	m_ServerVersion = 0;
    
	useSMserver = false;
	m_startupStatus = 0;	//By default, connection not tried.

	StartUp();
}

NetworkSyncManager::~NetworkSyncManager ()
{
	//Close Connection to server nicely.
    if (useSMserver)
        NetPlayerClient->close();
	delete NetPlayerClient;
}

void NetworkSyncManager::CloseConnection()
{
	if (!useSMserver)
		return ;
	m_ServerVersion = 0;
   	useSMserver = false;
	m_startupStatus = 0;
	NetPlayerClient->close();
}

void NetworkSyncManager::PostStartUp(CString ServerIP)
{
	CloseConnection();
	if( ServerIP!="LISTEN" )
	{
		if( !Connect(ServerIP.c_str(),8765) )
		{
			m_startupStatus = 2;
			LOG->Warn( "Network Sync Manager failed to connect" );
			return;
		}

	} else {
		if( !Listen(8765) )
		{
			m_startupStatus = 2;
			LOG->Warn( "Listen() failed");
			return;
		}

	}

	useSMserver = true;

	m_startupStatus = 1;	//Connection attepmpt sucessful

	int ClientCommand=3;
	NetPlayerClient->send((char*) &ClientCommand, 4);

	NetPlayerClient->receive(m_ServerVersion);
	// If network play is desired and the connection works,
	// halt until we know what server version we're dealing with
	vector <CString> ProfileNames;
	PROFILEMAN->GetLocalProfileNames(ProfileNames);
	
	netName PlayerName;
	if( ProfileNames.size() > 0 )
	{
		PlayerName.m_packID = 30;
		unsigned i;
		for( i=0; i < strlen(ProfileNames[0]); i++ )
			PlayerName.m_data[i] = ProfileNames[0][i];
		PlayerName.m_data[i] = 0;
		NetPlayerClient->send((char*) &PlayerName,20);
	}
	if( ProfileNames.size() > 1 )
	{
		PlayerName.m_packID = 31;
		unsigned i=0;
		for( i=0; i < strlen(ProfileNames[1]); i++ )
			PlayerName.m_data[i] = ProfileNames[1][i];
		PlayerName.m_data[i] = 0;
		NetPlayerClient->send( (char*) &PlayerName,20 );
	}

	LOG->Info("Server Version: %d",m_ServerVersion);
}


void NetworkSyncManager::StartUp()
{
	CString ServerIP;

	if( GetCommandlineArgument( "netip", &ServerIP ) )
	{
		PostStartUp(ServerIP);
	}
	else if( GetCommandlineArgument( "listen" ) )
	{
		PostStartUp("LISTEN");
	}
}


bool NetworkSyncManager::Connect(const CString& addy, unsigned short port)
{
	LOG->Info("Beginning to connect");
	if (port!=8765) 
		return false;
	//Make sure using port 8765
	//This may change in future versions
	//It is this way now for protocol's purpose.
	//If there is a new protocol developed down the road

	//Since under non-lan conditions, the client
	//will be connecting to localhost, this port does not matter.
    /* What do you mean it doesn't matter if you are connecting to localhost?
     * Also, when does it ever connect to localhost?
     * -- Steve
     */
    NetPlayerClient->create(); // Initilize Socket
    useSMserver = NetPlayerClient->connect(addy, port);
    
	return useSMserver;
}


//Listen (Wait for connection in-bound)
//NOTE: Right now, StepMania cannot connect back to StepMania!

bool NetworkSyncManager::Listen(unsigned short port)
{
	LOG->Info("Beginning to Listen");
	if (port!=8765) 
		return false;
	//Make sure using port 8765
	//This may change in future versions
	//It is this way now for protocol's purpose.
	//If there is a new protocol developed down the road


	EzSockets * EZListener = new EzSockets;

	EZListener->create();
	NetPlayerClient->create(); // Initilize Socket

	EZListener->bind(8765);
    
	useSMserver = EZListener->listen();
	useSMserver = EZListener->accept( *NetPlayerClient);  //Wait for someone to connect

	EZListener->close();	//Kill Listener
	delete EZListener;
    
	//LOG->Info("Accept Responce: ",useSMserver);
	useSMserver=true;
	return useSMserver;
}



void NetworkSyncManager::ReportScore(int playerID, int step, int score, int combo)
{
	if (!useSMserver) //Make sure that we are using the network
		return;

	netHolder SendNetPack;	//Create packet to send to server

	SendNetPack.m_playerID = playerID;
	SendNetPack.m_combo=combo;
	SendNetPack.m_score=score;			//Load packet with appropriate info
	SendNetPack.m_step=step-1;
	SendNetPack.m_life=m_playerLife[playerID];

    //Send packet to server
	NetPlayerClient->send((char*)&SendNetPack, sizeof(netHolder)); 

}
	

void NetworkSyncManager::ReportSongOver() 
{
	if (!useSMserver)	//Make sure that we are using the network
		return ;

	netHolder SendNetPack;	//Create packet to send to server

	SendNetPack.m_playerID = 21; // Song over Packet player ID
	SendNetPack.m_combo=0;
	SendNetPack.m_score=0;
	SendNetPack.m_step=0;
	SendNetPack.m_life=0;
	

	NetPlayerClient->send((char*)&SendNetPack, sizeof(netHolder)); 
	return;
}

void NetworkSyncManager::StartRequest() 
{
	if (!useSMserver)
		return ;

	//If it's going into demonstration (or jukebox) mode, do not 
	//bother to sync.
	if (GAMESTATE->m_bDemonstrationOrJukebox)
		return ;


	vector <char> tmp;	//Temporary vector used by receive function when waiting

	LOG->Trace("Requesting Start from Server.");

	netHolder SendNetPack;

	SendNetPack.m_playerID = 20; // Song Start Request Packet player ID
	SendNetPack.m_combo=0;
	SendNetPack.m_score=0;
	SendNetPack.m_step=0;
	SendNetPack.m_life=0;

	NetPlayerClient->send((char*)&SendNetPack, sizeof(netHolder)); 
	
	LOG->Trace("Waiting for RECV");

	//Block until go is recieved.
	NetPlayerClient->receive(tmp);	

	LOG->Trace("Starting Game.");
}

//This must be executed after NSMAN was started.

//this is for use with SMOnline.
//In order to gaurentee song title and group names
//line up, SM will be started and load, and then 
//send all info to the local client. 
//This information will be then relayed to the 
//SMOnline server.
void NetworkSyncManager::SendSongs()
{	
	if (!useSMserver)
		return ;

	vector <Song *> LogSongs;
	LogSongs = SONGMAN->GetAllSongs();
	unsigned i,j;
	CString SongInfo;
	char toSend[1020];	//Standard buffer is 1024 and 
						//we have to include 4 size bytes
	for (i=0;i<LogSongs.size();i++)
	{
		Song * CSong = LogSongs[i];
		SongInfo=char(1) + CSong->m_sGroupName;
		SongInfo+=char(2) + CSong->GetTranslitMainTitle();
		SongInfo+=char(3) + CSong->GetTranslitSubTitle();
		SongInfo+=char(4) + CSong->GetTranslitArtist(); 
		
		for (j=4;j<SongInfo.length()+4;j++)
			toSend[j] = SongInfo.c_str()[j-4];
		toSend[0]=char(35);
		toSend[1]=char(0);
		toSend[2]=char(0);
		toSend[3]=char(0);

		NetPlayerClient->send (toSend,SongInfo.length()+4);
	}
	toSend[0]=char(36);
	NetPlayerClient->send (toSend,4);

	//Exit because this is ONLY for use with SMOnline
	//If admins feel strongly, this can be exported
	//to another command line, like "--exitfirst"
	exit (0);

	//NOTE TO ADMINS: I do not actually know the 
	//safe way to exit stepmania, if you can, either tell me
	//or do it, please?
}


void ArgSetMode (CString cmdLineMode)
{
	CString PlayerModeStr;
	int loc=0;
	int loc2=1;

	while (loc2>=0) {
		loc2=cmdLineMode.Find(",",loc);
		if (loc2<0)
			PlayerModeStr = cmdLineMode.substr(loc,loc2-loc);
		else
			PlayerModeStr = cmdLineMode.substr(loc,loc2-loc);

		if (PlayerModeStr.CompareNoCase("1")==0) {
			GAMESTATE->m_CurStyle = STYLE_DANCE_SINGLE;
			GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;
			GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
		} else if (PlayerModeStr.CompareNoCase("2")==0) {
			GAMESTATE->m_CurStyle = STYLE_DANCE_SINGLE;
			GAMESTATE->m_bSideIsJoined[PLAYER_2] = true;
			GAMESTATE->m_MasterPlayerNumber = PLAYER_2;
		} else if (PlayerModeStr.CompareNoCase("VERSUS")==0) {
			GAMESTATE->m_CurStyle = STYLE_DANCE_VERSUS;
			GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;
			GAMESTATE->m_bSideIsJoined[PLAYER_2] = true;
		} else if (PlayerModeStr.CompareNoCase("VERSUS")==0) {
			GAMESTATE->m_CurStyle = STYLE_DANCE_VERSUS;
			GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;
			GAMESTATE->m_bSideIsJoined[PLAYER_2] = true;		
		} else if (PlayerModeStr.CompareNoCase("DOUBLE")==0) {
			GAMESTATE->m_CurStyle = STYLE_DANCE_DOUBLE;
			GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;
			GAMESTATE->m_bSideIsJoined[PLAYER_2] = true;
		} else if (PlayerModeStr.CompareNoCase("COUPLE")==0) {
			GAMESTATE->m_CurStyle = STYLE_DANCE_COUPLE;
			GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;
			GAMESTATE->m_bSideIsJoined[PLAYER_2] = true;			
		} else if (PlayerModeStr.CompareNoCase("FAILOFF")==0) {
			GAMESTATE->m_SongOptions.m_FailType = SongOptions::FAIL_OFF;
		} else if (PlayerModeStr.CompareNoCase("FAILENDOFSONG")==0) {
			GAMESTATE->m_SongOptions.m_FailType = SongOptions::FAIL_END_OF_SONG;
		} else if (PlayerModeStr.CompareNoCase("FAILARCADE")==0) {
			GAMESTATE->m_SongOptions.m_FailType = SongOptions::FAIL_ARCADE;
		} else if (PlayerModeStr.CompareNoCase("ARCADE")==0) {
			GAMESTATE->m_PlayMode = PLAY_MODE_ARCADE;
		} else if (PlayerModeStr.CompareNoCase("NONSTOP")==0) {
			GAMESTATE->m_PlayMode = PLAY_MODE_NONSTOP;
		} else if (PlayerModeStr.CompareNoCase("ONI")==0) {
			GAMESTATE->m_PlayMode = PLAY_MODE_ONI;
		} else if (PlayerModeStr.CompareNoCase("BATTLE")==0) {
			GAMESTATE->m_PlayMode = PLAY_MODE_BATTLE;
		} else if (PlayerModeStr.CompareNoCase("RAVE")==0) {
			GAMESTATE->m_PlayMode = PLAY_MODE_RAVE;
		} 
		loc = loc2+1;
	}
}


//I am adding this for Network support
//Feel free to change the way --course is used
//I am not building any part of SMOnline specific to this yet.

void ArgStartCourse(CString CourseName)
{
	Course * desCourse = SONGMAN->GetCourseFromName(CourseName);
	LOG->Info ("Course Desied: %s",CourseName.c_str());
	if (desCourse == 0)
	{
		LOG->Info ("Desired Course not found!");
		SCREENMAN->SystemMessage("Failed to find course");
		return ;
	}


	/*
		--PLAYERS=
			1		-> STYLE_DANCE_SINGLE, (Default)
			2		-> STYLE_DANCE_SINGLE,
			VERSUS	-> STYLE_DANCE_VERSUS,
			DOUBLE	-> STYLE_DANCE_DOUBLE,
			COUPLE	-> STYLE_DANCE_COUPLE,
	*/
	if ((!GAMESTATE->m_bSideIsJoined[PLAYER_1]) && (!GAMESTATE->m_bSideIsJoined[PLAYER_2]))
	{
		GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
		GAMESTATE->m_CurStyle = STYLE_DANCE_SINGLE;
		GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;	//Default
			//Also take Default Fail Type
	}

	GAMESTATE->m_pCurCourse = desCourse;

	GAMESTATE->m_PlayMode = desCourse->GetPlayMode();
	if (desCourse->IsOni())
		GAMESTATE->m_SongOptions.m_LifeType = SongOptions::LIFE_BATTERY;
	else
		GAMESTATE->m_SongOptions.m_LifeType = SongOptions::LIFE_BAR;

	GAMESTATE->m_SongOptions.m_iBatteryLives = desCourse->m_iLives;

	GAMESTATE->PlayersFinalized();

	//Go to Gameplay Screen
	SCREENMAN->SetNewScreen(NEXT_SCREEN);

	//Not sure if this is correct, it may make more sense
	//to have a metric to allow/disallow user options at this point.

	GAMESTATE->BeginGame();
	GAMESTATE->BeginStage();
}

	
void NetworkSyncManager::DisplayStartupStatus()
{
	CString sMessage("");

	switch (m_startupStatus)
	{
	case 0:
		//Networking wasn't attepmpted
		return;
		break;
	case 1:
		sMessage = "Connect to server successful.";
		break;
	case 2:
		sMessage = "Connection failed.";
		break;
	}
	SCREENMAN->SystemMessage(sMessage);
}


//Global and accessable from anywhere
NetworkSyncManager *NSMAN;

