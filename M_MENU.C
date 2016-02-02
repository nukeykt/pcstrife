//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
// Copyright (C) 2005-2014 Simon Howard
// Copyright (C) 2015 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>


#include "doomdef.h"
#include "dstrings.h"

#include "d_main.h"

#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"

#include "r_local.h"


#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_swap.h"
#include "m_saves.h"    // [STRIFE]

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

#include "m_menu.h"
#include "p_dialog.h"


extern void M_QuitStrife(int);

extern patch_t*     hu_font[HU_FONTSIZE];
extern boolean      message_dontfuckwithme;

extern boolean      chat_on;        // in heads-up code
extern boolean      sendsave;       // [STRIFE]

//
// defaulted values
//
int         mouseSensitivity;       // has default

// [STRIFE]: removed this entirely
// Show messages has default, 0 = off, 1 = on
//int         showMessages;
	

// Blocky mode, has default, 0 = high, 1 = normal
int         detailLevel;		
int         screenblocks;		// has default

// temp for screenblocks (0-9)
int         screenSize;		

// -1 = no quicksave slot picked!
int         quickSaveSlot;          

 // 1 = message to be printed
int         messageToPrint;
// ...and here is the message string!
char*       messageString;		

// message x & y
int         messx;			
int         messy;
int         messageLastMenuActive;

// timed message = no input from user
boolean     messageNeedsInput;     

void        (*messageRoutine)(int response);

#define SAVESTRINGSIZE 	24

char gammamsg[5][26] =
{
    GAMMALVL0,
    GAMMALVL1,
    GAMMALVL2,
    GAMMALVL3,
    GAMMALVL4
};

// we are going to be entering a savegame string
int         saveStringEnter;              
int         saveSlot;	// which slot to save in
int         saveCharIndex;	// which char we're editing
// old save description before edit
char        saveOldString[SAVESTRINGSIZE];  

boolean     inhelpscreens;
boolean     menuactive;
boolean     menupause;      // haleyjd 08/29/10: [STRIFE] New global
boolean     menuindialog;   // haleyjd 09/04/10: ditto

// haleyjd 08/27/10: [STRIFE] SKULLXOFF == -28, LINEHEIGHT == 19
#define CURSORXOFF       -28
#define LINEHEIGHT      19

extern boolean      sendpause;
char                savegamestrings[77][SAVESTRINGSIZE];

char                endstring[160];


// haleyjd 09/04/10: [STRIFE] Moved menuitem / menu structures into header
// because they are needed externally by the dialog engine.

// haleyjd 08/27/10: [STRIFE] skull* stuff changed to cursor* stuff
short       itemOn;             // menu item skull is on
short       cursorAnimCounter;  // skull animation counter
short       whichCursor;        // which skull to draw


//  Sound FX volume has default, 0 - 15
//  Music volume has default, 0 - 15
// These are multiplied by 8.
int sfxVolume;
int musicVolume;
int voiceVolume;


// This can be toggled at runtime to determine if the full dialog messages
// are subtitled on screen or not. Defaults to off.
boolean dialogshowtext;

// graphic name of skulls
// haleyjd 08/27/10: [STRIFE] M_SKULL* -> M_CURS*
char    cursorName[8][8] = {"M_CURS1", "M_CURS2", "M_CURS3", "M_CURS4",
                            "M_CURS5", "M_CURS6", "M_CURS7", "M_CURS8" };

// haleyjd 20110210 [STRIFE]: skill level for menus
int menuskill;

// current menudef
menu_t*     currentMenu;                          

// haleyjd 03/01/13: [STRIFE] v1.31-only:
// Keeps track of whether the save game menu is being used to name a new
// character slot, or to just save the current game. In the v1.31 disassembly
// this was the new dword_8632C variable.
boolean namingCharacter = false;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_ReadThis3(int choice); // [STRIFE]

//void M_ChangeMessages(int choice); [STRIFE]
void M_ChangeSensitivity(int choice);
void M_SfxVol(int choice);
void M_VoiceVol(int choice); // [STRIFE]
void M_MusicVol(int choice);
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

//void M_FinishReadThis(int choice); - [STRIFE] unused
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawReadThis3(void); // [STRIFE]
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawLoad(void);
void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x, int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(int x, int y, int thermWidth, int thermDot);
void M_DrawEmptyCell(menu_t *menu, int item);
void M_DrawSelCell(menu_t *menu, int item);
int  M_StringWidth(char *string);
int  M_StringHeight(char *string);
void M_StartMessage(char *string, void *routine, boolean input);
void M_StopMessage(void);


//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,
    readthis,
    quitdoom,
    main_end
} main_e;

menuitem_t MainMenu[]=
{
    {1,"M_NGAME",M_NewGame,'n'},
    {1,"M_OPTION",M_Options,'o'},
    {1,"M_LOADG",M_LoadGame,'l'},
    {1,"M_SAVEG",M_SaveGame,'s'},
    // Another hickup with Special edition.
    {1,"M_RDTHIS",M_ReadThis,'h'}, // haleyjd 08/28/10: 'r' -> 'h'
    {1,"M_QUITG",M_QuitStrife,'q'}
};

menu_t  MainDef =
{
    main_end,
    NULL,
    MainMenu,
    M_DrawMainMenu,
    97,45, // haleyjd 08/28/10: [STRIFE] changed y coord
    0
};


