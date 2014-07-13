// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h



#include <Windows.h>

#include <al/alut.h>
#include <al/al.h>

#include <string.h>

#include "../resource/resmanager.h"
#include "../common/memorymanager.h"
#include "../common/3dmath.h"
#include "../main/main.h"
#include "soundlist.h"
// from the open AL SDK

#include "Vorbis\vorbisfile.h"

#include "musicfile.h"

static int g_bVorbisInit = 0;
static HINSTANCE g_hVorbisFileDLL = NULL;
static LUXImutex playmutex;
// Variables

/*extern int ov_clear(OggVorbis_File *vf);
extern int ov_open(FILE *f,OggVorbis_File *vf,char *initial,long ibytes);
extern int ov_open_callbacks(void *datasource, OggVorbis_File *vf,
    char *initial, long ibytes, ov_callbacks callbacks);

extern int ov_test(FILE *f,OggVorbis_File *vf,char *initial,long ibytes);
extern int ov_test_callbacks(void *datasource, OggVorbis_File *vf,
    char *initial, long ibytes, ov_callbacks callbacks);
extern int ov_test_open(OggVorbis_File *vf);

extern long ov_bitrate(OggVorbis_File *vf,int i);
extern long ov_bitrate_instant(OggVorbis_File *vf);
extern long ov_streams(OggVorbis_File *vf);
extern long ov_seekable(OggVorbis_File *vf);
extern long ov_serialnumber(OggVorbis_File *vf,int i);

extern ogg_int64_t ov_raw_total(OggVorbis_File *vf,int i);
extern ogg_int64_t ov_pcm_total(OggVorbis_File *vf,int i);
extern double ov_time_total(OggVorbis_File *vf,int i);

extern int ov_raw_seek(OggVorbis_File *vf,ogg_int64_t pos);
extern int ov_pcm_seek(OggVorbis_File *vf,ogg_int64_t pos);
extern int ov_pcm_seek_page(OggVorbis_File *vf,ogg_int64_t pos);
extern int ov_time_seek(OggVorbis_File *vf,double pos);
extern int ov_time_seek_page(OggVorbis_File *vf,double pos);

extern int ov_raw_seek_lap(OggVorbis_File *vf,ogg_int64_t pos);
extern int ov_pcm_seek_lap(OggVorbis_File *vf,ogg_int64_t pos);
extern int ov_pcm_seek_page_lap(OggVorbis_File *vf,ogg_int64_t pos);
extern int ov_time_seek_lap(OggVorbis_File *vf,double pos);
extern int ov_time_seek_page_lap(OggVorbis_File *vf,double pos);

extern ogg_int64_t ov_raw_tell(OggVorbis_File *vf);
extern ogg_int64_t ov_pcm_tell(OggVorbis_File *vf);
extern double ov_time_tell(OggVorbis_File *vf);

extern vorbis_info *ov_info(OggVorbis_File *vf,int link);
extern vorbis_comment *ov_comment(OggVorbis_File *vf,int link);

extern long ov_read_float(OggVorbis_File *vf,float ***pcm_channels,int samples,
        int *bitstream);
extern long ov_read(OggVorbis_File *vf,char *buffer,int length,
        int bigendianp,int word,int sgned,int *bitstream);
extern int ov_crosslap(OggVorbis_File *vf1,OggVorbis_File *vf2);

extern int ov_halfrate(OggVorbis_File *vf,int flag);
extern int ov_halfrate_p(OggVorbis_File *vf);*/
typedef int OVCLEAR (OggVorbis_File *vf);
typedef long OVREAD (OggVorbis_File *vf,char *buffer,int length,
        int bigendianp,int word,int sgned,int *bitstream);
typedef ogg_int64_t OVPCMTOTAL (OggVorbis_File *vf,int i);
typedef vorbis_info *OVINFO(OggVorbis_File *vf,int link);
typedef vorbis_comment *OVCOMMENT(OggVorbis_File *vf,int link);
typedef int OVOPENCALLBACKS (void *datasource, OggVorbis_File *vf,
    char *initial, long ibytes, ov_callbacks callbacks);
