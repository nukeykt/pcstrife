//
// Copyright (C) 1993-1996 Id Software, Inc.
// Copyright (C) 1993-2008 Raven Software
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
//

#include <stdio.h>
#include "doomdef.h"
#include "dmx.h"
#include "sounds.h"
#include "d_main.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

//
// I_StartupTimer
//

int tsm_ID = -1;

#define SND_TICRATE 140
#define SND_MAXSONGS 40
#define SND_SAMPLERATE 11025

void I_StartupTimer(void)
{
    extern int I_TimerISR(void);

    if (devparm)
    {
        printf("I_StartupTimer()\n");
    }
    // installs master timer.  Must be done before StartupTimer()!
    TSM_Install(SND_TICRATE);
    tsm_ID = TSM_NewService(I_TimerISR, 35, 255, 0); // max priority
    if (tsm_ID == -1)
    {
        I_Error("Can't register 35 Hz timer w/ DMX library");
    }
}

void I_ShutdownTimer(void)
{
    TSM_DelService(tsm_ID);
    TSM_Remove();
}

//
//
//                          SOUND HEADER & DATA
//
//
//

// sound information
#if 0
const char *dnames[] = {"None",
                        "PC_Speaker",
                        "Adlib",
                        "Sound_Blaster",
                        "ProAudio_Spectrum16",
                        "Gravis_Ultrasound",
                        "MPU",
                        "AWE32"
                        };
#endif

const char snd_prefixen[] = { 'P', 'P', 'A', 'S', 'S', 'S', 'M', 'M', 'M', 'S' };

int snd_Channels;
int snd_DesiredMusicDevice, snd_DesiredSfxDevice;
int snd_MusicDevice,    // current music card # (index to dmxCodes)
        snd_SfxDevice,      // current sfx card # (index to dmxCodes)
        snd_MaxVolume,      // maximum volume for sound
        snd_MusicVolume;    // maximum volume for music

int dmxCodes[NUM_SCARDS]; // the dmx code for a given card
int snd_VoiceHandle = -1;
void *snd_VoiceData;

int     snd_SBport, snd_SBirq, snd_SBdma;       // sound blaster variables
int     snd_Mport;                              // midi variables

extern boolean  snd_MusicAvail, // whether music is available
                snd_SfxAvail;   // whether sfx are available

void I_PauseSong(int handle)
{
    MUS_PauseSong(handle);
}

void I_ResumeSong(int handle)
{
    MUS_ResumeSong(handle);
}

void I_SetMusicVolume(int volume)
{
    MUS_SetMasterVolume(volume);
    snd_MusicVolume = volume;
}

//
//
//                              SONG API
//
//

int I_RegisterSong(void *data)
{
    int rc = MUS_RegisterSong(data);
#ifdef SNDDEBUG
    if (rc<0) printf("MUS_Reg() returned %d\n", rc);
#endif
    return rc;
}

void I_UnRegisterSong(int handle)
{
    int rc = MUS_UnregisterSong(handle);
#ifdef SNDDEBUG
    if (rc < 0) printf("MUS_Unreg() returned %d\n", rc);
#endif
}

int I_QrySongPlaying(int handle)
{
    int rc = MUS_QrySongPlaying(handle);
#ifdef SNDDEBUG
    if (rc < 0) printf("MUS_QrySP() returned %d\n", rc);
#endif
    return rc;
}

// Stops a song.  MUST be called before I_UnregisterSong().

void I_StopSong(int handle)
{
    int rc;
    rc = MUS_StopSong(handle);
#ifdef SNDDEBUG
    if (rc < 0) printf("MUS_StopSong() returned %d\n", rc);
#endif
    // Fucking kluge pause
    {
        int s;
        extern volatile int ticcount;
        for (s = ticcount; ticcount - s < 10; );
    }
}

void I_PlaySong(int handle, boolean looping)
{
    int rc;
    rc = MUS_ChainSong(handle, looping ? handle : -1);
#ifdef SNDDEBUG
    if (rc < 0) printf("MUS_ChainSong() returned %d\n", rc);
#endif
    rc = MUS_PlaySong(handle, snd_MusicVolume);
#ifdef SNDDEBUG
    if (rc < 0) printf("MUS_PlaySong() returned %d\n", rc);
#endif

}