//
// EPISODE SELECT
//
/*
enum
{
    ep1,
    ep2,
    ep3,
    ep_end
} episodes_e;

menuitem_t EpisodeMenu[]=
{
    {1,"M_EPI1", M_Episode,'k'},
    {1,"M_EPI2", M_Episode,'t'},
    {1,"M_EPI3", M_Episode,'i'}
};

menu_t  EpiDef =
{
    ep_end,		// # of menu items
    &MainDef,		// previous menu
    EpisodeMenu,	// menuitem_t ->
    M_DrawEpisode,	// drawing routine ->
    48,63,              // x,y
    ep1			// lastOn
};
*/
//
// NEW GAME
//
enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

menuitem_t NewGameMenu[]=
{
    // haleyjd 08/28/10: [STRIFE] changed all shortcut letters
    {1,"M_JKILL",   M_ChooseSkill, 't'},
    {1,"M_ROUGH",   M_ChooseSkill, 'r'},
    {1,"M_HURT",    M_ChooseSkill, 'v'},
    {1,"M_ULTRA",   M_ChooseSkill, 'e'},
    {1,"M_NMARE",   M_ChooseSkill, 'b'}
};

menu_t  NewDef =
{
    newg_end,       // # of menu items
    &MainDef,       // previous menu - haleyjd [STRIFE] changed to MainDef
    NewGameMenu,    // menuitem_t ->
    M_DrawNewGame,  // drawing routine ->
    48,63,          // x,y
    hurtme          // lastOn
};



//
// OPTIONS MENU
//
enum
{
    // haleyjd 08/28/10: [STRIFE] Removed messages, mouse sens., detail.
    endgame,
    scrnsize,
    option_empty1,
    soundvol,
    opt_end
} options_e;

menuitem_t OptionsMenu[]=
{
    // haleyjd 08/28/10: [STRIFE] Removed messages, mouse sens., detail.
    {1,"M_ENDGAM",	M_EndGame,'e'},
    {2,"M_SCRNSZ",	M_SizeDisplay,'s'},
    {-1,"",0,'\0'},
    {1,"M_SVOL",	M_Sound,'s'}
};

menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    60,37,
    0
};

//
// Read This! MENU 1 & 2 & [STRIFE] 3
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

menuitem_t ReadMenu1[] =
{
    {1,"",M_ReadThis2,0}
};

menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280,185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

menuitem_t ReadMenu2[]=
{
    {1,"",M_ReadThis3,0} // haleyjd 08/28/10: [STRIFE] Go to ReadThis3
};

menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    250,185, // haleyjd 08/28/10: [STRIFE] changed coords
    0
};

// haleyjd 08/28/10: Added Read This! menu 3
enum
{
    rdthsempty3,
    read3_end
} read_e3;

menuitem_t ReadMenu3[]=
{
    {1,"",M_ClearMenus,0}
};

menu_t  ReadDef3 =
{
    read3_end,
    &ReadDef2,
    ReadMenu3,
    M_DrawReadThis3,
    250, 185,
    0
};

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    voice_vol,
    sfx_empty3,
    sfx_mouse,
    sfx_empty4,
    sound_end
} sound_e;

// haleyjd 08/29/10:
// [STRIFE] 
// * Added voice volume
// * Moved mouse sensitivity here (who knows why...)
menuitem_t SoundMenu[]=
{
    {2,"M_SFXVOL",M_SfxVol,'s'},
    {-1,"",0,'\0'},
    {2,"M_MUSVOL",M_MusicVol,'m'},
    {-1,"",0,'\0'},
    {2,"M_VOIVOL",M_VoiceVol,'v'}, 
    {-1,"",0,'\0'},
    {2,"M_MSENS",M_ChangeSensitivity,'m'},
    {-1,"",0,'\0'}
};

menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    80,35,       // [STRIFE] changed y coord 64 -> 35
    0
};

//
// LOAD GAME MENU
//
enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load_end
} load_e;

menuitem_t LoadMenu[]=
{
    {1,"", M_LoadSelect,'1'},
    {1,"", M_LoadSelect,'2'},
    {1,"", M_LoadSelect,'3'},
    {1,"", M_LoadSelect,'4'},
    {1,"", M_LoadSelect,'5'},
    {1,"", M_LoadSelect,'6'}
};

menu_t  LoadDef =
{
    load_end,
    &MainDef,
    LoadMenu,
    M_DrawLoad,
    80,54,
    0
};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[]=
{
    {1,"", M_SaveSelect,'1'},
    {1,"", M_SaveSelect,'2'},
    {1,"", M_SaveSelect,'3'},
    {1,"", M_SaveSelect,'4'},
    {1,"", M_SaveSelect,'5'},
    {1,"", M_SaveSelect,'6'}
};

menu_t  SaveDef =
{
    load_end,
    &MainDef,
    SaveMenu,
    M_DrawSave,
    80,54,
    0
};

void M_DrawNameChar(void);

//
// NAME CHARACTER MENU
//
// [STRIFE]
// haleyjd 20110210: New "Name Your Character" Menu
//
menu_t NameCharDef =
{
    load_end,
    &NewDef,
    SaveMenu,
    M_DrawNameChar,
    80,54,
    0
};


//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
// [STRIFE]
// haleyjd 20110210: Rewritten to read "name" file in each slot directory
//
void M_ReadSaveStrings(void)
{
    int handle;
    int count;
    int i;
    char    name[256];

    for (i = 0; i < load_end; i++)
    {
        if (M_CheckParm("-cdrom"))
            sprintf(name, "c:\\strife.cd\\strfsav%d.ssg\\name", i);
        else
            sprintf(name, "strfsav%d.ssg\\name", i);

        handle = open(name, O_RDONLY | 0, 0666);
        if (handle == -1)
        {
            strcpy(&savegamestrings[i][0], EMPTYSTRING);
            LoadMenu[i].status = 0;
            continue;
        }
        count = read(handle, &savegamestrings[i], SAVESTRINGSIZE);
        close(handle);
        LoadMenu[i].status = 1;
    }
}

