//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
// Copyright (C) 2005-2014 Simon Howard
// Copyright (C) 2015-2016 Alexey Khokholov (Nuke.YKT)
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
// DESCRIPTION:  none
//

#include <string.h>
#include <stdlib.h>

#include "doomdef.h" 
#include "doomstat.h"

#include "z_zone.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_saves.h"
#include "m_random.h"
#include "i_system.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h" 

#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"



#include "p_dialog.h"   // villsa [STRIFE]

#include "g_game.h"


#define SAVEGAMESIZE	0x2c000



boolean	G_CheckDemoStatus (void); 
void	G_ReadDemoTiccmd (ticcmd_t* cmd); 
void	G_WriteDemoTiccmd (ticcmd_t* cmd); 
void	G_PlayerReborn (int player);  
 
void	G_DoReborn (int playernum); 
 
void	G_DoLoadLevel (boolean userload);
void	G_DoNewGame (void);
void	G_DoPlayDemo (void); 
void	G_DoCompleted (void); 
void	G_DoVictory (void); 
void	G_DoWorldDone (void); 
void	G_DoSaveGame (char *path);
 
 
gameaction_t    gameaction; 
gamestate_t     gamestate; 
skill_t         gameskill = 2; // [STRIFE] Default value set to 2.
boolean		respawnmonsters;
//int             gameepisode; 
int             gamemap;

// haleyjd 08/24/10: [STRIFE] New variables
int             destmap;   // current destination map when exiting
int             riftdest;  // destination spot for player
angle_t         riftangle; // player angle saved during exit
 
boolean         paused; 
boolean         sendpause;             	// send a pause event next tic 
boolean         sendsave;             	// send a save event next tic 
boolean         usergame;               // ok to save / end game 
 
boolean         timingdemo;             // if true, exit with report on completion 
boolean         nodrawers;              // for comparative timing purposes 
boolean         noblit;                 // for comparative timing purposes 
int             starttime;          	// for comparative timing purposes  	 
 
boolean         viewactive; 
 
boolean         deathmatch;           	// only if started as net death 
boolean         netgame;                // only true if packets are broadcast 
boolean         playeringame[MAXPLAYERS]; 
player_t        players[MAXPLAYERS]; 
 
int             consoleplayer;          // player taking events and displaying 
int             displayplayer;          // view being displayed 
int             gametic; 
int             levelstarttic;          // gametic at level start 
int             totalkills, totalitems, totalsecret;    // for intermission 
 
char            demoname[32]; 
boolean         demorecording; 
boolean         demoplayback; 
boolean		netdemo; 
byte*		demobuffer;
byte*		demo_p;
byte*		demoend; 
boolean         singledemo;            	// quit after playing a demo from cmdline 
 
boolean         precache = true;        // if true, load all graphics at start 
 
wbstartstruct_t wminfo;               	// parms for world map / intermission 
 
short		consistancy[MAXPLAYERS][BACKUPTICS]; 
 
byte*		savebuffer;
 
 
// 
// controls (have defaults) 
// 
int             key_right;
int		        key_left;

int		        key_up;
int		        key_down; 
int             key_strafeleft;
int		        key_straferight; 
int             key_fire;
int		        key_use;
int		        key_strafe;
int		        key_speed; 

int             key_usehealth;
int             key_jump;
int             key_invquery;
int             key_mission;
int             key_invpop;
int             key_invkey;
int             key_invhome;
int             key_invend;
int             key_invleft;
int             key_invright;
int             key_invuse;
int             key_invdrop;
int             key_lookup;
int             key_lookdown;
 
int             mousebfire; 
int             mousebstrafe;
int             mousebforward;
int             mousebjump;
 
int             joybfire; 
int             joybstrafe; 
int             joybuse;
int             joybspeed;
int             joybjump;
 
 
 
#define MAXPLMOVE       (forwardmove[1]) 
 
#define TURBOTHRESHOLD  0x32

fixed_t     forwardmove[2] = {0x19, 0x32}; 
fixed_t     sidemove[2] = {0x18, 0x28}; 
fixed_t     angleturn[3] = {640, 1280, 320};	// + slow turn 

int mouse_fire_countdown = 0;    // villsa [STRIFE]
#define SLOWTURNTICS    6 
 
#define NUMKEYS         256 

boolean         gamekeydown[NUMKEYS]; 
int             turnheld;				// for accelerative turning 
 
boolean         mousearray[4]; 
boolean*        mousebuttons = &mousearray[1];		// allow [-1]

// mouse values are used once 
int             mousex;
int             mousey;         

int             dclicktime;
int             dclickstate;
int             dclicks;
int             dclicktime2;
int             dclickstate2;
int             dclicks2;

// joystick values are repeated 
int             joyxmove;
int             joyymove;
boolean         joyarray[5]; 
boolean*        joybuttons = &joyarray[1];      // allow [-1] 
 
int	        savegameslot = 6; // [STRIFE] initialized to 6
char        savedescription[32]; 
 
 
#define	BODYQUESIZE	32

mobj_t*     bodyque[BODYQUESIZE]; 
//int       bodyqueslot;  [STRIFE] unused
 
void*       statcopy;				// for statistics driver
 
 
 
