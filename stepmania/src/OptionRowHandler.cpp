#include "global.h"
#include "OptionRowHandler.h"
#include "LuaManager.h"
#include "ScreenOptionsMasterPrefs.h"
#include "NoteSkinManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "GameState.h"
#include "Course.h"
#include "Steps.h"
#include "Style.h"
#include "song.h"
#include "SongManager.h"
#include "Character.h"
#include "PrefsManager.h"
#include "StepsUtil.h"
#include "GameManager.h"
#include "Foreach.h"

#define ENTRY(s)					THEME->GetMetric ("ScreenOptionsMaster",s)
#define ENTRY_NAME(s)				THEME->GetMetric ("OptionNames", s)
#define ENTRY_MODE(s,i)				THEME->GetMetric ("ScreenOptionsMaster",ssprintf("%s,%i",(s).c_str(),(i+1)))
#define ENTRY_DEFAULT(s)			THEME->GetMetric ("ScreenOptionsMaster",(s) + "Default")


static void SelectExactlyOne( int iSelection, vector<bool> &vbSelectedOut )
{
	for( int i=0; i<(int)vbSelectedOut.size(); i++ )
		vbSelectedOut[i] = i==iSelection;
}

static int GetOneSelection( const vector<bool> &vbSelected )
{
	for( unsigned i=0; i<vbSelected.size(); i++ )
		if( vbSelected[i] )
			return i;
	ASSERT(0);	// shouldn't call this if not expecting one to be selected
	return -1;
}


class OptionRowHandlerList : public OptionRowHandler
{
public:
	vector<GameCommand> ListEntries;
	GameCommand Default;
	bool m_bUseModNameForIcon;

	OptionRowHandlerList::OptionRowHandlerList() { Init(); }
	virtual void Init()
	{
		OptionRowHandler::Init();
		ListEntries.clear();
		Default.Init();
		m_bUseModNameForIcon = false;
	}
	virtual void ImportOption( const OptionRowDefinition &row, PlayerNumber pn, vector<bool> &vbSelectedOut ) const;
	virtual int ExportOption( const OptionRowDefinition &def, PlayerNumber pn, const vector<bool> &vbSelected ) const;
	virtual CString GetIconText( const OptionRowDefinition &def, int iFirstSelection ) const
	{
		return m_bUseModNameForIcon ?
			ListEntries[iFirstSelection].m_sModifiers :
			def.choices[iFirstSelection];
	}
	virtual CString GetAndEraseScreen( int iChoice )
	{ 
		GameCommand &mc = ListEntries[iChoice];
		if( mc.m_sScreen != "" )
		{
			/* Hack: instead of applying screen commands here, store them in
			* m_sNextScreen and apply them after we tween out.  If we don't set
			* m_sScreen to "", we'll load it twice (once for each player) and
			* then again for m_sNextScreen. */
			CString sNextScreen = mc.m_sScreen;
			mc.m_sScreen = "";
			return sNextScreen;
		}
		return "";
	}

	void FillList( OptionRowDefinition &defOut, CString param );
	void FillSteps( OptionRowDefinition &defOut );
	void FillCharacters( OptionRowDefinition &defOut );
	void FillStyles( OptionRowDefinition &defOut );
	void FillGroups( OptionRowDefinition &defOut );
	void FillDifficulties( OptionRowDefinition &defOut );
};

class OptionRowHandlerLua : public OptionRowHandler
{
public:
	LuaExpression *m_pLuaTable;

	OptionRowHandlerLua::OptionRowHandlerLua() { m_pLuaTable = NULL; Init(); }
	void Init()
	{
		OptionRowHandler::Init();
		delete m_pLuaTable;
		m_pLuaTable = new LuaExpression;
	}
	virtual void ImportOption( const OptionRowDefinition &row, PlayerNumber pn, vector<bool> &vbSelectedOut ) const;
	virtual int ExportOption( const OptionRowDefinition &def, PlayerNumber pn, const vector<bool> &vbSelected ) const;
	virtual void Reload( OptionRowDefinition &defOut );

	void FillLua( OptionRowDefinition &defOut, CString sLuaFunction );
};