//
// M_DrawNameChar
//
// haleyjd 09/22/10: [STRIFE] New function
// Handler for drawing the "Name Your Character" menu.
//
void M_DrawNameChar(void)
{
    int i;

    M_WriteText(72, 28, "Name Your Character");

    for (i = 0; i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT*i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT*i, savegamestrings[i]);
    }

    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[quickSaveSlot]);
        M_WriteText(LoadDef.x + i, LoadDef.y + LINEHEIGHT*quickSaveSlot, "_");
    }
}

//
// M_DoNameChar
//
// haleyjd 09/22/10: [STRIFE] New function
// Handler for items in the "Name Your Character" menu.
//
void M_DoNameChar(int choice)
{
    int map;

    // 20130301: clear naming character flag for 1.31 save logic
    namingCharacter = false;
    sendsave = 1;
    ClearTmp();
    G_SaveGame(choice, savegamestrings[choice]);
    quickSaveSlot = choice;
    SaveDef.lastOn = choice;
    ClearSlot();
    FromCurr(choice);

    if (isdemoversion)
        map = 33;
    else
        map = 2;

    G_DeferedInitNew(menuskill, map);
    M_ClearMenus(0);
}


//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
    int             i;

    V_DrawPatchDirect(72, 28, 0, W_CacheLumpName("M_LOADG", PU_CACHE));
    for (i = 0; i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT*i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT*i, savegamestrings[i]);
    }
}



//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
    int             i;

    V_DrawPatchDirect(x - 8, y + 7, 0, W_CacheLumpName("M_LSLEFT", PU_CACHE));

    for (i = 0; i < 24; i++)
    {
        V_DrawPatchDirect(x, y + 7, 0, W_CacheLumpName("M_LSCNTR", PU_CACHE));
        x += 8;
    }

    V_DrawPatchDirect(x, y + 7, 0, W_CacheLumpName("M_LSRGHT", PU_CACHE));
}



//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
    // [STRIFE]: completely rewritten
    char    name[256];

    G_SaveGame(choice, savegamestrings[choice]);
    ToCurr(choice);

    if (M_CheckParm("-cdrom"))
        sprintf(name, "c:\\strife.cd\\strfsav%d.ssg", choice);
    else
        sprintf(name, "strfsav%d.ssg", choice);

    G_ReadCurrent(name);
    quickSaveSlot = choice;
    M_ClearMenus(0);
}

//
// Selected from DOOM menu
//
void M_LoadGame(int choice)
{
    if (netgame)
    {
        M_StartMessage(LOADNET, NULL, false);
        return;
    }

    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}


//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
{
    int             i;

    V_DrawPatchDirect(72, 28, 0, W_CacheLumpName("M_SAVEG", PU_CACHE));
    for (i = 0; i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT*i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT*i, savegamestrings[i]);
    }

    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[quickSaveSlot]);
        M_WriteText(LoadDef.x + i, LoadDef.y + LINEHEIGHT*quickSaveSlot, "_");
    }
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    // [STRIFE]: completely rewritten
    if (slot >= 0)
    {
        sendsave = true;
        G_SaveGame(slot, savegamestrings[slot]);
        M_ClearMenus(0);
        quickSaveSlot = slot;
        FromCurr(slot);
    }
    else
    {
        M_StartMessage(QSAVESPOT, NULL, false);
    }
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
    // we are going to be intercepting all chars
    saveStringEnter = 1;

    // [STRIFE]
    quickSaveSlot = choice;
    //saveSlot = choice;
    strcpy(saveOldString, savegamestrings[choice]);
    if (!strcmp(savegamestrings[choice], EMPTYSTRING))
        savegamestrings[choice][0] = 0;
    saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame (int choice)
{
    // [STRIFE]
    if (netgame)
    {
        // haleyjd 20110211: Hooray for Rogue's awesome multiplayer support...
        M_StartMessage("You can't save a netgame", NULL, false);
        return;
    }
    if (!usergame)
    {
        M_StartMessage(SAVEDEAD, NULL, false);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}



//
//      M_QuickSave
//
char    tempstring[80];

void M_QuickSaveResponse(int ch)
{
    if (ch == 'y')
    {
        M_DoSave(quickSaveSlot);
        S_StartSound(NULL, sfx_mtalht); // [STRIFE] sound
    }
}

void M_QuickSave(void)
{
    if (netgame)
    {
        // haleyjd 20110211 [STRIFE]: More fun...
        M_StartMessage("You can't save a netgame", NULL, false);
        return;
    }
    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;

    if (quickSaveSlot < 0)
    {
        M_StartControlPanel();
        M_ReadSaveStrings();
        M_SetupNextMenu(&SaveDef);
        quickSaveSlot = -2;	// means to pick a slot now
        return;
    }
    sprintf(tempstring, QSPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickSaveResponse, true);
}



//
// M_QuickLoad
//
void M_QuickLoadResponse(int ch)
{
    if (ch == 'y')
    {
        M_LoadSelect(quickSaveSlot);
        S_StartSound(NULL, sfx_mtalht); // [STRIFE] sound
    }
}


void M_QuickLoad(void)
{
    if (netgame)
    {
        M_StartMessage(QLOADNET, NULL, false);
        return;
    }

    if (quickSaveSlot < 0)
    {
        M_StartMessage(QSAVESPOT, NULL, false);
        return;
    }
    sprintf(tempstring, QLPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickLoadResponse, true);
}




//
// Read This Menus
// Had a "quick hack to fix romero bug"
// haleyjd 08/28/10: [STRIFE] Draw HELP1, unconditionally.
//

void M_DrawReadThis1(void)
{
	inhelpscreens = true;
	V_DrawPatchDirect(0, 0, 0, W_CacheLumpName("HELP1", PU_CACHE));
}


//
// Read This Menus
// haleyjd 08/28/10: [STRIFE] Not optional, draws HELP2
//

void M_DrawReadThis2(void)
{
	inhelpscreens = true;
	V_DrawPatchDirect(0, 0, 0, W_CacheLumpName("HELP2", PU_CACHE));
}


//
// Read This Menus
// haleyjd 08/28/10: [STRIFE] New function to draw HELP3.
//
void M_DrawReadThis3(void)
{
    inhelpscreens = true;
    V_DrawPatchDirect(0, 0, 0, W_CacheLumpName("HELP3", PU_CACHE));
}


//
// Change Sfx & Music volumes
//
// haleyjd 08/29/10: [STRIFE]
// * Changed title graphic coordinates
// * Added voice volume and sensitivity sliders
//
void M_DrawSound(void)
{
    V_DrawPatchDirect(100, 10, 0, W_CacheLumpName("M_SVOL", PU_CACHE));

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_vol+1),
                 16,sfxVolume);

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(music_vol+1),
                 16,musicVolume);

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(voice_vol+1),
                 16,voiceVolume);

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_mouse+1),
                 16,mouseSensitivity);
}