int G_CmdChecksum (ticcmd_t* cmd) 
{ 
    int		i;
    int		sum = 0; 
	 
    for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++) 
	sum += ((int *)cmd)[i]; 
		 
    return sum; 
} 
 

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
// 
void G_BuildTiccmd (ticcmd_t* cmd) 
{ 
    int		i;
    boolean	strafe;
    boolean	bstrafe;
    int		speed;
    int		tspeed;
    int		forward;
    int		side;

    ticcmd_t*	base;

    base = I_BaseTiccmd();		// empty, or external driver
    memcpy(cmd, base, sizeof(*cmd));

    cmd->consistancy =
        consistancy[consoleplayer][maketic%BACKUPTICS];

    // villsa [STRIFE] look up key
    if (gamekeydown[key_lookup])
        cmd->buttons2 |= BT2_LOOKUP;

    // villsa [STRIFE] look down key
    if (gamekeydown[key_lookdown])
        cmd->buttons2 |= BT2_LOOKDOWN;

    // villsa [STRIFE] inventory use key
    if (gamekeydown[key_invuse])
    {
        player_t* player = &players[consoleplayer];
        if (player->numinventory > 0)
        {
            cmd->buttons2 |= BT2_INVUSE;
            cmd->inventory = player->inventory[player->inventorycursor].sprite;
        }
    }

    // villsa [STRIFE] inventory drop key
    if (gamekeydown[key_invdrop])
    {
        player_t* player = &players[consoleplayer];
        if (player->numinventory > 0)
        {
            cmd->buttons2 |= BT2_INVDROP;
            cmd->inventory = player->inventory[player->inventorycursor].sprite;
        }
    }

    // villsa [STRIFE] use medkit
    if (gamekeydown[key_usehealth])
        cmd->buttons2 |= BT2_HEALTH;

    strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe]
        || joybuttons[joybstrafe];
    speed = gamekeydown[key_speed] || joybuttons[joybspeed];

    forward = side = 0;

    // villsa [STRIFE] running causes centerview to occur
    if (speed)
        cmd->buttons2 |= BT2_CENTERVIEW;

    // villsa [STRIFE] disable running if low on health
    if (players[consoleplayer].health <= 15)
        speed = 0;

    // use two stage accelerative turning
    // on the keyboard and joystick
    if (joyxmove < 0
        || joyxmove > 0
        || gamekeydown[key_right]
        || gamekeydown[key_left])
        turnheld += ticdup;
    else
        turnheld = 0;

    if (turnheld < SLOWTURNTICS)
        tspeed = 2;             // slow turn 
    else
        tspeed = speed;

    // let movement keys cancel each other out
    if (strafe)
    {
        if (gamekeydown[key_right])
        {
            // fprintf(stderr, "strafe right\n");
            side += sidemove[speed];
        }
        if (gamekeydown[key_left])
        {
            //	fprintf(stderr, "strafe left\n");
            side -= sidemove[speed];
        }
        if (joyxmove > 0)
            side += sidemove[speed];
        if (joyxmove < 0)
            side -= sidemove[speed];

    }
    else
    {
        if (gamekeydown[key_right])
            cmd->angleturn -= angleturn[tspeed];
        if (gamekeydown[key_left])
            cmd->angleturn += angleturn[tspeed];
        if (joyxmove > 0)
            cmd->angleturn -= angleturn[tspeed];
        if (joyxmove < 0)
            cmd->angleturn += angleturn[tspeed];
    }

    if (gamekeydown[key_up])
    {
        // fprintf(stderr, "up\n");
        forward += forwardmove[speed];
    }
    if (gamekeydown[key_down])
    {
        // fprintf(stderr, "down\n");
        forward -= forwardmove[speed];
    }
    if (joyymove < 0)
        forward += forwardmove[speed];
    if (joyymove > 0)
        forward -= forwardmove[speed];
    if (gamekeydown[key_straferight])
        side += sidemove[speed];
    if (gamekeydown[key_strafeleft])
        side -= sidemove[speed];

    // buttons
    cmd->chatchar = HU_dequeueChatChar();

    // villsa [STRIFE] - add mouse button support for jump
    if (gamekeydown[key_jump] || mousebuttons[mousebjump]
        || joybuttons[joybjump])
        cmd->buttons2 |= BT2_JUMP;
    
    // villsa [STRIFE]: Moved mousebuttons[mousebfire] to below
    if (gamekeydown[key_fire] || joybuttons[joybfire]) 
        cmd->buttons |= BT_ATTACK;

    // villsa [STRIFE]
    if (mousebuttons[mousebfire])
    {
        if (mouse_fire_countdown <= 0)
            cmd->buttons |= BT_ATTACK;
        else
            --mouse_fire_countdown;
    }

    if (gamekeydown[key_use] || joybuttons[joybuse])
    {
        cmd->buttons |= BT_USE;
        // clear double clicks if hit use button 
        dclicks = 0;
    }

    // chainsaw overrides 
    for (i = 0; i<NUMWEAPONS - 1; i++)
        if (gamekeydown['1' + i])
        {
            cmd->buttons |= BT_CHANGE;
            cmd->buttons |= i << BT_WEAPONSHIFT;
            break;
        }

    // mouse
    if (mousebuttons[mousebforward])
        forward += forwardmove[speed];

    // forward double click
    if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1)
    {
        dclickstate = mousebuttons[mousebforward];
        if (dclickstate)
            dclicks++;
        if (dclicks == 2)
        {
            cmd->buttons |= BT_USE;
            dclicks = 0;
        }
        else
            dclicktime = 0;
    }
    else
    {
        dclicktime += ticdup;
        if (dclicktime > 20)
        {
            dclicks = 0;
            dclickstate = 0;
        }
    }

    // strafe double click
    bstrafe =
        mousebuttons[mousebstrafe]
        || joybuttons[joybstrafe];
    if (bstrafe != dclickstate2 && dclicktime2 > 1)
    {
        dclickstate2 = bstrafe;
        if (dclickstate2)
            dclicks2++;
        if (dclicks2 == 2)
        {
            cmd->buttons |= BT_USE;
            dclicks2 = 0;
        }
        else
            dclicktime2 = 0;
    }
    else
    {
        dclicktime2 += ticdup;
        if (dclicktime2 > 20)
        {
            dclicks2 = 0;
            dclickstate2 = 0;
        }
    }

    forward += mousey;
    if (strafe)
        side += mousex * 2;
    else
        cmd->angleturn -= mousex * 0x8;

    mousex = mousey = 0;

    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->forwardmove += forward;
    cmd->sidemove += side;

    // special buttons
    if (sendpause)
    {
        sendpause = false;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (sendsave)
    {
        sendsave = false;
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT);
    }
} 
 

//
// G_DoLoadLevel 
//
extern  gamestate_t     wipegamestate; 
 
