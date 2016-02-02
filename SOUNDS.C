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
//	Created by a sound utility.
//	Kept as a sample, DOOM2 sounds.
//


#include "doomtype.h"
#include "sounds.h"

//
// Information about all the music
//

// villsa [STRIFE]
musicinfo_t S_music[] =
{
    { 0 },
    { "logo", 0 },
    { "action", 0 },
    { "tavern", 0 },
    { "danger", 0 },
    { "fast", 0 },
    { "intro", 0 },
    { "darker", 0 },
    { "strike", 0 },
    { "slide", 0 },
    { "tribal", 0 },
    { "march", 0 },
    { "danger", 0 },
    { "mood", 0 },
    { "castle", 0 },
    { "darker", 0 },
    { "action", 0 },
    { "fight", 0 },
    { "spense", 0 },
    { "slide", 0 },
    { "strike", 0 },
    { "dark", 0 },
    { "tech", 0 },
    { "slide", 0 },
    { "drone", 0 },
    { "panthr", 0 },
    { "sad", 0 },
    { "instry", 0 },
    { "tech", 0 },
    { "action", 0 },
    { "instry", 0 },
    { "drone", 0 },
    { "fight", 0 },
    { "happy", 0 },
    { "end", 0 }
};


//
// Information about all the sfx
//

// villsa [STRIFE]
sfxinfo_t S_sfx[] =
{
  // S_sfx[0] needs to be a dummy for odd reasons.
  { 0, false,  0, 0 },

  { "swish", false, 64, 0 },
  { "meatht", false, 64, 0 },
  { "mtalht", false, 64, 0 },
  { "wpnup", true, 78, 0 },
  { "rifle", false, 64, 0 },
  { "mislht", false, 64, 0 },
  { "barexp", false, 32, 0 },
  { "flburn", true, 64, 0 },
  { "flidl", false, 118, 0 },
  { "agrsee", false, 98, 0 },
  { "plpain", false, 96, 0 },
  { "pcrush", false, 96, 0 },
  { "pespna", false, 98, 0 },
  { "pespnb", false, 98, 0 },
  { "pespnc", false, 98, 0 },
  { "pespnd", false, 98, 0 },
  { "agrdpn", false, 98, 0 },
  { "pldeth", false, 32, 0 },
  { "plxdth", false, 32, 0 },
  { "slop", false, 78, 0 },
  { "rebdth", false, 98, 0 },
  { "agrdth", false, 98, 0 },
  { "lgfire", true, 211, 0 },
  { "smfire", true, 211, 0 },
  { "alarm", true, 210, 0 },
  { "drlmto", false, 98, 0 },
  { "drlmtc", false, 98, 0 },
  { "drsmto", false, 98, 0 },
  { "drsmtc", false, 98, 0 },
  { "drlwud", false, 98, 0 },
  { "drswud", false, 98, 0 },
  { "drston", false, 98, 0 },
  { "bdopn", false, 98, 0 },
  { "bdcls", false, 98, 0 },
  { "swtchn", false, 78, 0 },
  { "swbolt", false, 98, 0 },
  { "swscan", false, 98, 0 },
  { "yeah", false, 10, 0 },
  { "mask", true, 210, 0 },
  { "pstart", false, 100, 0 },
  { "pstop", false, 100, 0 },
  { "itemup", true, 78, 0 },
  { "bglass", false, 200, 0 },
  { "wriver", true, 201, 0 },
  { "wfall", true, 201, 0 },
  { "wdrip", true, 201, 0 },
  { "wsplsh", true, 95, 0 },
  { "rebact", true, 200, 0 },
  { "agrac1", true, 98, 0 },
  { "agrac2", true, 98, 0 },
  { "agrac3", true, 98, 0 },
  { "agrac4", true, 98, 0 },
  { "ambppl", true, 218, 0 },
  { "ambbar", true, 218, 0 },
  { "telept", false, 32, 0 },
  { "ratact", true, 99, 0 },
  { "itmbk", false, 100, 0 },
  { "xbow", false, 99, 0 },
  { "burnme", false, 95, 0 },
  { "oof", false, 96, 0 },
  { "wbrldt", false, 98, 0 },
  { "psdtha", false, 109, 0 },
  { "psdthb", false, 109, 0 },
  { "psdthc", false, 109, 0 },
  { "rb2pn", false, 96, 0 },
  { "rb2dth", false, 32, 0 },
  { "rb2see", false, 98, 0 },
  { "rb2act", false, 98, 0 },
  { "firxpl", false, 70, 0 },
  { "stnmov", true, 100, 0 },
  { "noway", false, 78, 0 },
  { "rlaunc", false, 64, 0 },
  { "rflite", false, 65, 0 },
  { "radio", false, 60, 0 },
  { "pulchn", false, 98, 0 },
  { "swknob", false, 98, 0 },
  { "keycrd", false, 98, 0 },
  { "swston", false, 98, 0 },
  { "sntsee", false, 98, 0 },
  { "sntdth", true, 98, 0 },
  { "sntact", true, 98, 0 },
  { "pgrdat", true, 64, 0 },
  { "pgrsee", true, 90, 0 },
  { "pgrdpn", false, 96, 0 },
  { "pgrdth", false, 32, 0 },
  { "pgract", true, 120, 0 },
  { "proton", false, 64, 0 },
  { "protfl", false, 64, 0 },
  { "plasma", false, 64, 0 },
  { "dsrptr", false, 30, 0 },
  { "reavat", true, 64, 0 },
  { "revbld", true, 64, 0 },
  { "revsee", true, 90, 0 },
  { "reavpn", false, 96, 0 },
  { "revdth", false, 32, 0 },
  { "revact", true, 120, 0 },
  { "spisit", false, 90, 0 },
  { "spdwlk", false, 65, 0 },
  { "spidth", false, 32, 0 },
  { "spdatk", false, 32, 0 },
  { "chant", false, 218, 0 },
  { "static", false, 32, 0 },
  { "chain", false, 70, 0 },
  { "tend", true, 100, 0 },
  { "phoot", false, 32, 0 },
  { "explod", false, 32, 0 },
  { "sigil", false, 32, 0 },
  { "sglhit", false, 32, 0 },
  { "siglup", false, 32, 0 },
  { "prgpn", false, 96, 0 },
  { "progac", false, 120, 0 },
  { "lorpn", false, 96, 0 },
  { "lorsee", true, 90, 0 },
  { "difool", false, 32, 0 },
  { "inqdth", false, 32, 0 },
  { "inqact", true, 98, 0 },
  { "inqsee", true, 90, 0 },
  { "inqjmp", false, 65, 0 },
  { "amaln1", true, 99, 0 },
  { "amaln2", true, 99, 0 },
  { "amaln3", true, 99, 0 },
  { "amaln4", true, 99, 0 },
  { "amaln5", true, 99, 0 },
  { "amaln6", true, 99, 0 },
  { "mnalse", true, 64, 0 },
  { "alnsee", true, 64, 0 },
  { "alnpn", false, 96, 0 },
  { "alnact", false, 120, 0 },
  { "alndth", false, 32, 0 },
  { "mnaldt", false, 32, 0 },
  { "reactr", false, 31, 0 },
  { "airlck", false, 98, 0 },
  { "drchno", false, 98, 0 },
  { "drchnc", false, 98, 0 },
  { "valve", false, 98, 0 }
};