typedef int OVTIMESEEK(OggVorbis_File *vf,double pos);
typedef double OVTIMETELL(OggVorbis_File *vf);


static OVCLEAR      *fn_ov_clear = NULL;
static OVREAD     *fn_ov_read = NULL;
static OVPCMTOTAL   *fn_ov_pcm_total = NULL;
static OVINFO     *fn_ov_info = NULL;
static OVCOMMENT    *fn_ov_comment = NULL;
static OVOPENCALLBACKS  *fn_ov_open_callbacks = NULL;
static OVTIMESEEK   *fn_ov_time_seek = NULL;
static OVTIMETELL   *fn_ov_time_tell = NULL;






static void Swap(short *s1, short *s2)
{
  short sTemp = *s1;
  *s1 = *s2;
  *s2 = sTemp;
}

static size_t ov_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
  return fread(ptr, size, nmemb, (FILE*)datasource);
}

static int ov_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
  return fseek((FILE*)datasource, (long)offset, whence);
}

static int ov_close_func(void *datasource)
{
   return fclose((FILE*)datasource);
}

static long ov_tell_func(void *datasource)
{
  return ftell((FILE*)datasource);
}

static unsigned long DecodeOggVorbis(OggVorbis_File *psOggVorbisFile, char *pDecodeBuffer, unsigned long ulBufferSize, unsigned long ulChannels)
{
  int current_section;
  long lDecodeSize;
  unsigned long ulSamples;
  short *pSamples;

  unsigned long ulBytesDone = 0;
  while (1)
  {
    lDecodeSize = fn_ov_read(psOggVorbisFile, pDecodeBuffer + ulBytesDone, ulBufferSize - ulBytesDone, 0, 2, 1, &current_section);
    if (lDecodeSize > 0)
    {
      ulBytesDone += lDecodeSize;

      if (ulBytesDone >= ulBufferSize)
        break;
    }
    else
    {
      break;
    }
  }

  // Mono, Stereo and 4-Channel files decode into the same channel order as WAVEFORMATEXTENSIBLE,
  // however 6-Channels files need to be re-ordered
  if (ulChannels == 6)
  {
    pSamples = (short*)pDecodeBuffer;
    for (ulSamples = 0; ulSamples < (ulBufferSize>>1); ulSamples+=6)
    {
      // WAVEFORMATEXTENSIBLE Order : FL, FR, FC, LFE, RL, RR
      // OggVorbis Order            : FL, FC, FR,  RL, RR, LFE
      Swap(&pSamples[ulSamples+1], &pSamples[ulSamples+2]);
      Swap(&pSamples[ulSamples+3], &pSamples[ulSamples+5]);
      Swap(&pSamples[ulSamples+4], &pSamples[ulSamples+5]);
    }
  }

  return ulBytesDone;
}






#define NUMBUFFERS              (4)
#define SERVICE_UPDATE_PERIOD (20)

#define TEST_OGGVORBIS_FILE   "stereo.ogg"


static  ALuint        uiBuffers[NUMBUFFERS];
static  ALuint        uiSource;
static  ALuint      uiBuffer;
static  ALint     iState;
static  ALint     iLoop;
static  ALint     iBuffersProcessed, iTotalBuffersProcessed, iQueuedBuffers;
static  unsigned long ulFrequency = 0;
static  unsigned long ulFormat = 0;
static  unsigned long ulChannels = 0;
static  unsigned long ulBufferSize;
static  unsigned long ulBytesWritten;
static  char      *pDecodeBuffer;

static  ov_callbacks  sCallbacks;
static  OggVorbis_File  sOggVorbisFile;
static  vorbis_info   *psVorbisInfo;
static  FILE *pOggVorbisFile;


int g_playnow = 0;

