//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 2010 James Haley, Samuel Villarreal
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>

#include "z_zone.h"
#include "i_system.h"
#include "d_player.h"
#include "doomstat.h"
#include "m_misc.h"
#include "m_saves.h"
#include "p_dialog.h"

//
// File Paths
//
// Strife maintains multiple file paths related to savegames.
//
char savepath[SAVEDIR_LEN];     // The actual path of the selected saveslot
char savepathtemp[SAVEDIR_LEN]; // The path of the temporary saveslot (strfsav6.ssg)
char loadpath[SAVEDIR_LEN];     // Path used while loading the game

char character_name[CHARACTER_NAME_LEN]; // Name of "character" for saveslot

//
// ClearTmp
//
// Clear the temporary save directory
//
void ClearTmp(void)
{
    char temp[128];
    DIR *sp2dir = NULL;
    struct dirent *f = NULL;

    if(!savepathtemp[0])
        I_Error("you fucked up savedir man!");

    strcpy(temp, savepathtemp);
    temp[strlen(savepathtemp) - 1] = 0;

    if(!(sp2dir = opendir(temp)))
        I_Error("ClearTmp: Couldn't open dir %s", temp);

    // Skip '.' and '..'
    readdir(sp2dir);
    readdir(sp2dir);

    while((f = readdir(sp2dir)))
    {
        sprintf(temp, "%s%s", savepathtemp, f->d_name);
        remove(temp);
    }

    closedir(sp2dir);
}

//
// ClearSlot
//
// Clear a single save slot folder
//
void ClearSlot(void)
{
    char temp[128];
    DIR *spdir = NULL;
    struct dirent *f = NULL;

    if(!savepath[0])
        I_Error("userdir is fucked up man!");

    strcpy(temp, savepath);
    temp[strlen(savepath) - 1] = 0;

    if(!(spdir = opendir(temp)))
        I_Error("ClearSlot: Couldn't open dir %s", temp);

    // Skip '.' and '..'
    readdir(spdir);
    readdir(spdir);

    while((f = readdir(spdir)))
    {
        sprintf(temp, "%s%s", savepath, f->d_name);
        remove(temp);
    }

    closedir(spdir);
}

//
// FromCurr
//
// Copying files from savepathtemp to savepath
//
void FromCurr(int slot)
{
    char temp1[128];
    char temp2[32];
    byte *filebuffer;
    int   filelen;
    DIR *sp2dir = NULL;
    struct dirent *f = NULL;

    strcpy(temp2, savepathtemp);
    temp2[strlen(savepathtemp) - 1] = 0;

    if(!(sp2dir = opendir(temp2)))
        I_Error("FromCurr: Couldn't open dir %s", temp2);

    // Skip '.' and '..'
    readdir(sp2dir);
    readdir(sp2dir);

    while((f = readdir(sp2dir)))
    {
        sprintf(temp1, "%s%s", savepathtemp, f->d_name);

        filelen = M_ReadFile(temp1, &filebuffer);
        
        sprintf(temp1, "%s%s", savepath, f->d_name);

        M_WriteFile(temp1, filebuffer, filelen);

        Z_Free(filebuffer);
    }

    closedir(sp2dir);
}

//
// ToCurr
//
// Copying files from savepath to savepathtemp
//
void ToCurr(int unused)
{
    char temp1[128];
    char temp2[32];
    byte *filebuffer;
    int   filelen;
    DIR *sp2dir = NULL;
    struct dirent *f = NULL;

    ClearTmp();

    strcpy(temp2, savepathtemp);
    temp2[strlen(savepathtemp) - 1] = 0;

    // BUG: Rogue copypasta'd this error message, which is why we don't know
    // the real original name of this function.
    if(!(sp2dir = opendir(temp2)))
        I_Error("ClearSlot: Couldn't open dir %s", temp2);

    // Skip '.' and '..'
    readdir(sp2dir);
    readdir(sp2dir);

    while((f = readdir(sp2dir)))
    {
        sprintf(temp1, "%s%s", savepath, f->d_name);

        filelen = M_ReadFile(temp1, &filebuffer);
        
        sprintf(temp1, "%s%s", savepathtemp, f->d_name);

        M_WriteFile(temp1, filebuffer, filelen);

        Z_Free(filebuffer);
    }

    closedir(sp2dir);
}

//
// M_SaveMoveMapToHere
//
// Moves a map to the "HERE" save.
//
void M_SaveMoveMapToHere(void)
{
    char heresave[128];
    char mapsave[128];

    sprintf(mapsave, "%s%i", savepath, gamemap);
    sprintf(heresave, "%s%s", savepath, "here");

    if (!access(mapsave, R_OK))
    {
        remove(heresave);
        rename(mapsave, heresave);
    }
}

//
// M_SaveMoveHereToMap
//
// Moves the "HERE" save to a map.
//
void M_SaveMoveHereToMap(void)
{
    char mapsave[128];
    char heresave[128];

    sprintf(mapsave, "%s%i", savepathtemp, gamemap);
    sprintf(heresave, "%s%s", savepathtemp, "here");

    if (!access(heresave, R_OK))
    {
        remove(mapsave);
        rename(heresave, mapsave);
    }
}

//
// M_SaveMisObj
//
// Writes the mission objective into the MIS_OBJ file.
//
boolean M_SaveMisObj(char *path)
{
    boolean result;
    char destpath[100];

    sprintf(destpath, "%smis_obj", path);
    result = M_WriteFile(destpath, mission_objective, OBJECTIVE_LEN);

    return result;
}

//
// M_ReadMisObj
//
// Reads the mission objective from the MIS_OBJ file.
//
void M_ReadMisObj(void)
{
    int handle;
    char srcpath[100];

    sprintf(srcpath, "%smis_obj", savepathtemp);

    handle = open(srcpath, O_RDONLY | 0, 0666);

    if(handle != -1)
    {
        read(handle, mission_objective, OBJECTIVE_LEN);
        close(handle);
    }
}