void G_DoLoadLevel (boolean userload)
{ 
    int             i;

    levelstarttic = gametic;        // for time calculation

    if (wipegamestate == GS_LEVEL)
        wipegamestate = -1;             // force a wipe 

    gamestate = GS_LEVEL;

    for (i = 0; i<MAXPLAYERS; i++)
    {
        // haleyjd 20110204 [STRIFE]: PST_REBORN if players[i].health <= 0
        if (playeringame[i] && (players[i].playerstate == PST_DEAD || players[i].health <= 0))
            players[i].playerstate = PST_REBORN;
    }

    P_SetupLevel(gamemap);
    displayplayer = consoleplayer;		// view the guy you are playing    
    starttime = I_GetTime();
    gameaction = ga_nothing;
    Z_CheckHeap();

    // clear cmd building stuff
    memset(gamekeydown, 0, sizeof(gamekeydown));
    joyxmove = joyymove = 0;
    mousex = mousey = 0;
    sendpause = sendsave = paused = false;
    memset(mousebuttons, 0, sizeof(mousebuttons));
    memset(joybuttons, 0, sizeof(joybuttons));

    P_DialogLoad(); // villsa [STRIFE]
} 
 
 
//
// G_Responder  
// Get info needed to make ticcmd_ts for the players.
// 
boolean G_Responder (event_t* ev) 
{ 
    // allow spy mode changes even during the demo
    if (gamestate == GS_LEVEL && ev->type == ev_keydown
        && ev->data1 == KEY_F12 && (singledemo || !gameskill)) // [STRIFE]: o_O
    {
        // spy mode 
        do
        {
            displayplayer++;
            if (displayplayer == MAXPLAYERS)
                displayplayer = 0;
        } while (!playeringame[displayplayer] && displayplayer != consoleplayer);
        return true;
    }

    // any other key pops up menu if in demos
    if (gameaction == ga_nothing && !singledemo &&
        (demoplayback || gamestate == GS_DEMOSCREEN)
        )
    {
        if (ev->type == ev_keydown ||
            (ev->type == ev_mouse && ev->data1) ||
            (ev->type == ev_joystick && ev->data1))
        {
            if (devparm && ev->data1 == 'g')
                D_PageTicker(); // [STRIFE]: wat? o_O
            else
                M_StartControlPanel();
            return true;
        }
        return false;
    }

    if (gamestate == GS_LEVEL)
    {
#if 0 
        if (devparm && ev->type == ev_keydown && ev->data1 == ';')
        {
            G_DeathMatchSpawnPlayer(0);
            return true;
}
#endif 
        if (HU_Responder(ev))
            return true;	// chat ate the event 
        if (ST_Responder(ev))
            return true;	// status window ate it 
        if (AM_Responder(ev))
            return true;	// automap ate it 
    }

    if (gamestate == GS_FINALE)
    {
        if (F_Responder(ev))
            return true;	// finale ate the event 
    }

    switch (ev->type)
    {
    case ev_keydown:
        if (ev->data1 == KEY_PAUSE)
        {
            sendpause = true;
            return true;
        }
        if (ev->data1 <NUMKEYS)
            gamekeydown[ev->data1] = true;
        return true;    // eat key down events 

    case ev_keyup:
        if (ev->data1 <NUMKEYS)
            gamekeydown[ev->data1] = false;
        return false;   // always let key up events filter down 

    case ev_mouse:
        mousebuttons[0] = ev->data1 & 1;
        mousebuttons[1] = ev->data1 & 2;
        mousebuttons[2] = ev->data1 & 4;
        mousex = ev->data2*(mouseSensitivity + 5) / 10;
        mousey = ev->data3*(mouseSensitivity + 5) / 10;
        return true;    // eat events 

    case ev_joystick:
        joybuttons[0] = ev->data1 & 1;
        joybuttons[1] = ev->data1 & 2;
        joybuttons[2] = ev->data1 & 4;
        joybuttons[3] = ev->data1 & 8;
        joyxmove = ev->data2;
        joyymove = ev->data3;
        return true;    // eat events 

    default:
        break;
    }

    return false;
} 
 
 
 