static void finishplaying();

void Music_load(const char *oggfile)
{
  finishplaying();
  g_playnow = 0;
  g_LuxiniaWinMgr.luxiLockMutex(playmutex);

  pOggVorbisFile = fopen(oggfile, "rb");
  if (fn_ov_open_callbacks(pOggVorbisFile, &sOggVorbisFile, NULL, 0, sCallbacks) != 0)
  {
    if (pOggVorbisFile) fclose(pOggVorbisFile);
    pOggVorbisFile = NULL;
    bprintf("Error loading OGG file\n");
    g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
    return;
  }

  {
    // Get some information about the file (Channels, Format, and Frequency)
    psVorbisInfo = fn_ov_info(&sOggVorbisFile, -1);
    if (psVorbisInfo)
    {
      ulFrequency = psVorbisInfo->rate;
      ulChannels = psVorbisInfo->channels;
      if (psVorbisInfo->channels == 1)
      {
        ulFormat = AL_FORMAT_MONO16;
        // Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
        ulBufferSize = ulFrequency >> 1;
        // IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
        ulBufferSize -= (ulBufferSize % 2);
      }
      else if (psVorbisInfo->channels == 2)
      {
        ulFormat = AL_FORMAT_STEREO16;
        // Set BufferSize to 250ms (Frequency * 4 (16bit stereo) divided by 4 (quarter of a second))
        ulBufferSize = ulFrequency;
        // IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
        ulBufferSize -= (ulBufferSize % 4);
      }
      else if (psVorbisInfo->channels == 4)
      {
        ulFormat = alGetEnumValue("AL_FORMAT_QUAD16");
        // Set BufferSize to 250ms (Frequency * 8 (16bit 4-channel) divided by 4 (quarter of a second))
        ulBufferSize = ulFrequency * 2;
        // IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
        ulBufferSize -= (ulBufferSize % 8);
      }
      else if (psVorbisInfo->channels == 6)
      {
        ulFormat = alGetEnumValue("AL_FORMAT_51CHN16");
        // Set BufferSize to 250ms (Frequency * 12 (16bit 6-channel) divided by 4 (quarter of a second))
        ulBufferSize = ulFrequency * 3;
        // IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
        ulBufferSize -= (ulBufferSize % 12);
      }
    }

    if (ulFormat != 0)
    {
      // Allocate a buffer to be used to store decoded data for all Buffers
      pDecodeBuffer = (char*)MEMORY_GLOBAL_MALLOC(ulBufferSize);
      if (!pDecodeBuffer)
      {
        bprintf("Failed to allocate memory for decoded OggVorbis data\n");
        fn_ov_clear(&sOggVorbisFile);

        fclose(pOggVorbisFile);
        pOggVorbisFile = NULL;
        g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
        return;
      }

      // Generate some AL Buffers for streaming
      alGenBuffers( NUMBUFFERS, uiBuffers );

      // Generate a Source to playback the Buffers
      alGenSources( 1, &uiSource );

      // Fill all the Buffers with decoded audio data from the OggVorbis file
      for (iLoop = 0; iLoop < NUMBUFFERS; iLoop++)
      {
        ulBytesWritten = DecodeOggVorbis(&sOggVorbisFile, pDecodeBuffer, ulBufferSize, ulChannels);
        if (ulBytesWritten)
        {
          alBufferData(uiBuffers[iLoop], ulFormat, pDecodeBuffer, ulBytesWritten, ulFrequency);
          alSourceQueueBuffers(uiSource, 1, &uiBuffers[iLoop]);
        }
      }

      // Start playing source


      iTotalBuffersProcessed = 0;
////////////////////////////////////////////////////////////////


    }
    else
    {

      fclose(pOggVorbisFile);
      pOggVorbisFile = NULL;
      bprintf("Failed to find format information, or unsupported format\n");
    }

    // Close OggVorbis stream
  }
  g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
}

