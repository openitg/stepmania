#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: BGAnimation

 Desc: Particles used initially for background effects

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Ben Nordstrom
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "BGAnimationLayer.h"
#include "PrefsManager.h"
#include "GameState.h"
#include "IniFile.h"
#include "RageMath.h"
#include "SDL_utils.h"
#include <math.h>
#include "RageTimer.h"
#include "RageLog.h"
#include "song.h"
#include "ThemeManager.h"
#include "ActorCollision.h"
#include "Sprite.h"
#include "RageDisplay.h"
#include "ActorUtil.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "RageTextureManager.h"
#include "RageFile.h"


const float PARTICLE_SPEED = 300;

const float SPIRAL_MAX_ZOOM = 2;
const float SPIRAL_MIN_ZOOM = 0.3f;

#define MAX_TILES_WIDE (SCREEN_WIDTH/32+2)
#define MAX_TILES_HIGH (SCREEN_HEIGHT/32+2)
#define MAX_SPRITES (MAX_TILES_WIDE*MAX_TILES_HIGH)

static const RectI FullScreenRectI(SCREEN_LEFT,SCREEN_TOP,SCREEN_RIGHT,SCREEN_BOTTOM);


BGAnimationLayer::BGAnimationLayer( bool Generic )
{
	/* If Generic is false, this is a layer in a real BGA--one that was loaded
	 * by simply constructing a BGAnimation.  These normally have a position
	 * of 0,0 (top-left of the screen).  Loaded images are given a default position
	 * of 320x240, centered in the screen.  Additionally, the "On" command will
	 * be run automatically.  Example:
	 *
	 * BGAnimation bga;
	 * bga.Load( path );
	 * this->AddChind( &bga );
	 *
	 * If Generic is true, then we act like any other actor.  We assume we don't
	 * know anything about where we're positioned.  Loaded images are given a
	 * default position of 0x0.  The "On" command is not run; we'll receive that
	 * from the owner through COMMAND().  Example:
	 * 
	 * AutoActor image;
	 * image.Load( path );
	 * ON_COMMAND( image );
	 * this->AddChind( &image );
	 *
	 * TYPE_PARTICLES and TYPE_TILES are currently not supported in this mode.
	 * 
	 */
	m_bGeneric = Generic;

	Init();
}

BGAnimationLayer::~BGAnimationLayer()
{
	Unload();
}

void BGAnimationLayer::Unload()
{
	for( unsigned i=0; i<m_pActors.size(); i++ )
		delete m_pActors[i];
	m_pActors.clear();
}

void BGAnimationLayer::Init()
{
	Unload();

	m_fRepeatCommandEverySeconds = -1;
	m_fSecondsUntilNextCommand = 0;
	m_fUpdateRate = 1;
	m_fFOV = -1;	// no change
	m_bLighting = false;

//	m_bCycleColor = false;
//	m_bCycleAlpha = false;
//	m_Effect = EFFECT_STRETCH_STILL;

	m_vParticleVelocity.clear();

	m_Type = TYPE_SPRITE;

	m_fTexCoordVelocityX = 0;
	m_fTexCoordVelocityY = 0;
	m_fZoomMin = 1;
	m_fZoomMax = 1;
	m_fVelocityXMin = 10;
	m_fVelocityXMax = 10;
	m_fVelocityYMin = 0;
	m_fVelocityYMax = 0;
	m_fVelocityZMin = 0;
	m_fVelocityZMax = 0;
	m_fOverrideSpeed = 0;
	m_iNumParticles = 10;
	m_bParticlesBounce = false;
	m_iNumTilesWide = -1;
	m_iNumTilesHigh = -1;
	m_fTilesStartX = 0;
	m_fTilesStartY = 0;
	m_fTilesSpacingX = -1;
	m_fTilesSpacingY = -1;
	m_fTileVelocityX = 0;
	m_fTileVelocityY = 0;

	/*
	m_PosX = m_PosY = 0;
	m_Zoom = 0;
	m_Rot = 0;
	m_ShowTime = 0;
	m_HideTime = 0;
	m_TweenStartTime = 0;
	m_TweenX = m_TweenY = 0.0;
	m_TweenSpeed = 0;
	m_TweenState = 0;
	m_TweenPassedX = m_TweenPassedY = 0;
	*/
}

/* Static background layers are simple, uncomposited background images with nothing
 * behind them.  Since they have nothing behind them, they have no need for alpha,
 * so turn that off. */
void BGAnimationLayer::LoadFromStaticGraphic( CString sPath )
{
	Init();
	RageTextureID ID(sPath);
	ID.iAlphaBits = 0;
	Sprite* pSprite = new Sprite;
	pSprite->LoadBG( ID );
	pSprite->StretchTo( FullScreenRectI );
	m_pActors.push_back( pSprite );
}

void BGAnimationLayer::LoadFromMovie( CString sMoviePath )
{
	Init();
	Sprite* pSprite = new Sprite;
	pSprite->LoadBG( sMoviePath );
	pSprite->StretchTo( FullScreenRectI );
	pSprite->GetTexture()->Pause();
	m_pActors.push_back( pSprite );
}