//
//
//                                  SOUND FX API
//
//

// Gets lump nums of the named sound.  Returns pointer which will be
// passed to I_StartSound() when you want to start an SFX.  Must be
// sure to pass this to UngetSoundEffect() so that they can be
// freed!


int I_GetSfxLumpNum(sfxinfo_t *sound)
{
    char namebuf[9];
    char *name;

    sprintf(namebuf, "d%c%s", snd_prefixen[snd_SfxDevice], sound->name);

    if (snd_prefixen[snd_SfxDevice] == 'P'
        && W_CheckNumForName(namebuf) == -1)
    {
        name = "dprifle";
    }
    else
        name = namebuf;
    return W_GetNumForName(name);
}

int I_StartSound(int id, void *data, int vol, int sep, int pitch)
{
    if (snd_SfxDevice == snd_PC
     && data == S_sfx[sfx_rifle].data)
    {
        return -1;
    }
    else
    {
        return SFX_PlayPatch(data, pitch, sep, vol, 0, 100);
    }
}

void I_StartVoice(char *name)
{
    char namebuf[9];
    int voice;

    if (netgame || snd_prefixen[snd_SfxDevice] != 'S' || disable_voices)
    {
        return;
    }

    if (snd_VoiceHandle > 0)
    {
        SFX_StopPatch(snd_VoiceHandle);
        Z_ChangeTag(snd_VoiceData, PU_CACHE);
        snd_VoiceHandle = -1;
    }

    strcpy(namebuf, name);

    voice = W_CheckNumForName(namebuf);

    if (voice != -1)
    {
        snd_VoiceData = W_CacheLumpNum(voice, PU_LEVEL);
        snd_VoiceHandle = SFX_PlayPatch(snd_VoiceData, 128, 128, snd_VoiceVolume, 0, 10);
    }
}

void I_StopSound(int handle)
{
    //  extern volatile long gDmaCount;
    //  long waittocount;
    SFX_StopPatch(handle);
    //  waittocount = gDmaCount + 2;
    //  while (gDmaCount < waittocount) ;
}

int I_SoundIsPlaying(int handle)
{
    return SFX_Playing(handle);
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
    SFX_SetOrigin(handle, pitch, sep, vol);
}

//
//
//                                          SOUND STARTUP STUFF
//
//
//

//
// Why PC's Suck, Reason #8712
//

