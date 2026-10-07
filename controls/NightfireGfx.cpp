/*
NightfireGfx.cpp -- Nightfire (2002) front end look for mainui_cpp
Copyright (C) 2026 mogeldev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/
#include "NightfireGfx.h"
#include "Bitmap.h"
#include "enginecallback_menu.h"
#include "Utils.h"
#include "gameinfo.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace Nf
{

struct nfglyph_t
{
	int x, y, w, h;
};

struct nffontdata_t
{
	HIMAGE pic;
	int cellw, cellh;
	nfglyph_t glyphs[256];
};

static cvar_t *nf_menu_cvar;
static cvar_t *nf_iamdone_cvar;

void RegisterCvars( void )
{
	// opt-out of the Nightfire front end (stock menu)
	nf_menu_cvar = EngFuncs::CvarRegister( "ui_nfmenu", "1", FCVAR_ARCHIVE );
	// retail engine cvar, set to 1 by trigger_endgame status 2 (game won),
	// unlocks MISSION SELECT; archived so the unlock survives a restart
	// [assumed: the retail flags (constructor argument 8) are not decoded]
	nf_iamdone_cvar = EngFuncs::CvarRegister( "sv_iamdone", "0", FCVAR_ARCHIVE );
}

bool GameWon( void )
{
	return nf_iamdone_cvar && nf_iamdone_cvar->value >= 1.0f;
}

bool Active( void )
{
	if( nf_menu_cvar && nf_menu_cvar->value <= 0.0f )
		return false;

	if( stricmp( gMenu.m_gameinfo.gamefolder, "bond" ))
		return false;

	// the rotating layers need the engine fork's pfnPIC_DrawRotated; without
	// it (stock engine) the retail front end would look wrong, so stay stock
	return EngFuncs::HasPIC_DrawRotated();
}

void GetBox( float &x, float &y, float &k )
{
	k = Q_min( ScreenWidth / 640.0f, ScreenHeight / 480.0f );
	x = ( ScreenWidth - 640.0f * k ) * 0.5f;
	y = ( ScreenHeight - 480.0f * k ) * 0.5f;
}

void MapRect( float rx, float ry, float rw, float rh, int &x, int &y, int &w, int &h )
{
	float bx, by, k;
	GetBox( bx, by, k );
	x = (int)( bx + rx * k );
	y = (int)( by + ry * k );
	w = (int)ceilf( rw * k );
	h = (int)ceilf( rh * k );
}

static void DrawPicTiled( HIMAGE h, int x, int y, int w, int hh, float u0, float v0, float u1, float v1, unsigned rgba, int mode )
{
	int tw = EngFuncs::PIC_Width( h ), th = EngFuncs::PIC_Height( h );
	float dx = u1 - u0, dy = v1 - v0;
	float xa = w / dx, ya = hh / dy;

	if( !tw || !th || w <= 0 || hh <= 0 )
		return;

	// engine cannot wrap: split each axis into the piece that fits the texture
	for( float u = u0; u < u1 - 0.001f; )
	{
		float next = ( floorf( u / tw ) + 1.0f ) * tw;
		float uw = Q_min( next, u1 ) - u;
		for( float v = v0; v < v1 - 0.001f; )
		{
			float vnext = ( floorf( v / th ) + 1.0f ) * th;
			float vh = Q_min( vnext, v1 ) - v;
			int px = x + (int)(( u - u0 ) * xa );
			int py = y + (int)(( v - v0 ) * ya );
			int pw = (int)ceilf( uw * xa );
			int ph = (int)ceilf( vh * ya );
			wrect_t rc;

			rc.left = (int)u % tw;
			rc.top = (int)v % th;
			rc.right = (int)( rc.left + uw + 0.5f );
			rc.bottom = (int)( rc.top + vh + 0.5f );
			if( rc.right > tw ) rc.right = tw;
			if( rc.bottom > th ) rc.bottom = th;

			EngFuncs::PIC_Set( h, Red( rgba ), Green( rgba ), Blue( rgba ), Alpha( rgba ));
			if( mode == NF_ADDITIVE )
				EngFuncs::PIC_DrawAdditive( px, py, pw, ph, &rc );
			else
				EngFuncs::PIC_DrawTrans( px, py, pw, ph, &rc );

			v = vnext;
		}
		u = next;
	}
}

static HIMAGE GetPic( const char *path )
{
	return EngFuncs::PIC_Load( path );
}

void DrawPicWindow( const char *path, float rx, float ry, float rw, float rh,
	float u0, float v0, float u1, float v1, unsigned rgba, int mode )
{
	HIMAGE h = GetPic( path );
	int x, y, w, hh;

	MapRect( rx, ry, rw, rh, x, y, w, hh );
	DrawPicTiled( h, x, y, w, hh, u0, v0, u1, v1, rgba, mode );
}

void DrawPic( const char *path, float rx, float ry, float rw, float rh, unsigned rgba, int mode, const wrect_t *src )
{
	HIMAGE h = GetPic( path );
	int x, y, w, hh;

	MapRect( rx, ry, rw, rh, x, y, w, hh );
	EngFuncs::PIC_Set( h, Red( rgba ), Green( rgba ), Blue( rgba ), Alpha( rgba ));
	if( mode == NF_ADDITIVE )
		EngFuncs::PIC_DrawAdditive( x, y, w, hh, src );
	else
		EngFuncs::PIC_DrawTrans( x, y, w, hh, src );
}

void FillRect( float rx, float ry, float rw, float rh, unsigned rgba )
{
	int x, y, w, h;

	MapRect( rx, ry, rw, rh, x, y, w, h );
	UI_FillRect( x, y, w, h, rgba );
}

void DrawPicRot( const char *path, float rx, float ry, float rw, float rh, float degrees, unsigned rgba, int mode )
{
	HIMAGE h = GetPic( path );
	int x, y, w, hh;

	MapRect( rx, ry, rw, rh, x, y, w, hh );
	EngFuncs::PIC_Set( h, Red( rgba ), Green( rgba ), Blue( rgba ), Alpha( rgba ));
	if( !EngFuncs::PIC_DrawRotated(
		( x + w * 0.5f ), ( y + hh * 0.5f ), (float)w, (float)hh, degrees, mode == NF_ADDITIVE ))
	{
		if( mode == NF_ADDITIVE )
			EngFuncs::PIC_DrawAdditive( x, y, w, hh );
		else
			EngFuncs::PIC_DrawTrans( x, y, w, hh );
	}
}

bool LoadFont( nffont_t &font, const char *name )
{
	char path[128];
	char *buf;
	int len;
	nffontdata_t *d = (nffontdata_t *)malloc( sizeof( nffontdata_t ));

	memset( d, 0, sizeof( *d ));
	snprintf( path, sizeof( path ), "gui/fonts/%s.dat", name );
	buf = (char *)EngFuncs::COM_LoadFile( path, &len );
	if( !buf )
	{
		free( d );
		Con_Printf( "Nightfire menu: font %s not found\n", path );
		return false;
	}

	// text file, one "code width height x y" per glyph
	{
		char *line = buf;
		while( line && *line )
		{
			char *nl = strchr( line, '\n' );
			int code, gw, gh, gx, gy;

			if( nl ) *nl = 0;
			if( sscanf( line, "%d %d %d %d %d", &code, &gw, &gh, &gx, &gy ) == 5 &&
				code >= 0 && code < 256 )
			{
				d->glyphs[code].w = gw;
				d->glyphs[code].h = gh;
				d->glyphs[code].x = gx;
				d->glyphs[code].y = gy;
			}
			else if( !strncmp( line, "cellsize", 8 ))
			{
				sscanf( line, "cellsize %d %d", &d->cellw, &d->cellh );
			}
			line = nl ? nl + 1 : NULL;
		}
	}

	snprintf( path, sizeof( path ), "gui/fonts/%s.png", name );
	d->pic = EngFuncs::PIC_Load( path );
	EngFuncs::COM_FreeFile( buf );

	font.name = name;
	font.data = d;
	return d->pic != 0;
}

const nffont_t &GetFont( const char *name )
{
	// the menus use two or three fonts; load each once
	static nffont_t fonts[8];
	static int numFonts;
	int i;

	for( i = 0; i < numFonts; i++ )
	{
		if( !strcmp( fonts[i].name, name ))
			return fonts[i];
	}

	if( numFonts == 8 )
		return fonts[0];

	LoadFont( fonts[numFonts], name );
	fonts[numFonts].name = name; // literals only
	return fonts[numFonts++];
}

float TextWidth( const nffont_t &font, const char *text, float scale )
{
	nffontdata_t *d = (nffontdata_t *)font.data;
	int w = 0;

	if( !d ) return 0;
	for( ; *text; text++ )
		w += d->glyphs[(byte)*text].w;
	return w * scale;
}

float FontHeight( const nffont_t &font, float scale )
{
	nffontdata_t *d = (nffontdata_t *)font.data;

	return d ? d->cellh * scale : 0.0f;
}

static float TextWidthN( const nffont_t &font, const char *text, int len, float sx )
{
	nffontdata_t *d = (nffontdata_t *)font.data;
	int w = 0;

	if( !d ) return 0;
	for( int i = 0; i < len && text[i]; i++ )
		w += d->glyphs[(byte)text[i]].w;
	return w * sx;
}

void DrawText( const nffont_t &font, const char *text, float rx, float ry, float boxw,
	float scale, unsigned rgba, int justify, bool dropshadow )
{
	DrawText2( font, text, -1, rx, ry, boxw, scale, scale, rgba, justify, dropshadow );
}

void DrawText2( const nffont_t &font, const char *text, int len, float rx, float ry, float boxw,
	float sx, float sy, unsigned rgba, int justify, bool dropshadow )
{
	nffontdata_t *d = (nffontdata_t *)font.data;
	float bx, by, k, x;
	int y;

	if( !d || !d->pic )
		return;

	if( len < 0 )
		len = (int)strlen( text );

	// all metrics in retail 640x480 units, mapped per glyph
	switch( justify )
	{
	case QM_CENTER: x = rx + ( boxw - TextWidthN( font, text, len, sx )) * 0.5f; break;
	case QM_RIGHT:  x = rx + boxw - TextWidthN( font, text, len, sx ); break;
	default:        x = rx; break;
	}

	GetBox( bx, by, k );
	y = (int)( by + ry * k );

	for( int i = 0; i < len && text[i]; i++ )
	{
		nfglyph_t &g = d->glyphs[(byte)text[i]];
		int px = (int)( bx + x * k );
		int gw = (int)ceilf( g.w * sx * k );
		int gh = (int)ceilf( g.h * sy * k );
		int shadow = Q_max( 1, (int)( k + 0.5f ));
		wrect_t rc;

		x += g.w * sx;
		if( g.w <= 0 || g.h <= 0 )
			continue;

		rc.left = g.x; rc.top = g.y;
		rc.right = g.x + g.w; rc.bottom = g.y + g.h;

		if( dropshadow )
		{
			EngFuncs::PIC_Set( d->pic, 0, 0, 0, Alpha( rgba ) / 2 );
			EngFuncs::PIC_DrawTrans( px + shadow, y + shadow, gw, gh, &rc );
		}

		EngFuncs::PIC_Set( d->pic, Red( rgba ), Green( rgba ), Blue( rgba ), Alpha( rgba ));
		EngFuncs::PIC_DrawTrans( px, y, gw, gh, &rc );
	}
}

float DrawTextWrap( const nffont_t &font, const char *text, float rx, float ry, float wrapw,
	float sx, float sy, unsigned rgba, bool dropshadow )
{
	// retail WRAP: break at spaces, one cell height per line
	float lineh = FontHeight( font, sy );
	float y = ry;

	while( *text )
	{
		int fit = 0, lastSpace = -1, n;

		while( text[fit] && text[fit] != '\n' )
		{
			if( text[fit] == ' ' )
				lastSpace = fit;
			if( TextWidthN( font, text, fit + 1, sx ) > wrapw )
				break;
			fit++;
		}

		if( text[fit] && text[fit] != '\n' && lastSpace > 0 )
			n = lastSpace;	// break at the last space that fits
		else
			n = Q_max( fit, 1 );

		DrawText2( font, text, n, rx, y, wrapw, sx, sy, rgba, QM_LEFT, dropshadow );
		y += lineh;

		text += n;
		while( *text == ' ' || *text == '\n' )
			text++;
	}
	return y - ry;
}

static int s_background = NF_BG_MAIN;

static void DrawBackgroundMain( float t );

void SetBackground( int kind )
{
	s_background = kind;
}

static void DrawBackgroundTab( float t )
{
	// retail sub screen background (MissionSelectMenu, Nightfire.txt pic_tabback ...)
	DrawPic( "gui/frontend/i_glow_bg2.png", 0, 0, 640, 480, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );

	DrawPicRot( "gui/frontend/i_swirl_01.png", -50, 14, 334, 320, 360.0f * fmodf( t / 10.0f, 1.0f ), PackRGBA( 255, 255, 255, 96 ), NF_ADDITIVE );
	DrawPicRot( "gui/frontend/i_swirl_06.png", -50, 14, 334, 320, 360.0f * fmodf( t / 20.0f, 1.0f ), PackRGBA( 255, 255, 255, 96 ), NF_ADDITIVE );
	DrawPicRot( "gui/frontend/i_spin_04.png", -10, 46, 256, 256, 360.0f * ( 1.0f - fmodf( t / 30.0f, 1.0f )), PackRGBA( 255, 255, 255, 255 ), NF_ADDITIVE );
	DrawPic( "gui/frontend/i_dial.png", -12, 46, 256, 256, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );

	DrawPic( "gui/frontend/i_logo_nightfire.png", 400, 36, 190, 45, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_tm.png", 450, 56, 12, 8, PackRGBA( 230, 179, 76, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_tm.png", 574, 56, 12, 8, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
}

void DrawBackground( void )
{
	// retail MainMenu background, Z order 1..8 (see docs/menu-plan.md)
	float t = uiStatic.realTime / 1000.0f;
	float bx, by, k;

	UI_FillRect( 0, 0, (int)ScreenWidth, (int)ScreenHeight, PackRGBA( 0, 0, 0, 255 ));

	if( s_background == NF_BG_TAB )
	{
		DrawBackgroundTab( t );
	}
	else
	{
		DrawBackgroundMain( t );
	}

	// the 640x480 screen ends at the box; layers that reach past it (swirls at
	// x -50, rotated quads) must not spill into the bars of wide windows
	GetBox( bx, by, k );
	UI_FillRect( 0, 0, (int)bx, (int)ScreenHeight, PackRGBA( 0, 0, 0, 255 ));
	UI_FillRect( (int)( bx + 640.0f * k ), 0, (int)ScreenWidth, (int)ScreenHeight, PackRGBA( 0, 0, 0, 255 ));
	UI_FillRect( 0, 0, (int)ScreenWidth, (int)by, PackRGBA( 0, 0, 0, 255 ));
	UI_FillRect( 0, (int)( by + 480.0f * k ), (int)ScreenWidth, (int)ScreenHeight, PackRGBA( 0, 0, 0, 255 ));
}

static void DrawBackgroundMain( float t )
{
	// stars: texture window slides 0..1024 -> 1024..2048 over 100 s, 100 s loop
	{
		// twinkle: starsFrame runs 1..7 in LERP_TIME 1 s (retail UpdateStarTexture)
		static const char *const frames[7] =
		{
			"gui/frontend/i_stars_bg_01.png", "gui/frontend/i_stars_bg_02.png",
			"gui/frontend/i_stars_bg_03.png", "gui/frontend/i_stars_bg_04.png",
			"gui/frontend/i_stars_bg_03.png", "gui/frontend/i_stars_bg_02.png",
			"gui/frontend/i_stars_bg_01.png"
		};
		float u = 1024.0f * fmodf( t / 100.0f, 1.0f );
		int frame = Q_min( 6, (int)( fmodf( t, 1.0f ) * 7.0f ));
		DrawPicWindow( frames[frame], 0, 0, 640, 256, u, 0, u + 1024.0f, 256.0f, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	}

	// material "frontend" like the dome / Bond layers (alpha blend); the RGB
	// picture is opaque and covers the stars, additive left a seam at y 256
	DrawPic( "gui/frontend/i_glow_bg.png", 0, 0, 640, 480, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );

	// swirls, 0->359 deg in 10 s / 20 s, alpha 96
	DrawPicRot( "gui/frontend/i_swirl_01.png", 152, 0, 334, 320, 360.0f * fmodf( t / 10.0f, 1.0f ), PackRGBA( 255, 255, 255, 96 ), NF_ADDITIVE );
	DrawPicRot( "gui/frontend/i_swirl_06.png", 152, 0, 334, 320, 360.0f * fmodf( t / 20.0f, 1.0f ), PackRGBA( 255, 255, 255, 96 ), NF_ADDITIVE );

	DrawPic( "gui/frontend/i_dome_01.png", 190, 32, 256, 256, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPicRot( "gui/frontend/i_spin_01.png", 192, 34, 256, 256, 360.0f * fmodf( t / 20.0f, 1.0f ), PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_spin_03.png", 256, 96, 128, 128, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPicRot( "gui/frontend/i_spin_04.png", 192, 32, 256, 256, 360.0f * ( 1.0f - fmodf( t / 30.0f, 1.0f )), PackRGBA( 255, 255, 255, 255 ), NF_ADDITIVE );

	// earth: 96x128 window slides x 0..256 over 25 s
	DrawPicWindow( "gui/frontend/i_earth.png", 295, 135, 50, 50, 256.0f * fmodf( t / 25.0f, 1.0f ), 0, 256.0f * fmodf( t / 25.0f, 1.0f ) + 96.0f, 128.0f, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_earth_light.png", 293, 133, 54, 54, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_dome_02.png", 256, 96, 128, 128, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );

	DrawPic( "gui/frontend/i_bond.png", 180, 20, 460, 460, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_bottom_curve.png", 0, 352, 640, 128, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );

	DrawPic( "gui/frontend/i_logo_nightfire.png", 224, 50, 192, 48, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_tm.png", 275, 75, 12, 8, PackRGBA( 230, 179, 76, 255 ), NF_NORMAL );
	DrawPic( "gui/frontend/i_tm.png", 399, 75, 12, 8, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
}

} // namespace Nf