class OptionRowHandlerConfig : public OptionRowHandler
{
public:
	const ConfOption *opt;

	OptionRowHandlerConfig::OptionRowHandlerConfig() { Init(); }
	void Init()
	{
		OptionRowHandler::Init();
		opt = NULL;
	}
	virtual void ImportOption( const OptionRowDefinition &row, PlayerNumber pn, vector<bool> &vbSelectedOut ) const;
	virtual int ExportOption( const OptionRowDefinition &def, PlayerNumber pn, const vector<bool> &vbSelected ) const;

	void FillConfig( OptionRowDefinition &defOut, CString param );
};


void OptionRowHandlerList::ImportOption( const OptionRowDefinition &row, PlayerNumber pn, vector<bool> &vbSelectedOut ) const
{
	int FallbackOption = -1;
	bool UseFallbackOption = true;

	for( unsigned e = 0; e < ListEntries.size(); ++e )
	{
		const GameCommand &mc = ListEntries[e];

		vbSelectedOut[e] = false;

		if( mc.IsZero() )
		{
			/* The entry has no effect.  This is usually a default "none of the
				* above" entry.  It will always return true for DescribesCurrentMode().
				* It's only the selected choice if nothing else matches. */
			if( row.selectType != SELECT_MULTIPLE )
				FallbackOption = e;
			continue;
		}

		if( row.bOneChoiceForAllPlayers )
		{
			if( mc.DescribesCurrentModeForAllPlayers() )
			{
				UseFallbackOption = false;
				if( row.selectType != SELECT_MULTIPLE )
					SelectExactlyOne( e, vbSelectedOut );
				else
					vbSelectedOut[e] = true;
			}
		}
		else
		{
			if( mc.DescribesCurrentMode(  pn) )
			{
				UseFallbackOption = false;
				if( row.selectType != SELECT_MULTIPLE )
					SelectExactlyOne( e, vbSelectedOut );
				else
					vbSelectedOut[e] = true;
			}
		}
	}

	if( row.selectType == SELECT_ONE && 
		UseFallbackOption && 
		FallbackOption != -1 )
	{
		SelectExactlyOne( FallbackOption, vbSelectedOut );
	}
}

int OptionRowHandlerList::ExportOption( const OptionRowDefinition &def, PlayerNumber pn, const vector<bool> &vbSelected ) const
{
	Default.Apply( pn );
	for( unsigned i=0; i<vbSelected.size(); i++ )
		if( vbSelected[i] )
			ListEntries[i].Apply( pn );
	return 0;
}


void OptionRowHandlerLua::ImportOption( const OptionRowDefinition &row, PlayerNumber pn, vector<bool> &vbSelectedOut ) const
{
	ASSERT( lua_gettop(LUA->L) == 0 );

	/* Evaluate the LoadSelections(self,array,pn) function, where array is a table
		* representing vbSelectedOut. */

	/* All selections default to false. */
	for( unsigned i = 0; i < vbSelectedOut.size(); ++i )
		vbSelectedOut[i] = false;

	/* Create the vbSelectedOut table. */
	LUA->CreateTableFromArrayB( vbSelectedOut );
	ASSERT( lua_gettop(LUA->L) == 1 ); /* vbSelectedOut table */

	/* Get the function to call from m_LuaTable. */
	m_pLuaTable->PushSelf( LUA->L );
	ASSERT( lua_istable( LUA->L, -1 ) );

	lua_pushstring( LUA->L, "LoadSelections" );
	lua_gettable( LUA->L, -2 );
	if( !lua_isfunction( LUA->L, -1 ) )
		RageException::Throw( "\"%s\" \"LoadSelections\" entry is not a function", row.name.c_str() );

	/* Argument 1 (self): */
	m_pLuaTable->PushSelf( LUA->L );

	/* Argument 2 (vbSelectedOut): */
	lua_pushvalue( LUA->L, 1 );

	/* Argument 3 (pn): */
	LUA->PushStack( (int) pn );

	ASSERT( lua_gettop(LUA->L) == 6 ); /* vbSelectedOut, m_iLuaTable, function, self, arg, arg */

	lua_call( LUA->L, 3, 0 ); // call function with 3 arguments and 0 results
	ASSERT( lua_gettop(LUA->L) == 2 );

	lua_pop( LUA->L, 1 ); /* pop option table */

	LUA->ReadArrayFromTableB( vbSelectedOut );
	
	lua_pop( LUA->L, 1 ); /* pop vbSelectedOut table */

	ASSERT( lua_gettop(LUA->L) == 0 );
}