void I_sndArbitrateCards(void)
{
    char string[256];
    boolean gus, adlib, pc, sb, midi;
    int i, rc, wait, dmxlump;

    snd_MaxVolume = 127;

    snd_MusicDevice = snd_DesiredMusicDevice;
    snd_SfxDevice = snd_DesiredSfxDevice;

    // check command-line parameters- overrides config file
    //
    if (M_CheckParm("-nosound")) snd_MusicDevice = snd_SfxDevice = snd_none;
    if (M_CheckParm("-nosfx")) snd_SfxDevice = snd_none;
    if (M_CheckParm("-nomusic")) snd_MusicDevice = snd_none;

    if (snd_MusicDevice > snd_MPU)
    {
        snd_MusicDevice = snd_MPU;
    }
    if (snd_MusicDevice == snd_SB)
    {
        snd_MusicDevice = snd_Adlib;
    }
    if (snd_MusicDevice == snd_PAS)
    {
        snd_MusicDevice = snd_Adlib;
    }
    // figure out what i've got to initialize
    //
    gus = snd_MusicDevice == snd_GUS || snd_SfxDevice == snd_GUS;
    sb = snd_SfxDevice == snd_SB || snd_MusicDevice == snd_SB;
    adlib = snd_MusicDevice == snd_Adlib;
    pc = snd_SfxDevice == snd_PC;
    midi = snd_MusicDevice == snd_MPU;

    // initialize whatever i've got
    //
    if (gus)
    {
        if (devparm)
        {
            printf("GUS\n");
        }
        fprintf(stderr, "GUS1\n");
        if (GF1_Detect())
        {
            D_DrawText("Dude.  The GUS ain't responding.\n");
        }
        else
        {
            fprintf(stderr, "GUS2\n");
            dmxlump = W_GetNumForName("dmxgus");
            GF1_SetMap(W_CacheLumpNum(dmxlump, PU_CACHE), lumpinfo[dmxlump].size);
        }

    }
    if (sb)
    {
        if (devparm)
        {
            printf("SB p=0x%x, i=%d, d=%d\n",
                snd_SBport, snd_SBirq, snd_SBdma);
        }
        if (SB_Detect(&snd_SBport, &snd_SBirq, &snd_SBdma, 0))
        {
            sprintf(string, "SB isn't responding at p=0x%x, i=%d, d=%d\n",
                snd_SBport, snd_SBirq, snd_SBdma);
            D_DrawText(string);
        }
        else
        {
            SB_SetCard(snd_SBport, snd_SBirq, snd_SBdma);
        }
        if (devparm)
        {
            printf("SB_Detect returned p=0x%x,i=%d,d=%d\n",
                snd_SBport, snd_SBirq, snd_SBdma);
        }
    }

    if (adlib)
    {
        if (devparm)
        {
            D_DrawText("Adlib\n");
        }

        if (AL_Detect(&wait, 0))
        {
            D_DrawText("Dude.  The Adlib isn't responding.\n");
        }
        else
        {
            AL_SetCard(wait, W_CacheLumpName("genmidi", PU_STATIC));
        }
    }

    if (midi)
    {
        if (devparm)
        {
            printf("Midi\n");
        }

        if (devparm)
        {
            printf("cfg p=0x%x\n", snd_Mport);
        }

        if (MPU_Detect(&snd_Mport, &i))
        {
            sprintf(string, "The MPU-401 isn't reponding @ p=0x%x.\n", snd_Mport);
            D_DrawText(string);
        }
        else
        {
            MPU_SetCard(snd_Mport);
        }
    }

}

// inits all sound stuff

void I_StartupSound (void)
{
    int rc;

    // initialize dmxCodes[]
    dmxCodes[0] = 0;
    dmxCodes[snd_PC] = AHW_PC_SPEAKER;
    dmxCodes[snd_Adlib] = AHW_ADLIB;
    dmxCodes[snd_SB] = AHW_SOUND_BLASTER;
    dmxCodes[snd_PAS] = AHW_MEDIA_VISION;
    dmxCodes[snd_GUS] = AHW_ULTRA_SOUND;
    dmxCodes[snd_MPU] = AHW_MPU_401;
    dmxCodes[snd_AWE] = AHW_AWE32;

    // inits sound library timer stuff
    I_StartupTimer();

    // pick the sound cards i'm going to use
    //
    I_sndArbitrateCards();

    if (devparm)
    {
        printf("  Music device #%d & dmxCode=%d", snd_MusicDevice,
            dmxCodes[snd_MusicDevice]);
        printf("  Sfx device #%d & dmxCode=%d\n", snd_SfxDevice,
            dmxCodes[snd_SfxDevice]);
    }

    // inits DMX sound library
    if (devparm)
    {
        printf("  calling DMX_Init");
    }
    rc = DMX_Init(SND_TICRATE, SND_MAXSONGS, dmxCodes[snd_MusicDevice],
        dmxCodes[snd_SfxDevice]);

    if (devparm)
    {
        printf("  DMX_Init() returned %d", rc);
    }

    if (disable_voices || M_CheckParm("-novoice"))
    {
        disable_voices = true;
        dialogshowtext = true;
    }
    else
    {
        disable_voices = false;
    }

    if (devparm)
    {
        printf("  Play Voices = %d\n", !disable_voices);
    }
}

// shuts down all sound stuff

void I_ShutdownSound (void)
{
    int s;
    extern volatile int ticcount;
    if (tsm_ID < 0)
        return;
    S_PauseSound();
    s = ticcount + 30;
    while (s != ticcount) {};
    DMX_DeInit();
}

void I_SetChannels(int channels)
{
    WAV_PlayMode(channels, SND_SAMPLERATE);
}
