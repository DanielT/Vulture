/*  SCCS Id: @(#)jtp_dirx.c  3.0 2000/11/12  */
/* Copyright (c) Jaakko Peltonen, 2000				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef _jtp_dirx_c_
#define _jtp_dirx_c_

/*-------------------------------------------------------------------
 jtp_dirx.c : DirextX API calls for Vulture's Eye windowing system.
 Requires DirextX 3.0 or newer.
-------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <dsound.h>
#include <malloc.h>
#include "jtp_def.h"
#include "jtp_gra.h"
#include "jtp_gen.h"
#include "jtp_txt.h"
#include "jtp_gfl.h"
#include "jtp_mou.h"
#include "jtp_win.h"
#include "jtp_dirx.h"


/* Definitions */
#define JTP_DX_LOG_FILENAME "jtp_log.txt"
#define JTP_DX_LOG_ERROR 1
#define JTP_DX_LOG_NOTE 2
#define JTP_DX_LOG_DEBUG 3
#define JTP_DX_LOG_WRITE_ERROR 1
#define JTP_DX_LOG_WRITE_NOTE 0
#define JTP_DX_LOG_WRITE_DEBUG 0

#define JTP_DX_MAX_BUFFERED_KEYS 100

#define JTP_DX_MAX_POLLED_MESSAGES 20
#define JTP_DX_NO_MESSAGE_POLLING -1
#define JTP_DX_POLLED_MESSAGE_NOT_ARRIVED 0
#define JTP_DX_POLLED_MESSAGE_ARRIVED 1

/* 
 * Sound capability: 
 *  4 simultaneous sounds, 
 *  each with 4 seconds of stereo 22050 Hz 8-bit data
 */
#define JTP_DX_MAX_SOUNDS 4
#define JTP_DX_MAX_CACHED_SOUNDS 40
#define JTP_DX_SOUND_BUFFER_SIZE 500000
#define JTP_DX_SOUND_BUFFER_FREQUENCY 44100
#define JTP_DX_SOUND_BUFFER_BITS 16
#define JTP_DX_SOUND_BUFFER_CHANNELS 1

typedef struct {
  char * samples;
  int    length;
  char * filename;
} jtp_dx_cached_sound;

/* Function prototypes */
LRESULT CALLBACK jtp_DXWndFunc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


/* NetHack interface -related objects */
int jtp_dx_appisactive = 1;
int jtp_dx_mousex, jtp_dx_mousey, jtp_dx_mouseb;

/* Key buffering */
int jtp_dx_n_keys_in_buffer = 0;
int jtp_dx_key_buffer[JTP_DX_MAX_BUFFERED_KEYS];
BYTE jtp_dx_kb_state_table[256];

/* Message polling */
UINT   jtp_dx_polled_messages[JTP_DX_MAX_POLLED_MESSAGES]; /* Messages that are polled for */
int    jtp_dx_n_polled_messages;      /* N. of messages polled for */
int    jtp_dx_polled_message_arrived; /* Has any polled message arrived? */
UINT   jtp_dx_polled_message;         /* The particular message that ended the polling */


/* Windows API -related objects */ 
HINSTANCE            jtp_dx_hThisInstance;
HINSTANCE            jtp_dx_hPrevInstance;
int                  jtp_dx_nCmdShow;
LPSTR                jtp_dx_lpszCmdParam;
HWND                 jtp_dx_hwndMain;


/* DirectDraw objects */
LPDIRECTDRAW         jtp_dx_lpDD;
LPDIRECTDRAWSURFACE  jtp_dx_lpDDSPrimary;
IDirectDrawPalette * jtp_dx_lpDDPal = NULL;
PALETTEENTRY       * jtp_dx_colors = NULL;


/* DirectSound objects */
LPDIRECTSOUND        jtp_dx_lpDS;
DSBUFFERDESC         jtp_dx_DSBDesc;
LPDIRECTSOUNDBUFFER  jtp_dx_lpDSBPrimary;
LPDIRECTSOUNDBUFFER  jtp_dx_lpDSBuffers[JTP_DX_MAX_SOUNDS];
int                  jtp_dx_oldest_sound = 0;
jtp_dx_cached_sound * jtp_dx_cached_sounds;
int                  jtp_dx_oldest_cached_sound = 0;

/* MCI objects */
MCI_OPEN_PARMS       jtp_dx_mciOpenParms;
MCI_SET_PARMS        jtp_dx_mciSetParms;
MCI_LOAD_PARMS       jtp_dx_mciLoadParms;
MCI_PLAY_PARMS       jtp_dx_mciPlayParms;
MCI_STATUS_PARMS     jtp_dx_mciStatusParms;
MCI_GENERIC_PARMS    jtp_dx_mciGenericParms;
int                  jtp_dx_mci_opened = 0;
int                  jtp_dx_mci_playing = 0;


/*--------------------------------------------------------------------------
 Log file writing
--------------------------------------------------------------------------*/
void jtp_DXWriteLogMessage(int msgtype, char * logmessage)
{
  FILE * f;

  if ((msgtype == JTP_DX_LOG_NOTE) && (JTP_DX_LOG_WRITE_NOTE == 0)) return;
  if ((msgtype == JTP_DX_LOG_ERROR) && (JTP_DX_LOG_WRITE_ERROR == 0)) return;
  if ((msgtype == JTP_DX_LOG_DEBUG) && (JTP_DX_LOG_WRITE_DEBUG == 0)) return;

  f = fopen(JTP_DX_LOG_FILENAME, "a");
  fprintf(f, "%s", logmessage);
  fclose(f);
}