int OptionRowHandlerLua::ExportOption( const OptionRowDefinition &def, PlayerNumber pn, const vector<bool> &vbSelected ) const
{
	ASSERT( lua_gettop(LUA->L) == 0 );

	/* Evaluate SaveSelections(self,array,pn) function, where array is a table
		* representing vbSelectedOut. */

	vector<bool> vbSelectedCopy = vbSelected;

	/* Create the vbSelectedOut table. */
	LUA->CreateTableFromArrayB( vbSelectedCopy );
	ASSERT( lua_gettop(LUA->L) == 1 ); /* vbSelectedOut table */

	/* Get the function to call. */
	m_pLuaTable->PushSelf( LUA->L );
	ASSERT( lua_istable( LUA->L, -1 ) );

	lua_pushstring( LUA->L, "SaveSelections" );
	lua_gettable( LUA->L, -2 );
	if( !lua_isfunction( LUA->L, -1 ) )
		RageException::Throw( "\"%s\" \"SaveSelections\" entry is not a function", def.name.c_str() );

	/* Argument 1 (self): */
	m_pLuaTable->PushSelf( LUA->L );

	/* Argument 2 (vbSelectedOut): */
	lua_pushvalue( LUA->L, 1 );

	/* Argument 3 (pn): */
	LUA->PushStack( (int) pn );

	ASSERT( lua_gettop(LUA->L) == 6 ); /* vbSelectedOut, m_iLuaTable, function, self, arg, arg */

	lua_call( LUA->L, 3, 0 ); // call function with 3 arguments and 0 results
	ASSERT( lua_gettop(LUA->L) == 2 );

	lua_pop( LUA->L, 1 ); /* pop option table */
	lua_pop( LUA->L, 1 ); /* pop vbSelected table */

	ASSERT( lua_gettop(LUA->L) == 0 );

	// XXX: allow specifying the mask
	return 0;
}

void OptionRowHandlerLua::Reload( OptionRowDefinition &defOut )
{
	FillLua( defOut, m_sName );
}


void OptionRowHandlerConfig::ImportOption( const OptionRowDefinition &row, PlayerNumber pn, vector<bool> &vbSelectedOut ) const
{
	int iSelection = opt->Get();
	SelectExactlyOne( iSelection, vbSelectedOut );
}

int OptionRowHandlerConfig::ExportOption( const OptionRowDefinition &def, PlayerNumber pn, const vector<bool> &vbSelected ) const
{
	int sel = GetOneSelection(vbSelected);

	/* Get the original choice. */
	int Original = opt->Get();

	/* Apply. */
	opt->Put( sel );

	/* Get the new choice. */
	int New = opt->Get();

	/* If it didn't change, don't return any side-effects. */
	if( Original == New )
		return 0;

	return opt->GetEffects();
}

///////////////////////////////////////////////////////////////////////////////////