//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker (void) 
{ 
    int     i;
    int     buf;
    ticcmd_t*   cmd;

    // do player reborns if needed
    for (i = 0; i<MAXPLAYERS; i++)
        if (playeringame[i] && players[i].playerstate == PST_REBORN)
            G_DoReborn(i);

    // do things to change the game state
    while (gameaction != ga_nothing)
    {
        switch (gameaction)
        {
        case ga_loadlevel:
            G_DoLoadLevel(true);
            break;
        case ga_newgame:
            G_DoNewGame();
            break;
        case ga_loadgame:
            G_DoLoadGame(true);
            M_SaveMoveHereToMap(); // [STRIFE]
            M_ReadMisObj();
            break;
        case ga_savegame:
            M_SaveMoveMapToHere(); // [STRIFE]
            M_SaveMisObj(savepath);
            G_DoSaveGame(savepath);
            break;
        case ga_playdemo:
            G_DoPlayDemo();
            break;
        case ga_completed:
            G_DoCompleted();
            break;
        case ga_victory:
            F_StartFinale();
            break;
        case ga_worlddone:
            G_DoWorldDone();
            break;
        case ga_screenshot:
            M_ScreenShot();
            gameaction = ga_nothing;
            break;
        case ga_nothing:
            break;
        }
    }

    // get commands, check consistancy,
    // and build new consistancy check
    buf = (gametic / ticdup) % BACKUPTICS;

    for (i = 0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            cmd = &players[i].cmd;

            memcpy(cmd, &netcmds[i][buf], sizeof(ticcmd_t));

            if (demoplayback)
                G_ReadDemoTiccmd(cmd);
            if (demorecording)
                G_WriteDemoTiccmd(cmd);

            // check for turbo cheats
            if (cmd->forwardmove > TURBOTHRESHOLD
                && !(gametic & 31) && ((gametic >> 5) & 3) == i)
            {
                static char turbomessage[80];
                sprintf(turbomessage, "%s is turbo!", player_names[i]);
                players[consoleplayer].message = turbomessage;
            }

            if (netgame && !netdemo && !(gametic%ticdup))
            {
                if (gametic > BACKUPTICS
                    && consistancy[i][buf] != cmd->consistancy)
                {
                    I_Error("consistency failure (%i should be %i)",
                        cmd->consistancy, consistancy[i][buf]);
                }
                if (players[i].mo)
                    consistancy[i][buf] = players[i].mo->x;
                else
                    consistancy[i][buf] = rndindex;
            }
        }
    }

    // check for special buttons
    for (i = 0; i<MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            if (players[i].cmd.buttons & BT_SPECIAL)
            {
                switch (players[i].cmd.buttons & BT_SPECIALMASK)
                {
                case BTS_PAUSE:
                    paused ^= 1;
                    if (paused)
                        S_PauseSound();
                    else
                        S_ResumeSound();
                    break;

                case BTS_SAVEGAME:
                    if (!savedescription[0])
                        strcpy(savedescription, "NET GAME");
                    savegameslot =
                        (players[i].cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
                    gameaction = ga_savegame;
                    break;
                }
            }
        }
    }

    // do main actions
    switch (gamestate)
    {
    case GS_LEVEL:
        P_Ticker();
        ST_Ticker();
        AM_Ticker();
        HU_Ticker();
        break;

        // haleyjd 08/23/10: [STRIFE] No intermission.
        /*
        case GS_INTERMISSION:
        WI_Ticker ();
        break;
        */
    case GS_UNKNOWN: // STRIFE-TODO: What is this? is it ever used??
        F_WaitTicker();
        break;

    case GS_FINALE:
        F_Ticker();
        break;

    case GS_DEMOSCREEN:
        D_PageTicker();
        break;
    }
} 
 
 
//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer 
// Called at the start.
// Called by the game initialization functions.
//
// [STRIFE] No such function.
/*
void G_InitPlayer (int player) 
{ 
    player_t*	p; 
 
    // set up the saved info         
    p = &players[player]; 
	 
    // clear everything else to defaults 
    G_PlayerReborn (player); 
	 
} 
*/
 
 

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
// [STRIFE] No such function. The equivalent to this logic was moved into
// G_DoCompleted.
/*
void G_PlayerFinishLevel (int player) 
{ 
    player_t*	p; 
	 
    p = &players[player]; 
	 
    memset (p->powers, 0, sizeof (p->powers)); 
    memset (p->cards, 0, sizeof (p->cards)); 
    p->mo->flags &= ~MF_SHADOW;		// cancel invisibility 
    p->extralight = 0;			// cancel gun flashes 
    p->fixedcolormap = 0;		// cancel ir gogles 
    p->damagecount = 0;			// no palette changes 
    p->bonuscount = 0; 
} 
*/
 

//
// G_PlayerReborn
// Called after a player dies 
// almost everything is cleared and initialized 
//
// [STRIFE] Small changes for allegiance, inventory, health auto-use, and
// mission objective.
//
void G_PlayerReborn (int player) 
{ 
    player_t*	p;
    int     i;
    int     frags[MAXPLAYERS];
    int     killcount;
    int     allegiance;

    memcpy(frags, players[player].frags, sizeof(frags));
    killcount = players[player].killcount;
    allegiance = players[player].allegiance; // [STRIFE]

    p = &players[player];
    memset(p, 0, sizeof(*p));

    memcpy(players[player].frags, frags, sizeof(players[player].frags));
    players[player].killcount = killcount;
    players[player].allegiance = allegiance;

    p->inventorydown = p->usedown = p->attackdown = true;   // don't do anything immediately 
    p->playerstate = PST_LIVE;
    p->health = MAXHEALTH;
    p->weaponowned[wp_fist] = true;
    p->readyweapon = p->pendingweapon = wp_fist;

    for (i = 0; i < NUMAMMO; i++)
        p->maxammo[i] = maxammo[i];


    // [STRIFE] clear inventory
    for (i = 0; i < NUMINVENTORY; i++)
        p->inventory[i].type = NUMMOBJTYPES;

    strncpy(mission_objective, "Find help", OBJECTIVE_LEN);
}

//
// G_CheckSpot  
// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
//
// [STRIFE] Changed to eliminate body queue and an odd error message was added.
//
void P_SpawnPlayer (mapthing_t* mthing); 
 
boolean
G_CheckSpot
( int		playernum,
  mapthing_t*	mthing ) 
{ 
    fixed_t		x;
    fixed_t		y;
    subsector_t*	ss;
    unsigned		an;
    mobj_t*		mo;
    int			i;

    if (!players[playernum].mo)
    {
        // [STRIFE] weird error message added here:
        if (leveltime > 0)
            players[playernum].message = "you didn't have a body!";

        // first spawn of level, before corpses
        for (i = 0; i<playernum; i++)
            if (players[i].mo->x == mthing->x << FRACBITS
                && players[i].mo->y == mthing->y << FRACBITS)
                return false;
        return true;
    }

    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    if (!P_CheckPosition(players[playernum].mo, x, y))
        return false;

    // flush an old corpse if needed
    // [STRIFE] player corpses remove themselves after a short time, so
    // evidently this wasn't needed.
    /*

    // flush an old corpse if needed 
    if (bodyqueslot >= BODYQUESIZE)
        P_RemoveMobj(bodyque[bodyqueslot%BODYQUESIZE]);
    bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo;
    bodyqueslot++;
    */

    // spawn a teleport fog 
    ss = R_PointInSubsector(x, y);
    an = (ANG45 * (mthing->angle / 45)) >> ANGLETOFINESHIFT;

    mo = P_SpawnMobj(x + 20 * finecosine[an], y + 20 * finesine[an]
        , ss->sector->floorheight
        , MT_TFOG);

    if (players[consoleplayer].viewz != 1)
        S_StartSound(mo, sfx_telept);	// don't start sound on first frame 

    return true;
} 