void jtp_DXCleanUp()
{
  DWORD err;
  char tempbuffer[200];
  int i;

  /* Clean up DirectDraw interface */
  if (jtp_dx_lpDD != NULL)
  {
    if (jtp_dx_lpDDSPrimary != NULL)
    {
      if (jtp_dx_lpDDPal != NULL)
      {
        jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note1] Releasing palette\n");
        jtp_dx_lpDDPal->Release();
        jtp_dx_lpDDPal = NULL;
      }
      jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note2] Releasing primary surface\n");
      jtp_dx_lpDDSPrimary->Release();
      jtp_dx_lpDDSPrimary = NULL;
    }

    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note4] Releasing DirectDraw object\n");
    jtp_dx_lpDD->Release(); /* This also restores the original screen mode */
    jtp_dx_lpDD = NULL;
  }

  /* Clean up DirectSound interface */
  if (jtp_dx_lpDS != NULL)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note5] Releasing cached sounds\n");
    if (jtp_dx_cached_sounds != NULL)
    {
      for (i = 0; i < JTP_DX_MAX_CACHED_SOUNDS; i++)
      {
        free(jtp_dx_cached_sounds[i].filename);
        free(jtp_dx_cached_sounds[i].samples);
      }
      free(jtp_dx_cached_sounds);
      jtp_dx_cached_sounds = NULL;
    }
    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note6] Releasing DirectSound object\n");
    jtp_dx_lpDS->Release(); /* This also releases all buffers */
    jtp_dx_lpDS = NULL;
  }

  /* Clean up MCI interface */
  if (jtp_dx_mci_opened)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note7] Stopping MCI device\n");
    jtp_dx_mciGenericParms.dwCallback = (DWORD)jtp_dx_hwndMain;
    if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_STOP, MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciGenericParms)) != 0)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI stop failed\n");
      if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
      {
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
      }
      return;
    }

    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note7] Closing sequencer device\n");
    jtp_dx_mciGenericParms.dwCallback = (DWORD)jtp_dx_hwndMain;
    if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciGenericParms)) != 0)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI close failed\n");
      if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
      {
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
      }
      return;
    }
    else jtp_dx_mci_opened = 0;
  }
}



bool jtp_DXInitDirectDraw
(
  HWND hwnd,
  jtp_screen_t * init_screen
)
{
  HRESULT ddrval;

  jtp_dx_lpDD = NULL;
  jtp_dx_lpDDSPrimary = NULL;

  ddrval = DirectDrawCreate(NULL, &jtp_dx_lpDD, NULL);
  if (ddrval != DD_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXInitDirectDraw/Check1] ERROR: Could not create DirectDraw object\n");
    return(false);
  }

  ddrval = jtp_dx_lpDD->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
  if (ddrval != DD_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXInitDirectDraw/Check2] ERROR: Could set cooperative level\n");
    jtp_DXCleanUp();
    return(false);
  }

  ddrval = jtp_dx_lpDD->SetDisplayMode(
             init_screen->width,
             init_screen->height,
             8 /* 8 bit (256 color) mode */
            );
  if (ddrval != DD_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXInitDirectDraw/Check3] ERROR: Could not set display mode\n");
    jtp_DXCleanUp();
    return(false);
  }

  return(true);
}


BOOL jtp_DXCreateSecondarySoundBuffer
(
  int samples_per_sec,
  int bits_per_sample,
  int nchannels,
  int nsamples,
  LPDIRECTSOUNDBUFFER *lplpDsb
)
{
  PCMWAVEFORMAT pcmwf; 
  DSBUFFERDESC dsbdesc; 
  HRESULT hr;

  /* Set up wave format structure */
  memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT)); 
  pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM; 
  pcmwf.wf.nChannels = nchannels; 
  pcmwf.wf.nSamplesPerSec = samples_per_sec; 
  pcmwf.wBitsPerSample = bits_per_sample; 
  pcmwf.wf.nBlockAlign = pcmwf.wBitsPerSample / 8 * pcmwf.wf.nChannels;
  pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec*pcmwf.wf.nBlockAlign;

  /* Set up DSBUFFERDESC structure. */
  ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
  dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
  /* Need default controls (pan, volume, frequency). */
  dsbdesc.dwFlags = DSBCAPS_CTRLDEFAULT;
  /* Size of buffer. */
  dsbdesc.dwBufferBytes = nsamples * pcmwf.wf.nBlockAlign;
  dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf; 

  /* Create buffer. */
  hr = IDirectSound_CreateSoundBuffer(jtp_dx_lpDS, &dsbdesc, lplpDsb, NULL);
  if (hr == DS_OK) 
  { 
    /* Valid interface is in *lplpDsb. */
    return(true); 
  } 
  else 
  { 
    /* Failed. */
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXCreateSecondarySoundBuffer/Check1] ERROR: Could create secondary sound buffer\n");
    *lplpDsb = NULL;
    return(false);
  }   
}