void BGAnimationLayer::LoadFromVisualization( CString sMoviePath )
{
	Init();
	Sprite* pSprite = new Sprite;
	m_pActors.push_back( pSprite );
	pSprite->LoadBG( sMoviePath );
	pSprite->StretchTo( FullScreenRectI );
	pSprite->SetBlendMode( BLEND_ADD );
}


void BGAnimationLayer::LoadFromAniLayerFile( CString sPath )
{
	/* Generic BGAs are new.  Animation directories with no INI are old and obsolete. 
	 * Don't combine them. */
	ASSERT( !m_bGeneric );
	Init();
	CString lcPath = sPath;
	lcPath.MakeLower();

	if( lcPath.Find("usesongbg") != -1 )
	{
		const Song* pSong = GAMESTATE->m_pCurSong;
		CString sSongBGPath;
		if( pSong && pSong->HasBackground() )
			sSongBGPath = pSong->GetBackgroundPath();
		else
			sSongBGPath = THEME->GetPathToG("Common fallback background");

		LoadFromStaticGraphic( sSongBGPath );
		return;		// this will ignore other effects in the file name
	}

	const CString EFFECT_STRING[NUM_EFFECTS] = {
		"center",
		"stretchstill",
		"stretchscrollleft",
		"stretchscrollright",
		"stretchscrollup",
		"stretchscrolldown",
		"stretchwater",
		"stretchbubble",
		"stretchtwist",
		"stretchspin",
		"particlesspiralout",
		"particlesspiralin",
		"particlesfloatup",
		"particlesfloatdown",
		"particlesfloatleft",
		"particlesfloatright",
		"particlesbounce",
		"tilestill",
		"tilescrollleft",
		"tilescrollright",
		"tilescrollup",
		"tilescrolldown",
		"tileflipx",
		"tileflipy",
		"tilepulse",
	};

	Effect effect = EFFECT_CENTER;

	for( int i=0; i<NUM_EFFECTS; i++ )
		if( lcPath.Find(EFFECT_STRING[i]) != -1 )
			effect = (Effect)i;

	switch( effect )
	{
	case EFFECT_CENTER:
		{
			m_Type = TYPE_SPRITE;
			Sprite* pSprite = new Sprite;
			m_pActors.push_back( pSprite );
			pSprite->Load( sPath );
			pSprite->SetXY( CENTER_X, CENTER_Y );
		}
		break;
	case EFFECT_STRETCH_STILL:
	case EFFECT_STRETCH_SCROLL_LEFT:
	case EFFECT_STRETCH_SCROLL_RIGHT:
	case EFFECT_STRETCH_SCROLL_UP:
	case EFFECT_STRETCH_SCROLL_DOWN:
	case EFFECT_STRETCH_WATER:
	case EFFECT_STRETCH_BUBBLE:
	case EFFECT_STRETCH_TWIST:
		{
			m_Type = TYPE_SPRITE;
			Sprite* pSprite = new Sprite;
			m_pActors.push_back( pSprite );
			RageTextureID ID(sPath);
			ID.bStretch = true;
			pSprite->LoadBG( ID );
			pSprite->StretchTo( FullScreenRectI );
			pSprite->SetCustomTextureRect( RectF(0,0,1,1) );

			switch( effect )
			{
			case EFFECT_STRETCH_SCROLL_LEFT:	m_fTexCoordVelocityX = +0.5f; m_fTexCoordVelocityY = 0;	break;
			case EFFECT_STRETCH_SCROLL_RIGHT:	m_fTexCoordVelocityX = -0.5f; m_fTexCoordVelocityY = 0;	break;
			case EFFECT_STRETCH_SCROLL_UP:		m_fTexCoordVelocityX = 0; m_fTexCoordVelocityY = +0.5f;	break;
			case EFFECT_STRETCH_SCROLL_DOWN:	m_fTexCoordVelocityX = 0; m_fTexCoordVelocityY = -0.5f;	break;
				break;
			}
		}
		break;
	case EFFECT_STRETCH_SPIN:
		{
			m_Type = TYPE_SPRITE;
			Sprite* pSprite = new Sprite;
			m_pActors.push_back( pSprite );
			pSprite->LoadBG( sPath );
			const RectI StretchedFullScreenRectI(
				FullScreenRectI.left-200,
				FullScreenRectI.top-200,
				FullScreenRectI.right+200,
				FullScreenRectI.bottom+200 );

			pSprite->ScaleToCover( StretchedFullScreenRectI );
			pSprite->SetEffectSpin( RageVector3(0,0,60) );
		}
		break;
	case EFFECT_PARTICLES_SPIRAL_OUT:
	case EFFECT_PARTICLES_SPIRAL_IN:
	case EFFECT_PARTICLES_FLOAT_UP:
	case EFFECT_PARTICLES_FLOAT_DOWN:
	case EFFECT_PARTICLES_FLOAT_LEFT:
	case EFFECT_PARTICLES_FLOAT_RIGHT:
	case EFFECT_PARTICLES_BOUNCE:
		{
			m_Type = TYPE_PARTICLES;
			Sprite s;
			s.Load( sPath );
			int iSpriteArea = int( s.GetUnzoomedWidth()*s.GetUnzoomedHeight() );
			const int iMaxArea = SCREEN_WIDTH*SCREEN_HEIGHT;
			m_iNumParticles = iMaxArea / iSpriteArea;
			m_iNumParticles = min( m_iNumParticles, MAX_SPRITES );

			for( int i=0; i<m_iNumParticles; i++ )
			{
				Sprite* pSprite = new Sprite;
				m_pActors.push_back( pSprite );
				pSprite->Load( sPath );
				pSprite->SetZoom( 0.7f + 0.6f*i/(float)m_iNumParticles );
				pSprite->SetX( randomf( GetGuardRailLeft(pSprite), GetGuardRailRight(pSprite) ) );
				pSprite->SetY( randomf( GetGuardRailTop(pSprite), GetGuardRailBottom(pSprite) ) );

				switch( effect )
				{
				case EFFECT_PARTICLES_FLOAT_UP:
				case EFFECT_PARTICLES_SPIRAL_OUT:
					m_vParticleVelocity.push_back( RageVector3( 0, -PARTICLE_SPEED*pSprite->GetZoom(), 0 ) );
					break;
				case EFFECT_PARTICLES_FLOAT_DOWN:
				case EFFECT_PARTICLES_SPIRAL_IN:
					m_vParticleVelocity.push_back( RageVector3( 0, PARTICLE_SPEED*pSprite->GetZoom(), 0 ) );
					break;
				case EFFECT_PARTICLES_FLOAT_LEFT:
					m_vParticleVelocity.push_back( RageVector3( -PARTICLE_SPEED*pSprite->GetZoom(), 0, 0 ) );
					break;
				case EFFECT_PARTICLES_FLOAT_RIGHT:
					m_vParticleVelocity.push_back( RageVector3( +PARTICLE_SPEED*pSprite->GetZoom(), 0, 0 ) );
					break;
				case EFFECT_PARTICLES_BOUNCE:
					m_bParticlesBounce = true;
					pSprite->SetZoom( 1 );
					m_vParticleVelocity.push_back( RageVector3( randomf(), randomf(), 0 ) );
					RageVec3Normalize( &m_vParticleVelocity[i], &m_vParticleVelocity[i] );
					break;
				default:
					ASSERT(0);
				}
			}
		}
		break;
	case EFFECT_TILE_STILL:
	case EFFECT_TILE_SCROLL_LEFT:
	case EFFECT_TILE_SCROLL_RIGHT:
	case EFFECT_TILE_SCROLL_UP:
	case EFFECT_TILE_SCROLL_DOWN:
	case EFFECT_TILE_FLIP_X:
	case EFFECT_TILE_FLIP_Y:
	case EFFECT_TILE_PULSE:
		{
			m_Type = TYPE_TILES;
			RageTextureID ID(sPath);
			ID.bStretch = true;
			Sprite s;
			s.Load( ID );
			m_iNumTilesWide = 2+int(SCREEN_WIDTH /s.GetUnzoomedWidth());
			m_iNumTilesWide = min( m_iNumTilesWide, MAX_TILES_WIDE );
			m_iNumTilesHigh = 2+int(SCREEN_HEIGHT/s.GetUnzoomedHeight());
			m_iNumTilesHigh = min( m_iNumTilesHigh, MAX_TILES_HIGH );
			m_fTilesStartX = s.GetUnzoomedWidth() / 2;
			m_fTilesStartY = s.GetUnzoomedHeight() / 2;
			m_fTilesSpacingX = s.GetUnzoomedWidth();
			m_fTilesSpacingY = s.GetUnzoomedHeight();
			for( int x=0; x<m_iNumTilesWide; x++ )
			{
				for( int y=0; y<m_iNumTilesHigh; y++ )
				{
					Sprite* pSprite = new Sprite;
					m_pActors.push_back( pSprite );
					pSprite->Load( ID );
					pSprite->SetTextureWrapping( true );	// gets rid of some "cracks"

					switch( effect )
					{
					case EFFECT_TILE_STILL:
						break;
					case EFFECT_TILE_SCROLL_LEFT:
						m_fTileVelocityX = -PARTICLE_SPEED;
						break;
					case EFFECT_TILE_SCROLL_RIGHT:
						m_fTileVelocityX = +PARTICLE_SPEED;
						break;
					case EFFECT_TILE_SCROLL_UP:
						m_fTileVelocityY = -PARTICLE_SPEED;
						break;
					case EFFECT_TILE_SCROLL_DOWN:
						m_fTileVelocityY = +PARTICLE_SPEED;
						break;
					case EFFECT_TILE_FLIP_X:
						pSprite->SetEffectSpin( RageVector3(2,0,0) );
						break;
					case EFFECT_TILE_FLIP_Y:
						pSprite->SetEffectSpin( RageVector3(0,2,0) );
						break;
					case EFFECT_TILE_PULSE:
						pSprite->SetEffectPulse( 1, 0.3f, 1.f );
						break;
					default:
						ASSERT(0);
					}
				}
			}
		}
		break;
	default:
		ASSERT(0);
	}


	sPath.MakeLower();

	if( sPath.Find("cyclecolor") != -1 )
		for( unsigned i=0; i<m_pActors.size(); i++ )
			m_pActors[i]->SetEffectRainbow( 5 );

	if( sPath.Find("cyclealpha") != -1 )
		for( unsigned i=0; i<m_pActors.size(); i++ )
			m_pActors[i]->SetEffectDiffuseShift( 2, RageColor(1,1,1,1), RageColor(1,1,1,0) );

	if( sPath.Find("startonrandomframe") != -1 )
		for( unsigned i=0; i<m_pActors.size(); i++ )
			m_pActors[i]->SetState( rand()%m_pActors[i]->GetNumStates() );

	if( sPath.Find("dontanimate") != -1 )
		for( unsigned i=0; i<m_pActors.size(); i++ )
			m_pActors[i]->StopAnimating();

	if( sPath.Find("add") != -1 )
		for( unsigned i=0; i<m_pActors.size(); i++ )
			m_pActors[i]->SetBlendMode( BLEND_ADD );
}