/* Add the list named "ListName" to the given row/handler. */
void OptionRowHandlerList::FillList( OptionRowDefinition &row, CString sListName )
{
	Init();
	row.Init();

	m_sName = sListName;
	m_bUseModNameForIcon = true;
		
	row.name = sListName;
	if( !sListName.CompareNoCase("noteskins") )
	{
		Default.Init(); /* none */
		row.bOneChoiceForAllPlayers = false;

		CStringArray arraySkinNames;
		NOTESKIN->GetNoteSkinNames( arraySkinNames );
		for( unsigned skin=0; skin<arraySkinNames.size(); skin++ )
		{
			arraySkinNames[skin].MakeUpper();

			GameCommand mc;
			mc.m_sModifiers = arraySkinNames[skin];
			ListEntries.push_back( mc );
			row.choices.push_back( arraySkinNames[skin] );
		}
		return;
	}

	Default.Load( -1, ParseCommands(ENTRY_DEFAULT(sListName)) );

	/* Parse the basic configuration metric. */
	Commands cmds = ParseCommands( ENTRY(sListName) );
	if( cmds.v.size() < 1 )
		RageException::Throw( "Parse error in OptionRowHandlerUtilEntries::ListName%s", sListName.c_str() );

	row.bOneChoiceForAllPlayers = false;
	const int NumCols = atoi( cmds.v[0].m_vsArgs[0] );
	for( unsigned i=1; i<cmds.v.size(); i++ )
	{
		const Command &cmd = cmds.v[i];
		CString sName = cmd.GetName();

		if(		 sName == "together" )			row.bOneChoiceForAllPlayers = true;
		else if( sName == "selectmultiple" )	row.selectType = SELECT_MULTIPLE;
		else if( sName == "selectnone" )		row.selectType = SELECT_NONE;
		else if( sName == "showoneinrow" )		row.layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		else if( sName == "reloadrownames" )
		{
			for( unsigned a=1; a<cmd.m_vsArgs.size(); a++ )
				m_vsRefreshRowNames.push_back( cmd.m_vsArgs[a] );
		}
		else if( sName == "enabledforplayers" )
		{
			row.m_vEnabledForPlayers.clear();
			for( unsigned a=1; a<cmd.m_vsArgs.size(); a++ )
			{
				CString sArg = cmd.m_vsArgs[a];
				PlayerNumber pn = (PlayerNumber)(atoi(sArg)-1);
				ASSERT( pn >= 0 && pn < NUM_PLAYERS );
				row.m_vEnabledForPlayers.insert( pn );
			}
		}
		else if( sName == "exportonchange" )	m_bExportOnChange = true;
		else		RageException::Throw( "Unkown row flag \"%s\"", sName.c_str() );
	}

	for( int col = 0; col < NumCols; ++col )
	{
		GameCommand mc;
		mc.Load( 0, ParseCommands(ENTRY_MODE(sListName, col)) );
		if( mc.m_sName == "" )
			RageException::Throw( "List \"%s\", col %i has no name", sListName.c_str(), col );

		if( !mc.IsPlayable() )
			continue;

		ListEntries.push_back( mc );

		CString sName = mc.m_sName;
		CString sChoice = ENTRY_NAME(mc.m_sName);
		row.choices.push_back( sChoice );
	}
}