void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

void M_SfxVol(int choice)
{
    switch (choice)
    {
    case 0:
        if (sfxVolume)
            sfxVolume--;
        break;
    case 1:
        if (sfxVolume < 15)
            sfxVolume++;
        break;
    }

    S_SetSfxVolume(sfxVolume * 8);
}

void M_MusicVol(int choice)
{
    switch (choice)
    {
    case 0:
        if (musicVolume)
            musicVolume--;
        break;
    case 1:
        if (musicVolume < 15)
            musicVolume++;
        break;
    }

    S_SetMusicVolume(musicVolume * 8);
}

//
// M_VoiceVol
//
// haleyjd 08/29/10: [STRIFE] New function
// Sets voice volume level.
//
void M_VoiceVol(int choice)
{
    switch (choice)
    {
    case 0:
        if (voiceVolume)
            voiceVolume--;
        break;
    case 1:
        if (voiceVolume < 15)
            voiceVolume++;
        break;
    }

    S_SetVoiceVolume(voiceVolume * 8);
}




//
// M_DrawMainMenu
//
// haleyjd 08/27/10: [STRIFE] Changed x coordinate; M_DOOM -> M_STRIFE
//
void M_DrawMainMenu(void)
{
    V_DrawPatchDirect(84, 2, 0, W_CacheLumpName("M_STRIFE", PU_CACHE));
}




//
// M_NewGame
//
// haleyjd 08/31/10: [STRIFE] Changed M_NEWG -> M_NGAME
//
void M_DrawNewGame(void)
{
    V_DrawPatchDirect (96,14,0,W_CacheLumpName("M_NGAME",PU_CACHE));
    V_DrawPatchDirect (54,38,0,W_CacheLumpName("M_SKILL",PU_CACHE));
}

void M_NewGame(int choice)
{
    if (netgame && !demoplayback)
    {
        M_StartMessage(NEWGAME, NULL, false);
        return;
    }

    namingCharacter = true;
    M_SetupNextMenu(&NewDef);
}


//
//      M_Episode
//

// haleyjd: [STRIFE] Unused
/*
int     epi;

void M_DrawEpisode(void)
{
    V_DrawPatchDirect (54,38,0,W_CacheLumpName("M_EPISOD",PU_CACHE));
}

void M_VerifyNightmare(int ch)
{
    if (ch != 'y')
	return;
		
    G_DeferedInitNew(nightmare,epi+1,1);
    M_ClearMenus ();
}



*/
void M_ChooseSkill(int choice)
{
    // haleyjd 09/07/10: Removed nightmare confirmation
    // [STRIFE]: start "Name Your Character" menu
    menuskill = choice;
    currentMenu = &NameCharDef;
    itemOn = NameCharDef.lastOn;
    M_ReadSaveStrings();
}

/*
// haleyjd [STRIFE] Unused

void M_Episode(int choice)
{
    if (shareware && choice)
    {
	M_StartMessage(SWSTRING,NULL,false);
	M_SetupNextMenu(&ReadDef1);
	return;
    }
	 
    epi = choice;
    M_SetupNextMenu(&NewDef);
}*/
//
// M_Options
//
char    detailNames[2][9]	= {"M_GDHIGH","M_GDLOW"};
char    msgNames[2][9]		= {"M_MSGOFF","M_MSGON"};


void M_DrawOptions(void)
{
    // haleyjd 08/27/10: [STRIFE] M_OPTTTL -> M_OPTION
    V_DrawPatchDirect (108,15,0,W_CacheLumpName("M_OPTION",PU_CACHE));

    // haleyjd 08/26/10: [STRIFE] Removed messages, sensitivity, detail.

    M_DrawThermo(OptionsDef.x,OptionsDef.y+LINEHEIGHT*(scrnsize+1),
                 9,screenSize);
}

void M_Options(int choice)
{
    M_SetupNextMenu(&OptionsDef);
}

//
// M_AutoUseHealth
//
// [STRIFE] New function
// haleyjd 20110211: toggle autouse health state
//
void M_AutoUseHealth(void)
{
    if(!netgame && usergame)
    {
        players[consoleplayer].cheats ^= CF_AUTOHEALTH;

        if(players[consoleplayer].cheats & CF_AUTOHEALTH)
            players[consoleplayer].message = "Auto use health ON";
        else
            players[consoleplayer].message = "Auto use health OFF";
    }
}

