//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
// Copyright (C) 2005-2014 Simon Howard
// Copyright (C) 2010 James Haley, Samuel Villarreal
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
//	Game completion, final screen animation.
//

#include <ctype.h>

// Functions.
#include "d_main.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"

#include "p_dialog.h" // [STRIFE]

// ?
//#include "doomstat.h"
//#include "r_local.h"
//#include "f_finale.h"

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int     finalestage;

int     finalecount;

// haleyjd 09/12/10: [STRIFE] Slideshow variables
char         *slideshow_panel;
unsigned int  slideshow_tics;
int           slideshow_state;

// haleyjd 09/13/10: [STRIFE] All this is unused.
/*

#define	TEXTSPEED	3
#define	TEXTWAIT	250

char*	e1text = E1TEXT;
char*	e2text = E2TEXT;
char*	e3text = E3TEXT;

char*	c1text = C1TEXT;
char*	c2text = C2TEXT;
char*	c3text = C3TEXT;
char*	c4text = C4TEXT;
char*	c5text = C5TEXT;
char*	c6text = C6TEXT;

char*	finaletext;
char*	finaleflat;

*/

void	F_StartCast (void);
void	F_CastTicker (void);
boolean F_CastResponder (event_t *ev);
void	F_CastDrawer (void);


// [STRIFE] - Slideshow states enumeration
enum
{
    // Exit states
    SLIDE_EXITHACK = -99, // Hacky exit - start a new dialog
    SLIDE_HACKHACK = -9, // Bizarre unused state
    SLIDE_EXIT = -1, // Exit to next finale state
//    SLIDE_CHOCO = -2, // haleyjd: This state is Choco-specific... see below.

                      // Unknown
    SLIDE_UNKNOWN = 0, // Dunno what it's for, possibly unused

                       // MAP03 - Macil's Programmer exposition
    SLIDE_PROGRAMMER1 = 1,
    SLIDE_PROGRAMMER2,
    SLIDE_PROGRAMMER3,
    SLIDE_PROGRAMMER4, // Next state = -99

                       // MAP10 - Macil's Sigil exposition
    SLIDE_SIGIL1 = 5,
    SLIDE_SIGIL2,
    SLIDE_SIGIL3,
    SLIDE_SIGIL4, // Next state = -99

                  // MAP29 - Endings
                  // Good Ending
    SLIDE_GOODEND1 = 10,
    SLIDE_GOODEND2,
    SLIDE_GOODEND3,
    SLIDE_GOODEND4, // Next state = -1

                    // Bad Ending
    SLIDE_BADEND1 = 14,
    SLIDE_BADEND2,
    SLIDE_BADEND3, // Next state = -1

                   // Blah Ending
    SLIDE_BLAHEND1 = 17,
    SLIDE_BLAHEND2,
    SLIDE_BLAHEND3, // Next state = -1

                    // Demo Ending - haleyjd 20130301: v1.31 only
    SLIDE_DEMOEND1 = 25,
    SLIDE_DEMOEND2 // Next state = -1
};

