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
#include "CroppedSprite.h"



const float PARTICLE_SPEED = 300;

const float SPIRAL_MAX_ZOOM = 2;
const float SPIRAL_MIN_ZOOM = 0.3f;



BGAnimationLayer::BGAnimationLayer()
{
	Init();
}

BGAnimationLayer::~BGAnimationLayer()
{
	Unload();
}

void BGAnimationLayer::Unload()
{
	for( unsigned i=0; i<m_Sprites.size(); i++ )
		delete m_Sprites[i];
	m_Sprites.clear();
}

void BGAnimationLayer::Init()
{
	Unload();

	m_fUpdateRate = 1;

//	m_bCycleColor = false;
//	m_bCycleAlpha = false;
//	m_Effect = EFFECT_STRETCH_STILL;

	for( int i=0; i<MAX_SPRITES; i++ )
		m_vParticleVelocity[i] = RageVector3( 0, 0, 0 );

	m_Type = TYPE_SPRITE;

	m_fStretchTexCoordVelocityX = 0;
	m_fStretchTexCoordVelocityY = 0;
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
	m_Sprites.push_back(new Sprite);
	m_Sprites.back()->LoadBG( ID );
	m_Sprites.back()->StretchTo( RectI(SCREEN_LEFT,SCREEN_TOP,SCREEN_RIGHT,SCREEN_BOTTOM) );
}

void BGAnimationLayer::LoadFromMovie( CString sMoviePath )
{
	Init();
	m_Sprites.push_back(new Sprite);
	m_Sprites.back()->LoadBG( sMoviePath );
	m_Sprites.back()->StretchTo( RectI(SCREEN_LEFT,SCREEN_TOP,SCREEN_RIGHT,SCREEN_BOTTOM) );
	m_Sprites.back()->GetTexture()->Play();
	SDL_Delay( 50 );	// decode a frame so we don't see a black flash at the beginning
	m_Sprites.back()->GetTexture()->Pause();
}

void BGAnimationLayer::LoadFromVisualization( CString sMoviePath )
{
	Init();
	m_Sprites.push_back(new Sprite);
	m_Sprites.back()->LoadBG( sMoviePath );
	m_Sprites.back()->StretchTo( RectI(SCREEN_LEFT,SCREEN_TOP,SCREEN_RIGHT,SCREEN_BOTTOM) );
	m_Sprites.back()->SetBlendMode( BLEND_ADD );
}


