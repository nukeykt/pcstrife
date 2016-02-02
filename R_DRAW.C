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
// DESCRIPTION:
//      The actual span/column drawing functions.
//      Here find the main potential for optimization,
//       e.g. inline assembly, different algorithms.
//

#include <conio.h>
#include "doomdef.h"

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

// Needs access to LFB (guess what).
#include "v_video.h"

// State.
#include "doomstat.h"


// ?
#define MAXWIDTH                        1120
#define MAXHEIGHT                       832

// status bar height at bottom of screen
// haleyjd 08/31/10: Verified unmodified.
#define SBARHEIGHT              32

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//


byte*           viewimage; 
int             viewwidth;
int             scaledviewwidth;
int             viewheight;
int             viewwindowx;
int             viewwindowy; 
byte*           ylookup[MAXHEIGHT]; 
int             columnofs[MAXWIDTH]; 

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
// [STRIFE] Unused.
//byte            translations[3][256];   
 


// haleyjd 08/29/10: [STRIFE] Rogue added the ability to customize the view
// border flat by storing it in the configuration file.
char *back_flat;


//
// R_DrawColumn
// Source is the top of the column to scale.
//
lighttable_t*           dc_colormap; 
int                     dc_x; 
int                     dc_yl; 
int                     dc_yh; 
fixed_t                 dc_iscale; 
fixed_t                 dc_texturemid;

// first pixel in a column (possibly virtual) 
byte*                   dc_source;              

// just for profiling 
int                     dccount;

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//
#if 1
void R_DrawColumn (void)
{
    int                 count; 
    byte*               dest; 
    fixed_t             frac;
    fixed_t             fracstep;        
 
    count = dc_yh - dc_yl; 

    // Zero length, column does not exceed a pixel.
    if (count < 0) 
        return; 
                                 
#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT) 
        I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x); 
#endif 

        outp (SC_INDEX+1,1<<(dc_x&3)); 

        dest = destview + dc_yl*80 + (dc_x>>2); 

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale; 
    frac = dc_texturemid + (dc_yl-centery)*fracstep; 

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.
    do 
    {
        // Re-map color indices from wall texture column
        //  using a lighting/special effects LUT.
        *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
        
        dest += SCREENWIDTH/4; 
        frac += fracstep;
        
    } while (count--); 

}

#endif

void R_UnkColumn(void)
{
    int                 count;
    byte*               dest;
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
        I_Error("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif 

    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + dc_yl * 80 + (dc_x >> 2);

    fracstep = pspriteiscale;
    frac = dc_texturemid + (dc_yl - centery)*fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) % SCREENHEIGHT]];

        dest += SCREENWIDTH / 4;
        frac += fracstep;

    } while (count--);

}

//
// Spectre/Invisibility.
//


// haleyjd 09/06/10: ]STRIFE] replaced fuzzdraw with translucency.

byte *xlatab;

//
// R_DrawTLColumn
//
// villsa [STRIFE] new function
// Achieves a second translucency level using the same lookup table,
// via inversion of the colors in the index computation.
//
void R_DrawTLColumn(void)
{
    int                 count;
    byte*               dest;
    fixed_t             frac;
    fixed_t             fracstep;

    // Adjust borders. Low... 
    if (!dc_yl)
        dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
        I_Error("R_DrawFuzzColumn2: %i to %i at %i",
            dc_yl, dc_yh, dc_x);
    }
#endif

    outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + dc_yl * 80 + (dc_x >> 2);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery)*fracstep;

    do
    {
        byte src = dc_colormap[dc_source[(frac >> FRACBITS) & 127]];
        byte col = xlatab[(*dest << 8) + src];
        *dest = col;
        dest += SCREENWIDTH/4;
        frac += fracstep;
    } while (count--);
}


//
// R_DrawMVisTLColumn
//
// villsa [STRIFE] new function
// Replacement for R_DrawFuzzColumn
//
void R_DrawMVisTLColumn(void)
{
    int                 count;
    byte*               dest;
    fixed_t             frac;
    fixed_t             fracstep;

    // Adjust borders. Low... 
    if (!dc_yl)
        dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
        I_Error("R_DrawFuzzColumn: %i to %i at %i",
            dc_yl, dc_yh, dc_x);
    }
#endif


    outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + dc_yl * 80 + (dc_x >> 2);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery)*fracstep;

    do
    {
        byte src = dc_colormap[dc_source[(frac >> FRACBITS) & 127]];
        byte col = xlatab[*dest + (src << 8)];
        *dest = col;
        dest += SCREENWIDTH/4;
        frac += fracstep;
    } while (count--);
}