void OptionRowHandlerLua::FillLua( OptionRowDefinition &row, CString sLuaFunction )
{
	Init();
	row.Init();

	m_sName = sLuaFunction;
//	m_bUseModNameForIcon = true;

	/* Run the Lua expression.  It should return a table. */
	m_pLuaTable->SetFromExpression( sLuaFunction );

	if( m_pLuaTable->GetLuaType() != LUA_TTABLE )
		RageException::Throw( "Result of \"%s\" is not a table", sLuaFunction.c_str() );

	{
		m_pLuaTable->PushSelf( LUA->L );

		lua_pushstring( LUA->L, "Name" );
		lua_gettable( LUA->L, -2 );
		const char *pStr = lua_tostring( LUA->L, -1 );
		if( pStr == NULL )
			RageException::Throw( "\"%s\" \"Name\" entry is not a string", sLuaFunction.c_str() );
		row.name = pStr;
		lua_pop( LUA->L, 1 );


		lua_pushstring( LUA->L, "OneChoiceForAllPlayers" );
		lua_gettable( LUA->L, -2 );
		row.bOneChoiceForAllPlayers = !!lua_toboolean( LUA->L, -1 );
		lua_pop( LUA->L, 1 );


		lua_pushstring( LUA->L, "LayoutType" );
		lua_gettable( LUA->L, -2 );
		pStr = lua_tostring( LUA->L, -1 );
		if( pStr == NULL )
			RageException::Throw( "\"%s\" \"LayoutType\" entry is not a string", sLuaFunction.c_str() );
		row.layoutType = StringToLayoutType( pStr );
		ASSERT( row.layoutType != LAYOUT_INVALID );
		lua_pop( LUA->L, 1 );


		lua_pushstring( LUA->L, "SelectType" );
		lua_gettable( LUA->L, -2 );
		pStr = lua_tostring( LUA->L, -1 );
		if( pStr == NULL )
			RageException::Throw( "\"%s\" \"SelectType\" entry is not a string", sLuaFunction.c_str() );
		row.selectType = StringToSelectType( pStr );
		ASSERT( row.selectType != SELECT_INVALID );
		lua_pop( LUA->L, 1 );


		/* Iterate over the "Choices" table. */
		lua_pushstring( LUA->L, "Choices" );
		lua_gettable( LUA->L, -2 );
		if( !lua_istable( LUA->L, -1 ) )
			RageException::Throw( "\"%s\" \"Choices\" is not a table", sLuaFunction.c_str() );

		lua_pushnil( LUA->L );
		while( lua_next(LUA->L, -2) != 0 )
		{
			/* `key' is at index -2 and `value' at index -1 */
			const char *pValue = lua_tostring( LUA->L, -1 );
			if( pValue == NULL )
				RageException::Throw( "\"%s\" Column entry is not a string", sLuaFunction.c_str() );
			LOG->Trace( "'%s'", pValue);

			row.choices.push_back( pValue );

			lua_pop( LUA->L, 1 );  /* removes `value'; keeps `key' for next iteration */
		}

		lua_pop( LUA->L, 1 ); /* pop choices table */


		/* Iterate over the "EnabledForPlayers" table. */
		lua_pushstring( LUA->L, "EnabledForPlayers" );
		lua_gettable( LUA->L, -2 );
		if( !lua_isnil( LUA->L, -1 ) )
		{
			if( !lua_istable( LUA->L, -1 ) )
				RageException::Throw( "\"%s\" \"EnabledForPlayers\" is not a table", sLuaFunction.c_str() );

			row.m_vEnabledForPlayers.clear();	// and fill in with supplied PlayerNumbers below

			lua_pushnil( LUA->L );
			while( lua_next(LUA->L, -2) != 0 )
			{
				/* `key' is at index -2 and `value' at index -1 */
				PlayerNumber pn = (PlayerNumber)luaL_checkint( LUA->L, -1 );

				row.m_vEnabledForPlayers.insert( pn );

				lua_pop( LUA->L, 1 );  /* removes `value'; keeps `key' for next iteration */
			}
		}
		lua_pop( LUA->L, 1 ); /* pop EnabledForPlayers table */

		
		/* Look for "ExportOnChange" value. */
		lua_pushstring( LUA->L, "ExportOnChange" );
		lua_gettable( LUA->L, -2 );
		if( !lua_isnil( LUA->L, -1 ) )
		{
			m_bExportOnChange = !!MyLua_checkboolean( LUA->L, -1 );
		}
		lua_pop( LUA->L, 1 ); /* pop ExportOnChange value */

		
		/* Iterate over the "RefreshRowNames" table. */
		lua_pushstring( LUA->L, "RefreshRowNames" );
		lua_gettable( LUA->L, -2 );
		if( !lua_isnil( LUA->L, -1 ) )
		{
			if( !lua_istable( LUA->L, -1 ) )
				RageException::Throw( "\"%s\" \"RefreshRowNames\" is not a table", sLuaFunction.c_str() );

			m_vsRefreshRowNames.clear();	// and fill in with supplied PlayerNumbers below

			lua_pushnil( LUA->L );
			while( lua_next(LUA->L, -2) != 0 )
			{
				/* `key' is at index -2 and `value' at index -1 */
				const char *pValue = lua_tostring( LUA->L, -1 );
				if( pValue == NULL )
					RageException::Throw( "\"%s\" Column entry is not a string", sLuaFunction.c_str() );
				LOG->Trace( "'%s'", pValue);

				m_vsRefreshRowNames.push_back( pValue );

				lua_pop( LUA->L, 1 );  /* removes `value'; keeps `key' for next iteration */
			}
		}
		lua_pop( LUA->L, 1 ); /* pop RefreshRowNames table */


		lua_pop( LUA->L, 1 ); /* pop main table */
		ASSERT( lua_gettop(LUA->L) == 0 );
	}
}