//
// F_StartFinale
//
void F_StartFinale (void)
{
    patch_t *panel;

    gameaction = ga_nothing;
    gamestate = GS_FINALE;
    viewactive = false;
    automapactive = false;
    wipegamestate = -1; // [STRIFE]

    // [STRIFE] Setup the slide show
    slideshow_panel = "PANEL0";

    // haleyjd 20111006: These two lines of code *are* in vanilla Strife; 
    // however, there, they were completely inconsequential due to the dirty
    // rects system. No intervening V_MarkRect call means PANEL0 was never 
    // drawn to the framebuffer. In Chocolate Strife, however, with no such
    // system in place, this only manages to fuck up the fade-out that is
    // supposed to happen at the beginning of all finales. So, don't do it!
    // 
    panel = (patch_t *)W_CacheLumpName(slideshow_panel, PU_CACHE);
    V_DrawPatch(0, 0, 0, panel);

    switch (gamemap)
    {
    case 3:  // Macil's exposition on the Programmer
        slideshow_state = SLIDE_PROGRAMMER1;
        break;
    case 9:  // Super hack for death of Programmer
        slideshow_state = SLIDE_EXITHACK;
        break;
    case 10: // Macil's exposition on the Sigil
        slideshow_state = SLIDE_SIGIL1;
        break;
    case 29: // Endings
        if (!netgame)
        {
            if (players[0].health <= 0)            // Bad ending 
                slideshow_state = SLIDE_BADEND1;  // - Humanity goes extinct
            else
            {
                if ((players[0].questflags & QF_QUEST25) && // Converter destroyed
                    (players[0].questflags & QF_QUEST27))   // Computer destroyed (wtf?!)
                {
                    // Good ending - You get the hot babe.
                    slideshow_state = SLIDE_GOODEND1;
                }
                else
                {
                    // Blah ending - You win the battle, but fail at life.
                    slideshow_state = SLIDE_BLAHEND1;
                }
            }
        }
        break;
    case 34: // For the demo version ending
        slideshow_state = SLIDE_EXIT;

        // haleyjd 20130301: Somebody noticed the demo levels were missing the
        // ending they used to have in the demo version EXE, I guess. But the
        // weird thing is, this will only trigger if you run with strife0.wad,
        // and no released version thereof actually works with the 1.31 EXE
        // due to differing dialog formats... was there to be an updated demo
        // that never got released?!
        if (isdemoversion)
            slideshow_state = SLIDE_DEMOEND1;
        break;
    }

    S_ChangeMusic(mus_dark, 1);
    slideshow_tics = 7;
    finalestage = 0;
    finalecount = 0;
}

boolean F_Responder (event_t *event)
{
    if (finalestage == 2)
        return F_CastResponder(event);

    return false;
}

//
// F_WaitTicker
//
// [STRIFE] New function
// haleyjd 09/13/10: This is called from G_Ticker if gamestate is 1, but we
// have no idea for what it's supposed to be. It is unused.
//
void F_WaitTicker(void)
{
    if (++finalecount >= 250)
    {
        gamestate = GS_FINALE;
        finalestage = 0;
        finalecount = 0;
    }
}