//
// G_DeathMatchSpawnPlayer 
// Spawns a player at one of the random death match spots 
// called at level load and each death 
//
// [STRIFE]: Modified exit message to match binary.
//
void G_DeathMatchSpawnPlayer (int playernum) 
{ 
    int             i, j;
    int				selections;

    selections = deathmatch_p - deathmatchstarts;
    if (selections < 4)
        I_Error("Only %i deathmatch spots, at least 4 required!", selections);

    for (j = 0; j<20; j++)
    {
        i = P_Random() % selections;
        if (G_CheckSpot(playernum, &deathmatchstarts[i]))
        {
            deathmatchstarts[i].type = playernum + 1;
            P_SpawnPlayer(&deathmatchstarts[i]);
            return;
        }
    }

    // no good spot, so the player will probably get stuck 
    P_SpawnPlayer(&playerstarts[playernum]);
}

//
// G_LoadPath
//
// haleyjd 20101003: [STRIFE] New function
// Sets loadpath based on the map and "savepathtemp"
//
void G_LoadPath(int map)
{
    sprintf(loadpath, "%s%d", savepathtemp, map);
}


//
// G_DoReborn 
// 
void G_DoReborn (int playernum) 
{ 
    int i;

    if (!netgame)
    {
        // reload the level from scratch
        // [STRIFE] Reborn level load
        G_LoadPath(gamemap);
        gameaction = ga_loadlevel;
    }
    else
    {
        // respawn at the start

        // first dissasociate the corpse 
        // [STRIFE] Checks for NULL first
        if (players[playernum].mo)
            players[playernum].mo->player = NULL;

        // spawn at random spot if in death match 
        if (deathmatch)
        {
            G_DeathMatchSpawnPlayer(playernum);
            return;
        }

        if (G_CheckSpot(playernum, &playerstarts[playernum]))
        {
            P_SpawnPlayer(&playerstarts[playernum]);
            return;
        }

        // try to spawn at one of the other players spots 
        for (i = 0; i<MAXPLAYERS; i++)
        {
            if (G_CheckSpot(playernum, &playerstarts[i]))
            {
                playerstarts[i].type = playernum + 1;	// fake as other player 
                P_SpawnPlayer(&playerstarts[i]);
                playerstarts[i].type = i + 1;		// restore 
                return;
            }
            // he's going to be inside something.  Too bad.
        }
        P_SpawnPlayer(&playerstarts[playernum]);
    }
} 
 
 
void G_ScreenShot (void) 
{ 
    gameaction = ga_screenshot; 
} 
 

// haleyjd 20100823: [STRIFE] Removed par times.

//
// G_DoCompleted 
//
//boolean		secretexit; 
extern char*	pagename;

//
// G_RiftExitLevel
//
// haleyjd 20100824: [STRIFE] New function
// * Called from some exit linedefs to exit to a specific riftspot in the 
//   given destination map.
//
void G_RiftExitLevel(int map, int spot, angle_t angle)
{
    gameaction = ga_completed;

    // special handling for post-Sigil map changes
    if (players[0].weaponowned[wp_sigil])
    {
        if (map == 3) // Front Base -> Abandoned Front Base
            map = 30;
        if (map == 7) // Castle -> New Front Base
            map = 10;
    }

    // no rifting in deathmatch games
    if (deathmatch)
        spot = 0;

    riftangle = angle;
    riftdest = spot;
    destmap = map;
}

//
// G_Exit2
//
// haleyjd 20101003: [STRIFE] New function.
// No xrefs to this, doesn't seem to be used. Could have gotten inlined
// somewhere but I haven't seen it.
//
void G_Exit2(int dest, angle_t angle)
{
    riftdest = dest;
    gameaction = ga_completed;
    riftangle = angle;
    destmap = gamemap;
}

//
// G_ExitLevel
//
// haleyjd 20100824: [STRIFE]:
// * Default to next map in numeric order; init destmap and riftdest.
//
void G_ExitLevel (int dest)
{
    if (dest == 0)
        dest = gamemap + 1;
    destmap = dest;
    riftdest = 0;
    gameaction = ga_completed;
} 


/*
// haleyjd 20100823: [STRIFE] No secret exits in Strife.
// Here's for the german edition.
void G_SecretExitLevel (void) 
{ 
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if (commercial
      && (W_CheckNumForName("map31")<0))
	secretexit = false;
    else
	secretexit = true; 
    gameaction = ga_completed; 
} 
*/

//
// G_StartFinale
//
// haleyjd 20100921: [STRIFE] New function.
// This replaced G_SecretExitLevel in Strife. I don't know that it's actually
// used anywhere in the game, but it *is* usable in mods via linetype 124,
// W1 Start Finale.
//
void G_StartFinale(void)
{
    gameaction = ga_victory;
}

//
// G_DoCompleted
//
// haleyjd 20100823: [STRIFE]:
// * Removed G_PlayerFinishLevel and just sets some powerup states.
// * Removed Chex, as not relevant to Strife.
// * Removed DOOM level transfer logic 
// * Removed intermission code.
// * Added setting gameaction to ga_worlddone.
//
 
void G_DoCompleted (void) 
{ 
    int i;

    // deal with powerup states
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            // [STRIFE] restore pw_allmap power from mapstate cache
            if (destmap < 40)
                players[i].powers[pw_allmap] = players[i].mapstate[destmap];

            // Shadowarmor doesn't persist between maps in netgames
            if (netgame)
                players[i].powers[pw_invisibility] = 0;
        }
    }

    stonecold = false;  // villsa [STRIFE]

    if (automapactive)
        AM_Stop();

    // [STRIFE] HUB SAVE
    if (!deathmatch)
        G_DoSaveGame(savepathtemp);

    gameaction = ga_worlddone;
} 

// haleyjd 20100824: [STRIFE] No secret exits.
/*
//
// G_WorldDone 
//
void G_WorldDone (void) 
{ 
    gameaction = ga_worlddone; 

    if (secretexit) 
	players[consoleplayer].didsecret = true; 

    if (commercial)
    {
	switch (gamemap)
	{
	  case 15:
	  case 31:
	    if (!secretexit)
		break;
	  case 6:
	  case 11:
	  case 20:
	  case 30:
	    F_StartFinale ();
	    break;
	}
    }
} 
*/