void BGAnimationLayer::LoadFromIni( CString sAniDir, CString sLayer )
{
	Init();
	if( sAniDir.Right(1) != SLASH )
		sAniDir += SLASH;

	ASSERT( IsADirectory(sAniDir) );

	CString sPathToIni = sAniDir + "BGAnimation.ini";

	IniFile ini(sPathToIni);
	ini.ReadFile();

	bool IsBanner = false;

	CString sFile;
	ini.GetValue( sLayer, "File", sFile );
	FixSlashesInPlace( sFile );
	
	CString sPath = sAniDir+sFile;
	CollapsePath( sPath );

	if( sFile.CompareNoCase("songbackground")==0 )
	{
		Song *pSong = GAMESTATE->m_pCurSong;
		if( pSong && pSong->HasBackground() )
			sPath = pSong->GetBackgroundPath();
		else
			sPath = THEME->GetPathToG("Common fallback background");
	}
	else if( sFile.CompareNoCase("songbanner")==0 )
	{
		const Song *pSong = GAMESTATE->m_pCurSong;
		if( pSong && pSong->HasBanner() )
			sPath = pSong->GetBannerPath();
		else
			sPath = THEME->GetPathToG("Common fallback banner");
		IsBanner = true;
	}
	else if( sFile == "" )
	{
		HOOKS->MessageBoxOK( ssprintf( 
			"In the ini file for BGAnimation '%s', '%s' is missing a the line 'File='.", sAniDir.c_str(), sLayer.c_str() ) );
	}


	/* XXX: Search the BGA dir first, then search the Graphics directory if this
	 * is a theme BGA, so common BG graphics can be overridden. */
	{
		vector<CString> asElementPaths;
		GetDirListing( sPath + "*", asElementPaths, false, true );
		if(asElementPaths.size() == 0)
		{
			CString sError = ssprintf("In the ini file for BGAnimation '%s', the specified File '%s' does not exist.", sAniDir.c_str(), sFile.c_str());
			HOOKS->MessageBoxOK( sError );
			LOG->Warn( sError );
			return;
		}
		if(asElementPaths.size() > 1)
		{
			CString sError = ssprintf( 
					"There is more than one file that matches "
					"'%s/%s'.  Please remove all but one of these matches.",
					sAniDir.c_str(), sFile.c_str() );
			HOOKS->MessageBoxOK( sError );
			LOG->Warn( sError );
		}
		sPath = asElementPaths[0];
	}

	bool Stretch = false;
	{
		CString type;
		ini.GetValue( sLayer, "Type", type );
		type.MakeLower();

		/* The preferred way of stretching a sprite to fit the screen is "Type=sprite"
		 * and "stretch=1".  "type=1" is for backwards-compatibility. */
		ini.GetValue( sLayer, "Stretch", Stretch );

			 if( atoi(type) == 0 || type == "sprite" ) m_Type = TYPE_SPRITE;
		else if( atoi(type) == 1 ) { m_Type = TYPE_SPRITE; Stretch = true; }
		else if( atoi(type) == 2 || type == "particles" ) m_Type = TYPE_PARTICLES;
		else if( atoi(type) == 3 || type == "tiles" ) m_Type = TYPE_TILES;
		else
			RageException::Throw("Unknown %s::type in %s: \"%s\"",
				sLayer.c_str(), sAniDir.c_str(), type.c_str() );
	}

	{
		const IniFile::key *key = ini.GetKey( sLayer );
		for( IniFile::key::const_iterator i = key->begin();
			 i != key->end(); ++i)
		{
			CString KeyName = i->first; /* "OnCommand" */
			KeyName.MakeLower();

			if( KeyName.Right(7) != "command" )
				continue; /* not a command */

			const CString &Data = i->second;
			CString CmdName;
			/* Special case: "Command=foo" -> "OnCommand=foo" */
			if( KeyName.size() == 7 )
				CmdName="on";
			else
				CmdName = KeyName.Left( KeyName.size()-7 );
			m_asCommands[CmdName] = Data;
		}
	}
		

	ini.GetValue( sLayer, "CommandRepeatSeconds", m_fRepeatCommandEverySeconds );
	m_fSecondsUntilNextCommand = m_fRepeatCommandEverySeconds;
	ini.GetValue( sLayer, "FOV", m_fFOV );
	ini.GetValue( sLayer, "Lighting", m_bLighting );
	ini.GetValue( sLayer, "TexCoordVelocityX", m_fTexCoordVelocityX );
	ini.GetValue( sLayer, "TexCoordVelocityY", m_fTexCoordVelocityY );
	// compat:
	ini.GetValue( sLayer, "StretchTexCoordVelocityX", m_fTexCoordVelocityX );
	ini.GetValue( sLayer, "StretchTexCoordVelocityY", m_fTexCoordVelocityY );
	ini.GetValue( sLayer, "ZoomMin", m_fZoomMin );
	ini.GetValue( sLayer, "ZoomMax", m_fZoomMax );
	ini.GetValue( sLayer, "VelocityXMin", m_fVelocityXMin );
	ini.GetValue( sLayer, "VelocityXMax", m_fVelocityXMax );
	ini.GetValue( sLayer, "VelocityYMin", m_fVelocityYMin );
	ini.GetValue( sLayer, "VelocityYMax", m_fVelocityYMax );
	ini.GetValue( sLayer, "VelocityZMin", m_fVelocityZMin );
	ini.GetValue( sLayer, "VelocityZMax", m_fVelocityZMax );
	ini.GetValue( sLayer, "OverrideSpeed", m_fOverrideSpeed );
	ini.GetValue( sLayer, "NumParticles", m_iNumParticles );
	ini.GetValue( sLayer, "ParticlesBounce", m_bParticlesBounce );
	ini.GetValue( sLayer, "TilesStartX", m_fTilesStartX );
	ini.GetValue( sLayer, "TilesStartY", m_fTilesStartY );
	ini.GetValue( sLayer, "TilesSpacingX", m_fTilesSpacingX );
	ini.GetValue( sLayer, "TilesSpacingY", m_fTilesSpacingY );
	ini.GetValue( sLayer, "TileVelocityX", m_fTileVelocityX );
	ini.GetValue( sLayer, "TileVelocityY", m_fTileVelocityY );


	bool NeedTextureStretch = false;
	if( m_fTexCoordVelocityX != 0 ||
		m_fTexCoordVelocityY != 0 )
		NeedTextureStretch = true;
	if( IsBanner )
		TEXTUREMAN->DisableOddDimensionWarning();

	switch( m_Type )
	{
	case TYPE_SPRITE:
		{
			RageTextureID ID(sPath);
			if( NeedTextureStretch )
				ID.bStretch = true;

			Actor* pActor = MakeActor( ID );
			m_pActors.push_back( pActor );
			ASSERT( !(m_bGeneric && Stretch) );
			if( !m_bGeneric )
			{
				if( Stretch )
					pActor->StretchTo( FullScreenRectI );
				else
					pActor->SetXY( CENTER_X, CENTER_Y );
			}
		}
		break;
	case TYPE_PARTICLES:
		{
			ASSERT( !m_bGeneric );
			for( int i=0; i<m_iNumParticles; i++ )
			{
				Actor* pActor = MakeActor( sPath );
				m_pActors.push_back( pActor );
				pActor->SetXY( randomf(float(FullScreenRectI.left),float(FullScreenRectI.right)),
							   randomf(float(FullScreenRectI.top),float(FullScreenRectI.bottom)) );
				pActor->SetZoom( randomf(m_fZoomMin,m_fZoomMax) );
				m_vParticleVelocity.push_back( RageVector3( 
					randomf(m_fVelocityXMin,m_fVelocityXMax),
					randomf(m_fVelocityYMin,m_fVelocityYMax),
					randomf(m_fVelocityZMin,m_fVelocityZMax) ) );
				if( m_fOverrideSpeed != 0 )
				{
					RageVec3Normalize( &m_vParticleVelocity[i], &m_vParticleVelocity[i] );
					m_vParticleVelocity[i] *= m_fOverrideSpeed;
				}
			}
		}
		break;
	case TYPE_TILES:
		{
			ASSERT( !m_bGeneric );
			Sprite s;
			RageTextureID ID(sPath);
			ID.bStretch = true;
			s.Load( ID );
			if( m_fTilesSpacingX == -1 )
				m_fTilesSpacingX = s.GetUnzoomedWidth();
			if( m_fTilesSpacingY == -1 )
				m_fTilesSpacingY = s.GetUnzoomedHeight();
			m_iNumTilesWide = 2+(int)(SCREEN_WIDTH /m_fTilesSpacingX);
			m_iNumTilesHigh = 2+(int)(SCREEN_HEIGHT/m_fTilesSpacingY);
			unsigned NumSprites = m_iNumTilesWide * m_iNumTilesHigh;
			for( unsigned i=0; i<NumSprites; i++ )
			{
				Sprite* pSprite = new Sprite;
				m_pActors.push_back( pSprite );
				pSprite->Load( ID );
				pSprite->SetTextureWrapping( true );		// gets rid of some "cracks"
				pSprite->SetZoom( randomf(m_fZoomMin,m_fZoomMax) );
			}
		}
		break;
	default:
		ASSERT(0);
	}
	if( IsBanner )
		TEXTUREMAN->EnableOddDimensionWarning();

	bool bStartOnRandomFrame = false;
	ini.GetValue( sLayer, "StartOnRandomFrame", bStartOnRandomFrame );
	if( bStartOnRandomFrame )
	{
		for( unsigned i=0; i<m_pActors.size(); i++ )
			m_pActors[i]->SetState( rand()%m_pActors[i]->GetNumStates() );
	}

	if( !m_bGeneric )
		PlayCommand( "On" );
}