// 
// F_DoSlideShow
//
// [STRIFE] New function
// haleyjd 09/13/10: Handles slideshow states. Begging to be tabulated!
//
void F_DoSlideShow(void)
{
    switch (slideshow_state)
    {
    case SLIDE_UNKNOWN: // state #0, seems to be unused
        slideshow_tics = 700;
        slideshow_state = SLIDE_EXIT;
        // falls through into state 1, so above is pointless? ...

    case SLIDE_PROGRAMMER1: // state #1
        slideshow_panel = "SS2F1";
        I_StartVoice("MAC10");
        slideshow_state = SLIDE_PROGRAMMER2;
        slideshow_tics = 315;
        break;
    case SLIDE_PROGRAMMER2: // state #2
        slideshow_panel = "SS2F2";
        I_StartVoice("MAC11");
        slideshow_state = SLIDE_PROGRAMMER3;
        slideshow_tics = 350;
        break;
    case SLIDE_PROGRAMMER3: // state #3
        slideshow_panel = "SS2F3";
        I_StartVoice("MAC12");
        slideshow_state = SLIDE_PROGRAMMER4;
        slideshow_tics = 420;
        break;
    case SLIDE_PROGRAMMER4: // state #4
        slideshow_panel = "SS2F4";
        I_StartVoice("MAC13");
        slideshow_state = SLIDE_EXITHACK; // End of slides
        slideshow_tics = 595;
        break;

    case SLIDE_SIGIL1: // state #5
        slideshow_panel = "SS3F1";
        I_StartVoice("MAC16");
        slideshow_state = SLIDE_SIGIL2;
        slideshow_tics = 350;
        break;
    case SLIDE_SIGIL2: // state #6
        slideshow_panel = "SS3F2";
        I_StartVoice("MAC17");
        slideshow_state = SLIDE_SIGIL3;
        slideshow_tics = 420;
        break;
    case SLIDE_SIGIL3: // state #7
        slideshow_panel = "SS3F3";
        I_StartVoice("MAC18");
        slideshow_tics = 420;
        slideshow_state = SLIDE_SIGIL4;
        break;
    case SLIDE_SIGIL4: // state #8
        slideshow_panel = "SS3F4";
        I_StartVoice("MAC19");
        slideshow_tics = 385;
        slideshow_state = SLIDE_EXITHACK; // End of slides
        break;

    case SLIDE_GOODEND1: // state #10
        slideshow_panel = "SS4F1";
        S_StartMusic(mus_happy);
        I_StartVoice("RIE01");
        slideshow_state = SLIDE_GOODEND2;
        slideshow_tics = 455;
        break;
    case SLIDE_GOODEND2: // state #11
        slideshow_panel = "SS4F2";
        I_StartVoice("BBX01");
        slideshow_state = SLIDE_GOODEND3;
        slideshow_tics = 385;
        break;
    case SLIDE_GOODEND3: // state #12
        slideshow_panel = "SS4F3";
        I_StartVoice("BBX02");
        slideshow_state = SLIDE_GOODEND4;
        slideshow_tics = 490;
        break;
    case SLIDE_GOODEND4: // state #13
        slideshow_panel = "SS4F4";
        slideshow_state = SLIDE_EXIT; // Go to end credits
        slideshow_tics = 980;
        break;

    case SLIDE_BADEND1: // state #14
        S_StartMusic(mus_sad);
        slideshow_panel = "SS5F1";
        I_StartVoice("SS501b");
        slideshow_state = SLIDE_BADEND2;
        slideshow_tics = 385;
        break;
    case SLIDE_BADEND2: // state #15
        slideshow_panel = "SS5F2";
        I_StartVoice("SS502b");
        slideshow_state = SLIDE_BADEND3;
        slideshow_tics = 350;
        break;
    case SLIDE_BADEND3: // state #16
        slideshow_panel = "SS5F3";
        I_StartVoice("SS503b");
        slideshow_state = SLIDE_EXIT; // Go to end credits
        slideshow_tics = 385;
        break;

    case SLIDE_BLAHEND1: // state #17
        S_StartMusic(mus_end);
        slideshow_panel = "SS6F1";
        I_StartVoice("SS601A");
        slideshow_state = SLIDE_BLAHEND2;
        slideshow_tics = 280;
        break;
    case SLIDE_BLAHEND2: // state #18
        S_StartMusic(mus_end);
        slideshow_panel = "SS6F2";
        I_StartVoice("SS602A");
        slideshow_state = SLIDE_BLAHEND3;
        slideshow_tics = 280;
        break;
    case SLIDE_BLAHEND3: // state #19
        S_StartMusic(mus_end);
        slideshow_panel = "SS6F3";
        I_StartVoice("SS603A");
        slideshow_state = SLIDE_EXIT; // Go to credits
        slideshow_tics = 315;
        break;

    case SLIDE_DEMOEND1: // state #25 - only exists in 1.31
        slideshow_panel = "PANEL7";
        slideshow_tics = 175;
        slideshow_state = SLIDE_DEMOEND2;
        break;
    case SLIDE_DEMOEND2: // state #26 - ditto
        slideshow_panel = "VELLOGO";
        slideshow_tics = 175;
        slideshow_state = SLIDE_EXIT; // Go to end credits
        break;

    case SLIDE_EXITHACK: // state -99: super hack state
        gamestate = GS_LEVEL;
        P_DialogStartP1();
        break;
    case SLIDE_HACKHACK: // state -9: unknown bizarre unused state
        S_StartSound(NULL, sfx_rifle);
        slideshow_tics = 3150;
        break;
    case SLIDE_EXIT: // state -1: proceed to next finale stage
        finalecount = 0;
        finalestage = 1;
        wipegamestate = -1;
        S_StartMusic(mus_fast);
        break;
    default:
        break;
    }

    finalecount = 0;
}