bool jtp_DXInitDirectSound
(
  HWND hwnd
)
{
  HRESULT hr;
  WAVEFORMATEX wfx;
  int i;

  jtp_dx_lpDS = NULL;
  jtp_dx_lpDSBPrimary = NULL;

  /* Create the DirectSound object */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXInitDirectSound/Note1] Creating DirectSound object\n");
  hr = DirectSoundCreate(NULL, &jtp_dx_lpDS, NULL);
  if (hr != DS_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXInitDirectSound/Check1] ERROR: Could not create DirectSound object\n");
    return(false);
  }

  /* Set the cooperative level */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXInitDirectSound/Note2] Setting cooperative level\n");
  hr = IDirectSound_SetCooperativeLevel(jtp_dx_lpDS, hwnd, DSSCL_PRIORITY);
  if (hr != DS_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXInitDirectSound/Check2] ERROR: Could set cooperative level\n");
    jtp_DXCleanUp();
    return(false);
  }

  /* Create the primary sound buffer */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXInitDirectSound/Note3] Creating primary sound buffer\n");
  ZeroMemory(&jtp_dx_DSBDesc, sizeof(DSBUFFERDESC));
  jtp_dx_DSBDesc.dwSize = sizeof(DSBUFFERDESC);
  jtp_dx_DSBDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
  hr = IDirectSound_CreateSoundBuffer(jtp_dx_lpDS, &jtp_dx_DSBDesc, &jtp_dx_lpDSBPrimary, NULL);
  if (hr != DS_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXInitDirectSound/Check3] ERROR: Could not create primary sound buffer\n");
    jtp_DXCleanUp();
    return(false);
  }

  /* Set the format of the primary sound buffer */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXInitDirectSound/Note4] Setting primary buffer format\n");
  memset(&wfx, 0, sizeof(WAVEFORMATEX));
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = 2;
  wfx.nSamplesPerSec = 44100;
  wfx.wBitsPerSample = 16;
  wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
  hr = IDirectSoundBuffer_SetFormat(jtp_dx_lpDSBPrimary, &wfx); 
  if (hr != DS_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXInitDirectSound/Check4] ERROR: Could not set primary buffer format\n");
    jtp_DXCleanUp();
    return(false);
  }

  /* Create the secondary sound buffers */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXInitDirectSound/Note4] Creating secondary buffers\n");
  for (i = 0; i < JTP_DX_MAX_SOUNDS; i++)
  {
    jtp_DXCreateSecondarySoundBuffer(
              JTP_DX_SOUND_BUFFER_FREQUENCY,
              JTP_DX_SOUND_BUFFER_BITS,
              JTP_DX_SOUND_BUFFER_CHANNELS,
              JTP_DX_SOUND_BUFFER_SIZE,
              &(jtp_dx_lpDSBuffers[i]));    
  }
  
  /* Create the sound cache */
  jtp_dx_cached_sounds = (jtp_dx_cached_sound *)malloc(JTP_DX_MAX_CACHED_SOUNDS*sizeof(jtp_dx_cached_sound));
  for (i = 0; i < JTP_DX_MAX_CACHED_SOUNDS; i++)
  {
    jtp_dx_cached_sounds[i].length = 0;
    jtp_dx_cached_sounds[i].samples = NULL;
    jtp_dx_cached_sounds[i].filename = NULL;
  }
}



void jtp_DXWriteSoundData
(
  int nbytes,
  char * samples,
  LPDIRECTSOUNDBUFFER lpDsb
)
{
  LPVOID audio1;
  LPVOID audio2;
  DWORD audiosize1, audiosize2;
  HRESULT hr;
  int i;

  /* Lock the (entire) sound buffer for writing */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXWriteSoundData/Note1]: Locking buffer\n");
  hr = IDirectSoundBuffer_Lock(lpDsb, 0, 0, 
                        &audio1, &audiosize1, 
                        &audio2, &audiosize2,
                        DSBLOCK_FROMWRITECURSOR | DSBLOCK_ENTIREBUFFER);
  /* If DSERR_BUFFERLOST is returned, restore and retry lock. */
  if (hr == DSERR_BUFFERLOST) 
  { 
    IDirectSoundBuffer_Restore(lpDsb);
    hr = IDirectSoundBuffer_Lock(lpDsb, 0, 0, 
            &audio1, &audiosize1, &audio2, &audiosize2, DSBLOCK_FROMWRITECURSOR | DSBLOCK_ENTIREBUFFER);
  } 
  if (hr != DS_OK)
  { 
    /* Lock failed. */
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXWriteSoundData/Check1] ERROR: Could not lock sound buffer\n");
    return;
  }

  /* Write the wave data into the sound buffer */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXWriteSoundData/Note2]: Filling buffer\n");
  if (audiosize1 >= nbytes)
  {
    for (i = 0; i < nbytes; i++) ((char *)audio1)[i] = samples[i];
    for (i = nbytes; i < audiosize1; i++) ((signed char *)audio1)[i] = 0;
    for (i = 0; i < audiosize2; i++) ((signed char *)audio2)[i] = 0;
  }
  else if (audiosize1 > 0)
  {
    for (i = 0; i < audiosize1; i++) ((char *)audio1)[i] = samples[i];
    if (audiosize2 >= nbytes-audiosize1)
    {
      for (i = 0; i < nbytes-audiosize1; i++) ((char *)audio2)[i] = samples[i+audiosize1];
      for (i = nbytes-audiosize1; i < audiosize2; i++) ((signed char *)audio2)[i] = 0;
    }
    else
    {
      for (i = 0; i < audiosize2; i++) ((char *)audio2)[i] = samples[i+audiosize1];
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXWriteSoundData/Check1] ERROR: Could not lock entire sound buffer\n");
    }
  }

  /* Unlock the sound buffer */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXWriteSoundData/Note3]: Unlocking buffer\n");
  hr = IDirectSoundBuffer_Unlock(lpDsb, audio1, audiosize1, audio2, audiosize2);
  if (hr != DS_OK)
  { 
    /* Unlock failed. */
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXWriteSoundData/Check2] ERROR: Could not unlock sound buffer\n");
    return;
  }   
}


void jtp_DXPlaySound
(
  int nbytes,
  char * samples
)
{
  LPDIRECTSOUNDBUFFER lpDsb;
  HRESULT hr;
  WAVEFORMATEX wfx;

  /* Use the oldest sound buffer */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXPlaySound/Note1]: Finding buffer\n");
  lpDsb = jtp_dx_lpDSBuffers[jtp_dx_oldest_sound];
  if (!lpDsb)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlaySound/Check1]: Oldest buffer is NULL\n");
    return;
  }

  /* Stop any sound already playing */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXPlaySound/Note2]: Stopping buffer\n");
  hr = IDirectSoundBuffer_Stop(lpDsb);
  if (hr != DS_OK)
  { 
    /* Stop failed. */
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlaySound/Check2] ERROR: Could not stop sound buffer\n");
    return;
  }   

  /* Write the sound data to the buffer */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXPlaySound/Note4]: Writing buffer\n");
  jtp_DXWriteSoundData(nbytes, samples, lpDsb);

  /* Play the sound (with lowest priority) */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXPlaySound/Note5]: Playing sound\n");
  hr = IDirectSoundBuffer_Play(lpDsb, 0, 0, 0);
  if (hr != DS_OK)
  { 
    /* Play failed. */
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlaySound/Check2] ERROR: Could not play sound buffer\n");
    return;
  }

  /* Update the index of the oldest sound */
  jtp_dx_oldest_sound++;
  if (jtp_dx_oldest_sound >= JTP_DX_MAX_SOUNDS)
    jtp_dx_oldest_sound = 0; 
}