/* Add a list of difficulties/edits to the given row/handler. */
void OptionRowHandlerList::FillSteps( OptionRowDefinition &row )
{
	Init();
	row.Init();

	row.name = "Steps";
	row.bOneChoiceForAllPlayers = false;

	// fill in difficulty names
	if( GAMESTATE->m_bEditing )
	{
		row.choices.push_back( "" );
		ListEntries.push_back( GameCommand() );
	}
	else if( GAMESTATE->IsCourseMode() )   // playing a course
	{
		row.bOneChoiceForAllPlayers = PREFSMAN->m_bLockCourseDifficulties;

		vector<Trail*> vTrails;
		GAMESTATE->m_pCurCourse->GetTrails( vTrails, GAMESTATE->GetCurrentStyle()->m_StepsType );
		for( unsigned i=0; i<vTrails.size(); i++ )
		{
			Trail* pTrail = vTrails[i];

			CString s = CourseDifficultyToThemedString( pTrail->m_CourseDifficulty );
			row.choices.push_back( s );
			GameCommand mc;
			mc.m_pTrail = pTrail;
			ListEntries.push_back( mc );
		}
	}
	else // !GAMESTATE->IsCourseMode(), playing a song
	{
		vector<Steps*> vSteps;
		GAMESTATE->m_pCurSong->GetSteps( vSteps, GAMESTATE->GetCurrentStyle()->m_StepsType );
		StepsUtil::SortNotesArrayByDifficulty( vSteps );
		for( unsigned i=0; i<vSteps.size(); i++ )
		{
			Steps* pSteps = vSteps[i];

			CString s;
			if( pSteps->GetDifficulty() == DIFFICULTY_EDIT )
				s = pSteps->GetDescription();
			else
				s = DifficultyToThemedString( pSteps->GetDifficulty() );
			s += ssprintf( " (%d)", pSteps->GetMeter() );

			row.choices.push_back( s );
			GameCommand mc;
			mc.m_pSteps = pSteps;
			mc.m_dc = pSteps->GetDifficulty();
			ListEntries.push_back( mc );
		}
	}
}


/* Add the given configuration value to the given row/handler. */
void OptionRowHandlerConfig::FillConfig( OptionRowDefinition &row, CString param )
{
	Init();
	row.Init();

	/* Configuration values are never per-player. */
	row.bOneChoiceForAllPlayers = true;

	ConfOption *pConfOption = ConfOption::Find( param );
	if( pConfOption == NULL )
		RageException::Throw( "Invalid Conf type \"%s\"", param.c_str() );

	pConfOption->UpdateAvailableOptions();

	opt = pConfOption;
	opt->MakeOptionsList( row.choices );

	row.name = opt->name;
}

/* Add a list of available characters to the given row/handler. */
void OptionRowHandlerList::FillCharacters( OptionRowDefinition &row )
{
	Init();
	row.Init();

	row.bOneChoiceForAllPlayers = false;
	row.name = "Characters";
	Default.m_pCharacter = GAMESTATE->GetDefaultCharacter();

	{
		row.choices.push_back( ENTRY_NAME("Off") );
		GameCommand mc;
		mc.m_pCharacter = NULL;
		ListEntries.push_back( mc );
	}

	vector<Character*> apCharacters;
	GAMESTATE->GetCharacters( apCharacters );
	for( unsigned i=0; i<apCharacters.size(); i++ )
	{
		Character* pCharacter = apCharacters[i];
		CString s = pCharacter->m_sName;
		s.MakeUpper();

		row.choices.push_back( s ); 
		GameCommand mc;
		mc.m_pCharacter = pCharacter;
		ListEntries.push_back( mc );
	}
}

