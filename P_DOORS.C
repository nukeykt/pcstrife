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
// DESCRIPTION: Door animation code (opening/closing)
//

#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"


// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "r_data.h"
#include "p_dialog.h"
#include "i_system.h"



//
// VERTICAL DOORS
//

//
// T_VerticalDoor
//
void T_VerticalDoor (vldoor_t* door)
{
    result_e	res1;
    result_e	res2;
	
    switch(door->direction)
    {
      case 0:
	// WAITING
	if (!--door->topcountdown)
	{
	    switch(door->type)
	    {
	      case vld_blazeRaise:
		door->direction = -1; // time to go back down
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     sfx_bdcls);
		break;
		
	      case vld_normal:
		door->direction = -1; // time to go back down
        // villsa [STRIFE] closesound added
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     door->closesound);
		break;

          // villsa [STRIFE]
          case vld_shopClose:
        door->direction = 1;
        door->speed = VDOORSPEED;
        S_StartSound((mobj_t *)&door->sector->soundorg,
                 door->opensound);
        break;
		
	      case vld_close30ThenOpen:
		door->direction = 1;

        // villsa [STRIFE] opensound added
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     door->opensound);
		break;
		
	      default:
		break;
	    }
	}
	break;
	
      case 2:
	//  INITIAL WAIT
	if (!--door->topcountdown)
	{
	    switch(door->type)
	    {
	      case vld_raiseIn5Mins:
		door->direction = 1;
		door->type = vld_normal;
        // villsa [STRIFE] opensound added
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     door->opensound);
		break;
		
	      default:
		break;
	    }
	}
	break;

      // villsa [STRIFE]
      case -2:
    // SPLIT
    res1 = T_MovePlane(door->sector, door->speed, door->topheight, false, 1, 1);
    res2 = T_MovePlane(door->sector, door->speed, door->topwait, false, 0, -1);

    if(res1 == pastdest && res2 == pastdest)
    {
        door->sector->specialdata = NULL;
        P_RemoveThinker(&door->thinker);  // unlink and free
    }

    break;
	
      case -1:
	// DOWN
	res1 = T_MovePlane(door->sector,
			  door->speed,
			  door->sector->floorheight,
			  false,1,door->direction);
	if (res1 == pastdest)
	{
	    switch(door->type)
	    {
	      case vld_normal:
	      case vld_close:
	      case vld_blazeRaise:
	      case vld_blazeClose:
		door->sector->specialdata = NULL;
		P_RemoveThinker (&door->thinker);  // unlink and free
        // villsa [STRIFE] no sounds
		break;
		
	      case vld_close30ThenOpen:
		door->direction = 0;
		door->topcountdown = 35*30;
		break;

          // villsa [STRIFE]
          case vld_shopClose:
        door->direction = 0;
        door->topcountdown = 35*120;
        break;
		
	      default:
		break;
	    }
	}
	else if (res1 == crushed)
	{
	    switch(door->type)
	    {
	      case vld_blazeClose:
	      case vld_close:		// DO NOT GO BACK UP!
          case vld_shopClose:     // villsa [STRIFE]
		break;
		
	      default:
		door->direction = 1;
        // villsa [STRIFE] opensound added
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     door->opensound);
		break;
	    }
	}
	break;
	
      case 1:
	// UP
	res1 = T_MovePlane(door->sector,
			  door->speed,
			  door->topheight,
			  false,1,door->direction);
	
	if (res1 == pastdest)
	{
	    switch(door->type)
	    {
	      case vld_blazeRaise:
	      case vld_normal:
		door->direction = 0; // wait at top
		door->topcountdown = door->topwait;
		break;
		
	      case vld_close30ThenOpen:
	      case vld_blazeOpen:
	      case vld_open:
          case vld_shopClose:     // villsa [STRIFE]
		door->sector->specialdata = NULL;
		P_RemoveThinker (&door->thinker);  // unlink and free
		break;
		
	      default:
		break;
	    }
	}
	break;
    }
}


