/*
NightfireMenu.h -- Nightfire (2002) styled front end pages
Copyright (C) 2026 mogeldev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Retail layout: game data gui/Scripts/Mainmenu/Nightfire.txt (MainMenu,
SinglePlayerMenu, MissionSelectMenu), gui/Scripts/Escapemenu/menu.txt and
mpmenu.txt (EscapeScreen, MultiplayerEscapeScreen),
gui/Scripts/Dialogs/dlg_difficulty.txt. Rows at y = 267 + 27*i, text x = 120
width 400; see docs/menu-plan.md.
*/
#pragma once
#ifndef NIGHTFIRE_MENU_H
#define NIGHTFIRE_MENU_H

#include "Framework.h"
#include "NightfireGfx.h"
#include "YesNoMessageBox.h"
#include <string.h>

class CMenuNfButton;

// the buttons of one Nightfire page or dialog: caption rows with the hilite
// glow, small captions (RESUME / BACK / START) and picture buttons (arrows)
class CNfButtonList
{
public:
	CNfButtonList() : numButtons( 0 ) { }

	enum { NF_MAX_BUTTONS = 12 };

	// big caption row 0..5 (retail main rows)
	CMenuNfButton *AddRow( CMenuItemsHolder *holder, const char *text, int row, const CEventCallback &cb );
	// small caption (retail RESUME / BACK: ConduitMdITCTT-20, 0.65, grey, hot white)
	CMenuNfButton *AddSmall( CMenuItemsHolder *holder, const char *text, float rx, float ry, float rw, float rh, const CEventCallback &cb );
	// hot / pressed state of every button (call from Think)
	void Think( CMenuBaseItem *itemAtCursor );

private:
	CMenuNfButton *buttons[NF_MAX_BUTTONS];
	int numButtons;
};

// a full-screen retail page over one of the two backgrounds
class CMenuNfPage : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;

	CMenuNfPage( const char *name, int background ) : CMenuFramework( name ), bgKind( background ) { }

protected:
	CMenuNfButton *AddRow( const char *text, int row, const CEventCallback &cb )
	{ return list.AddRow( this, text, row, cb ); }
	CMenuNfButton *AddSmall( const char *text, float rx, float ry, float rw, float rh, const CEventCallback &cb )
	{ return list.AddSmall( this, text, rx, ry, rw, rh, cb ); }

	void Think( void ) override;
	void Draw( void ) override;

	CNfButtonList list;
	int bgKind;

	CMenuYesNoMessageBox dialog;
};

class CMenuNfMain : public CMenuNfPage
{
public:
	CMenuNfMain() : CMenuNfPage( "CMenuNfMain", Nf::NF_BG_MAIN ), cont( NULL ) { }

	bool KeyDown( int key ) override;

	void QuitDialogCb( void );

	static bool GameHasSaves( void );

private:
	void _Init( void ) override;
	void _VidInit( void ) override;

	void ContinueCb( void );

	CMenuNfButton *cont;
};

// retail SinglePlayerMenu: NEW GAME, LOAD GAME, MISSION SELECT (after the game is won)
class CMenuNfSingle : public CMenuNfPage
{
public:
	CMenuNfSingle() : CMenuNfPage( "CMenuNfSingle", Nf::NF_BG_MAIN ), missions( NULL ) { }

private:
	void _Init( void ) override;
	void _VidInit( void ) override;

	void NewGameCb( void );

	CMenuNfButton *missions;
};

// retail MissionSelectMenu: briefing per mission, arrows, START -> difficulty
class CMenuNfMissions : public CMenuNfPage
{
public:
	CMenuNfMissions() : CMenuNfPage( "CMenuNfMissions", Nf::NF_BG_TAB ), current( 0 ) { }

	void Draw( void ) override;

	// 0..8 = m1 .. m9
	void SetMission( int i ) { current = (( i % 9 ) + 9 ) % 9; }

private:
	void _Init( void ) override;

	void PrevCb( void );
	void NextCb( void );
	void StartCb( void );

	int current;
};

// in-game escape menu (single player and multiplayer variants)
class CMenuNfEscape : public CMenuNfPage
{
public:
	CMenuNfEscape() : CMenuNfPage( "CMenuNfEscape", Nf::NF_BG_MAIN ), multiplayer( false ) { }

	bool KeyDown( int key ) override;

	void QuitDialogCb( void );
	void EndGameDialogCb( void );

private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void Think( void ) override;

	void EndGameCb( void );

	CMenuNfButton *rows[5];
	CMenuNfButton *mpRows[5];
	bool multiplayer;
};

// retail dlg_difficulty.txt over the current page: OPERATIVE / AGENT / 00 AGENT
class CMenuNfDifficulty : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;

	CMenuNfDifficulty() : CMenuBaseWindow( "CMenuNfDifficulty" ) { map[0] = 0; }

	void Open( const char *mapname );
	void Draw( void ) override;
	void Think( void ) override;

private:
	void _Init( void ) override;

	void Start( int skill );
	void EasyCb( void ) { Start( 1 ); }
	void NormalCb( void ) { Start( 2 ); }
	void HardCb( void ) { Start( 3 ); }

	CNfButtonList list;
	char map[64];
};

// shows the Nightfire main page (or the escape page in game) when the
// Nightfire menu is active, otherwise returns false (stock menu)
bool UI_NfMain_Show( void );
// quit dialog of the active Nightfire page; false when the menu is not active
bool UI_NfMain_QuitDialog( void );
void UI_NfMain_Credits( void );

// the Nightfire credits: our own scroller over bond/gui/scripts/mainmenu/credits.txt
class CMenuNfCredits : public CMenuBaseWindow
{
public:
	CMenuNfCredits() : CMenuBaseWindow( "CMenuNfCredits" )
	{
		lines = NULL;
		numLines = 0;
		startTime = 0;
	}

	void Show( void ) override
	{
		CMenuBaseWindow::Show();
		startTime = uiStatic.realTime;
	}

	bool KeyDown( int key ) override
	{
		if( UI::Key::IsEscape( key ) || UI::Key::IsEnter( key ) || UI::Key::IsLeftMouse( key ))
		{
			Hide();
			return true;
		}
		return true;
	}

	void Draw( void ) override;
	void _Init( void ) override;

private:
	char **lines;
	int numLines;
	int startTime;
};

#endif // NIGHTFIRE_MENU_H