bool jtp_DXCreatePrimarySurface()
{
  DDSURFACEDESC ddsd;
  DDSCAPS ddscaps;
  HRESULT ddrval;

  memset(&ddsd, 0, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
  ddsd.dwBackBufferCount = 1;

  ddrval = jtp_dx_lpDD->CreateSurface(&ddsd, &jtp_dx_lpDDSPrimary, NULL);
  if (ddrval != DD_OK)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXCreatePrimarySurface/Check1] ERROR: Could not create primary surface\n");
    jtp_DXCleanUp();
    return(false);
  }
  
  return(true);
}




void jtp_DXRestoreAll()
{
  HRESULT ddrval;

  ddrval = jtp_dx_lpDDSPrimary->Restore();
}





BOOL jtp_DXInitWindow
( 
  HINSTANCE hThisInstance, 
  int nCmdShow,
  int modewidth,
  int modeheight
)
{
  WNDCLASS    wndclass;
  char szAppName[] = "NetHack";
    
  wndclass.style         = CS_DBLCLKS;
  wndclass.lpfnWndProc   = jtp_DXWndFunc;	      //the windows message handling function
  wndclass.cbClsExtra    = 0;				//not used anymore 
  wndclass.cbWndExtra    = 0;				//not used anymore 
  wndclass.hInstance     = hThisInstance; 	//you can run the windows programs more than once, which one is this
  /* wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION); */ 
  wndclass.hIcon         = LoadIcon( hThisInstance, MAKEINTRESOURCE(JTP_DX_ICON_NETHACK) ); //icon used for window
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);		//type of mouse cursor used
  wndclass.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);	//background color of window
  wndclass.lpszMenuName  = NULL;			//not used
  wndclass.lpszClassName = szAppName;		//name for this windows class	
  RegisterClass (&wndclass);
  jtp_dx_hwndMain = CreateWindowEx (
                WS_EX_TOPMOST,         
                szAppName,             
                "NetHack",
                WS_VISIBLE | WS_POPUP, 
                CW_USEDEFAULT,         
                CW_USEDEFAULT,         
                modewidth,
                modeheight,
                NULL,                  
                NULL,                  
                hThisInstance,        
                NULL);              
  if (!jtp_dx_hwndMain)
    return(FALSE);
  ShowWindow (jtp_dx_hwndMain, nCmdShow);
  UpdateWindow (jtp_dx_hwndMain);
  SetFocus(jtp_dx_hwndMain);
  ShowCursor(FALSE);	
  
  return(TRUE);
}