//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
byte*   dc_translation;
byte*   translationtables;

void R_DrawTranslatedColumn (void) 
{ 
    int                 count; 
    byte*               dest; 
    fixed_t             frac;
    fixed_t             fracstep;        
 
    count = dc_yh - dc_yl; 
    if (count < 0) 
        return; 
                                 
#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
    {
        I_Error ( "R_DrawColumn: %i to %i at %i",
                  dc_yl, dc_yh, dc_x);
    }
    
#endif 

    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + dc_yl * 80 + (dc_x >> 2);

    // Looks familiar.
    fracstep = dc_iscale; 
    frac = dc_texturemid + (dc_yl-centery)*fracstep; 

    // Here we do an additional index re-mapping.
    do 
    {
        // Translation tables are used
        //  to map certain colorramps to other ones,
        //  used with PLAY sprites.
        // Thus the "green" ramp of the player 0 sprite
        //  is mapped to gray, red, black/indigo. 
        *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
        dest += SCREENWIDTH/4;
        
        frac += fracstep; 
    } while (count--); 
} 



//
// R_DrawTRTLColumn
//
// villsa [STRIFE] new function
// Combines translucency and color translation.
//
void R_DrawTRTLColumn(void)
{
    int                 count;
    byte*               dest;
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
    {
        I_Error("R_DrawColumn: %i to %i at %i",
            dc_yl, dc_yh, dc_x);
    }
#endif 

    outpw(GC_INDEX, GC_READMAP + ((dc_x & 3) << 8));
    outp(SC_INDEX + 1, 1 << (dc_x & 3));

    dest = destview + dc_yl * 80 + (dc_x >> 2);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl - centery)*fracstep;

    // Here we do an additional index re-mapping.
    do
    {
        byte src = dc_colormap[dc_translation[dc_source[frac >> FRACBITS & 127]]];
        byte col = xlatab[(*dest << 8) + src];
        *dest = col;
        dest += SCREENWIDTH/4;
        frac += fracstep;
    } while (count--);
}


//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
// haleyjd 08/26/10: [STRIFE]
// * Added loading of XLATAB
//
void R_InitTranslationTables (void)
{
    int         i;
    byte*       lump;

    // villsa [STRIFE] allocate a larger size for translation tables   
    translationtables = Z_Malloc (256*7+255, PU_STATIC, 0);
    translationtables = (byte *)(( (int)translationtables + 255 )& ~255);

    lump = W_CacheLumpName("XLATAB", PU_CACHE);

    xlatab = (byte *)(((int)Z_Malloc(0x20000, PU_STATIC, 0) + 0xffff) & ~0xffff);

    memcpy(xlatab, lump, 0x10000);

    // villsa [STRIFE] setup all translation tables
    for (i=0 ; i<256 ; i++)
    {
        if (i >= 0x20 && i < 0x40)
        {
            translationtables[i     ] = i - 0x20;
            translationtables[i+ 256] = i - 0x20;
            translationtables[i+ 512] = 0xd0 + (i & 0xf);
            translationtables[i+ 768] = 0xd0 + (i & 0xf);
            translationtables[i+1024] = i - 0x20;
            translationtables[i+1280] = i - 0x20;
            translationtables[i+1536] = i - 0x20;
        }
        else if (i >= 0x50 && i <= 0x5f)
        {
            translationtables[i     ] = i;
            translationtables[i+ 256] = i;
            translationtables[i+ 512] = i;
            translationtables[i+ 768] = i;
            translationtables[i+1024] = 0x80 + (i & 0xf);
            translationtables[i+1280] = 0x10 + (i & 0xf);
            translationtables[i+1536] = 0x40 + (i & 0xf);
        }
        else if (i >= 0xf1 && i <= 0xf6)
        {
            translationtables[i     ] = 0xdf + (i & 0xf);
            translationtables[i+ 256] = i;
            translationtables[i+ 512] = i;
            translationtables[i+ 768] = i;
            translationtables[i+1024] = i;
            translationtables[i+1280] = i;
            translationtables[i+1536] = i;
        }
        else if (i >= 0xf7 && i <= 0xfb)
        {
            translationtables[i     ] = i - 0x6;
            translationtables[i+ 256] = i;
            translationtables[i+ 512] = i;
            translationtables[i+ 768] = i;
            translationtables[i+1024] = i;
            translationtables[i+1280] = i;
            translationtables[i+1536] = i;
        }
        else if (i >= 0x80 && i <= 0x8f)
        {
            translationtables[i     ] = 0x40 + (i & 0xf);
            translationtables[i+ 256] = 0xb0 + (i & 0xf);
            translationtables[i+ 512] = 0x10 + (i & 0xf);
            translationtables[i+ 768] = 0x30 + (i & 0xf);
            translationtables[i+1024] = 0x50 + (i & 0xf);
            translationtables[i+1280] = 0x60 + (i & 0xf);
            translationtables[i+1536] = 0x90 + (i & 0xf);
        }
        else if (i >= 0xc0 && i <= 0xcf)
        {
            translationtables[i     ] = i;
            translationtables[i+ 256] = i;
            translationtables[i+ 512] = i;
            translationtables[i+ 768] = i;
            translationtables[i+1024] = 0xa0 + (i & 0xf);
            translationtables[i+1280] = 0x20 + (i & 0xf);
            translationtables[i+1536] = i & 0xf;
            // haleyjd 20110213: missing code:
            if (!(i & 0x0f))
                translationtables[i+1536] = 0x1;
        }
        else if (i >= 0xd0 && i <= 0xdf)
        {
            translationtables[i     ] = i;
            translationtables[i+ 256] = i;
            translationtables[i+ 512] = i;
            translationtables[i+ 768] = i;
            translationtables[i+1024] = 0xb0 + (i & 0xf);
            translationtables[i+1280] = 0x30 + (i & 0xf);
            translationtables[i+1536] = 0x10 + (i & 0xf);
        }
        else
        {
            // Keep all other colors as is.
            translationtables[i     ] =
            translationtables[i+ 256] =
            translationtables[i+ 512] =
            translationtables[i+ 768] =
            translationtables[i+1024] =
            translationtables[i+1280] =
            translationtables[i+1536] = i;
        }
    }
}