//
// M_ChangeShowText
//
// [STRIFE] New function
//
void M_ChangeShowText(void)
{
    dialogshowtext ^= true;

    if(dialogshowtext)
        players[consoleplayer].message = "Conversation Text On";
    else
        players[consoleplayer].message = "Conversation Text Off";
}

//
//      Toggle messages on/off
//
// [STRIFE] Messages cannot be disabled in Strife
/*
void M_ChangeMessages(int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;
	
    if (!showMessages)
	players[consoleplayer].message = MSGOFF;
    else
	players[consoleplayer].message = MSGON ;

    message_dontfuckwithme = true;
}
*/



//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
    if (ch != 'y')
        return;

    currentMenu->lastOn = itemOn;
    M_ClearMenus(0);
    D_StartTitle();
}

void M_EndGame(int choice)
{
    choice = 0;
    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    if (netgame)
    {
        M_StartMessage(NETEND, NULL, false);
        return;
    }

    M_StartMessage(ENDGAME, M_EndGameResponse, true);
}




//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef2);
}

//
// M_ReadThis3
//
// haleyjd 08/28/10: [STRIFE] New function.
//
void M_ReadThis3(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef3);
}

/*
void M_FinishReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
}
*/


//
// M_ClearMenus
//
// haleyjd 08/28/10: [STRIFE] Added an int param so this can be called by menus.
//         09/08/10: Added menupause.
//
void M_ClearMenus(int choice)
{
    menuactive = 0;
    menupause = 0;
}


extern void F_StartCast(void);
//
// M_CheckStartCast
//
// [STRIFE] New but unused function. Was going to start a cast
//   call from within the menu system... not functional even in
//   the earliest demo version.
//
void M_CheckStartCast()
{
    if (usergame)
    {
        M_StartMessage("You have to end your game first.", NULL, false);
        return;
    }
    F_StartCast();
    M_ClearMenus(0);
}


//
// M_QuitDOOM
//
/*
int     quitsounds[8] =
{
    sfx_pldeth,
    sfx_dmpain,
    sfx_popain,
    sfx_slop,
    sfx_telept,
    sfx_posit1,
    sfx_posit3,
    sfx_sgtatk
};

int     quitsounds2[8] =
{
    sfx_vilact,
    sfx_getpow,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk
};
*/
//
// M_QuitResponse
//
// haleyjd 09/11/10: [STRIFE] Modifications to start up endgame
// demosequence.
//
void M_QuitResponse(int key)
{
    char buffer[13];

    if (key != 'y')
        return;

    if(netgame)
        I_Quit();
    else
    {
        sprintf(buffer, "qfmrm%i", (gametic % 8) + 1);
        I_StartVoice(buffer);
        D_QuitGame();
    }
}





//
// M_QuitStrife
//
// [STRIFE] Renamed from M_QuitDOOM
// haleyjd 09/11/10: No randomized text message; that's taken care of
// by the randomized voice message after confirmation.
//
void M_QuitStrife(int choice)
{
    sprintf(endstring,
        "Do you really want to leave?\n\n" DOSY);

    M_StartMessage(endstring, M_QuitResponse, true);
}





void M_ChangeSensitivity(int choice)
{
    switch (choice)
    {
    case 0:
        if (mouseSensitivity)
            mouseSensitivity--;
        break;
    case 1:
        if (mouseSensitivity < 9)
            mouseSensitivity++;
        break;
    }
}

/*
// haleyjd [STRIFE] Unused
void M_ChangeDetail(int choice)
{
    choice = 0;
    detailLevel = 1 - detailLevel;

    R_SetViewSize (screenblocks, detailLevel);

    if (!detailLevel)
	players[consoleplayer].message = DETAILHI;
    else
	players[consoleplayer].message = DETAILLO;
}
*/

// [STRIFE] Verified unmodified
void M_SizeDisplay(int choice)
{
    switch (choice)
    {
    case 0:
        if (screenSize > 0)
        {
            screenblocks--;
            screenSize--;
        }
        break;
    case 1:
        if (screenSize < 8)
        {
            screenblocks++;
            screenSize++;
        }
        break;
    }


    R_SetViewSize(screenblocks);
}




//
//      Menu Functions
//

//
// M_DrawThermo
//
// haleyjd 08/28/10: [STRIFE] Changes to some patch coordinates.
//
void
M_DrawThermo
( int	x,
  int	y,
  int	thermWidth,
  int	thermDot )
{
    int     xx;
    int         yy; // [STRIFE] Needs a temp y coordinate variable
    int     i;

    xx = x;
    yy = y + 6;
    V_DrawPatchDirect(xx, yy, 0, W_CacheLumpName("M_THERML", PU_CACHE));
    xx += 8;
    for (i = 0; i<thermWidth; i++)
    {
        V_DrawPatchDirect(xx, yy, 0, W_CacheLumpName("M_THERMM", PU_CACHE));
        xx += 8;
    }
    V_DrawPatchDirect(xx, yy, 0, W_CacheLumpName("M_THERMR", PU_CACHE));

    // [STRIFE] +2 to initial y coordinate
    V_DrawPatchDirect((x + 8) + thermDot * 8, y + 2,
        0, W_CacheLumpName("M_THERMO", PU_CACHE));
}


void
M_StartMessage
( char*		string,
  void*		routine,
  boolean	input )
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;
    menuactive = true;
    return;
}



void M_StopMessage(void)
{
    menuactive = messageLastMenuActive;
    messageToPrint = 0;
}