float BGAnimationLayer::GetMaxTweenTimeLeft() const
{
	float ret = 0;

	for( unsigned i=0; i<m_pActors.size(); i++ )
		ret = max(ret, m_pActors[i]->GetTweenTimeLeft());

	return ret;
}

void BGAnimationLayer::FinishTweening()
{
	for( unsigned i=0; i<m_pActors.size(); i++ )
		m_pActors[i]->FinishTweening();
}

void BGAnimationLayer::Update( float fDeltaTime )
{
	fDeltaTime *= m_fUpdateRate;

	const float fSongBeat = GAMESTATE->m_fSongBeat;
	
	unsigned i;
	for( i=0; i<m_pActors.size(); i++ )
		m_pActors[i]->Update( fDeltaTime );


	switch( m_Type )
	{
	case TYPE_SPRITE:
		for( i=0; i<m_pActors.size(); i++ )
		{
			if( m_fTexCoordVelocityX || m_fTexCoordVelocityY )
			{
				m_pActors[i]->Command( ssprintf("StretchTexCoords,%f,%f",
					fDeltaTime*m_fTexCoordVelocityX,
					fDeltaTime*m_fTexCoordVelocityY) );
			}
		}
		break;
/*	case EFFECT_PARTICLES_SPIRAL_OUT:
		for( i=0; i<m_iNumSprites; i++ )
		{
			m_pActors[i].SetZoom( m_pActors[i].GetZoom() + fDeltaTime );
			if( m_pActors[i].GetZoom() > SPIRAL_MAX_ZOOM )
				m_pActors[i].SetZoom( SPIRAL_MIN_ZOOM );

			m_pActors[i].SetRotationZ( m_pActors[i].GetRotationZ() + fDeltaTime );

			float fRadius = (m_pActors[i].GetZoom()-SPIRAL_MIN_ZOOM);
			fRadius *= fRadius;
			fRadius *= 200;
			m_pActors[i].SetX( CENTER_X + cosf(m_pActors[i].GetRotationZ())*fRadius );
			m_pActors[i].SetY( CENTER_Y + sinf(m_pActors[i].GetRotationZ())*fRadius );
		}
		break;
	case EFFECT_PARTICLES_SPIRAL_IN:
		for( i=0; i<m_iNumSprites; i++ )
		{
			m_pActors[i].SetZoom( m_pActors[i].GetZoom() - fDeltaTime );
			if( m_pActors[i].GetZoom() < SPIRAL_MIN_ZOOM )
				m_pActors[i].SetZoom( SPIRAL_MAX_ZOOM );

			m_pActors[i].SetRotationZ( m_pActors[i].GetRotationZ() - fDeltaTime );

			float fRadius = (m_pActors[i].GetZoom()-SPIRAL_MIN_ZOOM);
			fRadius *= fRadius;
			fRadius *= 200;
			m_pActors[i].SetX( CENTER_X + cosf(m_pActors[i].GetRotationZ())*fRadius );
			m_pActors[i].SetY( CENTER_Y + sinf(m_pActors[i].GetRotationZ())*fRadius );
		}
		break;
*/
	case TYPE_PARTICLES:
		for( i=0; i<m_pActors.size(); i++ )
		{
			Actor* pActor = m_pActors[i];
			RageVector3 &vel = m_vParticleVelocity[i];

			m_pActors[i]->SetX( pActor->GetX() + fDeltaTime*vel.x );
			m_pActors[i]->SetY( pActor->GetY() + fDeltaTime*vel.y );
			pActor->SetZ( pActor->GetZ() + fDeltaTime*vel.z  );
			if( m_bParticlesBounce )
			{
				if( HitGuardRailLeft(pActor) )	
				{
					vel.x *= -1;
					pActor->SetX( GetGuardRailLeft(pActor) );
				}
				if( HitGuardRailRight(pActor) )	
				{
					vel.x *= -1;
					pActor->SetX( GetGuardRailRight(pActor) );
				}
				if( HitGuardRailTop(pActor) )	
				{
					vel.y *= -1;
					pActor->SetY( GetGuardRailTop(pActor) );
				}
				if( HitGuardRailBottom(pActor) )	
				{
					vel.y *= -1;
					pActor->SetY( GetGuardRailBottom(pActor) );
				}
			}
			else // !m_bParticlesBounce 
			{
				if( vel.x<0  &&  IsOffScreenLeft(pActor) )
					pActor->SetX( GetOffScreenRight(pActor) );
				if( vel.x>0  &&  IsOffScreenRight(pActor) )
					pActor->SetX( GetOffScreenLeft(pActor) );
				if( vel.y<0  &&  IsOffScreenTop(pActor) )
					pActor->SetY( GetOffScreenBottom(pActor) );
				if( vel.y>0  &&  IsOffScreenBottom(pActor) )
					pActor->SetY( GetOffScreenTop(pActor) );
			}
		}
		break;
	case TYPE_TILES:
		{
			float fSecs = RageTimer::GetTimeSinceStart();
			float fTotalWidth = m_iNumTilesWide * m_fTilesSpacingX;
			float fTotalHeight = m_iNumTilesHigh * m_fTilesSpacingY;
			
			ASSERT( int(m_pActors.size()) == m_iNumTilesWide * m_iNumTilesHigh );

			for( int x=0; x<m_iNumTilesWide; x++ )
			{
				for( int y=0; y<m_iNumTilesHigh; y++ )
				{
					int i = y*m_iNumTilesWide + x;

					float fX = m_fTilesStartX + m_fTilesSpacingX * x + fSecs * m_fTileVelocityX;
					float fY = m_fTilesStartY + m_fTilesSpacingY * y + fSecs * m_fTileVelocityY;

					fX += m_fTilesSpacingX/2;
					fY += m_fTilesSpacingY/2;

					fX = fmodf( fX, fTotalWidth );
					fY = fmodf( fY, fTotalHeight );

					if( fX < 0 )	fX += fTotalWidth;
					if( fY < 0 )	fY += fTotalHeight;

					fX -= m_fTilesSpacingX/2;
					fY -= m_fTilesSpacingY/2;
					
					m_pActors[i]->SetX( fX );
					m_pActors[i]->SetY( fY );
				}
			}
/*			
		for( i=0; i<m_iNumSprites; i++ )
		{
			m_pActors[i].SetX( m_pActors[i].GetX() + fDeltaTime*  );
			m_pActors[i].SetY( m_pActors[i].GetY() + fDeltaTime*m_vParticleVelocity[i].y  );
			m_pActors[i].SetZ( m_pActors[i].GetZ() + fDeltaTime*m_vParticleVelocity[i].z  );
			if( IsOffScreenLeft(&m_pActors[i]) )
				m_pActors[i].SetX( m_pActors[i].GetX()-GetOffScreenLeft(&m_pActors[i]) + GetOffScreenRight(&m_pActors[i]) );
			if( IsOffScreenRight(&m_pActors[i]) )
				m_pActors[i].SetX( m_pActors[i].GetX()-GetOffScreenRight(&m_pActors[i]) + GetOffScreenLeft(&m_pActors[i]) );
			if( IsOffScreenTop(&m_pActors[i]) )
				m_pActors[i].SetY( m_pActors[i].GetY()-GetOffScreenTop(&m_pActors[i]) + GetOffScreenBottom(&m_pActors[i]) );
			if( IsOffScreenBottom(&m_pActors[i]) )
				m_pActors[i].SetY( m_pActors[i].GetY()-GetOffScreenBottom(&m_pActors[i]) + GetOffScreenTop(&m_pActors[i]) );
				*/
		}
		break;
	case EFFECT_TILE_PULSE:
		for( i=0; i<m_pActors.size(); i++ )
			m_pActors[i]->SetZoom( sinf( fSongBeat*PI/2 ) );

		break;
	default:
		ASSERT(0);
	}

	/*
	if(m_TweenStartTime != 0 && !(m_TweenStartTime < 0))
	{
		m_TweenStartTime -= fDeltaTime;
		if(m_TweenStartTime <= 0) // if we've gone past the magic point... show the beast....
		{
		//	m_pActors[0].SetXY( m_TweenX, m_TweenY);
			
			// WHAT WOULD BE NICE HERE:
			// Set the Sprite Tweening To m_TweenX and m_TweenY
			// Going as fast as m_TweenSpeed specifies.
			// however, TWEEN falls over on its face at this point.
			// Lovely.
			// Instead: Manual tweening. Blah.
			m_TweenState = 1;
			if(m_PosX == m_TweenX)
			{
				m_TweenPassedX = 1;
			}
			if(m_PosY == m_TweenY)
			{
				m_TweenPassedY = 1;
			}
		}		
	}

	if(m_TweenState) // A FAR from perfect Tweening Mechanism.
	{
		if(m_TweenPassedY != 1) // Check to see if we still need to Tween Along the Y Axis
		{
			if(m_pActors[0].GetY() < m_TweenY) // it needs to travel down
			{
				// Speed = Distance / Time....
				// Take away from the current position... the distance it has to travel divided by the time they want it done in...
				m_pActors[0].SetY(m_pActors[0].GetY() + ((m_TweenY - m_PosY)/(m_TweenSpeed*60)));

				if(m_pActors[0].GetY() > m_TweenY) // passed the location we wanna go to?
				{
					m_pActors[0].SetY(m_TweenY); // set it to the exact location we want
					m_TweenPassedY = 1; // say we passed it.
				}
			}
			else // travelling up
			{
				m_pActors[0].SetY(m_pActors[0].GetY() - ((m_TweenY + m_PosY)/(m_TweenSpeed*60)));

				if(m_pActors[0].GetY() < m_TweenY)
				{
					m_pActors[0].SetY(m_TweenY);
					m_TweenPassedY = 1;
				}
			}
		}

		if(m_TweenPassedX != 1) // Check to see if we still need to Tween Along the X Axis
		{
			if(m_pActors[0].GetX() < m_TweenX) // it needs to travel right
			{
				m_pActors[0].SetX(m_pActors[0].GetX() + ((m_TweenX - m_PosX)/(m_TweenSpeed*60)));
				if(m_pActors[0].GetX() > m_TweenX)
				{
					m_pActors[0].SetX(m_TweenX);
					m_TweenPassedX = 1;
				}
			}
			else // travelling left
			{
				m_pActors[0].SetX(m_pActors[0].GetX() - ((m_TweenX + m_PosX)/(m_TweenSpeed*60)));
				if(m_pActors[0].GetX() < m_TweenX)
				{
					m_pActors[0].SetX(m_TweenX);
					m_TweenPassedX = 1;
				}
			}
		}

		if(m_TweenPassedY == 1 && m_TweenPassedX == 1) // totally passed both X and Y? Stop tweening.
		{
			m_TweenState = 0;
		}
	}

	if(m_ShowTime != 0 && !(m_ShowTime < 0))
	{
		m_ShowTime -= fDeltaTime;
		if(m_ShowTime <= 0) // if we've gone past the magic point... show the beast....
		{
			m_pActors[0].SetDiffuse( RageColor(1,1,1,1) );
		}		
	}
	if(m_HideTime != 0 && !(m_HideTime < 0)) // make sure it's not 0 or less than 0...
	{
		m_HideTime -= fDeltaTime;
		if(m_HideTime <= 0) // if we've gone past the magic point... hide the beast....
		{
			m_pActors[0].SetDiffuse( RageColor(0,0,0,0) );
		}
		
	}
	*/

	if( m_fRepeatCommandEverySeconds != -1 )	// if repeating
	{
		m_fSecondsUntilNextCommand -= fDeltaTime;
		if( m_fSecondsUntilNextCommand <= 0 )
		{
			PlayCommand( "On" );
			m_fSecondsUntilNextCommand += m_fRepeatCommandEverySeconds;
		}
	}
}