//
// G_RiftPlayer
//
// haleyjd 20100824: [STRIFE] New function
// Teleports the player to the appropriate rift spot.
//
void G_RiftPlayer(void)
{
    if (riftdest)
    {
        P_TeleportMove(players[0].mo,
            riftSpots[riftdest - 1].x << FRACBITS,
            riftSpots[riftdest - 1].y << FRACBITS);
        players[0].mo->angle = riftangle;
        players[0].mo->health = players[0].health;
    }
}

//
// G_RiftCheat
//
// haleyjd 20100824: [STRIFE] New function
// Called from the cheat code to jump to a rift spot.
//
boolean G_RiftCheat(int riftSpotNum)
{
    return P_TeleportMove(players[0].mo,
        riftSpots[riftSpotNum - 1].x << FRACBITS,
        riftSpots[riftSpotNum - 1].y << FRACBITS);
}

//
// G_DoWorldDone
//
// haleyjd 20100824: [STRIFE] Added destmap -> gamemap set.
//
 
void G_DoWorldDone (void) 
{
    int temp_leveltime = leveltime;
    boolean temp_shadow = false;
    boolean temp_mvis = false;

    gamestate = GS_LEVEL;
    gamemap = destmap;

    // [STRIFE] HUB LOAD
    G_LoadPath(destmap);
    if (!deathmatch)
    {
        // Remember Shadowarmor across hub loads
        if (players[0].mo->flags & MF_SHADOW)
            temp_shadow = true;
        if (players[0].mo->flags & MF_MVIS)
            temp_mvis = true;
    }
    G_DoLoadGame(false);

    // [STRIFE] leveltime carries over between maps
    leveltime = temp_leveltime;

    if (!deathmatch)
    {
        // [STRIFE]: transfer saved powerups
        players[0].mo->flags &= ~(MF_SHADOW | MF_MVIS);
        if (temp_shadow)
            players[0].mo->flags |= MF_SHADOW;
        if (temp_mvis)
            players[0].mo->flags |= MF_MVIS;

        // [STRIFE] HUB SAVE
        G_RiftPlayer();
        G_DoSaveGame(savepathtemp);
        M_SaveMisObj(savepathtemp);
    }

    gameaction = ga_nothing; 
    viewactive = true; 
}

//
// G_DoWorldDone2
//
// haleyjd 20101003: [STRIFE] New function. No xrefs; unused.
//
void G_DoWorldDone2(void)
{
    gamestate = GS_LEVEL;
    gameaction = ga_nothing;
    viewactive = true;
}

//
// G_ReadCurrent
//
// haleyjd 20101003: [STRIFE] New function.
// Reads the "CURRENT" file from the given path and then sets it to
// gamemap.
//

void G_ReadCurrent(char *path)
{
    char *temppath = NULL;

    sprintf(temppath, "%s\\current", path);

    if (M_ReadFile(temppath, &savebuffer) <= 0)
        gameaction = ga_newgame;
    else
    {
        save_p = savebuffer;
        gamemap = *savebuffer;
        gameaction = ga_loadgame;
        Z_Free(savebuffer);
    }

    G_LoadPath(gamemap);
}

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task. 
//
extern boolean setsizeneeded;
void R_ExecuteSetViewSize (void);


// [STRIFE]: No such function.
/*
void G_LoadGame (char* name) 
{ 
    strcpy (savename, name); 
    gameaction = ga_loadgame; 
} 
*/


// haleyjd 20100928: [STRIFE] VERSIONSIZE == 8
#define VERSIONSIZE		8 

void G_DoLoadGame (boolean userload)
{ 
    int     length; 
    int     i; 
    int     a,b,c;
    char    vcheck[VERSIONSIZE];
	 
    gameaction = ga_nothing; 
	 
    length = M_ReadFile (loadpath, &savebuffer);
    if (!length)
    {
        G_DoLoadLevel(true);
        return;
    }
    
    save_p = savebuffer;
    
    // skip the description field 
    memset (vcheck,0,sizeof(vcheck)); 
    sprintf (vcheck,"ver %i",VERSION); 
    if (strcmp ((char*)save_p, vcheck)) 
	    return;             // bad version 
    save_p += VERSIONSIZE; 
			 
    gameskill = *save_p++;
    for (i = 0; i < MAXPLAYERS; i++)
        playeringame[i] = *save_p++;

    // load a base level
    if (userload)
    {
        G_InitNew(gameskill, gamemap);
    }
    else
    {
        G_DoLoadLevel(true);
    }
 
    // get the times 
    a = *save_p++; 
    b = *save_p++; 
    c = *save_p++;
    leveltime = (a << 16) + (b << 8) + c;
	 
    // dearchive all the modifications
    P_UnArchivePlayers (userload);
    P_UnArchiveWorld (); 
    P_UnArchiveThinkers (); 
    P_UnArchiveSpecials (); 

    if (*save_p != 0x1d)
        I_Error("Bad savegame");

    // done 
    Z_Free(savebuffer);
    
    // draw the pattern into the back screen
    R_FillBackScreen ();   
} 
 

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string 
//
void
G_SaveGame
( int	slot,
  char*	description ) 
{
    char name[128];

    savegameslot = slot;
    if (M_CheckParm("-cdrom"))
    {
        sprintf(savepathtemp, "c:\\strife.cd\\strfsav%d.ssg\\", 6);
        sprintf(savepath, "c:\\strife.cd\\strfsav%d.ssg\\", savegameslot);
    }
    else
    {
        sprintf(savepathtemp, "strfsav%d.ssg\\", 6);
        sprintf(savepath, "strfsav%d.ssg\\", savegameslot);
    }
    strcpy (savedescription, description);
    sprintf(name, "%sname", savepathtemp);
    M_WriteFile(name, savedescription, sizeof(savedescription));
} 

char savename[80];
 