LRESULT CALLBACK jtp_DXWndFunc
(
  HWND hwnd, 
  UINT message, 
  WPARAM wParam, 
  LPARAM lParam
)
{
  HDC         hdc;
  PAINTSTRUCT ps;
  int i;
  WORD translated_keys[10];

  /* Check for polled messages */
  if (jtp_dx_polled_message_arrived == JTP_DX_POLLED_MESSAGE_NOT_ARRIVED)
  {
    if (jtp_dx_n_polled_messages > 0) /* Only certain messages polled for */
    {
      for (i = 0; i < jtp_dx_n_polled_messages; i++)
        if (message == jtp_dx_polled_messages[i])
        {
          jtp_dx_polled_message_arrived = JTP_DX_POLLED_MESSAGE_ARRIVED;
          jtp_dx_polled_message = message;
        }
    }
    else /* Any message is accepted */
    {
      jtp_dx_polled_message_arrived = JTP_DX_POLLED_MESSAGE_ARRIVED;
      jtp_dx_polled_message = message;
    }
  }    
    
  /* Process the message */
  switch (message)
  {
    case WM_PAINT:
      hdc = BeginPaint (hwnd, &ps);
      EndPaint (hwnd, &ps);
      return(0);
      break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      switch(wParam)
      {
/*
        case VK_ESCAPE:
          jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "NOTE[jtp_dirx.c/jtp_DXWndFunc]: Received VK_ESCAPE\n");
          jtp_DXCleanUp();
          PostQuitMessage(0);
          return(0);
          break;
*/
        case VK_TAB:     /* Allow Alt+Tab task switching */
        case VK_SHIFT:   /* Case shift is not a separate key */
        case VK_CONTROL: /* Control key is not a separate key */
        case VK_MENU:    /* ALT/Meta key is not a separate key */
          break;
        case VK_UP:
          if (jtp_dx_n_keys_in_buffer < JTP_DX_MAX_BUFFERED_KEYS)
          {
            jtp_dx_key_buffer[jtp_dx_n_keys_in_buffer] = JTP_KEY_MENU_SCROLLUP;
            jtp_dx_n_keys_in_buffer++;
          }
          break;
        case VK_DOWN:
          if (jtp_dx_n_keys_in_buffer < JTP_DX_MAX_BUFFERED_KEYS)
          {
            jtp_dx_key_buffer[jtp_dx_n_keys_in_buffer] = JTP_KEY_MENU_SCROLLDOWN;
            jtp_dx_n_keys_in_buffer++;
          }
          break;
        case VK_PRIOR:
          if (jtp_dx_n_keys_in_buffer < JTP_DX_MAX_BUFFERED_KEYS) 
          {
            jtp_dx_key_buffer[jtp_dx_n_keys_in_buffer] = JTP_KEY_MENU_SCROLLPAGEUP;
            jtp_dx_n_keys_in_buffer++;
          }          
          break;
        case VK_NEXT:
          if (jtp_dx_n_keys_in_buffer < JTP_DX_MAX_BUFFERED_KEYS) 
          {
            jtp_dx_key_buffer[jtp_dx_n_keys_in_buffer] = JTP_KEY_MENU_SCROLLPAGEDOWN;
            jtp_dx_n_keys_in_buffer++;
          }
          break;
        default:
          if (jtp_dx_n_keys_in_buffer < JTP_DX_MAX_BUFFERED_KEYS)
          {
            i = (((int)lParam) >> 2) & 255;

            /* Translate scan code to ASCII */
            GetKeyboardState(jtp_dx_kb_state_table);
            i = ToAscii((UINT)wParam, i, jtp_dx_kb_state_table, translated_keys, 0); 

            if (i >= 0)
            {
              i = translated_keys[0];

              /* Allow ALT/Meta key for alphanumeric characters */
              if (lParam & (1 << 29))
                if (((i > 'a') && (i < 'z')) || ((i > 'A') && (i < 'Z'))) 
                  i = i | (1 << 7);
              /* Allow Control key for alphanumeric characters */
              if (jtp_dx_kb_state_table[VK_CONTROL] & 0x80)
                if (((i > 'a') && (i < 'z')) || ((i > 'A') && (i < 'Z'))) 
                  i = i & 0x1f;
              /* Store the key */
              jtp_dx_key_buffer[jtp_dx_n_keys_in_buffer] = (i % 256);
              jtp_dx_n_keys_in_buffer++;
            }
            else
            {
              jtp_dx_key_buffer[jtp_dx_n_keys_in_buffer] = '?';
              jtp_dx_n_keys_in_buffer++;
            }
          }
          /* return(0); */
          break;
      }
      break;

    case WM_LBUTTONDOWN:
      if (jtp_dx_mouseb == JTP_MBUTTON_RIGHT)
        jtp_dx_mouseb = JTP_MBUTTON_LEFT | JTP_MBUTTON_RIGHT;
      else jtp_dx_mouseb = JTP_MBUTTON_LEFT;
      jtp_dx_mousex=LOWORD(lParam);
      jtp_dx_mousey=HIWORD(lParam);
      return(0);
      break;

    case WM_LBUTTONUP:
      if ((jtp_dx_mouseb & JTP_MBUTTON_RIGHT))
        jtp_dx_mouseb = JTP_MBUTTON_RIGHT;
      else jtp_dx_mouseb = JTP_MBUTTON_NONE;
      jtp_dx_mousex=LOWORD(lParam);
      jtp_dx_mousey=HIWORD(lParam);
      return(0);
      break;

    case WM_RBUTTONDOWN:
      if (jtp_dx_mouseb == JTP_MBUTTON_LEFT)
        jtp_dx_mouseb = JTP_MBUTTON_LEFT | JTP_MBUTTON_RIGHT;
      else jtp_dx_mouseb = JTP_MBUTTON_RIGHT;
      jtp_dx_mousex=LOWORD(lParam);
      jtp_dx_mousey=HIWORD(lParam);
      return(0);
      break;

    case WM_RBUTTONUP:
      if ((jtp_dx_mouseb & JTP_MBUTTON_LEFT))
        jtp_dx_mouseb = JTP_MBUTTON_LEFT;
      else jtp_dx_mouseb = JTP_MBUTTON_NONE;
      jtp_dx_mousex=LOWORD(lParam);
      jtp_dx_mousey=HIWORD(lParam);
      return(0);
      break;

    case WM_MOUSEMOVE:
      jtp_dx_mousex = LOWORD(lParam);
      jtp_dx_mousey = HIWORD(lParam);
      jtp_dx_mouseb = JTP_MBUTTON_NONE;
      if (wParam&MK_LBUTTON) jtp_dx_mouseb = JTP_MBUTTON_LEFT;
      if (wParam&MK_RBUTTON)
        if (jtp_dx_mouseb == JTP_MBUTTON_LEFT)
          jtp_dx_mouseb = JTP_MBUTTON_LEFT | JTP_MBUTTON_RIGHT;
        else jtp_dx_mouseb = JTP_MBUTTON_RIGHT;
      return(0);
      break;
      
    case WM_DESTROY:
      jtp_DXCleanUp();
      jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXWndFunc]: Received WM_DESTROY\n");
      PostQuitMessage (0);
      break;
  }       
  return DefWindowProc (hwnd, message, wParam, lParam);
}

/*
 * The function that requests message polling must fill out the
 * jtp_dx_polled_messages array.
 */
void jtp_DXPollForMessage(char waitformessage)
{
  MSG msg;

  /* Process any waiting messages */
  jtp_dx_polled_message_arrived = JTP_DX_POLLED_MESSAGE_NOT_ARRIVED;
  if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
  {
    if (!GetMessage (&msg, NULL, 0, 0)) return; /* msg.wParam; */
    TranslateMessage (&msg);
    DispatchMessage (&msg);        
  }

  /* If requested, process messages until a polled one arrives */
  if (waitformessage)
  {
    while (jtp_dx_polled_message_arrived == JTP_DX_POLLED_MESSAGE_NOT_ARRIVED)
    {
      if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
      {
        if (!GetMessage (&msg, NULL, 0, 0)) return; /* msg.wParam; */
        TranslateMessage (&msg);
        DispatchMessage (&msg);        
      }
    }
  }
  
  /* Clean up */  
  jtp_dx_polled_message_arrived = JTP_DX_NO_MESSAGE_POLLING;  
}