void BGAnimationLayer::Draw()
{
	if( m_fFOV != -1 )
	{
		DISPLAY->CameraPushMatrix();
		DISPLAY->LoadMenuPerspective( m_fFOV, CENTER_X, CENTER_Y );
	}

	if( m_bLighting )
	{
		DISPLAY->SetLighting( true );
		DISPLAY->SetLightDirectional( 
			0, 
			RageColor(0.6f,0.6f,0.6f,1), 
			RageColor(0.9f,0.9f,0.9f,1),
			RageColor(0,0,0,1),
			RageVector3(0, 0, 1) );
	}

	for( unsigned i=0; i<m_pActors.size(); i++ )
		m_pActors[i]->Draw();
	
	if( m_fFOV != -1 )
	{
		DISPLAY->CameraPopMatrix();
	}

	if( m_bLighting )
	{
		DISPLAY->SetLightOff( 0 );
		DISPLAY->SetLighting( false );
	}
}

void BGAnimationLayer::SetDiffuse( RageColor c )
{
	for(unsigned i=0; i<m_pActors.size(); i++) 
		m_pActors[i]->SetDiffuse(c);
}

void BGAnimationLayer::GainingFocus( float fRate, bool bRewindMovie, bool bLoop )
{
	m_fUpdateRate = fRate;

	ASSERT( m_pActors.size() );

	//
	// The order of these actions is important.
	// At this point, the movie is probably paused (by LosingFocus()).
	// Play the movie, then set the playback rate (which can 
	// potentially pause the movie again).
	//
	if( bRewindMovie )
		m_pActors[0]->Command( "position,0" );
	m_pActors[0]->Command( ssprintf("loop,%i",bLoop) );
	m_pActors[0]->Command( "play" );
	m_pActors[0]->Command( ssprintf("rate,%f",fRate) );

	if( m_fRepeatCommandEverySeconds == -1 )	// if not repeating
		PlayCommand( "On" );
}

void BGAnimationLayer::LosingFocus()
{
	m_pActors[0]->Command( "pause" );
}

void BGAnimationLayer::PlayCommand( CString cmd )
{
	unsigned i;

	for( i=0; i<m_pActors.size(); i++ )
		m_pActors[i]->Command( ssprintf("playcommand,%s", cmd.c_str()) );

	cmd.MakeLower();
	map<CString, CString>::const_iterator it = m_asCommands.find( cmd );

	if( it == m_asCommands.end() )
		return;

	for( i=0; i<m_pActors.size(); i++ )
		m_pActors[i]->Command( it->second );
}