void G_DoSaveGame (char *path)
{ 
    char	name2[VERSIONSIZE];
    int		length; 
    int		i; 
	
    sprintf(savename, "%scurrent", path);
    M_WriteFile(savename, &gamemap, 4);

    sprintf(savename, "%s%d", path, gamemap);

    save_p = savebuffer = screens[1] + 0x4000;

    memset(name2, 0, sizeof(name2));
    sprintf(name2, "ver %i", VERSION);
    memcpy (save_p, name2, VERSIONSIZE);
    save_p += VERSIONSIZE; 
	 
    *save_p++ = gameskill;

    for (i = 0; i<MAXPLAYERS; i++)
        *save_p++ = playeringame[i];
    *save_p++ = leveltime>>16; 
    *save_p++ = leveltime>>8; 
    *save_p++ = leveltime; 
 
    P_ArchivePlayers (); 
    P_ArchiveWorld (); 
    P_ArchiveThinkers (); 
    P_ArchiveSpecials (); 
	 
    *save_p++ = 0x1d;		// consistancy marker 
	 
    length = save_p - savebuffer;
    if (length > SAVEGAMESIZE)
        I_Error("Savegame buffer overrun");
    M_WriteFile (savename, savebuffer, length);
    gameaction = ga_nothing; 

    sprintf(savename, "%s saved.", savedescription);

    players[consoleplayer].message = savename;

    // draw the pattern into the back screen
    R_FillBackScreen ();	
} 
 

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set. 
//
skill_t	d_skill;
//int     d_episode; [STRIFE] No such thing as episodes in Strife
int     d_map;

//
// G_DeferedInitNew
//
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set. 
//
// haleyjd 20100922: [STRIFE] Removed episode parameter
//
 
void
G_DeferedInitNew
( skill_t	skill,
  int		map) 
{
    d_map = map;
    d_skill = skill;
    gameaction = ga_newgame; 
} 


//
// G_DoNewGame
//
// [STRIFE] Code added to turn off the stonecold effect.
//   Someone also removed the nomonsters reset...
//

void G_DoNewGame (void) 
{
    demoplayback = false; 
    netdemo = false;
    netgame = false;
    deathmatch = false;
    playeringame[1] = playeringame[2] = playeringame[3] = 0;
    respawnparm = false;
    fastparm = false;
    stonecold = false;
    consoleplayer = 0;
    G_InitNew (d_skill, d_map); 
    gameaction = ga_nothing; 
} 


//
// G_InitNew
//
// haleyjd 20100824: [STRIFE]:
// * Added riftdest initialization
// * Removed episode parameter
//
void
G_InitNew
( skill_t	skill,
  int		map ) 
{ 
    char *skytexturename;
    int i;

    if (paused)
    {
        paused = false;
        S_ResumeSound();
    }


    if (skill > sk_nightmare)
        skill = sk_nightmare;

    // [STRIFE] Removed episode nonsense and gamemap clipping

    M_ClearRandom();

    if (skill == sk_nightmare || respawnparm)
        respawnmonsters = true;
    else
        respawnmonsters = false;

    // [STRIFE] Strife skill level mobjinfo/states tweaking
    // BUG: None of this code runs properly when loading save games, so
    // basically it's impossible to play any skill level properly unless
    // you never quit and reload from the command line.
    if (!skill && gameskill)
    {
        // Setting to Baby skill... make things easier.

        // Acolytes walk, attack, and feel pain slower
        for (i = S_AGRD_13; i <= S_AGRD_23; i++)
            states[i].tics *= 2;

        // Reavers attack slower
        for (i = S_ROB1_10; i <= S_ROB1_15; i++)
            states[i].tics *= 2;

        // Turrets attack slower
        for (i = S_TURT_02; i <= S_TURT_03; i++)
            states[i].tics *= 2;

        // Crusaders attack and feel pain slower
        for (i = S_ROB2_09; i <= S_ROB2_19; i++)
            states[i].tics *= 2;

        // Stalkers think, walk, and attack slower
        for (i = S_SPID_03; i <= S_SPID_10; i++)
            states[i].tics *= 2;

        // The Bishop's homing missiles are faster (what?? BUG?)
        mobjinfo[MT_SEEKMISSILE].speed *= 2;
    }
    if (skill && !gameskill)
    {
        // Setting a higher skill when previously on baby... make things normal

        // Acolytes
        for (i = S_AGRD_13; i <= S_AGRD_23; i++)
            states[i].tics >>= 1;

        // Reavers
        for (i = S_ROB1_10; i <= S_ROB1_15; i++)
            states[i].tics >>= 1;

        // Turrets
        for (i = S_TURT_02; i <= S_TURT_03; i++)
            states[i].tics >>= 1;

        // Crusaders
        for (i = S_ROB2_09; i <= S_ROB2_19; i++)
            states[i].tics >>= 1;

        // Stalkers
        for (i = S_SPID_03; i <= S_SPID_10; i++)
            states[i].tics >>= 1;

        // The Bishop's homing missiles - again, seemingly backward.
        mobjinfo[MT_SEEKMISSILE].speed >>= 1;
    }
    if (fastparm || (skill == sk_nightmare && skill != gameskill))
    {
        // BLOODBATH! Make some things super-aggressive.

        // Acolytes walk, attack, and feel pain twice as fast
        // (This makes just getting out of the first room almost impossible)
        for (i = S_AGRD_13; i <= S_AGRD_23; i++)
            states[i].tics >>= 1;

        // Bishop's homing missiles again get SLOWER and not faster o_O
        mobjinfo[MT_SEEKMISSILE].speed >>= 1;
    }
    else if (skill != sk_nightmare && gameskill == sk_nightmare)
    {
        // Setting back to an ordinary skill after being on Bloodbath?
        // Put stuff back to normal.

        // Acolytes
        for (i = S_AGRD_13; i <= S_AGRD_23; i++)
            states[i].tics *= 2;

        // Bishop's homing missiles
        mobjinfo[MT_SEEKMISSILE].speed *= 2;
    }

    gamemap = map;
    gameskill = skill;

    // force players to be initialized upon first level load         
    for (i = 0; i<MAXPLAYERS; i++)
        players[i].playerstate = PST_REBORN;

    usergame = true;                // will be set false if a demo 
    viewactive = true;
    paused = false;
    demoplayback = false;
    automapactive = false;
    riftdest = 0; // haleyjd 08/24/10: [STRIFE] init riftdest to zero on new game

    // set the sky map for the episode

    // [STRIFE] Strife skies (of which there are but two)
    if (gamemap >= 9 && gamemap < 32)
        skytexturename = "skymnt01";
    else
        skytexturename = "skymnt02";

    skytexture = R_TextureNumForName(skytexturename);

    G_LoadPath(gamemap);

    G_DoLoadLevel(true);
} 
 