//
// R_DrawSpan 
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int                     ds_y; 
int                     ds_x1; 
int                     ds_x2;

lighttable_t*           ds_colormap; 

fixed_t                 ds_xfrac; 
fixed_t                 ds_yfrac; 
fixed_t                 ds_xstep; 
fixed_t                 ds_ystep;

// start of a 64*64 tile image 
byte*                   ds_source;      

// just for profiling
int                     dscount;



//
// Draws the actual span
// Slower than vanilla
void R_DrawSpan (void) 
{ 
    fixed_t             xfrac;
    fixed_t             yfrac;
    byte*               dest;
    int                 spot;
    int                     i;
    int                     prt;
    int                     dsp_x1;
    int                     dsp_x2;
    int                     countp;

#ifdef RANGECHECK 
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2 >= SCREENWIDTH
        || (unsigned)ds_y>SCREENHEIGHT)
    {
        I_Error("R_DrawSpan: %i to %i at %i",
            ds_x1, ds_x2, ds_y);
    }
#endif 

    for (i = 0; i < 4; i++)
    {
        outp(SC_INDEX + 1, 1 << i);
        dsp_x1 = (ds_x1 - i) / 4;
        if (dsp_x1 * 4 + i<ds_x1)
            dsp_x1++;
        dest = destview + ds_y * 80 + dsp_x1;
        dsp_x2 = (ds_x2 - i) / 4;
        countp = dsp_x2 - dsp_x1;

        xfrac = ds_xfrac;
        yfrac = ds_yfrac;

        prt = dsp_x1 * 4 - ds_x1 + i;

        xfrac += ds_xstep*prt;
        yfrac += ds_ystep*prt;
        if (countp < 0) {
            continue;
        }
        do
        {
            // Current texture index in u,v.
            spot = ((yfrac >> (16 - 6))&(63 * 64)) + ((xfrac >> 16) & 63);

            // Lookup pixel from flat texture tile,
            //  re-index using light/colormap.
            *dest++ = ds_colormap[ds_source[spot]];
            // Next step in u,v.
            xfrac += ds_xstep * 4;
            yfrac += ds_ystep * 4;
        } while (countp--);
    }
}


//
// R_InitBuffer 
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void
R_InitBuffer
( int           width,
  int           height ) 
{ 
    int         i; 

    // Handle resize,
    //  e.g. smaller view windows
    //  with border and/or status bar.
    viewwindowx = (SCREENWIDTH-width) >> 1; 

    // Column offset. For windows.
    for (i=0 ; i<width ; i++) 
        columnofs[i] = viewwindowx + i;

    // Samw with base row offset.
    if (width == SCREENWIDTH) 
        viewwindowy = 0; 
    else 
        viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1; 

    // Preclaculate all row offsets.
    for (i=0 ; i<height ; i++) 
        ylookup[i] = screens[0] + (i+viewwindowy)*SCREENWIDTH; 
} 
 
 