//
// Find string width from hu_font chars
//
int M_StringWidth(char* string)
{
    int             i;
    int             w = 0;
    int             c;
	
    for (i = 0;i < strlen(string);i++)
    {
	c = toupper(string[i]) - HU_FONTSTART;
	if (c < 0 || c >= HU_FONTSIZE)
	    w += 4;
	else
	    w += SHORT (hu_font[c]->width);
    }
		
    return w;
}



//
//      Find string height from hu_font chars
//
int M_StringHeight(char* string)
{
    int             i;
    int             h;
    int             height = SHORT(hu_font[0]->height);
	
    h = height;
    for (i = 0;i < strlen(string);i++)
	if (string[i] == '\n')
	    h += height;
		
    return h;
}


//
//      Write a string using the hu_font
//
int
M_WriteText
( int		x,
  int		y,
  char*		string)
{
    int     w;
    char*   ch;
    int     c;
    int     cx;
    int     cy;


    ch = string;
    cx = x;
    cy = y;

    while (1)
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = x;
            cy += 11; // haleyjd 09/04/10: [STRIFE]: Changed 12 -> 11
            continue;
        }

        // haleyjd 09/04/10: [STRIFE] Don't draw spaces at the start of lines.
        if (c == ' ' && cx == x)
            continue;

        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        w = SHORT(hu_font[c]->width);

        // haleyjd 09/04/10: [STRIFE] Different linebreak handling
        if (cx + w > SCREENWIDTH - 20)
        {
            cx = x;
            cy += 11;
            --ch;
        }
        else
        {
            V_DrawPatchDirect(cx, cy, 0, hu_font[c]);
            cx += w;
        }
    }

    // [STRIFE] Return final y coordinate.
    return cy + 12;
}

//
// M_DialogDimMsg
//
// [STRIFE] New function
// haleyjd 09/04/10: Painstakingly transformed from the assembly code, as the
// decompiler could not touch it. Redimensions a string to fit on screen, leaving
// at least a 20 pixel margin on the right side. The string passed in must be
// writable.
//