//
// DEMO RECORDING 
// 
#define DEMOMARKER  0x80


void G_ReadDemoTiccmd (ticcmd_t* cmd) 
{ 
    if (*demo_p == DEMOMARKER)
    {
        // end of demo data stream 
        G_CheckDemoStatus();
        return;
    }
    cmd->forwardmove = ((signed char)*demo_p++);
    cmd->sidemove = ((signed char)*demo_p++);
    cmd->angleturn = ((unsigned char)*demo_p++) << 8;
    cmd->buttons = (unsigned char)*demo_p++;
    cmd->buttons2 = (unsigned char)*demo_p++; // [STRIFE]
    cmd->inventory = (int)*demo_p++;          // [STRIFE]
} 


void G_WriteDemoTiccmd (ticcmd_t* cmd) 
{ 
    if (gamekeydown['q'])           // press q to end demo recording 
        G_CheckDemoStatus();
    *demo_p++ = cmd->forwardmove;
    *demo_p++ = cmd->sidemove;
    *demo_p++ = (cmd->angleturn + 128) >> 8;
    *demo_p++ = cmd->buttons;
    *demo_p++ = cmd->buttons2;                 // [STRIFE]
    *demo_p++ = (byte)(cmd->inventory & 0xff); // [STRIFE]
    demo_p -= 6;
    if (demo_p > demoend - 16)
    {
        // no more space 
        G_CheckDemoStatus();
        return;
    }

    G_ReadDemoTiccmd(cmd);         // make SURE it is exactly the same 
} 
 
 
 
//
// G_RecordDemo 
// 
void G_RecordDemo (char* name) 
{ 
    int             i; 
    int				maxsize;
	
    usergame = false; 
    strcpy (demoname, name); 
    strcat (demoname, ".lmp"); 
    maxsize = 0x20000;
    i = M_CheckParm ("-maxdemo");
    if (i && i<myargc - 1)
        maxsize = atoi(myargv[i + 1]) * 1024;
    demobuffer = Z_Malloc(maxsize, PU_STATIC, NULL);
    demoend = demobuffer + maxsize;
	
    demorecording = true; 
} 
 
 
void G_BeginRecording (void) 
{ 
    int i;

    demo_p = demobuffer;

    *demo_p++ = VERSION;
    *demo_p++ = gameskill;
    *demo_p++ = gamemap;
    *demo_p++ = deathmatch;
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = consoleplayer;

    for (i = 0; i<MAXPLAYERS; i++)
        *demo_p++ = playeringame[i];
} 
 

//
// G_PlayDemo 
//

char*	defdemoname; 
 
void G_DeferedPlayDemo (char* name) 
{ 
    defdemoname = name; 
    gameaction = ga_playdemo; 
} 
 
void G_DoPlayDemo (void) 
{ 
    skill_t skill; 
    int i, map; 
	 
    gameaction = ga_nothing; 
    demobuffer = demo_p = W_CacheLumpName (defdemoname, PU_STATIC); 
    if (*demo_p++ != VERSION)
    {
        I_Error("Demo is from a different game version!");
        gameaction = ga_nothing;
        return;
    }
    
    skill = *demo_p++; 
    map = *demo_p++; 
    deathmatch = *demo_p++;
    respawnparm = *demo_p++;
    fastparm = *demo_p++;
    nomonsters = *demo_p++;
    consoleplayer = *demo_p++;
	
    for (i = 0; i<MAXPLAYERS; i++)
        playeringame[i] = *demo_p++;
    if (playeringame[1])
    {
        netgame = true;
        netdemo = true;
    }

    // don't spend a lot of time in loadlevel 
    precache = false;
    G_InitNew(skill, map);
    precache = true;

    demoplayback = true;
    usergame = false;
} 

//
// G_TimeDemo 
//
void G_TimeDemo (char* name) 
{ 	 
    nodrawers = M_CheckParm ("-nodraw"); 
    noblit = M_CheckParm ("-noblit"); 
    timingdemo = true; 
    singletics = true; 

    defdemoname = name; 
    gameaction = ga_playdemo; 
} 
 
 
/* 
=================== 
= 
= G_CheckDemoStatus 
= 
= Called after a death or level completion to allow demos to be cleaned up 
= Returns true if a new demo loop action will take place 
=================== 
*/ 
 
boolean G_CheckDemoStatus (void) 
{ 
    int endtime;

    if (timingdemo)
    {
        endtime = I_GetTime();
        I_Error("timed %i gametics in %i realtics", gametic
            , endtime - starttime);
    }

    if (demoplayback)
    {
        if (singledemo)
            I_Quit();

        Z_ChangeTag(demobuffer, PU_CACHE);
        demoplayback = false;
        netdemo = false;
        netgame = false;
        deathmatch = false;
        playeringame[1] = playeringame[2] = playeringame[3] = 0;
        respawnparm = false;
        fastparm = false;
        nomonsters = false;
        consoleplayer = 0;
        D_AdvanceDemo();
        return true;
    }

    if (demorecording)
    {
        *demo_p++ = DEMOMARKER;
        M_WriteFile(demoname, demobuffer, demo_p - demobuffer);
        Z_Free(demobuffer);
        demorecording = false;
        I_Error("Demo %s recorded", demoname);
    }

    return false;
} 