void jtp_DXSetInstance
(
  HINSTANCE hThisInstance, 
  HINSTANCE hPrevInstance, 
  LPSTR lpszCmdParam, 
  int nCmdShow
)
{
  int i;

  /* Set initial values for all the objects (variables) used */

  /* Windows API -related objects */ 
  jtp_dx_hThisInstance = hThisInstance;
  jtp_dx_hPrevInstance = hPrevInstance;
  jtp_dx_lpszCmdParam = lpszCmdParam;
  jtp_dx_nCmdShow = nCmdShow;
  
  /* NetHack interface -related objects */
  jtp_dx_appisactive = 1;
  jtp_dx_mousex = 100; jtp_dx_mousey = 100; jtp_dx_mouseb = 0;

  /* Key buffering */
  jtp_dx_n_keys_in_buffer = 0;

  /* Message polling */
  jtp_dx_n_polled_messages = 0;
  jtp_dx_polled_message_arrived = JTP_DX_NO_MESSAGE_POLLING;

  /* DirectDraw objects */
  jtp_dx_lpDD = NULL;
  jtp_dx_lpDDSPrimary = NULL;
  jtp_dx_lpDDPal = NULL;
  jtp_dx_colors = (PALETTEENTRY *)malloc(256*sizeof(PALETTEENTRY));

  /* DirectSound objects */
  jtp_dx_lpDS = NULL;
  jtp_dx_lpDSBPrimary = NULL;
  for (i = 0; i < JTP_DX_MAX_SOUNDS; i++)
    jtp_dx_lpDSBuffers[i] = NULL;
  jtp_dx_oldest_sound = 0;
  jtp_dx_cached_sounds = NULL;
  jtp_dx_oldest_cached_sound = 0;

  /* MCI objects */
  jtp_dx_mci_opened = 0; 
}





extern "C"
{

int jtp_DXGetch()
{
  int current_key;
  int i;
  
  /* Poll for a key until there's one in the buffer */
  while (jtp_dx_n_keys_in_buffer == 0)
  {
    jtp_dx_polled_messages[0] = WM_KEYUP;
    jtp_dx_polled_message_arrived = JTP_DX_POLLED_MESSAGE_NOT_ARRIVED;
    jtp_dx_n_polled_messages = 1;
    jtp_DXPollForMessage(1);
    jtp_dx_polled_message_arrived = JTP_DX_NO_MESSAGE_POLLING;
    jtp_dx_n_polled_messages = 0;
  }
  /* Remove key from buffer */
  current_key = jtp_dx_key_buffer[0];
  for (i = 0; i < jtp_dx_n_keys_in_buffer - 1; i++)
    jtp_dx_key_buffer[i] = jtp_dx_key_buffer[i+1];
  jtp_dx_n_keys_in_buffer--;
      
  return(current_key);
}

int jtp_DXKbHit()
{
  /* Just process any waiting events, then return */
  jtp_dx_polled_message_arrived = JTP_DX_NO_MESSAGE_POLLING;
  jtp_dx_n_polled_messages = 0;
  jtp_DXPollForMessage(0); 

  if (jtp_dx_n_keys_in_buffer > 0) return(1);
  return(0);
}

void jtp_DXReadMouse()
{
  /* Just process any waiting events, then return */
  jtp_dx_polled_message_arrived = JTP_DX_NO_MESSAGE_POLLING;
  jtp_dx_n_polled_messages = 0;
  jtp_DXPollForMessage(0); 
}

void jtp_DXProcessEvents()
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
  {
    if(!GetMessage (&msg, NULL, 0, 0)) return; /* msg.wParam; */
    TranslateMessage (&msg);
    DispatchMessage (&msg);        
  }
}

void jtp_DXEnterGraphicsMode(jtp_screen_t *newscreen)
{
  if(!jtp_DXInitWindow(jtp_dx_hThisInstance, jtp_dx_nCmdShow, newscreen->width, newscreen->height))
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXEnterGraphicMode/Check1] ERROR: Could not initialize window\n");
    exit(1);
  }
  if(!jtp_DXInitDirectDraw(jtp_dx_hwndMain, newscreen))
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXEnterGraphicMode/Check2] ERROR: Could not initialize DirectDraw\n");
    exit(1);
  }
  if (!jtp_DXCreatePrimarySurface())
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXEnterGraphicMode/Check3] ERROR: Could not create primary surface\n");
    exit(1);
  }
  if ((jtp_play_effects) && (!jtp_DXInitDirectSound(jtp_dx_hwndMain)))
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXEnterGraphicMode/Check4] ERROR: Could not initialize DirectSound\n");
    exit(1);
  }
}

void jtp_DXExitGraphicsMode()
{
  jtp_DXCleanUp();
}

void jtp_DXRecordColor(int cindex, int r, int g, int b)
{
  jtp_dx_colors[cindex].peRed = (r*255)/63;
  jtp_dx_colors[cindex].peGreen = (g*255)/63;
  jtp_dx_colors[cindex].peBlue = (b*255)/63;
}

void jtp_DXSetPalette()
{
  HRESULT ddrval;
  int i, j;
  char tempbuffer[200];

/*
  for (i = 0; i < 256; i++)
  {
    sprintf(tempbuffer, "%d %d %d\n", 
            jtp_dx_colors[i].peRed, jtp_dx_colors[i].peGreen, jtp_dx_colors[i].peBlue);
    jtp_DXError(tempbuffer);
  }
  jtp_DXError("256 entries listed.\n");
*/  

  if (!jtp_dx_lpDDPal)
  {
    ddrval = jtp_dx_lpDD->CreatePalette(
               DDPCAPS_8BIT | DDPCAPS_ALLOW256, 
               jtp_dx_colors, 
               &jtp_dx_lpDDPal, 
               NULL);
    if ((ddrval != DD_OK) || (!jtp_dx_lpDDPal))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXSetPalette/Check1] ERROR: Could not set palette\n");
      return;
    }
    jtp_dx_lpDDSPrimary->SetPalette(jtp_dx_lpDDPal);
    if (ddrval != DD_OK)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXSetPalette/Check2] ERROR: Could not set palette\n");
      return;
    }
  }
  else
  {
    ddrval = jtp_dx_lpDDPal->SetEntries(0, 0, 256, jtp_dx_colors);
    if (ddrval != DD_OK)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXSetPalette/Check3] ERROR: Could not set palette\n");
      return;
    }
  }
}


