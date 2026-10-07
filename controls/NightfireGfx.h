/*
NightfireGfx.h -- Nightfire (2002) front end look for mainui_cpp
Copyright (C) 2026 mogeldev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Retail layout source: game data `gui/Scripts/Mainmenu/Nightfire.txt`
(virtual 640x480 canvas).  See docs/menu-plan.md in the port repo.
*/
#pragma once
#ifndef NIGHTFIRE_GFX_H
#define NIGHTFIRE_GFX_H

#include "BaseMenu.h"

enum NfDrawMode
{
	NF_NORMAL = 0,
	NF_TRANS,
	NF_ADDITIVE
};

namespace Nf
{
// ui_nfmenu, sv_iamdone; called from UI_Init (before config.cfg runs)
void RegisterCvars( void );
// true when the current game is the Nightfire port (gamefolder "bond")
bool Active( void );
// sv_iamdone: the campaign was finished once (unlocks MISSION SELECT)
bool GameWon( void );

// centred 4:3 box (640x480) inside the current window, in screen pixels
void GetBox( float &x, float &y, float &k );
// retail 640x480 coordinates -> screen pixels
void MapRect( float rx, float ry, float rw, float rh, int &x, int &y, int &w, int &h );
inline int MapX( float rx ) { float x, y, k; GetBox( x, y, k ); return (int)( x + rx * k ); }
inline int MapY( float ry ) { float x, y, k; GetBox( x, y, k ); return (int)( y + ry * k ); }
inline int MapS( float rs ) { float x, y, k; GetBox( x, y, k ); return (int)( rs * k ); }

// picture, at retail rectangle; u0/v0/u1/v1 is a source window in texels and
// wraps around the texture (the retail stars and earth layers scroll like that)
// src: optional texel window; left > right mirrors (retail TEXTURE_RECT 32 0 0 32)
void DrawPic( const char *path, float rx, float ry, float rw, float rh, unsigned rgba, int mode, const wrect_t *src = NULL );
// solid colour, retail rectangle
void FillRect( float rx, float ry, float rw, float rh, unsigned rgba );
void DrawPicWindow( const char *path, float rx, float ry, float rw, float rh,
	float u0, float v0, float u1, float v1, unsigned rgba, int mode );
// rotated around the centre of the retail rectangle (needs the engine fork's
// pfnPIC_DrawRotated; falls back to an unrotated draw)
void DrawPicRot( const char *path, float rx, float ry, float rw, float rh, float degrees, unsigned rgba, int mode );

// retail bitmap font (gui/fonts/<name>.dat + .png), scaled
struct nffont_t
{
	const char *name;   // e.g. "SerpentineMedium-20"
	void *data;         // nffontdata_t
};

bool LoadFont( nffont_t &font, const char *name );
// cached LoadFont; name must be a string literal
const nffont_t &GetFont( const char *name );
// widths and heights in retail 640x480 units
float TextWidth( const nffont_t &font, const char *text, float scale );
float FontHeight( const nffont_t &font, float scale );
void DrawText( const nffont_t &font, const char *text, float rx, float ry, float boxw,
	float scale, unsigned rgba, int justify, bool dropshadow );
// separate x / y scale like the retail SCALE sx sy; len -1 = whole string
void DrawText2( const nffont_t &font, const char *text, int len, float rx, float ry, float boxw,
	float sx, float sy, unsigned rgba, int justify, bool dropshadow );
// left-justified, wrapped at spaces to wrapw (retail WRAP); returns the height used
float DrawTextWrap( const nffont_t &font, const char *text, float rx, float ry, float wrapw,
	float sx, float sy, unsigned rgba, bool dropshadow );

// the retail front end backgrounds; the shared CMenuBackgroundBitmap hook
// draws the one the top Nightfire page selected
enum NfBackground
{
	NF_BG_MAIN = 0,	// MainMenu / SinglePlayerMenu / escape: dome, Bond, curve
	NF_BG_TAB	// sub screens (MissionSelectMenu ...): pic_tabback, dial, logo top right
};
void SetBackground( int kind );
void DrawBackground( void );
}

#endif // NIGHTFIRE_GFX_H