//
// F_Ticker
//
// [STRIFE] Modifications for new finales
// haleyjd 09/13/10: Calls F_DoSlideShow
//
void F_Ticker(void)
{
    int i;

    // check for skipping
    if (finalecount > 50) // [STRIFE] No commercial check
    {
        // go on to the next level
        for (i = 0; i < MAXPLAYERS; i++)
            if (players[i].cmd.buttons)
                break;

        if (i < MAXPLAYERS)
            finalecount = slideshow_tics; // [STRIFE]
    }

    // advance animation
    finalecount++;

    if (finalestage == 2)
        F_CastTicker();
    else if (finalecount > slideshow_tics) // [STRIFE] Advance slideshow
        F_DoSlideShow();
}


// haleyjd 09/13/10: Not present in Strife: Cast drawing functions
#include "hu_stuff.h"
extern	patch_t *hu_font[HU_FONTSIZE];


/*
//
// F_TextWrite
//

void F_TextWrite (void)
{
    byte*	src;
    byte*	dest;
    
    int		x,y,w;
    int		count;
    char*	ch;
    int		c;
    int		cx;
    int		cy;
    
    // erase the entire screen to a tiled background
    src = W_CacheLumpName ( finaleflat , PU_CACHE);
    dest = screens[0];
	
    for (y=0 ; y<SCREENHEIGHT ; y++)
    {
	for (x=0 ; x<SCREENWIDTH/64 ; x++)
	{
	    memcpy (dest, src+((y&63)<<6), 64);
	    dest += 64;
	}
	if (SCREENWIDTH&63)
	{
	    memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
	    dest += (SCREENWIDTH&63);
	}
    }

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
    
    // draw some of the text onto the screen
    cx = 10;
    cy = 10;
    ch = finaletext;
	
    count = (finalecount - 10)/TEXTSPEED;
    if (count < 0)
	count = 0;
    for ( ; count ; count-- )
    {
	c = *ch++;
	if (!c)
	    break;
	if (c == '\n')
	{
	    cx = 10;
	    cy += 11;
	    continue;
	}
		
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}
		
	w = SHORT (hu_font[c]->width);
	if (cx+w > SCREENWIDTH)
	    break;
	V_DrawPatch(cx, cy, 0, hu_font[c]);
	cx+=w;
    }
	
}
*/
//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
    int         isindemo; // [STRIFE] Changed from name, which is in mobjinfo
    mobjtype_t  type;
} castinfo_t;

// haleyjd: [STRIFE] A new cast order was defined, however it is unused in any
// of the released versions of Strife, even including the demo version :(
castinfo_t      castorder[] = {
    { 1, MT_PLAYER     },
    { 1, MT_BEGGAR1    },
    { 1, MT_PEASANT2_A },
    { 1, MT_REBEL1     },
    { 1, MT_GUARD1     },
    { 1, MT_CRUSADER   },
    { 1, MT_RLEADER2   },
    { 0, MT_SENTINEL   },
    { 0, MT_STALKER    },
    { 0, MT_PROGRAMMER },
    { 0, MT_REAVER     },
    { 0, MT_PGUARD     },
    { 0, MT_INQUISITOR },
    { 0, MT_PRIEST     },
    { 0, MT_SPECTRE_A  },
    { 0, MT_BISHOP     },
    { 0, MT_ENTITY     },
    { 1, NUMMOBJTYPES  }
};

int         castnum;
int         casttics;
state_t*    caststate;
boolean     castdeath;
int         castframes;
int         castonmelee;
boolean     castattacking;


//
// F_StartCast
//