void jtp_DXRefreshRegion
(
  int x1, int y1, 
  int x2, int y2, 
  jtp_screen_t *newscreen
)
{
  HBITMAP         hbmTemp;
  RECT            rcRect;
  HRESULT         ddrval;
  HDC             hdc, hdcImage;
  DDSURFACEDESC   ddSurfaceInfo;
  unsigned char * ddSurfaceTable;
  int i;
  FILE * f;

  /* Clip edges */
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 >= newscreen->width) x2 = newscreen->width-1;
  if (y2 >= newscreen->height) y2 = newscreen->height-1;
  rcRect.top = y1; rcRect.left = x1;
  rcRect.bottom = y2; rcRect.right = x2;

  /* Get the rectangle to access as a table pointer */
  ddSurfaceInfo.dwSize = sizeof(ddSurfaceInfo);
  ddrval = jtp_dx_lpDDSPrimary->Lock(
             NULL/*&rcRect*/, 
             &ddSurfaceInfo, 
             DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, 
             NULL
           );
  if (ddrval == DD_OK)
  {
    /* Plot the selected region */
    ddSurfaceTable = (unsigned char *)ddSurfaceInfo.lpSurface;
    if (ddSurfaceInfo.dwHeight == 0) 
    { 
      return;
    }

    ddSurfaceTable += y1*ddSurfaceInfo.lPitch + x1;  
    for (i = y1; i <= y2; i++)
    {
      memcpy(ddSurfaceTable, newscreen->vpage + x1 + i*newscreen->width, x2-x1+1);
      ddSurfaceTable += ddSurfaceInfo.lPitch;
    }

    rcRect.top = y1; rcRect.left = x1;
    rcRect.bottom = y2; rcRect.right = x2;
    if ((ddrval = jtp_dx_lpDDSPrimary->Unlock(NULL/*&rcRect*/)) != DD_OK)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXRefreshRegion/Check1] ERROR: Could not unlock surface\n");
    }
  }
  else if (ddrval == DDERR_SURFACELOST)
  {
    jtp_DXRestoreAll();
    jtp_DXRefreshRegion(0, 0, newscreen->width-1, newscreen->height-1, newscreen);
  }
}

void jtp_DXRefresh(jtp_screen_t * newscreen)
{
  jtp_DXRefreshRegion(0, 0, newscreen->width-1, newscreen->height-1, newscreen);
}


void jtp_DXPlayMIDISong(char * midifilename)
{
  char tempbuffer[256];
  DWORD err;

  if (!jtp_play_music) return;

  if (jtp_dx_mci_opened)
  {
    /* Stop and close the MCI device */
    jtp_dx_mciGenericParms.dwCallback = (DWORD)jtp_dx_hwndMain;
    if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_STOP, MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciGenericParms)) != 0)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI stop failed\n");
      if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
      {
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
      }
      return;
    }

    /* jtp_dx_mciGenericParms.dwCallback = (DWORD)jtp_dx_hwndMain; */
    if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciGenericParms)) != 0)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI close failed.\n");
      if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
      {
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
      }
      return;
    }
    else jtp_dx_mci_opened = 0;
  }

  jtp_dx_mciOpenParms.dwCallback = (DWORD)jtp_dx_hwndMain;
  jtp_dx_mciOpenParms.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_SEQUENCER;
  jtp_dx_mciOpenParms.lpstrElementName = midifilename;
  if ((err = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_ELEMENT | MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciOpenParms)) != 0)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI open failed.\n");
    if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
    }
    return;
  }
  else jtp_dx_mci_opened = 1;    
  
  jtp_dx_mciPlayParms.dwCallback = (DWORD)jtp_dx_hwndMain;
  if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&jtp_dx_mciPlayParms)) != 0)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI play failed.\n");
    if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
    }
  }
}

void jtp_DXPlayCDTrack(char * midifilename)
{
  char tempbuffer[256];
  DWORD err;
  DWORD tracklength;
  int   nTrack;

  if (!jtp_play_music) return;

  /* Parse the track number from the given string */
  nTrack = atoi(midifilename);
  if (nTrack < 0) return;

  if (jtp_dx_mci_opened)
  {
    /* Stop and close the device, if it was already open */
    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXCleanUp/Note7] Stopping MCI device\n");
    jtp_dx_mciGenericParms.dwCallback = (DWORD)jtp_dx_hwndMain;
    if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_STOP, MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciGenericParms)) != 0)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI stop failed\n");
      if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
      {
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
      }
      return;
    }

    if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_CLOSE, MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciGenericParms)) != 0)
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayCDTrack/Check1] ERROR: MCI close failed.\n");
      if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
      {
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
        jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
      }
      return;
    }
    else jtp_dx_mci_opened = 0;
  }

  /* Open the CD audio device */
  jtp_dx_mciOpenParms.dwCallback = (DWORD)jtp_dx_hwndMain;
  jtp_dx_mciOpenParms.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  jtp_dx_mciOpenParms.lpstrElementName = NULL;
  if ((err = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciOpenParms)) != 0)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayCDTrack/Check2] ERROR: MCI open failed.\n");
    if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
    }
    return;
  }
  else jtp_dx_mci_opened = 1;    

  /* Set the time format to tracks, minutes, seconds and frames */
  jtp_dx_mciSetParms.dwCallback = (DWORD)jtp_dx_hwndMain;
  jtp_dx_mciSetParms.dwTimeFormat = (DWORD)MCI_FORMAT_TMSF;
  if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT | MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciSetParms)) != 0)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayCDTrack/Check3] ERROR: MCI set failed.\n");
    if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
    }
    return;
  }

  /* Find out the length (end position) of the requested track */
  jtp_dx_mciStatusParms.dwItem = MCI_STATUS_LENGTH;
  jtp_dx_mciStatusParms.dwTrack = nTrack;
  jtp_dx_mciStatusParms.dwReturn = 0;
  if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, (DWORD)(LPVOID)&jtp_dx_mciStatusParms)) != 0)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayCDTrack/Check4] ERROR: MCI status failed.\n");
    if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
    }
    return;
  }
  tracklength = jtp_dx_mciStatusParms.dwReturn;
  
  /* Play one complete track from the cd */
  jtp_dx_mciPlayParms.dwCallback = (DWORD)jtp_dx_hwndMain;
  jtp_dx_mciPlayParms.dwFrom = (DWORD)MCI_MAKE_TMSF(nTrack, 0, 0, 0);
  jtp_dx_mciPlayParms.dwTo = (DWORD)MCI_MAKE_TMSF(nTrack, MCI_MSF_MINUTE(tracklength), MCI_MSF_SECOND(tracklength), MCI_MSF_FRAME(tracklength));
  if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID)&jtp_dx_mciPlayParms)) != 0)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayCDTrack/Check5] ERROR: MCI play failed.\n");
    if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
    }
  }
}