/* Add a list of available styles to the given row/handler. */
void OptionRowHandlerList::FillStyles( OptionRowDefinition &row )
{
	Init();
	row.Init();

	row.bOneChoiceForAllPlayers = true;
	row.name = "Style";
	row.bOneChoiceForAllPlayers = true;

	vector<const Style*> vStyles;
	GAMEMAN->GetStylesForGame( GAMESTATE->m_pCurGame, vStyles );
	ASSERT( vStyles.size() );
	FOREACH_CONST( const Style*, vStyles, s )
	{
		row.choices.push_back( GAMEMAN->StyleToThemedString(*s) ); 
		GameCommand mc;
		mc.m_pStyle = *s;
		ListEntries.push_back( mc );
	}

	Default.m_pStyle = vStyles[0];
}

/* Add a list of available song groups to the given row/handler. */
void OptionRowHandlerList::FillGroups( OptionRowDefinition &row )
{
	Init();
	row.Init();

	row.bOneChoiceForAllPlayers = true;
	row.name = "Group";
	Default.m_sSongGroup = GROUP_ALL_MUSIC;

	vector<CString> vGroups;
	SONGMAN->GetGroupNames( vGroups );
	ASSERT( vGroups.size() );

	{
		row.choices.push_back( ENTRY_NAME("AllGroups") );
		GameCommand mc;
		mc.m_sSongGroup = GROUP_ALL_MUSIC;
		ListEntries.push_back( mc );
	}

	FOREACH_CONST( CString, vGroups, g )
	{
		row.choices.push_back( *g ); 
		GameCommand mc;
		mc.m_sSongGroup = *g;
		ListEntries.push_back( mc );
	}
}

/* Add a list of available difficulties to the given row/handler. */
void OptionRowHandlerList::FillDifficulties( OptionRowDefinition &row )
{
	Init();
	row.Init();

	set<Difficulty> vDifficulties;
	GAMESTATE->GetDifficultiesToShow( vDifficulties );

	row.bOneChoiceForAllPlayers = true;
	row.name = "Difficulty";
	Default.m_dc = DIFFICULTY_INVALID;

	{
		row.choices.push_back( ENTRY_NAME("AllDifficulties") );
		GameCommand mc;
		mc.m_dc = DIFFICULTY_INVALID;
		ListEntries.push_back( mc );
	}

	FOREACHS_CONST( Difficulty, vDifficulties, d )
	{
		CString s = DifficultyToThemedString( *d );

		row.choices.push_back( s ); 
		GameCommand mc;
		mc.m_dc = *d;
		ListEntries.push_back( mc );
	}
}


OptionRowHandler* OptionRowHandlerUtil::Make( const Command &command, OptionRowDefinition &defOut )
{
	OptionRowHandler* pHand = NULL;

	BeginHandleArgs;

	const CString &name = command.GetName();

	if(		 name == "list" )			{ OptionRowHandlerList *p = new OptionRowHandlerList;		p->FillList( defOut, sArg(1) );		pHand = p; }
	else if( name == "lua" )			{ OptionRowHandlerLua *p = new OptionRowHandlerLua;			p->FillLua( defOut, sArg(1) );		pHand = p; }
	else if( name == "steps" )			{ OptionRowHandlerList *p = new OptionRowHandlerList;		p->FillSteps( defOut );				pHand = p; }
	else if( name == "conf" )			{ OptionRowHandlerConfig *p = new OptionRowHandlerConfig;	p->FillConfig( defOut, sArg(1) );	pHand = p; }
	else if( name == "characters" )		{ OptionRowHandlerList *p = new OptionRowHandlerList;		p->FillCharacters( defOut );		pHand = p; }
	else if( name == "styles" )			{ OptionRowHandlerList *p = new OptionRowHandlerList;		p->FillStyles( defOut );			pHand = p; }
	else if( name == "groups" )			{ OptionRowHandlerList *p = new OptionRowHandlerList;		p->FillGroups( defOut );			pHand = p; }
	else if( name == "difficulties" )	{ OptionRowHandlerList *p = new OptionRowHandlerList;		p->FillDifficulties( defOut );		pHand = p; }

	EndHandleArgs;

	return pHand;
}


/*
 * (c) 2002-2004 Chris Danford
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