static void finishplaying()
{
  if (!pDecodeBuffer) return;
  g_LuxiniaWinMgr.luxiLockMutex(playmutex);
  // Stop the Source and clear the Queue
  alSourceStop(uiSource);
  alSourcei(uiSource, AL_BUFFER, 0);


  fclose(pOggVorbisFile);

  fn_ov_clear(&sOggVorbisFile);

  MEMORY_GLOBAL_FREE(pDecodeBuffer);
  pDecodeBuffer = NULL;

  // Clean up buffers and sources
  alDeleteSources( 1, &uiSource );
  alDeleteBuffers( NUMBUFFERS, uiBuffers );
  g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
}

static void playupdate()
{
  if (!pDecodeBuffer) return;
  if (!g_playnow) return;
  g_LuxiniaWinMgr.luxiLockMutex(playmutex);
  // Request the number of OpenAL Buffers have been processed (played) on the Source
  iBuffersProcessed = 0;
  alGetSourcei(uiSource, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

  // Keep a running count of number of buffers processed (for logging purposes only)
  iTotalBuffersProcessed += iBuffersProcessed;
//  bprintf("Buffers Processed %d\r", iTotalBuffersProcessed);

  // For each processed buffer, remove it from the Source Queue, read next chunk of audio
  // data from disk, fill buffer with new data, and add it to the Source Queue
  while (iBuffersProcessed)
  {
    // Remove the Buffer from the Queue.  (uiBuffer contains the Buffer ID for the unqueued Buffer)
    uiBuffer = 0;
    alSourceUnqueueBuffers(uiSource, 1, &uiBuffer);

    // Read more audio data (if there is any)
    ulBytesWritten = DecodeOggVorbis(&sOggVorbisFile, pDecodeBuffer, ulBufferSize, ulChannels);
    if (ulBytesWritten)
    {
      alBufferData(uiBuffer, ulFormat, pDecodeBuffer, ulBytesWritten, ulFrequency);
      alSourceQueueBuffers(uiSource, 1, &uiBuffer);
    }

    iBuffersProcessed--;
  }

  // Check the status of the Source.  If it is not playing, then playback was completed,
  // or the Source was starved of audio data, and needs to be restarted.
  alGetSourcei(uiSource, AL_SOURCE_STATE, &iState);
  if (iState != AL_PLAYING)
  {
    // If there are Buffers in the Source Queue then the Source was starved of audio
    // data, so needs to be restarted (because there is more audio data to play)
    alGetSourcei(uiSource, AL_BUFFERS_QUEUED, &iQueuedBuffers);
    if (iQueuedBuffers)
    {
      alSourcePlay(uiSource);
      g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
    }
    else
    {
      g_playnow = 0;
      g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
    }
  } else
    g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
}


double Music_getTime()
{
  double d = 0;
  g_LuxiniaWinMgr.luxiLockMutex(playmutex);
  if (pDecodeBuffer)
    d = fn_ov_time_tell(&sOggVorbisFile);
  g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
  return d;
}

void Music_setTime(double time)
{
  if (!pDecodeBuffer) return;
  g_LuxiniaWinMgr.luxiLockMutex(playmutex);
  fn_ov_time_seek(&sOggVorbisFile,time);
  g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
}

void Music_setGain(double g)
{
  if (!pDecodeBuffer) return;
  g = g>10?10:g<0?0:g;
  g_LuxiniaWinMgr.luxiLockMutex(playmutex);
  alSourcef (uiSource, AL_GAIN, g);
  g_LuxiniaWinMgr.luxiUnlockMutex(playmutex);
}


void Music_stop()
{
  if (!pDecodeBuffer) return;
  alSourcePause(uiSource);
  g_playnow = 0;
}


void Music_play()
{
  if (!pDecodeBuffer) return;
  alSourcePlay(uiSource);
  g_playnow = 1;
}

int Music_isPlaying()
{
  return g_playnow;
}

void  Music_update (void *arg)
{

  if (!g_bVorbisInit)
  {
    return;
  }
  while (1)
  {
    Sleep(100);
    playupdate();
  }






    return;
}





int Music_getFileInfo (const char *file, const char ** to)
{
  static OggVorbis_File ogg;
  static vorbis_comment *comment;
  static char * tx = NULL;
  static int n;
  FILE *h;

  h = fopen(file, "rb");

  if(!h || fn_ov_open_callbacks(h, &ogg, NULL, 0, sCallbacks))
    return -1;

  comment = fn_ov_comment(&ogg, -1);
  if (tx) {
    MEMORY_GLOBAL_FREE(tx);
    tx = NULL;
  }
  *to = NULL;
  n = 0;
  if (comment) {
    int len = 1;
    int i;
    char *ptr;
    for (i=0;i<comment->comments;i++)
      len+=comment->comment_lengths[i]+1;
    ptr = tx = MEMORY_GLOBAL_MALLOC(len);
    for (i=0;i<comment->comments;i++) {
      int n;
      for (n=0;n<comment->comment_lengths[i];n++)
      {
        *ptr = comment->user_comments[i][n];
        ptr++;
      }
      *ptr = 0;
      ptr++;
    }
    *to = tx;
    n = comment->comments;
  }


  fn_ov_clear(&ogg);
  fclose(h);

  return n;
}

void InitVorbisFile()
{
  if (g_bVorbisInit)
    return;

  // Try and load Vorbis DLLs (VorbisFile.dll will load ogg.dll and vorbis.dll)
  g_hVorbisFileDLL = LoadLibrary("vorbisfile.dll");
  if (g_hVorbisFileDLL)
  {
    fn_ov_clear = (OVCLEAR*)GetProcAddress(g_hVorbisFileDLL, "ov_clear");
    fn_ov_read = (OVREAD*)GetProcAddress(g_hVorbisFileDLL, "ov_read");
    fn_ov_pcm_total = (OVPCMTOTAL*)GetProcAddress(g_hVorbisFileDLL, "ov_pcm_total");
    fn_ov_info = (OVINFO*)GetProcAddress(g_hVorbisFileDLL, "ov_info");
    fn_ov_comment = (OVCOMMENT*)GetProcAddress(g_hVorbisFileDLL, "ov_comment");
    fn_ov_open_callbacks = (OVOPENCALLBACKS*)GetProcAddress(g_hVorbisFileDLL, "ov_open_callbacks");
    fn_ov_time_seek = (OVTIMESEEK*)GetProcAddress(g_hVorbisFileDLL, "ov_time_seek");
    fn_ov_time_tell = (OVTIMETELL*)GetProcAddress(g_hVorbisFileDLL, "ov_time_tell");

    if (fn_ov_clear && fn_ov_read && fn_ov_pcm_total && fn_ov_info &&
      fn_ov_comment && fn_ov_open_callbacks)
    {
      g_bVorbisInit = 1;



      sCallbacks.read_func = ov_read_func;
      sCallbacks.seek_func = ov_seek_func;
      sCallbacks.close_func = ov_close_func;
      sCallbacks.tell_func = ov_tell_func;

      playmutex = g_LuxiniaWinMgr.luxiCreateMutex();
      g_LuxiniaWinMgr.luxiCreateThread(Music_update,NULL);
    }
    else {
      bprintf("Warning: Vorbisfile support could not be initalized correctly, OGG playback will not work\n");
    }
  }
  else {
    bprintf("Warning: Vorbisfile support could not be loaded, OGG playback will not work\n");
  }
}

void ShutdownVorbisFile()
{
  finishplaying();
  if (g_hVorbisFileDLL)
  {
    FreeLibrary(g_hVorbisFileDLL);
    g_hVorbisFileDLL = NULL;
  }
  g_LuxiniaWinMgr.luxiDestroyMutex(playmutex);
  g_bVorbisInit = 0;
}



// end of copy & paste from openal