//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen (void) 
{ 
    byte*       src;
    byte*       dest;
    int         x;
    int         y;
    int         i;
    int         j;
    patch_t*    patch;

    if (scaledviewwidth == 320)
        return;

    // haleyjd 08/29/10: [STRIFE] Use configurable back_flat
    src = W_CacheLumpName(back_flat, PU_CACHE);
    dest = screens[1];

    for (y = 0; y<SCREENHEIGHT - SBARHEIGHT; y++)
    {
        for (x = 0; x<SCREENWIDTH / 64; x++)
        {
            memcpy(dest, src + ((y & 63) << 6), 64);
            dest += 64;
        }

        if (SCREENWIDTH & 63)
        {
            memcpy(dest, src + ((y & 63) << 6), SCREENWIDTH & 63);
            dest += (SCREENWIDTH & 63);
        }
    }

    patch = W_CacheLumpName("brdr_t", PU_CACHE);

    for (x = 0; x<scaledviewwidth; x += 8)
        V_DrawPatch(viewwindowx + x, viewwindowy - 8, 1, patch);
    patch = W_CacheLumpName("brdr_b", PU_CACHE);

    for (x = 0; x<scaledviewwidth; x += 8)
        V_DrawPatch(viewwindowx + x, viewwindowy + viewheight, 1, patch);
    patch = W_CacheLumpName("brdr_l", PU_CACHE);

    for (y = 0; y<viewheight; y += 8)
        V_DrawPatch(viewwindowx - 8, viewwindowy + y, 1, patch);
    patch = W_CacheLumpName("brdr_r", PU_CACHE);

    for (y = 0; y<viewheight; y += 8)
        V_DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, 1, patch);


    // Draw beveled edge. 
    V_DrawPatch(viewwindowx - 8,
        viewwindowy - 8,
        1,
        W_CacheLumpName("brdr_tl", PU_CACHE));

    V_DrawPatch(viewwindowx + scaledviewwidth,
        viewwindowy - 8,
        1,
        W_CacheLumpName("brdr_tr", PU_CACHE));

    V_DrawPatch(viewwindowx - 8,
        viewwindowy + viewheight,
        1,
        W_CacheLumpName("brdr_bl", PU_CACHE));

    V_DrawPatch(viewwindowx + scaledviewwidth,
        viewwindowy + viewheight,
        1,
        W_CacheLumpName("brdr_br", PU_CACHE));

    //Copy border to background buffer

    for (i = 0; i < 4; i++)
    {
        outp(SC_INDEX, SC_MAPMASK);
        outp(SC_INDEX + 1, 1 << i);
        for (j = 0; j < SCREENWIDTH*SCREENHEIGHT / 4; j++) {
            *(byte*)(0xac000 + j) = screens[1][j * 4 + i];
        }
    }
}
 

//
// Copy a screen buffer.
//
void
R_VideoErase
( unsigned      ofs,
  int           count ) 
{ 
    byte* dest;
    byte* source;
    int countp;

    outp(SC_INDEX, SC_MAPMASK);
    outp(SC_INDEX + 1, 0x0f);
    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1) | 1);

    dest = destscreen + (ofs >> 2);
    source = (byte*)0xac000 + (ofs >> 2);
    countp = count >> 2;
    countp--;
    if (countp < 0)
        return;
    dest += countp;
    source += countp;
    do
    {
        *dest-- = *source--;
    } while (--countp >= 0);

    outp(GC_INDEX, GC_MODE);
    outp(GC_INDEX + 1, inp(GC_INDEX + 1)&~1);
} 


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
 
void R_DrawViewBorder (void) 
{ 
    int         top;
    int         side;
    int         ofs;
    int         i;

    if (scaledviewwidth == SCREENWIDTH)
        return;

    top = ((SCREENHEIGHT - SBARHEIGHT) - viewheight) / 2;
    side = (SCREENWIDTH - scaledviewwidth) / 2;

    // copy top and one line of left side 
    R_VideoErase(0, top*SCREENWIDTH + side);

    // copy one line of right side and bottom 
    ofs = (viewheight + top)*SCREENWIDTH - side;
    R_VideoErase(ofs, top*SCREENWIDTH + side);

    // copy sides using wraparound 
    ofs = top*SCREENWIDTH + SCREENWIDTH - side;
    side <<= 1;

    for (i = 1; i<viewheight; i++)
    {
        R_VideoErase(ofs, side);
        ofs += SCREENWIDTH;
    }
} 