void M_DialogDimMsg(int x, int y, char *str, boolean useyfont)
{
    int rightbound = (SCREENWIDTH - 20) - x;
    patch_t **fontarray;  // ebp
    int linewidth = 0;    // esi
    int i = 0;            // edx
    char *message = str;  // edi
    char  bl;             // bl

    if(useyfont)
       fontarray = yfont;
    else
       fontarray = hu_font;

    bl = toupper(*message);

    if(!bl)
        return;

    // outer loop - run to end of string
    do
    {
        if(bl != '\n')
        {
            int charwidth; // eax
            int tempwidth; // ecx

            if(bl < HU_FONTSTART || bl > HU_FONTEND)
                charwidth = 4;
            else
                charwidth = SHORT(fontarray[bl - HU_FONTSTART]->width);

            tempwidth = linewidth + charwidth;

            // Test if the line still fits within the boundary...
            if(tempwidth >= rightbound)
            {
                // Doesn't fit...
                char *tempptr = &message[i]; // ebx
                char  al;                    // al

                // inner loop - run backward til a space (or the start of the
                // string) is found, subtracting width off the current line.
                // BUG: shouldn't we stop at a previous '\n' too?
                while(*tempptr != ' ' && i > 0)
                {
                    tempptr--;
                    // BUG: they didn't add the first char to linewidth yet...
                    linewidth -= charwidth; 
                    i--;
                    al = toupper(*tempptr);
                    if(al < HU_FONTSTART || al > HU_FONTEND)
                        charwidth = 4;
                    else
                        charwidth = SHORT(fontarray[al - HU_FONTSTART]->width);
                }
                // Replace the space with a linebreak.
                // BUG: what if i is zero? ... infinite loop time!
                message[i] = '\n';
                linewidth = 0;
            }
            else
            {
                // The line does fit.
                // Spaces at the start of a line don't count though.
                if(!(bl == ' ' && linewidth == 0))
                    linewidth += charwidth;
            }
        }
        else
            linewidth = 0; // '\n' seen, so reset the line width
    }
    while((bl = toupper(message[++i])) != 0); // step to the next character
}
//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder (event_t* ev)
{
    int             ch;
    int             i;
    static  int     joywait = 0;
    static  int     mousewait = 0;
    static  int     mousey = 0;
    static  int     lasty = 0;
    static  int     mousex = 0;
    static  int     lastx = 0;

    ch = -1;

    if (ev->type == ev_joystick && joywait < I_GetTime())
    {
        if (ev->data3 == -1)
        {
            ch = KEY_UPARROW;
            joywait = I_GetTime() + 5;
        }
        else if (ev->data3 == 1)
        {
            ch = KEY_DOWNARROW;
            joywait = I_GetTime() + 5;
        }

        if (ev->data2 == -1)
        {
            ch = KEY_LEFTARROW;
            joywait = I_GetTime() + 2;
        }
        else if (ev->data2 == 1)
        {
            ch = KEY_RIGHTARROW;
            joywait = I_GetTime() + 2;
        }

        if (ev->data1 & 1)
        {
            ch = KEY_ENTER;
            joywait = I_GetTime() + 5;
        }
        if (ev->data1 & 2)
        {
            ch = KEY_BACKSPACE;
            joywait = I_GetTime() + 5;
        }
    }
    else
    {
        if (ev->type == ev_mouse && mousewait < I_GetTime())
        {
            mousey += ev->data3;
            if (mousey < lasty - 30)
            {
                ch = KEY_DOWNARROW;
                mousewait = I_GetTime() + 5;
                mousey = lasty -= 30;
            }
            else if (mousey > lasty + 30)
            {
                ch = KEY_UPARROW;
                mousewait = I_GetTime() + 5;
                mousey = lasty += 30;
            }

            mousex += ev->data2;
            if (mousex < lastx - 30)
            {
                ch = KEY_LEFTARROW;
                mousewait = I_GetTime() + 5;
                mousex = lastx -= 30;
            }
            else if (mousex > lastx + 30)
            {
                ch = KEY_RIGHTARROW;
                mousewait = I_GetTime() + 5;
                mousex = lastx += 30;
            }

            if (ev->data1 & 1)
            {
                ch = KEY_ENTER;
                mousewait = I_GetTime() + 15;
                mouse_fire_countdown = 5;   // villsa [STRIFE]
            }

            if (ev->data1 & 2)
            {
                ch = KEY_BACKSPACE;
                mousewait = I_GetTime() + 15;
            }
        }
        else
            if (ev->type == ev_keydown)
            {
                ch = ev->data1;
            }
    }

    if (ch == -1)
        return false;


    // Save Game string input
    if (saveStringEnter)
    {
        switch (ch)
        {
        case KEY_BACKSPACE:
            if (saveCharIndex > 0)
            {
                saveCharIndex--;
                savegamestrings[quickSaveSlot][saveCharIndex] = 0;
            }
            break;

        case KEY_ESCAPE:
            saveStringEnter = 0;
            strcpy(&savegamestrings[quickSaveSlot][0], saveOldString);
            break;

        case KEY_ENTER:
            saveStringEnter = 0;
            if (!namingCharacter)
            {
                M_DoSave(quickSaveSlot);
                return true;
            }
            if (savegamestrings[quickSaveSlot][0])
                M_DoNameChar(quickSaveSlot);
            break;

        default:
            ch = toupper(ch);
            if (ch != 32)
                if (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE)
                    break;
            if (ch >= 32 && ch <= 127 &&
                saveCharIndex < SAVESTRINGSIZE - 1 &&
                M_StringWidth(savegamestrings[quickSaveSlot]) <
                (SAVESTRINGSIZE - 2) * 8)
            {
                savegamestrings[quickSaveSlot][saveCharIndex++] = ch;
                savegamestrings[quickSaveSlot][saveCharIndex] = 0;
            }
            break;
        }
        return true;
    }

    // Take care of any messages that need input
    if (messageToPrint)
    {
        if (messageNeedsInput == true &&
            !(ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE))
            return false;

        menuactive = messageLastMenuActive;
        messageToPrint = 0;
        if (messageRoutine)
            messageRoutine(ch);

        menupause = false;                // [STRIFE] unpause
        menuactive = false;
        S_StartSound(NULL, sfx_mtalht);   // [STRIFE] sound
        return true;
    }

    // [STRIFE]:
    // * In v1.2 this is moved to F9 (quickload)
    // * In v1.31 it is moved to F12 with DM spy, and quicksave
    //   functionality is restored separate from normal saving
    /*
    if (devparm && ch == KEY_F1)
    {
        G_ScreenShot();
        return true;
    }
    */


    // F-Keys
    if (!menuactive)
        switch (ch)
        {
        case KEY_MINUS:         // Screen size down
            if (automapactive || chat_on)
                return false;
            M_SizeDisplay(0);
            S_StartSound(NULL, sfx_stnmov);
            return true;

        case KEY_EQUALS:        // Screen size up
            if (automapactive || chat_on)
                return false;
            M_SizeDisplay(1);
            S_StartSound(NULL, sfx_stnmov);
            return true;

        case KEY_F1:            // Help key
            M_StartControlPanel();
            // haleyjd 08/29/10: [STRIFE] always ReadDef1
            currentMenu = &ReadDef1;

            itemOn = 0;
            S_StartSound(NULL, sfx_swtchn);
            return true;

        case KEY_F2:            // Save
            // [STRIFE]: Hub saves
            namingCharacter = false; // just saving normally, in 1.31
            if(netgame || players[consoleplayer].health <= 0 ||
                players[consoleplayer].cheats & CF_ONFIRE)
            {
                S_StartSound(NULL, sfx_oof);
            }
            else
            {
                M_StartControlPanel();
                S_StartSound(NULL, sfx_swtchn);
                M_SaveGame(0);
            }
            return true;

        case KEY_F3:            // Load
            // [STRIFE]: Hub saves
            namingCharacter = false;
            M_StartControlPanel();
            M_LoadGame(0);
            S_StartSound(NULL, sfx_swtchn);
            return true;

        case KEY_F4:            // Sound Volume
            M_StartControlPanel();
            currentMenu = &SoundDef;
            itemOn = sfx_vol;
            S_StartSound(NULL, sfx_swtchn);
            return true;

        case KEY_F5:            // Detail toggle
            //M_ChangeDetail(0);
            M_AutoUseHealth(); // [STRIFE]
            S_StartSound(NULL, sfx_swtchn);
            return true;

        case KEY_F6:            // Quicksave
            // [STRIFE]: Hub saves
            namingCharacter = false; // for 1.31 save changes

            if (netgame || players[consoleplayer].health <= 0 ||
                players[consoleplayer].cheats & CF_ONFIRE)
            {
                S_StartSound(NULL, sfx_oof);
            }
            else
            {
                S_StartSound(NULL, sfx_swtchn);
                M_QuickSave();
            }
            return true;

        case KEY_F7:            // End game
            S_StartSound(NULL, sfx_swtchn);
            M_EndGame(0);
            return true;

        case KEY_F8:            // Toggle messages
            //M_ChangeMessages(0);
            M_ChangeShowText(); // [STRIFE]
            S_StartSound(NULL, sfx_swtchn);
            return true;

        case KEY_F9:            // Quickload
            namingCharacter = false;
            S_StartSound(NULL, sfx_swtchn);
            M_QuickLoad();
            return true;

        case KEY_F10:           // Quit DOOM
            S_StartSound(NULL, sfx_swtchn);
            M_QuitStrife(0);
            return true;

        case KEY_F11:           // gamma toggle
            usegamma++;
            if (usegamma > 4)
                usegamma = 0;
            players[consoleplayer].message = gammamsg[usegamma];
            I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
            return true;

        case KEY_F12:
            // haleyjd 20130301: 1.31 moved screenshots to F12.
            G_ScreenShot();
            return true;
        }


    // Pop-up menu?
    if (!menuactive)
    {
        if (ch == KEY_ESCAPE)
        {
            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        return false;
    }


    // Keys usable within menu
    switch (ch)
    {
    case KEY_DOWNARROW:
        do
        {
            if (itemOn + 1 > currentMenu->numitems - 1)
                itemOn = 0;
            else itemOn++;
            S_StartSound(NULL, sfx_pstop);
        } while (currentMenu->menuitems[itemOn].status == -1);
        return true;

    case KEY_UPARROW:
        do
        {
            if (!itemOn)
                itemOn = currentMenu->numitems - 1;
            else itemOn--;
            S_StartSound(NULL, sfx_pstop);
        } while (currentMenu->menuitems[itemOn].status == -1);
        return true;

    case KEY_LEFTARROW:
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(0);
        }
        return true;

    case KEY_RIGHTARROW:
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(1);
        }
        return true;

    case KEY_ENTER:
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
        {
            currentMenu->lastOn = itemOn;
            if (currentMenu->menuitems[itemOn].status == 2)
            {
                currentMenu->menuitems[itemOn].routine(1);      // right arrow
                S_StartSound(NULL, sfx_stnmov);
            }
            else
            {
                currentMenu->menuitems[itemOn].routine(itemOn);
                //S_StartSound(NULL, sfx_swish); [STRIFE] No sound is played here.
            }
        }
        return true;

    case KEY_ESCAPE:
        // Deactivate menu
        namingCharacter = false; // [STRIFE]: 1.31 saving

        if (menuindialog) // [STRIFE] - Get out of dialog engine semi-gracefully
            P_DialogDoChoice(-1);
        currentMenu->lastOn = itemOn;
        M_ClearMenus(0);
        S_StartSound(NULL, sfx_mtalht); // villsa [STRIFE]: sounds
        return true;

    case KEY_BACKSPACE:
        currentMenu->lastOn = itemOn;
        if (currentMenu->prevMenu)
        {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
            S_StartSound(NULL, sfx_swtchn);
        }
        return true;

    default:
        for (i = itemOn + 1; i < currentMenu->numitems; i++)
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL, sfx_pstop);
                return true;
            }
        for (i = 0; i <= itemOn; i++)
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL, sfx_pstop);
                return true;
            }
        break;

    }

    return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
    // intro might call this repeatedly
    if (menuactive)
	return;
    
    menuactive = 1;
    menupause = true;
    currentMenu = &MainDef;         // JDC
    itemOn = currentMenu->lastOn;   // JDC
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
    static short	x;
    static short	y;
    short		i;
    short		max;
    char		string[40];
    int			start;

    inhelpscreens = false;


    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        start = 0;
        y = 100 - M_StringHeight(messageString) / 2;
        while (*(messageString + start))
        {
            for (i = 0; i < strlen(messageString + start); i++)
                if (*(messageString + start + i) == '\n')
                {
                    memset(string, 0, 40);
                    strncpy(string, messageString + start, i);
                    break;
                }

            if (i == strlen(messageString + start))
            {
                strcpy(string, messageString + start);
                start += i;
            }
            else
            {
                start += i + 1;
            }

            x = 160 - M_StringWidth(string) / 2;
            M_WriteText(x, y, string);
            y += SHORT(hu_font[0]->height);
        }
        return;
    }

    if (!menuactive)
        return;

    if (currentMenu->routine)
        currentMenu->routine();         // call Draw routine

                                        // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    max = currentMenu->numitems;

    for (i = 0; i<max; i++)
    {
        if (currentMenu->menuitems[i].name[0])
            V_DrawPatchDirect(x, y, 0,
                W_CacheLumpName(currentMenu->menuitems[i].name, PU_CACHE));
        y += LINEHEIGHT;
    }


    // haleyjd 08/27/10: [STRIFE] Adjust to draw spinning Sigil
    // DRAW SIGIL
    V_DrawPatchDirect(x + CURSORXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT, 0,
        W_CacheLumpName(cursorName[whichCursor], PU_CACHE));

}


//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}


//
// M_Ticker
//
// haleyjd 08/27/10: [STRIFE] Rewritten for Sigil cursor
//
void M_Ticker (void)
{
    if (--cursorAnimCounter <= 0)
    {
        whichCursor = (whichCursor + 1) % 8;
        cursorAnimCounter = 5;
    }
}


//
// M_Init
//
// haleyjd 08/27/10: [STRIFE] Removed DOOM gamemode stuff
//
void M_Init (void)
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichCursor = 0;
    cursorAnimCounter = 10;
    screenSize = screenblocks - 3;
    messageToPrint = 0;
    messageString = NULL;
    messageLastMenuActive = menuactive; // STRIFE-FIXME: assigns 0 here...
    quickSaveSlot = -1;

    // [STRIFE]: Initialize savegame paths and clear temporary directory
    G_SaveGame(5, "ME");
    ClearTmp();
}