void jtp_DXStopMusic()
{
  /* Stop any MIDI file or CD track playing */
}


int jtp_DXIsMusicPlaying()
{
  char tempbuffer[256];
  DWORD err;

  if ((!jtp_play_music) || (!jtp_dx_mci_opened)) return(0);

  jtp_dx_mciStatusParms.dwItem = MCI_STATUS_MODE;
  if ((err = mciSendCommand(jtp_dx_mciOpenParms.wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&jtp_dx_mciStatusParms)) != 0)
  {
    jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "[jtp_dirx.c/jtp_DXPlayMIDISong/Check1] ERROR: MCI status failed.\n");
    if (mciGetErrorString(err, &tempbuffer[0], sizeof(tempbuffer)))
    {
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "MCI error string: [");
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, tempbuffer);
      jtp_DXWriteLogMessage(JTP_DX_LOG_ERROR, "]\n");
    }
    return(0);
  }
  if (jtp_dx_mciStatusParms.dwReturn == MCI_MODE_PLAY) return(1);
  return(0);
}


void jtp_DXPlayWaveSound
(
  char * wavefilename,
  int samples_per_sec,
  int bits_per_sample,
  int nchannels
)
{
  FILE * f;
  int nbytes, nbytes2, nsamples;
  int cur_pos, i;
  int k;
  int sound_exists;
  char * samples;
  char * samples2;

  if (!jtp_play_effects) return;

  /* Check if the sound exists in the sound cache */
  sound_exists = 0;
  for (i = 0; i < JTP_DX_MAX_CACHED_SOUNDS; i++)
    if ((jtp_dx_cached_sounds[i].filename) &&
        (strcmp(wavefilename, jtp_dx_cached_sounds[i].filename) == 0))
    {
      sound_exists = 1;
      samples = jtp_dx_cached_sounds[i].samples;
      nbytes = jtp_dx_cached_sounds[i].length;
      break;
    }

  if (!sound_exists)
  {
    /* Open the file and get the file size */
    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXPlayWaveSound/Note1]: Opening file\n");
    f = fopen(wavefilename, "rb");
    if (f == NULL) return;
    cur_pos = ftell(f);
    fseek(f, 0, SEEK_END);
    nbytes = ftell(f);
    fseek(f, cur_pos, SEEK_SET);

    /* Allocate and read data, then close file */
    jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXPlayWaveSound/Note2]: Reading data\n");
    samples = (char *)malloc(nbytes*sizeof(char));
    fread(samples, sizeof(char), nbytes, f);
    fclose(f);

    /* Convert the sound into 44100Hz, 16-bit mono */
    if ((samples_per_sec == 22050) &&
        (bits_per_sample == 8) &&
        (nchannels == 1))
    {
      nbytes2 = nbytes*4;
      samples2 = (char *)malloc(nbytes2*sizeof(char));
      for (i = 0; i < nbytes; i++)
      {
        /*
         * A 16-bit sample is 2 bytes (most significant byte last),
         * and the value is signed
         */
        k = ((unsigned char *)samples)[i]; k -= 128;
        k *= 128;
        if (k < 0) k += 65536;
        ((unsigned char *)samples2)[i*4] = k % 256;
        ((unsigned char *)samples2)[i*4+1] = k / 256;
        samples2[i*4+2] = samples2[i*4];
        samples2[i*4+3] = samples2[i*4+1];
      }
      free(samples);
      samples = samples2; nbytes = nbytes2;
    }

    if (nbytes > JTP_DX_SOUND_BUFFER_SIZE)
      nbytes = JTP_DX_SOUND_BUFFER_SIZE;

    /* Place the sound in the sound cache */
    i = jtp_dx_oldest_cached_sound;
    free(jtp_dx_cached_sounds[i].filename);
    free(jtp_dx_cached_sounds[i].samples);
    jtp_dx_cached_sounds[i].filename = (char *)malloc(strlen(wavefilename)+1);
    strcpy(jtp_dx_cached_sounds[i].filename, wavefilename);
    jtp_dx_cached_sounds[i].samples = samples;
    jtp_dx_cached_sounds[i].length = nbytes;

    jtp_dx_oldest_cached_sound++;
    if (jtp_dx_oldest_cached_sound >= JTP_DX_MAX_CACHED_SOUNDS)
      jtp_dx_oldest_cached_sound = 0;
  }

  /* Play sound */
  jtp_DXWriteLogMessage(JTP_DX_LOG_DEBUG, "[jtp_dirx.c/jtp_DXPlayWaveSound/Note3]: Playing file\n");
  jtp_DXPlaySound(nbytes, samples);
}

} /* End of C interface */


#endif
