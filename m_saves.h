//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2010 James Haley, Samuel Villareal
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
//
// [STRIFE] New Module
//
// Strife Hub Saving Code
//

#ifndef M_SAVES_H__
#define M_SAVES_H__

#define SAVEDIR_LEN 128
#define CHARACTER_NAME_LEN 32

extern char savepath[SAVEDIR_LEN];
extern char savepathtemp[SAVEDIR_LEN];
extern char loadpath[SAVEDIR_LEN];
extern char character_name[CHARACTER_NAME_LEN];

// Strife Savegame Functions
void ClearTmp(void);
void ClearSlot(void);
void FromCurr(int slot);
void ToCurr(int slot);
void M_SaveMoveMapToHere(void);
void M_SaveMoveHereToMap(void);

boolean M_SaveMisObj(char *path);
void    M_ReadMisObj(void);

#endif