void F_StartCast (void)
{
    usergame = false;
    gameaction = false;
    viewactive = false;
    castnum = 0;
    gamestate = GS_FINALE;
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    wipegamestate = -1;     // force a screen wipe
    casttics = caststate->tics;
    if (casttics > 50)
        casttics = 50;
    castdeath = false;
    finalestage = 2;	
    castframes = 0;
    castonmelee = 0;
    castattacking = false;
}


//
// F_CastTicker
//
// [STRIFE] Heavily modified, but unused.
// haleyjd 09/13/10: Yeah, I bothered translating this even though it isn't
// going to be seen, in part because I hope some Strife port or another will
// pick it up and finish it, adding it as the optional menu item it was 
// meant to be, or just adding it as part of the ending sequence.
//
void F_CastTicker (void)
{
    int     st;

    if (--casttics > 0)
        return;			// not time to change state yet

    if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
        // switch from deathstate to next monster
        castnum++;
        castdeath = false;
        if (isdemoversion)
        {
            // [STRIFE] Demo version had a shorter cast
            if (!castorder[castnum].isindemo)
                castnum = 0;
        }
        // [STRIFE] Break on type == NUMMOBJTYPES rather than name == NULL
        if (castorder[castnum].type == NUMMOBJTYPES)
            castnum = 0;
        if (mobjinfo[castorder[castnum].type].seesound)
            S_StartSound(NULL, mobjinfo[castorder[castnum].type].seesound);
        caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        castframes = 0;
    }
    else
    {
        int sfx = 0;

        // just advance to next state in animation
        if (caststate == &states[S_PLAY_05])    // villsa [STRIFE] - updated
            goto stopattack;	// Oh, gross hack!
        st = caststate->nextstate;
        caststate = &states[st];
        castframes++;

        if (st != mobjinfo[castorder[castnum].type].meleestate &&
            st != mobjinfo[castorder[castnum].type].missilestate)
        {
            if (st == S_PLAY_05)
                sfx = sfx_rifle;
            else
                sfx = 0;
        }
        else
            sfx = mobjinfo[castorder[castnum].type].attacksound;

        if (sfx)
            S_StartSound(NULL, sfx);
    }

    if (!castdeath && castframes == 12)
    {
        // go into attack frame
        castattacking = true;
        if (castonmelee)
            caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
        else
            caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
        castonmelee ^= 1;
        if (caststate == &states[S_NULL])
        {
            if (castonmelee)
                caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
            else
                caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
        }
    }

    if (castattacking)
    {
        if (castframes == 24
            || caststate == &states[mobjinfo[castorder[castnum].type].seestate])
        {
        stopattack:
            castattacking = false;
            castframes = 0;
            caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
    }

    casttics = caststate->tics;
    if (casttics > 50) // [STRIFE] Cap tics
        casttics = 50;
    else if (casttics == -1)
        casttics = 15;
}


//
// F_CastResponder
//
// [STRIFE] This still exists in Strife but is never used.
// It was used at some point in development, however, as they made
// numerous modifications to the cast call system.
//
boolean F_CastResponder (event_t* ev)
{
    if (ev->type != ev_keydown)
        return false;

    if (castdeath)
        return true;                    // already in dying frames

    // go into death frame
    castdeath = true;
    caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
    casttics = caststate->tics;
    if (casttics > 50) // [STRIFE] Upper bound on casttics
        casttics = 50;
    castframes = 0;
    castattacking = false;
    if (mobjinfo[castorder[castnum].type].deathsound)
        S_StartSound(NULL, mobjinfo[castorder[castnum].type].deathsound);

    return true;
}