void BGAnimationLayer::LoadFromAniLayerFile( CString sPath )
{
	Song* pSong = GAMESTATE->m_pCurSong;
	CString sSongBGPath;
	if( pSong && pSong->HasBackground() )
		sSongBGPath = pSong->GetBackgroundPath();
	else
		sSongBGPath = THEME->GetPathToG("Common fallback background");

	Init();
	CString lcPath = sPath;
	lcPath.MakeLower();

	if( lcPath.Find("usesongbg") != -1 )
	{
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
		m_Type = TYPE_SPRITE;
		m_Sprites.push_back(new Sprite);
		m_Sprites.back()->Load( sPath );
		m_Sprites.back()->SetXY( CENTER_X, CENTER_Y );
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
			m_Type = TYPE_STRETCH;
			m_Sprites.push_back(new Sprite);
			RageTextureID ID(sPath);
			ID.bStretch = true;
			m_Sprites.back()->LoadBG( ID );
			m_Sprites.back()->StretchTo( RectI(SCREEN_LEFT,SCREEN_TOP,SCREEN_RIGHT,SCREEN_BOTTOM) );
			m_Sprites.back()->SetCustomTextureRect( RectF(0,0,1,1) );

			switch( effect )
			{
			case EFFECT_STRETCH_SCROLL_LEFT:	m_fStretchTexCoordVelocityX = +0.5f; m_fStretchTexCoordVelocityY = 0;	break;
			case EFFECT_STRETCH_SCROLL_RIGHT:	m_fStretchTexCoordVelocityX = -0.5f; m_fStretchTexCoordVelocityY = 0;	break;
			case EFFECT_STRETCH_SCROLL_UP:		m_fStretchTexCoordVelocityX = 0; m_fStretchTexCoordVelocityY = +0.5f;	break;
			case EFFECT_STRETCH_SCROLL_DOWN:	m_fStretchTexCoordVelocityX = 0; m_fStretchTexCoordVelocityY = -0.5f;	break;
				break;
			}
		}
		break;
	case EFFECT_STRETCH_SPIN:
		{
			m_Type = TYPE_STRETCH;
			m_Sprites.push_back(new Sprite);
			m_Sprites.back()->LoadBG( sPath );
			m_Sprites.back()->ScaleToCover( RectI(SCREEN_LEFT-200,SCREEN_TOP-200,SCREEN_RIGHT+200,SCREEN_BOTTOM+200) );
			m_Sprites.back()->SetEffectSpin( RageVector3(0,0,60) );
		}
		break;
	case EFFECT_PARTICLES_SPIRAL_OUT:
	case EFFECT_PARTICLES_SPIRAL_IN:
/*		{
			m_Type = TYPE_PARTICLES;
			m_Sprites.back()->Load( sPath );
			int iSpriteArea = int( m_Sprites.back()->GetUnzoomedWidth()*m_Sprites.back()->GetUnzoomedHeight() );
			int iMaxArea = SCREEN_WIDTH*SCREEN_HEIGHT;
			m_iNumSprites = m_iNumParticles = iMaxArea / iSpriteArea;
			m_iNumSprites = m_iNumParticles = min( m_iNumSprites, MAX_SPRITES );
			for( unsigned i=0; i<m_iNumSprites; i++ )
			{
				m_Sprites[i].Load( sPath );
				m_Sprites[i].SetZoom( randomf(0.2f,2) );
				m_Sprites[i].SetRotationZ( randomf(0,360) );
			}
		}
		break;
*/	case EFFECT_PARTICLES_FLOAT_UP:
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
				m_Sprites.push_back(new Sprite);
				m_Sprites.back()->Load( sPath );
				m_Sprites.back()->SetZoom( 0.7f + 0.6f*i/(float)m_iNumParticles );
				m_Sprites.back()->SetX( randomf( GetGuardRailLeft(m_Sprites.back()), GetGuardRailRight(m_Sprites.back()) ) );
				m_Sprites.back()->SetY( randomf( GetGuardRailTop(m_Sprites.back()), GetGuardRailBottom(m_Sprites.back()) ) );

				switch( effect )
				{
				case EFFECT_PARTICLES_FLOAT_UP:
				case EFFECT_PARTICLES_SPIRAL_OUT:
					m_vParticleVelocity[i] = RageVector3( 0, -PARTICLE_SPEED*m_Sprites.back()->GetZoom(), 0 );
					break;
				case EFFECT_PARTICLES_FLOAT_DOWN:
				case EFFECT_PARTICLES_SPIRAL_IN:
					m_vParticleVelocity[i] = RageVector3( 0, PARTICLE_SPEED*m_Sprites.back()->GetZoom(), 0 );
					break;
				case EFFECT_PARTICLES_FLOAT_LEFT:
					m_vParticleVelocity[i] = RageVector3( -PARTICLE_SPEED*m_Sprites.back()->GetZoom(), 0, 0 );
					break;
				case EFFECT_PARTICLES_FLOAT_RIGHT:
					m_vParticleVelocity[i] = RageVector3( +PARTICLE_SPEED*m_Sprites.back()->GetZoom(), 0, 0 );
					break;
				case EFFECT_PARTICLES_BOUNCE:
					m_bParticlesBounce = true;
					m_Sprites.back()->SetZoom( 1 );
					m_vParticleVelocity[i] = RageVector3( randomf(), randomf(), 0 );
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
//			m_fTilesSpacingX -= 1;	// HACK:  Fix textures with transparence have gaps
//			m_fTilesSpacingY -= 1;	// HACK:  Fix textures with transparence have gaps
			for( int x=0; x<m_iNumTilesWide; x++ )
			{
				for( int y=0; y<m_iNumTilesHigh; y++ )
				{
					m_Sprites.push_back(new Sprite);
					m_Sprites.back()->Load( ID );
					m_Sprites.back()->SetTextureWrapping( true );	// gets rid of some "cracks"

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
						m_Sprites.back()->SetEffectSpin( RageVector3(2,0,0) );
						break;
					case EFFECT_TILE_FLIP_Y:
						m_Sprites.back()->SetEffectSpin( RageVector3(0,2,0) );
						break;
					case EFFECT_TILE_PULSE:
						m_Sprites.back()->SetEffectPulse( 1, 0.3f, 1.f );
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
		for( unsigned i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->SetEffectRainbow( 5 );

	if( sPath.Find("cyclealpha") != -1 )
		for( unsigned i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->SetEffectDiffuseShift( 2, RageColor(1,1,1,1), RageColor(1,1,1,0) );

	if( sPath.Find("startonrandomframe") != -1 )
		for( unsigned i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->SetState( rand()%m_Sprites[i]->GetNumStates() );

	if( sPath.Find("dontanimate") != -1 )
		for( unsigned i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->StopAnimating();

	if( sPath.Find("add") != -1 )
		for( unsigned i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->SetBlendMode( BLEND_ADD );

	/*
	CString sDir, sFName, sExt;
	splitrelpath( sPath, sDir, sFName, sExt );
	CString sIniPath = sDir+"/"+sFName+".ini";
	IniFile ini;
	ini.SetPath( sIniPath );
	if( ini.ReadFile() )
	{
		ini.GetValueF( "BGAnimationLayer", "SetXpos", m_PosX );
		ini.GetValueF( "BGAnimationLayer", "SetYpos", m_PosY );
		ini.GetValueF( "BGAnimationLayer", "SetZoom", m_Zoom );
		ini.GetValueF( "BGAnimationLayer", "SetRot", m_Rot );
		ini.GetValueF( "BGAnimationLayer", "TweenStartTime", m_TweenStartTime );
		ini.GetValueF( "BGAnimationLayer", "TweenX", m_TweenX );
		ini.GetValueF( "BGAnimationLayer", "TweenY", m_TweenY );
		ini.GetValueF( "BGAnimationLayer", "TweenSpeed", m_TweenSpeed );
		ini.GetValueF( "BGAnimationLayer", "ShowTime", m_ShowTime );
		ini.GetValueF( "BGAnimationLayer", "HideTime", m_HideTime );
		ini.GetValueF( "BGAnimationLayer", "TexCoordVelocityX", m_vTexCoordVelocity.x );
		ini.GetValueF( "BGAnimationLayer", "TexCoordVelocityY", m_vTexCoordVelocity.y );
		ini.GetValueF( "BGAnimationLayer", "RotationalVelocity", m_fRotationalVelocity );
		ini.GetValueF( "BGAnimationLayer", "SetY", m_fStretchScrollH_Y );
	}

	if(m_ShowTime != 0) // they don't want to show until a certain point... hide it all
	{
		m_Sprites[0].SetDiffuse(RageColor(0,0,0,0));
	}
	if(m_PosX != 0)
	{
		m_Sprites[0].SetX(m_PosX);
	}
	if(m_PosY != 0)
	{
		m_Sprites[0].SetY(m_PosY);
	}
	if(m_Zoom != 0)
	{
		m_Sprites[0].SetZoom(m_Zoom);
	}
	if(m_Rot != 0)
	{
		m_Sprites[0].SetRotationZ(m_Rot);
	}
	*/
}

static Sprite *NewSprite(bool banner)
{
	if(banner) return new CroppedSprite;
	return new Sprite;
}


void BGAnimationLayer::LoadFromIni( CString sAniDir, CString sLayer )
{
	Init();
	if( sAniDir.Right(1) != "/" )
		sAniDir += "/";

	ASSERT( IsADirectory(sAniDir) );

	CString sPathToIni = sAniDir + "BGAnimation.ini";

	IniFile ini(sPathToIni);
	ini.ReadFile();

	bool IsBanner = false;

	CString sFile;
	ini.GetValue( sLayer, "File", sFile );
	
	CString sPath = sAniDir+sFile;

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
		Song *pSong = GAMESTATE->m_pCurSong;
		if( pSong && pSong->HasBanner() )
			sPath = pSong->GetBannerPath();
		else
			sPath = THEME->GetPathToG("Common fallback banner");
		IsBanner = true;
	}
	else if( sFile == "" )
	{
		RageException::Throw( "In the ini file for BGAnimation '%s', '%s' is missing a the line 'File='.", sAniDir.c_str(), sLayer.c_str() );
	}


	/* XXX: Search the BGA dir first, then search the Graphics directory if this
	 * is a theme BGA, so common BG graphics can be overridden. */
	{
		vector<CString> asElementPaths;
		GetDirListing( sPath + "*", asElementPaths, false, true );
		if(asElementPaths.size() == 0)
			RageException::Throw( "In the ini file for BGAnimation '%s', the specified File '%s' does not exist.", sAniDir.c_str(), sFile.c_str() );
		if(asElementPaths.size() > 1)
		{
			CString message = ssprintf( 
				"There is more than one file that matches "
				"'%s/%s'.  Please remove all but one of these matches.",
				sAniDir.c_str(), sFile.c_str() );

			RageException::Throw( message ); 
		}
		sPath = asElementPaths[0];
	}

	ini.GetValueI( sLayer, "Type", (int&)m_Type );
	ini.GetValue ( sLayer, "Command", m_sCommand );
	ini.GetValueF( sLayer, "StretchTexCoordVelocityX", m_fStretchTexCoordVelocityX );
	ini.GetValueF( sLayer, "StretchTexCoordVelocityY", m_fStretchTexCoordVelocityY );
	ini.GetValueF( sLayer, "ZoomMin", m_fZoomMin );
	ini.GetValueF( sLayer, "ZoomMax", m_fZoomMax );
	ini.GetValueF( sLayer, "VelocityXMin", m_fVelocityXMin );
	ini.GetValueF( sLayer, "VelocityXMax", m_fVelocityXMax );
	ini.GetValueF( sLayer, "VelocityYMin", m_fVelocityYMin );
	ini.GetValueF( sLayer, "VelocityYMax", m_fVelocityYMax );
	ini.GetValueF( sLayer, "VelocityZMin", m_fVelocityZMin );
	ini.GetValueF( sLayer, "VelocityZMax", m_fVelocityZMax );
	ini.GetValueF( sLayer, "OverrideSpeed", m_fOverrideSpeed );
	ini.GetValueI( sLayer, "NumParticles", m_iNumParticles );
	ini.GetValueB( sLayer, "ParticlesBounce", m_bParticlesBounce );
//	ini.GetValueI( sLayer, "NumTilesWide", m_iNumTilesWide );	// infer from spacing (or else the Update logic breaks)
//	ini.GetValueI( sLayer, "NumTilesHigh", m_iNumTilesHigh );	// infer from spacing (or else the Update logic breaks)
	ini.GetValueF( sLayer, "TilesStartX", m_fTilesStartX );
	ini.GetValueF( sLayer, "TilesStartY", m_fTilesStartY );
	ini.GetValueF( sLayer, "TilesSpacingX", m_fTilesSpacingX );
	ini.GetValueF( sLayer, "TilesSpacingY", m_fTilesSpacingY );
	ini.GetValueF( sLayer, "TileVelocityX", m_fTileVelocityX );
	ini.GetValueF( sLayer, "TileVelocityY", m_fTileVelocityY );


	switch( m_Type )
	{
	case TYPE_SPRITE:
		m_Sprites.push_back(NewSprite(IsBanner));
		m_Sprites.back()->Load( sPath );
		m_Sprites.back()->SetXY( CENTER_X, CENTER_Y );
		break;
	case TYPE_STRETCH:
		{
			m_Sprites.push_back(NewSprite(IsBanner));
			RageTextureID ID(sPath);
			ID.bStretch = true;
			m_Sprites.back()->LoadBG( ID );
			m_Sprites.back()->StretchTo( RectI(SCREEN_LEFT,SCREEN_TOP,SCREEN_RIGHT,SCREEN_BOTTOM) );
			m_Sprites.back()->SetCustomTextureRect( RectF(0,0,1,1) );
		}
		break;
	case TYPE_PARTICLES:
		{
			for( int i=0; i<m_iNumParticles; i++ )
			{
				m_Sprites.push_back(NewSprite(IsBanner));
				m_Sprites.back()->Load( sPath );
				m_Sprites.back()->SetXY( randomf(SCREEN_LEFT,SCREEN_RIGHT), randomf(SCREEN_TOP,SCREEN_BOTTOM) );
				m_Sprites.back()->SetZoom( randomf(m_fZoomMin,m_fZoomMax) );
				m_vParticleVelocity[i] = RageVector3( 
					randomf(m_fVelocityXMin,m_fVelocityXMax),
					randomf(m_fVelocityYMin,m_fVelocityYMax),
					randomf(m_fVelocityZMin,m_fVelocityZMax) );
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
				m_Sprites.push_back(NewSprite(IsBanner));
				m_Sprites.back()->Load( ID );
				m_Sprites.back()->SetTextureWrapping( true );		// gets rid of some "cracks"
				m_Sprites.back()->SetZoom( randomf(m_fZoomMin,m_fZoomMax) );
			}
		}
		break;
	default:
		ASSERT(0);
	}

	bool bStartOnRandomFrame = false;
	ini.GetValueB( sLayer, "StartOnRandomFrame", bStartOnRandomFrame );
	if( bStartOnRandomFrame )
	{
		for( unsigned i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->SetState( rand()%m_Sprites[i]->GetNumStates() );
	}

	if( m_sCommand != "" )
	{
		for( unsigned i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->Command( m_sCommand );
	}
}

float BGAnimationLayer::GetMaxTweenTimeLeft() const
{
	float ret = 0;

	for( unsigned i=0; i<m_Sprites.size(); i++ )
		ret = max(ret, m_Sprites[i]->GetTweenTimeLeft());

	return ret;
}

void BGAnimationLayer::Update( float fDeltaTime )
{
	fDeltaTime *= m_fUpdateRate;

	const float fSongBeat = GAMESTATE->m_fSongBeat;
	
	unsigned i;
	for( i=0; i<m_Sprites.size(); i++ )
		m_Sprites[i]->Update( fDeltaTime );


	switch( m_Type )
	{
	case TYPE_SPRITE:
		break;
	case TYPE_STRETCH:
		for( i=0; i<m_Sprites.size(); i++ )
		{
			float fTexCoords[8];
			m_Sprites[i]->GetActiveTexCoords( fTexCoords );

			for( int j=0; j<8; j+=2 )
			{
				fTexCoords[j  ] += fDeltaTime*m_fStretchTexCoordVelocityX;
				fTexCoords[j+1] += fDeltaTime*m_fStretchTexCoordVelocityY;
			}
 
			m_Sprites[i]->SetCustomTextureCoords( fTexCoords );
		}
		break;
/*	case EFFECT_PARTICLES_SPIRAL_OUT:
		for( i=0; i<m_iNumSprites; i++ )
		{
			m_Sprites[i].SetZoom( m_Sprites[i].GetZoom() + fDeltaTime );
			if( m_Sprites[i].GetZoom() > SPIRAL_MAX_ZOOM )
				m_Sprites[i].SetZoom( SPIRAL_MIN_ZOOM );

			m_Sprites[i].SetRotationZ( m_Sprites[i].GetRotationZ() + fDeltaTime );

			float fRadius = (m_Sprites[i].GetZoom()-SPIRAL_MIN_ZOOM);
			fRadius *= fRadius;
			fRadius *= 200;
			m_Sprites[i].SetX( CENTER_X + cosf(m_Sprites[i].GetRotationZ())*fRadius );
			m_Sprites[i].SetY( CENTER_Y + sinf(m_Sprites[i].GetRotationZ())*fRadius );
		}
		break;
	case EFFECT_PARTICLES_SPIRAL_IN:
		for( i=0; i<m_iNumSprites; i++ )
		{
			m_Sprites[i].SetZoom( m_Sprites[i].GetZoom() - fDeltaTime );
			if( m_Sprites[i].GetZoom() < SPIRAL_MIN_ZOOM )
				m_Sprites[i].SetZoom( SPIRAL_MAX_ZOOM );

			m_Sprites[i].SetRotationZ( m_Sprites[i].GetRotationZ() - fDeltaTime );

			float fRadius = (m_Sprites[i].GetZoom()-SPIRAL_MIN_ZOOM);
			fRadius *= fRadius;
			fRadius *= 200;
			m_Sprites[i].SetX( CENTER_X + cosf(m_Sprites[i].GetRotationZ())*fRadius );
			m_Sprites[i].SetY( CENTER_Y + sinf(m_Sprites[i].GetRotationZ())*fRadius );
		}
		break;
*/
	case TYPE_PARTICLES:
		for( i=0; i<m_Sprites.size(); i++ )
		{
			m_Sprites[i]->SetX( m_Sprites[i]->GetX() + fDeltaTime*m_vParticleVelocity[i].x  );
			m_Sprites[i]->SetY( m_Sprites[i]->GetY() + fDeltaTime*m_vParticleVelocity[i].y  );
			m_Sprites[i]->SetZ( m_Sprites[i]->GetZ() + fDeltaTime*m_vParticleVelocity[i].z  );
			if( m_bParticlesBounce )
			{
				if( HitGuardRailLeft(m_Sprites[i]) )	
				{
					m_vParticleVelocity[i].x *= -1;
					m_Sprites[i]->SetX( GetGuardRailLeft(m_Sprites[i]) );
				}
				if( HitGuardRailRight(m_Sprites[i]) )	
				{
					m_vParticleVelocity[i].x *= -1;
					m_Sprites[i]->SetX( GetGuardRailRight(m_Sprites[i]) );
				}
				if( HitGuardRailTop(m_Sprites[i]) )	
				{
					m_vParticleVelocity[i].y *= -1;
					m_Sprites[i]->SetY( GetGuardRailTop(m_Sprites[i]) );
				}
				if( HitGuardRailBottom(m_Sprites[i]) )	
				{
					m_vParticleVelocity[i].y *= -1;
					m_Sprites[i]->SetY( GetGuardRailBottom(m_Sprites[i]) );
				}
			}
			else // !m_bParticlesBounce 
			{
				if( m_vParticleVelocity[i].x<0  &&  IsOffScreenLeft(m_Sprites[i]) )
					m_Sprites[i]->SetX( GetOffScreenRight(m_Sprites[i]) );
				if( m_vParticleVelocity[i].x>0  &&  IsOffScreenRight(m_Sprites[i]) )
					m_Sprites[i]->SetX( GetOffScreenLeft(m_Sprites[i]) );
				if( m_vParticleVelocity[i].y<0  &&  IsOffScreenTop(m_Sprites[i]) )
					m_Sprites[i]->SetY( GetOffScreenBottom(m_Sprites[i]) );
				if( m_vParticleVelocity[i].y>0  &&  IsOffScreenBottom(m_Sprites[i]) )
					m_Sprites[i]->SetY( GetOffScreenTop(m_Sprites[i]) );
			}
		}
		break;
	case TYPE_TILES:
		{
			float fSecs = RageTimer::GetTimeSinceStart();
			float fTotalWidth = m_iNumTilesWide * m_fTilesSpacingX;
			float fTotalHeight = m_iNumTilesHigh * m_fTilesSpacingY;
			
			ASSERT( int(m_Sprites.size()) == m_iNumTilesWide * m_iNumTilesHigh );

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
					
					m_Sprites[i]->SetX( fX );
					m_Sprites[i]->SetY( fY );
				}
			}
/*			
		for( i=0; i<m_iNumSprites; i++ )
		{
			m_Sprites[i].SetX( m_Sprites[i].GetX() + fDeltaTime*  );
			m_Sprites[i].SetY( m_Sprites[i].GetY() + fDeltaTime*m_vParticleVelocity[i].y  );
			m_Sprites[i].SetZ( m_Sprites[i].GetZ() + fDeltaTime*m_vParticleVelocity[i].z  );
			if( IsOffScreenLeft(&m_Sprites[i]) )
				m_Sprites[i].SetX( m_Sprites[i].GetX()-GetOffScreenLeft(&m_Sprites[i]) + GetOffScreenRight(&m_Sprites[i]) );
			if( IsOffScreenRight(&m_Sprites[i]) )
				m_Sprites[i].SetX( m_Sprites[i].GetX()-GetOffScreenRight(&m_Sprites[i]) + GetOffScreenLeft(&m_Sprites[i]) );
			if( IsOffScreenTop(&m_Sprites[i]) )
				m_Sprites[i].SetY( m_Sprites[i].GetY()-GetOffScreenTop(&m_Sprites[i]) + GetOffScreenBottom(&m_Sprites[i]) );
			if( IsOffScreenBottom(&m_Sprites[i]) )
				m_Sprites[i].SetY( m_Sprites[i].GetY()-GetOffScreenBottom(&m_Sprites[i]) + GetOffScreenTop(&m_Sprites[i]) );
				*/
		}
		break;
	case EFFECT_TILE_PULSE:
		for( i=0; i<m_Sprites.size(); i++ )
			m_Sprites[i]->SetZoom( sinf( fSongBeat*PI/2 ) );

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
		//	m_Sprites[0].SetXY( m_TweenX, m_TweenY);
			
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
			if(m_Sprites[0].GetY() < m_TweenY) // it needs to travel down
			{
				// Speed = Distance / Time....
				// Take away from the current position... the distance it has to travel divided by the time they want it done in...
				m_Sprites[0].SetY(m_Sprites[0].GetY() + ((m_TweenY - m_PosY)/(m_TweenSpeed*60)));

				if(m_Sprites[0].GetY() > m_TweenY) // passed the location we wanna go to?
				{
					m_Sprites[0].SetY(m_TweenY); // set it to the exact location we want
					m_TweenPassedY = 1; // say we passed it.
				}
			}
			else // travelling up
			{
				m_Sprites[0].SetY(m_Sprites[0].GetY() - ((m_TweenY + m_PosY)/(m_TweenSpeed*60)));

				if(m_Sprites[0].GetY() < m_TweenY)
				{
					m_Sprites[0].SetY(m_TweenY);
					m_TweenPassedY = 1;
				}
			}
		}

		if(m_TweenPassedX != 1) // Check to see if we still need to Tween Along the X Axis
		{
			if(m_Sprites[0].GetX() < m_TweenX) // it needs to travel right
			{
				m_Sprites[0].SetX(m_Sprites[0].GetX() + ((m_TweenX - m_PosX)/(m_TweenSpeed*60)));
				if(m_Sprites[0].GetX() > m_TweenX)
				{
					m_Sprites[0].SetX(m_TweenX);
					m_TweenPassedX = 1;
				}
			}
			else // travelling left
			{
				m_Sprites[0].SetX(m_Sprites[0].GetX() - ((m_TweenX + m_PosX)/(m_TweenSpeed*60)));
				if(m_Sprites[0].GetX() < m_TweenX)
				{
					m_Sprites[0].SetX(m_TweenX);
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
			m_Sprites[0].SetDiffuse( RageColor(1,1,1,1) );
		}		
	}
	if(m_HideTime != 0 && !(m_HideTime < 0)) // make sure it's not 0 or less than 0...
	{
		m_HideTime -= fDeltaTime;
		if(m_HideTime <= 0) // if we've gone past the magic point... hide the beast....
		{
			m_Sprites[0].SetDiffuse( RageColor(0,0,0,0) );
		}
		
	}
	*/
}

void BGAnimationLayer::Draw()
{
	for( unsigned i=0; i<m_Sprites.size(); i++ )
		m_Sprites[i]->Draw();
}

void BGAnimationLayer::SetDiffuse( RageColor c )
{
	for(unsigned i=0; i<m_Sprites.size(); i++) 
		m_Sprites[i]->SetDiffuse(c);
}

void BGAnimationLayer::GainingFocus( float fRate, bool bRewindMovie, bool bLoop )
{
	m_fUpdateRate = fRate;
	m_Sprites.back()->GetTexture()->SetPlaybackRate(fRate);
	if( bRewindMovie )
		m_Sprites[0]->GetTexture()->SetPosition( 0 );
	m_Sprites.back()->GetTexture()->SetLooping(bLoop);

	// if movie texture, pause and play movie so we don't waste CPU cycles decoding frames that won't be shown
	m_Sprites[0]->GetTexture()->Play();

	for( unsigned i=0; i<m_Sprites.size(); i++ )
		m_Sprites[i]->Command( m_sCommand );
}

void BGAnimationLayer::LosingFocus()
{
	m_Sprites[0]->GetTexture()->Pause();
}
