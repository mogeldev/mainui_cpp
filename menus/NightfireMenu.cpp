/*
NightfireMenu.cpp -- Nightfire (2002) styled front end pages
Copyright (C) 2026 mogeldev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/
#include "NightfireMenu.h"
#include "NightfireGfx.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "Utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// retail Mainmenu/Nightfire.txt InitNightfire: GUI_ACTION_MUSIC PLAY gui ambient frontend_ectest.ogg
#define NF_FRONTEND_MUSIC "sound/music/mission/gui/frontend_ectest.ogg"

// retail caption colours of the six main rows (state 0 normal / 3 hot);
// state 2 is white (the script's own comments name 2 "pressed", 3 "hot")
static const unsigned int nfRowColor[6] = { 0xFFFFC962, 0xFFFEB152, 0xFFFF9942, 0xFFFE7D30, 0xFFFE621E, 0xFFFD4C0F };
static const unsigned int nfWhite = 0xFFFFFFFF;
// retail small captions RESUME / BACK / START (ConduitMdITCTT-20, state 0)
static const unsigned int nfSmallColor = 0xFF6F746E;
// retail caption colour of titles and texts on the sub screens
static const unsigned int nfTextColor = 0xFFFADBB3;

static void UI_NfSingle_Menu( void );
static void UI_NfMissions_Menu( void );
static void UI_NfDifficulty_Open( const char *map );

// =====================================================================

class CMenuNfButton : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuNfButton() : text( "" ), fontName( "SerpentineMedium-20" ), sx( 0.8f ), sy( 0.8f ),
		color( nfRowColor[0] ), hotColor( nfRowColor[0] ), pressedColor( nfWhite ),
		shadow( true ), glowRow( -1 ), pic( NULL ), rx( 0 ), ry( 0 ), rw( 0 ), rh( 0 ),
		m_hot( false ), m_down( false ), glow( 0.0f )
	{
		// rect in screen pixels, set from the retail 640x480 rect in VidInit
		iFlags = QMF_DISABLESCAILING;
		eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
		szName = "";
		picSrc.left = picSrc.top = picSrc.right = picSrc.bottom = 0;
	}

	bool IsAbsolutePositioned( void ) const override { return true; }

	void VidInit( void ) override
	{
		int x, y, w, h;

		Nf::MapRect( rx, ry, rw, rh, x, y, w, h );
		SetRect( x, y, w, h );
		BaseClass::VidInit();
	}

	// same key roles as the stock CMenuBitmap: down fires, up plays the sound
	bool KeyUp( int key ) override
	{
		m_down = false;
		if( !UI::Key::IsEnter( key ) && !( UI::Key::IsLeftMouse( key ) && FBitSet( iFlags, QMF_HASMOUSEFOCUS )))
			return false;

		_Event( QM_PRESSED );
		PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
		return true;
	}

	bool KeyDown( int key ) override
	{
		if( !UI::Key::IsEnter( key ) && !( UI::Key::IsLeftMouse( key ) && FBitSet( iFlags, QMF_HASMOUSEFOCUS )))
			return false;

		m_down = true;
		_Event( QM_RELEASED );
		return true;
	}

	void Draw( void ) override
	{
		unsigned rgba;

		if( FBitSet( iFlags, QMF_GRAYED ))
			rgba = 0xFF808080;
		else if( m_hot && m_down )
			rgba = pressedColor;
		else if( m_hot )
			rgba = hotColor;
		else
			rgba = color;

		// retail main_hilite_N: 145, row y - 21, 350x68, spr_additive; the
		// state 0..100 blends colour white/alpha 0 -> row colour/alpha 255
		if( glowRow >= 0 && glow > 0.0f )
		{
			float f = glow / 100.0f;
			unsigned rgb = PackRGBA(
				(unsigned)( 255 + ( (int)Red( color ) - 255 ) * f ),
				(unsigned)( 255 + ( (int)Green( color ) - 255 ) * f ),
				(unsigned)( 255 + ( (int)Blue( color ) - 255 ) * f ),
				(unsigned)( 255 * f ));
			Nf::DrawPic( "gui/frontend/i_menu_glow.png", 145, ry - 21, 350, 68, rgb, NF_ADDITIVE );
		}

		if( pic )
		{
			Nf::DrawPic( pic, rx, ry, rw, rh, rgba, NF_NORMAL, picSrc.right || picSrc.left ? &picSrc : NULL );
			return;
		}

		// retail captions: JUSTIFY CENTER, VERTICAL CENTER
		const Nf::nffont_t &font = Nf::GetFont( fontName );
		Nf::DrawText2( font, text, -1, rx, ry + ( rh - Nf::FontHeight( font, sy )) * 0.5f,
			rw, sx, sy, rgba, QM_CENTER, shadow );
	}

	void SetHot( bool h )
	{
		m_hot = h;
		if( !h ) m_down = false;
	}

	void ThinkGlow( void )
	{
		// retail UpdateHiliteState: jump to full on hover, -3 per frame when it goes away
		if( m_hot )
			glow = 100.0f;
		else if( glow > 0.0f )
			glow = Q_max( 0.0f, glow - 3.0f );
	}

	const char *text;
	const char *fontName;
	float sx, sy;
	unsigned color, hotColor, pressedColor;
	bool shadow;
	int glowRow;	// -1: no hilite glow
	const char *pic;	// picture button (arrows) instead of a caption
	wrect_t picSrc;
	float rx, ry, rw, rh;	// retail 640x480 rect

private:
	bool m_hot, m_down;
	float glow;
};

// =====================================================================

CMenuNfButton *CNfButtonList::AddRow( CMenuItemsHolder *holder, const char *text, int row, const CEventCallback &cb )
{
	CMenuNfButton *b = AddSmall( holder, text, 120, 267 + 27 * row, 400, 25, cb );

	// retail main rows: SerpentineMedium-20, 0.8, drop shadow; hot keeps the
	// row colour (the glow shows it), pressed white
	b->fontName = "SerpentineMedium-20";
	b->sx = b->sy = 0.8f;
	b->color = b->hotColor = nfRowColor[row];
	b->pressedColor = nfWhite;
	b->shadow = true;
	b->glowRow = row;
	return b;
}

CMenuNfButton *CNfButtonList::AddSmall( CMenuItemsHolder *holder, const char *text, float rx, float ry, float rw, float rh, const CEventCallback &cb )
{
	CMenuNfButton *b = new CMenuNfButton();

	b->text = text;
	b->fontName = "ConduitMdITCTT-20";
	b->sx = b->sy = 0.65f;
	b->color = nfSmallColor;
	b->hotColor = b->pressedColor = nfWhite;
	b->shadow = false;
	b->rx = rx; b->ry = ry; b->rw = rw; b->rh = rh;
	b->onReleased = cb;

	if( numButtons < NF_MAX_BUTTONS )
		buttons[numButtons++] = b;
	holder->AddItem( b );
	return b;
}

void CNfButtonList::Think( CMenuBaseItem *itemAtCursor )
{
	for( int i = 0; i < numButtons; i++ )
	{
		// the item under the cursor or with keyboard focus is "hot" (retail GET_HOT)
		buttons[i]->SetHot( buttons[i]->IsVisible() && itemAtCursor == buttons[i] );
		buttons[i]->ThinkGlow();
	}
}

// =====================================================================

void CMenuNfPage::Think( void )
{
	list.Think( ItemAtCursor( ));
	BaseClass::Think();
}

void CMenuNfPage::Draw( void )
{
	// the shared background hook draws the background of the page on top
	Nf::SetBackground( bgKind );
	BaseClass::Draw();
}

// =====================================================================
// main menu (retail MainMenu)

// newest loadable save (retail LoadLatestSaveGame); false if there is none
static bool NfLatestSave( char *name, size_t size )
{
	char **files;
	int numFiles, best = -1;

	files = EngFuncs::GetFilesList( "save/*.sav", &numFiles, true );
	for( int i = 0; i < numFiles; i++ )
	{
		char comment[256];
		int newer = 0;

		// same validity test as the stock load page (corrupted / old saves)
		if( !EngFuncs::GetSaveComment( files[i], comment ))
			continue;

		if( best >= 0 )
			EngFuncs::CompareFileTime( files[i], files[best], &newer );
		if( best < 0 || newer > 0 )
			best = i;
	}

	if( best < 0 )
		return false;

	COM_FileBase( files[best], name, size );
	return true;
}

bool CMenuNfMain::GameHasSaves( void )
{
	char name[64];

	return NfLatestSave( name, sizeof( name ));
}

void CMenuNfMain::_Init( void )
{
	cont = AddRow( "CONTINUE", 0, VoidCb( &CMenuNfMain::ContinueCb ));
	// retail SinglePlayerUse -> ActivateSinglePlayerMenu
	AddRow( "NIGHTFIRE", 1, UI_NfSingle_Menu );
	AddRow( "MULTIPLAYER", 2, UI_MultiPlayer_Menu );
	AddRow( "OPTIONS", 3, UI_Options_Menu );
	AddRow( "CREDITS", 4, UI_NfMain_Credits );
	AddRow( "QUIT", 5, VoidCb( &CMenuNfMain::QuitDialogCb ));

	dialog.Link( this );
}

void CMenuNfMain::_VidInit( void )
{
	// retail: CONTINUE only with saves (saveCount 0 moves it off screen)
	if( GameHasSaves( ))
		cont->Show();
	else
		cont->Hide();
}

bool CMenuNfMain::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ))
	{
		if( !dialog.IsVisible( ))
			QuitDialogCb();
		return true;
	}
	return BaseClass::KeyDown( key );
}

void CMenuNfMain::QuitDialogCb( void )
{
	// retail dlg_quit: "ARE YOU SURE YOU WANT TO QUIT?"
	dialog.SetMessage( "ARE YOU SURE YOU WANT TO QUIT?" );
	dialog.onPositive.SetCommand( false, "quit \"menu dialog\"\n" );
	dialog.Show();
}

void CMenuNfMain::ContinueCb( void )
{
	// retail LoadLatestSaveGame
	char name[64], cmd[128];

	if( !NfLatestSave( name, sizeof( name )))
	{
		UI_LoadGame_Menu();
		return;
	}

	snprintf( cmd, sizeof( cmd ), "load \"%s\"\n", name );
	EngFuncs::ClientCmd( false, cmd );
	UI_CloseMenu();
}

// =====================================================================
// single player (retail SinglePlayerMenu)

void CMenuNfSingle::_Init( void )
{
	// retail btn_sp_newgame: OpenDifficultyDialog -> DoOpenDifficultyDialog m1_austria01
	AddRow( "NEW GAME", 1, VoidCb( &CMenuNfSingle::NewGameCb ));
	AddRow( "LOAD GAME", 2, UI_LoadGame_Menu );
	missions = AddRow( "MISSION SELECT", 3, UI_NfMissions_Menu );
	// retail btn_sp_back: 272 450 96x16 -> ExitSinglePlayerMenu
	AddSmall( "BACK", 272, 450, 96, 16, VoidCb( &CMenuNfSingle::Hide ));
}

void CMenuNfSingle::_VidInit( void )
{
	// retail btn_sp_mission_select: off screen while sv_iamdone is 0
	missions->SetVisibility( Nf::GameWon( ));
}

void CMenuNfSingle::NewGameCb( void )
{
	UI_NfDifficulty_Open( "m1_austria01" );
}

// =====================================================================
// mission select (retail MissionSelectMenu)

static const struct
{
	const char *map;
	const char *pic;
	const char *title;
	const char *briefing;
} nfMissions[9] =
{
	{ "m1_austria01", "gui/frontend/i_m1_austria.png", "RENDEZVOUS",
	"The world is once again in need of your talents, 007. The Phoenix International Corporation, run by the international green industrialist Rafael Drake, has been decommissioning nuclear weapons for the last few years. Intelligence suggests there are more sinister motives behind Drake's actions. Unfortunately, Drake has been very careful to keep his plans well hidden. This is where you come in, 007. Drake is throwing a gala event in his Austrian castle, and we have reason to believe that the party is a cover for a secret meeting with his conspirators. I need you to leave immediately for Austria. Infiltrate the party at the castle, and learn what you can. Good luck, 007." },
	{ "m2_airfield01", "gui/frontend/i_m2_escape.png", "AIRFIELD AMBUSH",
	"Drake's private airfield is your way out, 007. Unfortunately, it is heavily guarded and well secured with powerful surveillance technology. Together, you and Agent Nightshade need to locate a means of escape. Your best bet is to use utmost stealth." },
	{ "m3_japan01", "gui/frontend/i_m3_japan.png", "UNINVITED GUESTS",
	"Alexander Mayhew, Drake's trusted partner turned informant, has requested your protection at his countryside estate, outside Tokyo. Mayhew is in possession of extremely sensitive files that cannot fall into the hands of Drake, or that of his Yakuza thugs. Be sure that Mayhew's employees remain unharmed, then secure the files and escape with Mayhew." },
	{ "m4_infiltrate01", "gui/frontend/i_m4_infiltrate.png", "PHOENIX RISING",
	"We now know that Drake is developing a technology code-named NightFire, and we've confirmed that the plans are being kept in a database within Phoenix International's headquarters building in downtown Tokyo. Covertly infiltrate the tower and plant the Q-Worm virus. We know the building is extremely well guarded, 007, and has a state-of-the-art alarm system. Setting off the alarm will guarantee a difficult route out of the building." },
	{ "m5_power01", "gui/frontend/i_m5_power.png", "HIDDEN AGENDA",
	"The NightFire files you retrieved confirmed that Drake is pursuing a dangerous plan involving a private nuclear arsenal. Evidence suggests that one of his factories is a cover for some kind of training facility. Gain access to the facility and learn what you can. Do use caution, 007, as Drake's henchman Rook is onsite, heading up the facility's security detail." },
	{ "m6_escape01", "gui/frontend/i_m6_escape.png", "HIGH TREASON",
	"You and Agent Paradis have been brought back to Drake's penthouse at the top of the Phoenix Tower. You must find your way safely out of the facility. We look forward to hearing of your successful departure." },
	{ "m7_island01", "gui/frontend/i_m7_island.png", "ISLAND GETAWAY",
	"The island you're on is the site of Drake's stronghold and the very heart of his NightFire operation. Satellite reconnaissance indicates that the island houses an elaborate system of subterranean caves. Infiltrate the facility and sabotage Drake's plans. Agent McCall will assist you in preventing Drake from continuing his nuclear reassembly plans." },
	{ "m8_missile01", "gui/frontend/i_m8_missile.png", "ZERO MINUS",
	"The intelligence you gathered strongly indicates that Drake has moved several nuclear missiles offsite. Locate Drake and determine where the missiles are secretly being stored. You must stop Drake from launching his arsenal. As you know, the consequences could be devastating." },
	{ "m9_space01", "gui/frontend/i_m9_space.png", "REENTRY",
	"We have tracked Drake's shuttle to the International Space Station. Your shuttle will dock shortly. You must get inside and stop Drake before it's too late. We now know he has the nuclear missiles onboard, and is more than willing to use them. Best of luck, 007. The fate of many nations rests in your hands." },
};

void CMenuNfMissions::_Init( void )
{
	CMenuNfButton *b;

	// retail mission_prev / mission_next: 550 / 582, 400, 32x32,
	// i_arrow_right.png (prev mirrored by TEXTURE_RECT 32 0 0 32), state 0
	// colour 220 grey, hot white
	b = AddSmall( "", 550, 400, 32, 32, VoidCb( &CMenuNfMissions::PrevCb ));
	b->pic = "gui/frontend/i_arrow_right.png";
	b->picSrc.left = 32; b->picSrc.top = 0; b->picSrc.right = 0; b->picSrc.bottom = 32;
	b->color = 0xFFDCDCDC;

	b = AddSmall( "", 582, 400, 32, 32, VoidCb( &CMenuNfMissions::NextCb ));
	b->pic = "gui/frontend/i_arrow_right.png";
	b->color = 0xFFDCDCDC;

	// retail btn_ms_back 272 450, btn_ms_start 540 450, 96x16
	AddSmall( "BACK", 272, 450, 96, 16, VoidCb( &CMenuNfMissions::Hide ));
	AddSmall( "START", 540, 450, 96, 16, VoidCb( &CMenuNfMissions::StartCb ));
}

void CMenuNfMissions::Draw( void )
{
	const Nf::nffont_t &serp = Nf::GetFont( "SerpentineMedium-20" );
	const Nf::nffont_t &cond = Nf::GetFont( "ConduitMdITCTT-20" );

	// retail gen_title_button: 13 42 220x22, SerpentineMedium-20 0.5 0.70, centred
	Nf::DrawText2( serp, "MISSION SELECT", -1, 13, 42, 220, 0.5f, 0.70f, nfTextColor, QM_CENTER, true );

	// retail common_dome_context_pic: 54 110 128x128, the mission picture in the dial
	Nf::DrawPic( nfMissions[current].pic, 54, 110, 128, 128, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );

	// retail mission_title: 240 93 135x21, SerpentineMedium-20 0.60 0.75, left
	Nf::DrawText2( serp, nfMissions[current].title, -1, 240, 93, 135, 0.60f, 0.75f, nfTextColor, QM_LEFT, true );

	// retail mission_desc: 250 120, WRAP 350, ConduitMdITCTT-20 0.4 0.55
	Nf::DrawTextWrap( cond, nfMissions[current].briefing, 250, 120, 350, 0.4f, 0.55f, nfTextColor, true );

	CMenuNfPage::Draw();
}

void CMenuNfMissions::PrevCb( void )
{
	// retail mission_select_current 0..8, WRAP TRUE
	current = ( current + 8 ) % 9;
}

void CMenuNfMissions::NextCb( void )
{
	current = ( current + 1 ) % 9;
}

void CMenuNfMissions::StartCb( void )
{
	// retail MissionChoose -> DoOpenDifficultyDialog <map>
	UI_NfDifficulty_Open( nfMissions[current].map );
}

// =====================================================================
// difficulty dialog (retail gui/Scripts/Dialogs/dlg_difficulty.txt)

void CMenuNfDifficulty::_Init( void )
{
	CMenuNfButton *b;
	static const struct { const char *text; float y; unsigned color; } rows[3] =
	{
		// retail dlg_btn_easy / _medium / _hard: 192 y 256x21
		{ "OPERATIVE", 190, 0xFFFFC962 },
		{ "AGENT",     220, 0xFFFF9942 },
		{ "00 AGENT",  250, 0xFFFE621E },
	};
	const CEventCallback cbs[3] =
	{
		VoidCb( &CMenuNfDifficulty::EasyCb ),
		VoidCb( &CMenuNfDifficulty::NormalCb ),
		VoidCb( &CMenuNfDifficulty::HardCb ),
	};

	// the whole screen, so clicks outside the dialog do not reach the page below
	iFlags |= QMF_DISABLESCAILING;
	pos.x = pos.y = 0;
	size.w = ScreenWidth;
	size.h = ScreenHeight;

	for( int i = 0; i < 3; i++ )
	{
		// SerpentineMedium-20 0.65 0.75, drop shadow; state 3 (hot) 117 41 0,
		// state 2 (pressed) 250 219 179
		b = list.AddSmall( this, rows[i].text, 192, rows[i].y, 256, 21, cbs[i] );
		b->fontName = "SerpentineMedium-20";
		b->sx = 0.65f; b->sy = 0.75f;
		b->color = rows[i].color;
		b->hotColor = 0xFF752900;
		b->pressedColor = nfTextColor;
		b->shadow = true;
	}

	// retail CANCEL: 270 300 90x21, ConduitITCTT_BI-20 0.45 0.8, 255 179 64
	b = list.AddSmall( this, "CANCEL", 270, 300, 90, 21, VoidCb( &CMenuNfDifficulty::Hide ));
	b->fontName = "ConduitITCTT_BI-20";
	b->sx = 0.45f; b->sy = 0.8f;
	b->color = 0xFFFFB340;
	b->hotColor = 0xFF752900;
	b->pressedColor = nfTextColor;
	b->shadow = true;
}

void CMenuNfDifficulty::Open( const char *mapname )
{
	Q_strncpy( map, mapname, sizeof( map ));
	Show();
}

void CMenuNfDifficulty::Think( void )
{
	list.Think( ItemAtCursor( ));
	BaseClass::Think();
}

void CMenuNfDifficulty::Draw( void )
{
	const Nf::nffont_t &font = Nf::GetFont( "ConduitITCTT_BI-20" );

	// retail: screen dimmed (solid 0 0 0 92) [assumed: whole screen], shadow
	// i_fade 130 134 380x212 black, i_dialog_bg 160 140 320x200, caption bar
	// i_fade 180 151 280x20 black, "SELECT A DIFFICULTY"
	UI_FillRect( 0, 0, ScreenWidth, ScreenHeight, PackRGBA( 0, 0, 0, 92 ));
	Nf::DrawPic( "gui/frontend/i_fade.png", 130, 134, 380, 212, PackRGBA( 0, 0, 0, 255 ), NF_NORMAL );
	Nf::DrawPic( "gui/frontend/i_dialog_bg.png", 160, 140, 320, 200, PackRGBA( 255, 255, 255, 255 ), NF_NORMAL );
	Nf::DrawPic( "gui/frontend/i_fade.png", 180, 151, 280, 20, PackRGBA( 0, 0, 0, 255 ), NF_NORMAL );
	Nf::DrawText2( font, "SELECT A DIFFICULTY", -1, 180, 150 + ( 20 - Nf::FontHeight( font, 0.75f )) * 0.5f,
		280, 0.45f, 0.75f, nfTextColor, QM_CENTER, false );

	BaseClass::Draw();
}

void CMenuNfDifficulty::Start( int skill )
{
	// retail StartGame: "skill N", close the main menu, "maxplayers 1;map <map>";
	// the cvars as the stock new game page sets them
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", skill );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f );
	EngFuncs::CvarSetValue( "coop", 0.0f );

	EngFuncs::PlayBackgroundTrack( NULL, NULL );
	EngFuncs::ClientCmdF( false, "map %s\n", map );
	UI_CloseMenu();
}

// =====================================================================
// in-game escape menu (retail EscapeScreen / MultiplayerEscapeScreen)

void CMenuNfEscape::_Init( void )
{
	// single player: escapemenu/menu.txt EscapeScreen
	rows[0] = AddRow( "SAVE GAME", 0, UI_SaveGame_Menu );
	rows[1] = AddRow( "LOAD GAME", 1, UI_LoadGame_Menu );
	rows[2] = AddRow( "OPTIONS", 2, UI_Options_Menu );
	rows[3] = AddRow( "END GAME", 3, VoidCb( &CMenuNfEscape::EndGameDialogCb ));
	rows[4] = AddRow( "QUIT", 4, VoidCb( &CMenuNfEscape::QuitDialogCb ));

	// multiplayer: escapemenu/mpmenu.txt MultiplayerEscapeScreen; row 0
	// CHOOSE TEAM is left out (no team choice dialog in the port)
	mpRows[0] = NULL;
	mpRows[1] = AddRow( "MULTIPLAYER", 1, UI_MultiPlayer_Menu );
	mpRows[2] = AddRow( "OPTIONS", 2, UI_Options_Menu );
	mpRows[3] = AddRow( "END GAME", 3, VoidCb( &CMenuNfEscape::EndGameDialogCb ));
	mpRows[4] = AddRow( "QUIT", 4, VoidCb( &CMenuNfEscape::QuitDialogCb ));

	// retail btn_main_resume: 272 450 96x16
	AddSmall( "RESUME", 272, 450, 96, 16, UI_CloseMenu );

	dialog.Link( this );
}

void CMenuNfEscape::_VidInit( void )
{
	multiplayer = gpGlobals->maxClients > 1;

	for( int i = 0; i < 5; i++ )
	{
		rows[i]->SetVisibility( !multiplayer );
		if( mpRows[i] )
			mpRows[i]->SetVisibility( multiplayer );
	}
}

bool CMenuNfEscape::KeyDown( int key )
{
	// retail CANCEL -> ExitEscapeMenu
	if( UI::Key::IsEscape( key ))
	{
		if( !dialog.IsVisible( ))
			UI_CloseMenu();
		return true;
	}
	return BaseClass::KeyDown( key );
}

void CMenuNfEscape::QuitDialogCb( void )
{
	dialog.SetMessage( "ARE YOU SURE YOU WANT TO QUIT?" );
	dialog.onPositive.SetCommand( false, "quit \"menu dialog\"\n" );
	dialog.Show();
}

void CMenuNfEscape::EndGameDialogCb( void )
{
	// retail dlg_endgame: CL_Disconnect, back to the main menu
	dialog.SetMessage( "ARE YOU SURE YOU WANT TO END THE CURRENT GAME?" );
	dialog.onPositive = VoidCb( &CMenuNfEscape::EndGameCb );
	dialog.Show();
}

void CMenuNfEscape::EndGameCb( void )
{
	// Think() swaps to the main page once the client is really gone
	EngFuncs::ClientCmd( false, "disconnect\n" );
}

void CMenuNfEscape::Think( void )
{
	// the game ended (END GAME, mission failed, server gone): the escape page
	// must not stay; open the main page first, the stack never drops its last
	// window. Once only: a closing window keeps thinking during its fade-out,
	// and hiding it again restarts the fade, so it never left the stack
	if( !CL_IsActive( ) && IsVisible( ) && !FBitSet( iFlags, QMF_CLOSING ) && !dialog.IsVisible( ))
	{
		UI_NfMain_Show();
		Hide();
		return;
	}

	// not BaseClass (= CMenuFramework): the page's Think runs the hover / glow
	CMenuNfPage::Think();
}

// =====================================================================

// Nightfire credits: a simple scroller over the parsed lines
void CMenuNfCredits::_Init( void )
{
	char *buf;
	int len;

	if( lines )
		return;

	buf = (char *)EngFuncs::COM_LoadFile( "gui/scripts/mainmenu/credits.txt", &len );
	if( !buf )
		return;

	// count the lines, then split
	for( int i = 0; buf[i]; i++ )
		if( buf[i] == '\n' )
			numLines++;
	numLines += 2;

	lines = (char **)malloc( sizeof( char * ) * numLines );
	numLines = 0;

	{
		char *line = buf;

		while( line && *line )
		{
			char *nl = strchr( line, '\n' );

			if( nl ) *nl = 0;
			if( line[0] && line[0] != '/' && line[0] != '#' && !isspace( (unsigned char)line[0] ))
			{
				size_t l = strlen( line );
				char *copy = (char *)malloc( l + 1 );
				memcpy( copy, line, l + 1 );
				lines[numLines++] = copy;
			}
			else
				lines[numLines++] = NULL;
			line = nl ? nl + 1 : NULL;
		}
	}
	// the copies keep their text
	EngFuncs::COM_FreeFile( buf );
}

void CMenuNfCredits::Draw( void )
{
	int y0 = 480;
	float speed = 40.0f; // retail text scrolls up; speed [assumed]
	int scrolled = (int)( speed * ( uiStatic.realTime - startTime ) / 1000.0f );
	int y = y0 - scrolled;
	const Nf::nffont_t &font = Nf::GetFont( "SerpentineMedium-20" );

	if( !lines )
		return;

	for( int i = 0; i < numLines; i++ )
	{
		int ly = y + 27 * i;

		if( ly < -30 || ly > 510 )
			continue;
		if( !lines[i] )
			continue;
		Nf::DrawText( font, lines[i], 120, (float)ly, 400, 0.8f, nfRowColor[0], QM_CENTER, true );
	}
}

// =====================================================================

static CMenuNfMain *nfMain;
static CMenuNfSingle *nfSingle;
static CMenuNfMissions *nfMissions_;
static CMenuNfDifficulty *nfDifficulty;
static CMenuNfEscape *nfEscape;
static CMenuNfCredits *nfCredits;

void UI_NfMain_Credits( void )
{
	if( !nfCredits )
		nfCredits = new CMenuNfCredits();

	nfCredits->Show();
}

static void UI_NfSingle_Menu( void )
{
	if( !nfSingle )
		nfSingle = new CMenuNfSingle();

	nfSingle->Show();
}

static void UI_NfMissions_Menu( void )
{
	if( !nfMissions_ )
		nfMissions_ = new CMenuNfMissions();

	nfMissions_->Show();
}

static void UI_NfDifficulty_Open( const char *map )
{
	if( !nfDifficulty )
		nfDifficulty = new CMenuNfDifficulty();

	nfDifficulty->Open( map );
}

bool UI_NfMain_Show( void )
{
	if( !Nf::Active( ))
		return false;

	if( CL_IsActive( ))
	{
		if( !nfEscape )
			nfEscape = new CMenuNfEscape();
		nfEscape->Show();
	}
	else
	{
		if( !nfMain )
			nfMain = new CMenuNfMain();
		nfMain->Show();

		// retail InitNightfire: the main menu starts the front end music; the
		// engine keeps it when it already plays (NF_Intro) and stops it when
		// a game starts
		EngFuncs::PlayBackgroundTrack( NF_FRONTEND_MUSIC, NF_FRONTEND_MUSIC );
	}
	return true;
}

bool UI_NfMain_QuitDialog( void )
{
	if( !Nf::Active( ))
		return false;

	if( !UI_IsVisible( ) || ( CL_IsActive( ) ? !nfEscape : !nfMain ))
		UI_NfMain_Show();

	if( CL_IsActive( ))
		nfEscape->QuitDialogCb();
	else
		nfMain->QuitDialogCb();
	return true;
}

// test hooks for scenarios (simulated clicks do not reach the game):
// ui_nf_page single | missions [n] | difficulty <map>
static void UI_NfPage_f( void )
{
	const char *page = EngFuncs::CmdArgc() > 1 ? EngFuncs::CmdArgv( 1 ) : "";

	if( !Nf::Active( ))
		return;

	if( !UI_IsVisible( ))
		UI_Main_Menu();

	if( !stricmp( page, "single" ))
		UI_NfSingle_Menu();
	else if( !stricmp( page, "missions" ))
	{
		UI_NfMissions_Menu();
		if( EngFuncs::CmdArgc() > 2 )
			nfMissions_->SetMission( atoi( EngFuncs::CmdArgv( 2 )));
	}
	else if( !stricmp( page, "difficulty" ))
		UI_NfDifficulty_Open( EngFuncs::CmdArgc() > 2 ? EngFuncs::CmdArgv( 2 ) : "m1_austria01" );
	else
		Con_Printf( "usage: ui_nf_page single | missions | difficulty <map>\n" );
}
ADD_COMMAND( ui_nf_page, UI_NfPage_f );