//
// F_CastPrint
//
// [STRIFE] Verified unmodified, and unused.
//
void F_CastPrint (char* text)
{
    char*   ch;
    int     c;
    int     cx;
    int     w;
    int     width;

    // find width
    ch = text;
    width = 0;

    while (ch)
    {
        c = *ch++;
        if (!c)
            break;
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            width += 4;
            continue;
        }

        w = SHORT(hu_font[c]->width);
        width += w;
    }

    // draw it
    cx = 160 - width / 2;
    ch = text;
    while (ch)
    {
        c = *ch++;
        if (!c)
            break;
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        w = SHORT(hu_font[c]->width);
        V_DrawPatch(cx, 180, 0, hu_font[c]);
        cx += w;
    }

}


// haleyjd 09/13/10: [STRIFE] Unfortunately they removed whatever was
// partway finished of this function from the binary, as there is no
// trace of it. This means we cannot know for sure what the cast call
// would have looked like. :(
/*

//
// F_CastDrawer
//
void V_DrawPatchFlipped (int x, int y, int scrn, patch_t *patch);

void F_CastDrawer (void)
{
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    boolean		flip;
    patch_t*		patch;
    
    // erase the entire screen to a background
    V_DrawPatch (0,0,0, W_CacheLumpName ("BOSSBACK", PU_CACHE));

    F_CastPrint (castorder[castnum].name);
    
    // draw the current frame in the middle of the screen
    sprdef = &sprites[caststate->sprite];
    sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];
    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];
			
    patch = W_CacheLumpNum (lump+firstspritelump, PU_CACHE);
    if (flip)
	V_DrawPatchFlipped (160,170,0,patch);
    else
	V_DrawPatch (160,170,0,patch);
}
*/


//
// F_DrawPatchCol
//
// [STRIFE] Verified unmodified, but not present in 1.2
// It WAS present in the demo version, however...
//
/*
void
F_DrawPatchCol
( int		x,
  patch_t*	patch,
  int		col )
{
    column_t*	column;
    byte*	source;
    byte*	dest;
    byte*	desttop;
    int		count;
	
    column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
    desttop = screens[0]+x;

    // step through the posts in a column
    while (column->topdelta != 0xff )
    {
	source = (byte *)column + 3;
	dest = desttop + column->topdelta*SCREENWIDTH;
	count = column->length;
		
	while (count--)
	{
	    *dest = *source++;
	    dest += SCREENWIDTH;
	}
	column = (column_t *)(  (byte *)column + column->length + 4 );
    }
}
*/

//
// F_DrawMap34End
//
// [STRIFE] Modified from F_BunnyScroll
// * In 1.2 and up this just causes a weird black screen.
// * In the demo version, it was an actual scroll between two screens.
// I have implemented both code segments, though only the black screen
// one will currently be used, as full demo version support isn't looking
// likely right now.
//
void F_DrawMap34End(void)
{
    int         scrolled;
    int         x;
    patch_t*    p1;
    patch_t*    p2;
		
    p1 = W_CacheLumpName ("credit", PU_LEVEL);
    p2 = W_CacheLumpName ("vellogo", PU_LEVEL);

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
	
    scrolled = 320 - (finalecount-230)/2;
    if (scrolled > 320)
        scrolled = 320;
    if (scrolled < 0)
        scrolled = 0;
    // wtf this is supposed to do, I have no idea!
    x = 1;
    do
    {
        x += 11;
    } while (x < 320);
}


//
// F_Drawer
//
// [STRIFE]
// haleyjd 09/13/10: Modified for slideshow, demo version, etc.
//
void F_Drawer (void)
{
    if (finalestage == 2)
    {
        //F_CastDrawer();
        return;
    }

    if (!finalestage)
    {
        // Draw slideshow panel
        patch_t *slide = W_CacheLumpName(slideshow_panel, PU_CACHE);
        V_DrawPatch(0, 0, 0, slide);
    }
    else
    {
        patch_t *credits;
        switch (gamemap)
        {
        case 29:
            // draw credits
            credits = W_CacheLumpName("CREDIT", PU_CACHE);
            V_DrawPatch(0, 0, 0, credits);
            break;
        case 34:
            // demo version - does nothing meaningful in the final version
            F_DrawMap34End();
        }
    }
}