//
// EV_DoLockedDoor
// Move a locked door up/down
//
// [STRIFE] This game has a crap load of keys. And this function doesn't even
// deal with all of them...
//
int
EV_DoLockedDoor
( line_t*	line,
  vldoor_e	type,
  mobj_t*	thing )
{
    player_t*	p;
	
    p = thing->player;
	
    if (!p)
        return 0;

    switch (line->special)
    {
    case 99:
    case 133:
        if (!p->cards[key_IDCard])
        {
            p->message = "You need an id card";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 134:
    case 135:
        if (!p->cards[key_IDBadge])
        {
            p->message = "You need an id badge";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 136:
    case 137:
        if (!p->cards[key_Passcard])
        {
            p->message = "You need a pass card";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 151:
    case 164:
        if (!p->cards[key_GoldKey])
        {
            p->message = "You need a gold key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 153:
    case 163:
        if (!p->cards[key_SilverKey])
        {
            p->message = "You need a silver key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 152:
    case 162:
        if (!p->cards[key_BrassKey])
        {
            p->message = "You need a brass key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 167:
    case 168:
        if (!p->cards[key_SeveredHand])
        {
            p->message = "Hand print not on file";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 171:
        if (!p->cards[key_PrisonKey])
        {
            p->message = "You don't have the key to the prison";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 172:
        if (!p->cards[key_Power1Key])
        {
            p->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 173:
        if (!p->cards[key_Power2Key])
        {
            p->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 176:
        if (!p->cards[key_Power3Key])
        {
            p->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 189:
        if (!p->cards[key_OracleKey])
        {
            p->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 191:
        if (!p->cards[key_MilitaryID])
        {
            p->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 192:
        if (!p->cards[key_WarehouseKey])
        {
            p->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;

    case 223:
        if (!p->cards[key_MineKey])
        {
            p->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return 0;
        }
        break;
    }

    return EV_DoDoor(line,type);
}


int
EV_DoDoor
( line_t*	line,
  vldoor_e	type )
{
    int		secnum,rtn;
    sector_t*	sec;
    vldoor_t*	door;
	
    secnum = -1;
    rtn = 0;
    
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
	sec = &sectors[secnum];
	if (sec->specialdata)
	    continue;
		
	
	// new door thinker
	rtn = 1;
	door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
	P_AddThinker (&door->thinker);
	sec->specialdata = door;

	door->thinker.function = T_VerticalDoor;
	door->sector = sec;
	door->type = type;
	door->topwait = VDOORWAIT;
	door->speed = VDOORSPEED;
    R_SoundNumForDoor(door);    // villsa [STRIFE] set door sounds
		
	switch(type)
	{
	  case vld_blazeClose:
      case vld_shopClose:     // villsa [STRIFE]
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->direction = -1;
	    door->speed = VDOORSPEED * 4;
        // villsa [STRIFE] set door sounds
        if (sec->ceilingheight != sec->floorheight)
	    S_StartSound((mobj_t *)&door->sector->soundorg,
			 sfx_bdcls);
	    break;
	    
	  case vld_close:
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->direction = -1;
        // villsa [STRIFE] set door sounds
        if(sec->ceilingheight != sec->floorheight)
	    S_StartSound((mobj_t *)&door->sector->soundorg,
			 door->opensound);
	    break;
	    
	  case vld_close30ThenOpen:
	    door->topheight = sec->ceilingheight;
	    door->direction = -1;
	    S_StartSound((mobj_t *)&door->sector->soundorg,
			 door->closesound);
	    break;
	    
	  case vld_blazeRaise:
	  case vld_blazeOpen:
	    door->direction = 1;
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->speed = VDOORSPEED * 4;
	    if (door->topheight != sec->ceilingheight)
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     sfx_bdopn);
	    break;
	    
	  case vld_normal:
	  case vld_open:
	    door->direction = 1;
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    if (door->topheight != sec->ceilingheight)
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     door->opensound);
	    break;
	    
      case vld_splitRaiseNearest:
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->direction = -2;
	    door->speed = VDOORSPEED / 2;
        door->topwait = P_FindHighestFloorSurrounding(sec);
	    if (door->topheight != sec->ceilingheight)
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     door->opensound);
        break;

      case vld_splitOpen:
	    door->topheight = P_FindLowestCeilingSurrounding(sec);
	    door->topheight -= 4*FRACUNIT;
	    door->direction = -2;
	    door->speed = VDOORSPEED / 2;
        door->topwait = P_FindLowestFloorSurrounding(sec);
	    if (door->topheight != sec->ceilingheight)
		S_StartSound((mobj_t *)&door->sector->soundorg,
			     door->opensound);
        break;

	  default:
	    break;
	}
		
    }
    return rtn;
}

//
// EV_ClearForceFields
//
// villsa [STRIFE] new function
//
boolean EV_ClearForceFields(line_t* line)
{
    int         secnum;
    sector_t*   sec;
    int         i;
    line_t*     secline;
    boolean     ret = false;

    secnum = -1;

    while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];

        ret = true;
        sec->special = 0;

        // haleyjd 09/18/10: fixed to continue w/linecount == 0, not return
        for(i = 0; i < sec->linecount; i++)
        {
            secline = sec->lines[i];
            if(!(secline->flags & ML_TWOSIDED))
                continue;
            if(secline->special != 148)
                continue;

            secline->flags &= ~ML_BLOCKING;
            secline->special = 0;
            sides[secline->sidenum[0]].midtexture = 0;
            sides[secline->sidenum[1]].midtexture = 0;
        }
    }

    return ret;
}


//
// EV_VerticalDoor : open a door manually, no tag value
//
// [STRIFE] Tons of new door types were added.
//
void
EV_VerticalDoor
( line_t*	line,
  mobj_t*	thing )
{
    player_t*	player;
    sector_t*	sec;
    vldoor_t*	door;
    int		side;
	
    side = 0;	// only front sides can be used

    //	Check for locks
    player = thing->player;

    // haleyjd 09/15/10: [STRIFE] myriad checks here...
    switch (line->special)
    {
    case 26:  // DR ID Card door
    case 32:  // D1 ID Card door
        if(!player->cards[key_IDCard])
        {
            player->message = "You need an id card to open this door";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 27:  // DR Pass Card door
    case 34:  // D1 Pass Card door
        if(!player->cards[key_Passcard])
        {
            player->message = "You need a pass card key to open this door";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 28:  // DR ID Badge door
    case 33:  // D1 ID Badge door
        if(!player->cards[key_IDBadge])
        {
            player->message = "You need an id badge to open this door";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 156: // D1 brass key door
    case 161: // DR brass key door
        if(!player->cards[key_BrassKey])
        {
            player->message = "You need a brass key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 157: // D1 silver key door
    case 160: // DR silver key door
        if(!player->cards[key_SilverKey])
        {
            player->message = "You need a silver key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 158: // D1 gold key door
    case 159: // DR gold key door
        if(!player->cards[key_GoldKey])
        {
            player->message = "You need a gold key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

        // villsa [STRIFE] added 09/15/10
    case 165:
        player->message = "That doesn't seem to work";
        S_StartSound(NULL, sfx_oof);
        return;

    case 166: // DR Hand Print door
        if(!player->cards[key_SeveredHand])
        {
            player->message = "Hand print not on file";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 169: // DR Base key door
        if(!player->cards[key_BaseKey])
        {
            player->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 170: // DR Gov's Key door
        if(!player->cards[key_GovsKey])
        {
            player->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 190: // DR Order Key door
        if(!player->cards[key_OrderKey])
        {
            player->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 205: // DR "Only in retail"
        player->message = "THIS AREA IS ONLY AVAILABLE IN THE "
                                     "RETAIL VERSION OF STRIFE";
        S_StartSound(NULL, sfx_oof);
        return;

    case 213: // DR Chalice door
        if(!P_PlayerHasItem(player, MT_INV_CHALICE))
        {
            player->message = "You need the chalice!";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 217: // DR Core Key door
        if(!player->cards[key_CoreKey])
        {
            player->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 221: // DR Mauler Key door
        if(!player->cards[key_MaulerKey])
        {
            player->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 224: // DR Chapel Key door
        if(!player->cards[key_ChapelKey])
        {
            player->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 225: // DR Catacomb Key door
        if(!player->cards[key_CatacombKey])
        {
            player->message = "You don't have the key";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    case 232: // DR Oracle Pass door
        if(!(player->questflags & QF_QUEST18))
        {
            player->message = "You need the Oracle Pass!";
            S_StartSound(NULL, sfx_oof);
            return;
        }
        break;

    default:
        break;
    }

    // if the sector has an active thinker, use it
    sec = sides[line->sidenum[side ^ 1]].sector;

    if (sec->specialdata)
    {
        door = sec->specialdata;
        // [STRIFE] Adjusted to handle linetypes handled here by Strife.
        // BUG: Not all door types are checked here. This means that certain 
        // door lines are allowed to fall through and start a new thinker on the
        // sector! This is why some doors can become jammed in Strife - stuck in 
        // midair, or unable to be opened at all. Multiple thinkers will fight 
        // over how to move the door. They should have added a default return if
        // they weren't going to handle this unconditionally..
        switch (line->special)
        {
        case 1:     // ONLY FOR "RAISE" DOORS, NOT "OPEN"s
        case 26:
        case 27:
        case 28:
        case 117:
        case 159:       // villsa
        case 160:       // haleyjd
        case 161:       // villsa
        case 166:       // villsa
        case 169:       // villsa
        case 170:       // villsa
        case 190:       // villsa
        case 213:       // villsa
        case 232:       // villsa
            if (door->direction == -1)
                door->direction = 1;	// go back up
            else
            {
                if (!thing->player)
                    return;		// JDC: bad guys never close doors

                door->direction = -1;	// start going down immediately
            }
            return;
        }
    }

    // haleyjd 09/15/10: [STRIFE] Removed DOOM door sounds


    // new door thinker
    door = Z_Malloc(sizeof(*door), PU_LEVSPEC, 0);
    P_AddThinker(&door->thinker);
    sec->specialdata = door;
    door->thinker.function = T_VerticalDoor;
    door->sector = sec;
    door->direction = 1;
    door->speed = VDOORSPEED;
    door->topwait = VDOORWAIT;
    R_SoundNumForDoor(door);   // haleyjd 09/15/10: [STRIFE] Get door sounds
    
    // for proper sound - [STRIFE] - verified complete
    switch (line->special)
    {
    case 117:   // BLAZING DOOR RAISE
    case 118:   // BLAZING DOOR OPEN
        S_StartSound((mobj_t*)&sec->soundorg, sfx_bdopn);
        break;

    case 1:     // NORMAL DOOR SOUND
    case 31:
        S_StartSound((mobj_t*)&sec->soundorg, door->opensound);
        break;
    default:	// LOCKED DOOR SOUND
        S_StartSound((mobj_t*)&sec->soundorg, door->opensound);
        break;
    }

    // haleyjd: [STRIFE] - verified all.
    switch (line->special)
    {
    case 1:
    case 26:
    case 27:
    case 28:
        door->type = vld_normal;
        break;

    case 31:
    case 32:
    case 33:
    case 34:
    case 156:   // villsa [STRIFE]
    case 157:   // villsa [STRIFE]
    case 158:   // villsa [STRIFE]
        door->type = vld_open;
        line->special = 0;
        break;

    case 117:   // blazing door raise
        door->type = vld_blazeRaise;
        door->speed = VDOORSPEED * 4;
        break;
    case 118:   // blazing door open
        door->type = vld_blazeOpen;
        line->special = 0;
        door->speed = VDOORSPEED * 4;
        break;

    default:
        // haleyjd: [STRIFE] pretty important to have this here!
        door->type = vld_normal;
        break;
    }

    // find the top and bottom of the movement range
    door->topheight = P_FindLowestCeilingSurrounding(sec);
    door->topheight -= 4 * FRACUNIT;
}


//
// Spawn a door that closes after 30 seconds
//
void P_SpawnDoorCloseIn30 (sector_t* sec)
{
    vldoor_t*	door;
	
    door = Z_Malloc ( sizeof(*door), PU_LEVSPEC, 0);

    P_AddThinker (&door->thinker);

    sec->specialdata = door;
    sec->special = 0;

    door->thinker.function = T_VerticalDoor;
    door->sector = sec;
    door->direction = 0;
    door->type = vld_normal;
    door->speed = VDOORSPEED;
    door->topcountdown = 30 * 35;
}

//
// Spawn a door that opens after 5 minutes
//
void
P_SpawnDoorRaiseIn5Mins
( sector_t*	sec,
  int		secnum )
{
    vldoor_t*	door;
	
    door = Z_Malloc ( sizeof(*door), PU_LEVSPEC, 0);
    
    P_AddThinker (&door->thinker);

    sec->specialdata = door;
    sec->special = 0;

    door->thinker.function = T_VerticalDoor;
    door->sector = sec;
    door->direction = 2;
    door->type = vld_raiseIn5Mins;
    door->speed = VDOORSPEED;
    door->topheight = P_FindLowestCeilingSurrounding(sec);
    door->topheight -= 4*FRACUNIT;
    door->topwait = VDOORWAIT;
    door->topcountdown = 5 * 60 * 35;
}



// villsa [STRIFE] resurrected sliding doors
//

//
// villsa [STRIFE]
//
// Sliding door name information
//
slidename_t slideFrameNames[MAXSLIDEDOORS] =
{
    // SIGLDR
    {
        "SIGLDR01",  // frame1
        "SIGLDR02",  // frame2
        "SIGLDR03",  // frame3
        "SIGLDR04",  // frame4
        "SIGLDR05",  // frame5
        "SIGLDR06",  // frame6
        "SIGLDR07",  // frame7
        "SIGLDR08"   // frame8
    },
    // DORSTN
    {
        "DORSTN01",  // frame1
        "DORSTN02",  // frame2
        "DORSTN03",  // frame3
        "DORSTN04",  // frame4
        "DORSTN05",  // frame5
        "DORSTN06",  // frame6
        "DORSTN07",  // frame7
        "DORSTN08"   // frame8
    },

    // DORQTR
    {
        "DORQTR01",  // frame1
        "DORQTR02",  // frame2
        "DORQTR03",  // frame3
        "DORQTR04",  // frame4
        "DORQTR05",  // frame5
        "DORQTR06",  // frame6
        "DORQTR07",  // frame7
        "DORQTR08"   // frame8
    },

    // DORCRG
    {
        "DORCRG01",  // frame1
        "DORCRG02",  // frame2
        "DORCRG03",  // frame3
        "DORCRG04",  // frame4
        "DORCRG05",  // frame5
        "DORCRG06",  // frame6
        "DORCRG07",  // frame7
        "DORCRG08"   // frame8
    },

    // DORCHN
    {
        "DORCHN01",  // frame1
        "DORCHN02",  // frame2
        "DORCHN03",  // frame3
        "DORCHN04",  // frame4
        "DORCHN05",  // frame5
        "DORCHN06",  // frame6
        "DORCHN07",  // frame7
        "DORCHN08"   // frame8
    },

    // DORIRS
    {
        "DORIRS01",  // frame1
        "DORIRS02",  // frame2
        "DORIRS03",  // frame3
        "DORIRS04",  // frame4
        "DORIRS05",  // frame5
        "DORIRS06",  // frame6
        "DORIRS07",  // frame7
        "DORIRS08"   // frame8
    },

    // DORALN
    {
        "DORALN01",  // frame1
        "DORALN02",  // frame2
        "DORALN03",  // frame3
        "DORALN04",  // frame4
        "DORALN05",  // frame5
        "DORALN06",  // frame6
        "DORALN07",  // frame7
        "DORALN08"   // frame8
    },

    {"\0","\0","\0","\0","\0","\0","\0","\0"}
};

//
// villsa [STRIFE]
//
// Sliding door open sounds
//
sfxenum_t slideOpenSounds[MAXSLIDEDOORS] =
{
    sfx_drlmto, sfx_drston, sfx_airlck, sfx_drsmto,
    sfx_drchno, sfx_airlck, sfx_airlck, sfx_None
};

//
// villsa [STRIFE]
//
// Sliding door close sounds
//
sfxenum_t slideCloseSounds[MAXSLIDEDOORS] =
{
    sfx_drlmtc, sfx_drston, sfx_airlck, sfx_drsmtc,
    sfx_drchnc, sfx_airlck, sfx_airlck, sfx_None
};

slideframe_t slideFrames[MAXSLIDEDOORS];

//
// EV_SlidingDoor : slide a door horizontally
// (animate midtexture, then set noblocking line)
//

void P_InitSlidingDoorFrames(void)
{
    int i;

    memset(slideFrames, 0, sizeof(slideframe_t) * MAXSLIDEDOORS);

    for (i = 0; i < MAXSLIDEDOORS; i++)
    {
        if (!slideFrameNames[i].frame1[0])
            break;

        if (R_CheckTextureNumForName(slideFrameNames[i].frame1) == -1)
            continue;

        slideFrames[i].frames[0] = R_TextureNumForName(slideFrameNames[i].frame1);
        slideFrames[i].frames[1] = R_TextureNumForName(slideFrameNames[i].frame2);
        slideFrames[i].frames[2] = R_TextureNumForName(slideFrameNames[i].frame3);
        slideFrames[i].frames[3] = R_TextureNumForName(slideFrameNames[i].frame4);
        slideFrames[i].frames[4] = R_TextureNumForName(slideFrameNames[i].frame5);
        slideFrames[i].frames[5] = R_TextureNumForName(slideFrameNames[i].frame6);
        slideFrames[i].frames[6] = R_TextureNumForName(slideFrameNames[i].frame7);
        slideFrames[i].frames[7] = R_TextureNumForName(slideFrameNames[i].frame8);
    }
}


//
// P_FindSlidingDoorType
//
// Return index into "slideFrames" array
// for which door type to use
//
// villsa [STRIFE] resurrected
//
int P_FindSlidingDoorType(line_t* line)
{
    int i;
    int val;

    for (i = 0; i < MAXSLIDEDOORS; i++)
    {
        val = sides[line->sidenum[0]].toptexture;
        if (val == slideFrames[i].frames[0])
            return i;
    }

    return -1;
}

//
// T_SlidingDoor
//
// villsa [STRIFE] resurrected
//

void T_SlidingDoor (slidedoor_t* door)
{
    sector_t* sec;
    switch (door->status)
    {
    case sd_opening:
        if (!door->timer--)
        {
            if (++door->frame == SNUMFRAMES)
            {
                // IF DOOR IS DONE OPENING...
                door->line1->flags &= ML_BLOCKING ^ 0xff;
                door->line1->flags &= ML_BLOCKING ^ 0xff;

                if (door->type == sdt_openOnly)
                {
                    door->frontsector->specialdata = NULL;
                    P_RemoveThinker(&door->thinker);
                    break;
                }

                door->timer = SDOORWAIT;
                door->status = sd_waiting;
            }
            else
            {
                // IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...
                door->timer = SWAITTICS;

                sides[door->line1->sidenum[0]].midtexture =
                sides[door->line1->sidenum[1]].midtexture =
                sides[door->line2->sidenum[0]].midtexture =
                sides[door->line2->sidenum[1]].midtexture =
                    slideFrames[door->whichDoorIndex].frames[door->frame];
            }
        }
        break;

    case sd_waiting:
        // IF DOOR IS DONE WAITING...
        if (!door->timer--)
        {
            fixed_t speed;
            fixed_t cheight;

            sec = door->frontsector;
            // CAN DOOR CLOSE?
            if (sec->thinglist != NULL)
            {
                door->timer = SDOORWAIT;
                break;
            }

            cheight = sec->ceilingheight;
            speed = cheight - sec->floorheight - (10 * FRACUNIT);

            // something blocking it?
            if (T_MovePlane(sec, speed, sec->floorheight, false, 1, -1) == crushed)
            {
                door->timer = SDOORWAIT;
                break;
            }

            // Instantly move plane
            T_MovePlane(sec, 128 * FRACUNIT, cheight, false, 1, 1);

            // turn line blocking back on
            door->line1->flags |= ML_BLOCKING;
            door->line2->flags |= ML_BLOCKING;

            // play close sound
            S_StartSound((mobj_t*)&sec->soundorg, slideCloseSounds[door->whichDoorIndex]);

            door->status = sd_closing;
            door->timer = SWAITTICS;
           
        }
        break;

    case sd_closing:
        if (!door->timer--)
        {
            if (--door->frame < 0)
            {
                sec = door->frontsector;
                // IF DOOR IS DONE CLOSING...
                T_MovePlane(sec, 128 * FRACUNIT, sec->floorheight, false, 1, -1);
                sec->specialdata = NULL;
                P_RemoveThinker(&door->thinker);
                break;
            }
            else
            {
                // IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...
                door->timer = SWAITTICS;

                sides[door->line1->sidenum[0]].midtexture =
                sides[door->line1->sidenum[1]].midtexture =
                sides[door->line2->sidenum[0]].midtexture =
                sides[door->line2->sidenum[1]].midtexture =
                    slideFrames[door->whichDoorIndex].frames[door->frame];
            }
        }
        break;
    }
}

//
// EV_RemoteSlidingDoor
//
// villsa [STRIFE] new function
//
int EV_RemoteSlidingDoor(line_t* line, mobj_t* thing)
{
    int		    secnum;
    sector_t*       sec;
    int             i;
    int             rtn;
    line_t*         secline;
	
    secnum = -1;
    rtn = 0;
    
    while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
        if(sec->specialdata)
            continue;

        for(i = 0; i < 4; i++)
        {
            secline = sec->lines[i];

            if(P_FindSlidingDoorType(secline) < 0)
                continue;

            EV_SlidingDoor(secline, thing);
            rtn = 1;
        }
    }

    return rtn;
}


//
// EV_SlidingDoor
//
// villsa [STRIFE]
//

void
EV_SlidingDoor
( line_t*	line,
  mobj_t*	thing )
{
    sector_t*       sec;
    slidedoor_t*    door;
    line_t*     secline;
    int     i;
    
    // Make sure door isn't already being animated
    sec = sides[line->sidenum[1]].sector;
    if (sec->specialdata)
    {
        if (!thing->player)
            return;

        door = sec->specialdata;
        if (door->type == sdt_openAndClose)
        {
            if (door->status == sd_waiting)
            {
                door->timer = SWAITTICS;    // villsa [STRIFE]
                door->status = sd_closing;
            }
        }
        return;
    }

    // Init sliding door vars
    if (!door)
    {
        door = Z_Malloc(sizeof(*door), PU_LEVSPEC, 0);
        P_AddThinker(&door->thinker);
        sec->specialdata = door;

        door->type = sdt_openAndClose;
        door->status = sd_opening;
        door->whichDoorIndex = P_FindSlidingDoorType(line);

        // villsa [STRIFE] different error message
        if (door->whichDoorIndex < 0)
            I_Error("EV_SlidingDoor: Textures are not defined for sliding door!");

        sides[line->sidenum[0]].midtexture = sides[line->sidenum[0]].toptexture;

        // villsa [STRIFE]
        door->line1 = door->line2 = line;

        // villsa [STRIFE] this loop assumes that the sliding door is made up
        // of only four linedefs!
        for(i = 0; i < 4; i++)
        {
            secline = sec->lines[i];
            if(secline != line)
            {
                side_t* side1;
                side_t* side2;

                side1 = &sides[secline->sidenum[0]];
                side2 = &sides[line->sidenum[0]];

                if(side1->toptexture == side2->toptexture)
                    door->line2 = secline;
            }
        }
        door->frontsector = sec;
        door->thinker.function = T_SlidingDoor;
        door->timer = SWAITTICS;
        door->frame = 0;

        // villsa [STRIFE] preset flags
        door->line1->flags |= ML_BLOCKING;
        door->line2->flags |= ML_BLOCKING;

        // villsa [STRIFE] set the closing sector
        T_MovePlane(
            door->frontsector,
            128 * FRACUNIT,
            P_FindLowestCeilingSurrounding(door->frontsector),
            false,
            1,
            1);

        // villsa [STRIFE] play open sound
        S_StartSound((mobj_t*)&door->frontsector->soundorg, slideOpenSounds[door->whichDoorIndex]);
    }
}
