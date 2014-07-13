// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "fnpublish.h"
#include "../common/3dmath.h"
#include "../resource/resmanager.h"
#include "../sound/soundlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "../main/main.h"
#include <Windows.h>
#include <mmsystem.h>

#include "../sound/musicfile.h"
// Published here:
// LUXI_CLASS_SOUND
// LUXI_CLASS_SOUNDNODE

static int PubSound_load (PState pstate,PubFunction_t *fn, int n)
{
  char *path;
  int snd;

  if (n!=1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_STRING,&path)!=1)
    return FunctionPublish_returnError(pstate,"Argument 1 required and must be string");
  Main_startTask(LUX_TASK_RESOURCE);
  snd = ResData_addSound(path);
  Main_endTask();
  if (snd==-1)
    return FunctionPublish_returnError(pstate,"Soundfile loading failed");

  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_SOUND,(void*)snd);
}

static int PubSound_play (PState pstate,PubFunction_t *fn, int n)
{
  int snd;
  SoundlxListNode_t *node;

  if (n!=1 || FunctionPublish_getArg(pstate,1,LUXI_CLASS_SOUND, &snd)!=1)
    return FunctionPublish_returnError(pstate,"Argument 1 required and must be sound");

  if (g_nosound)
    return 0;

  node = SoundList_addResource(snd);
  return FunctionPublish_returnBool(pstate,SoundlxListNode_play(node));
}

static int PubSoundList_new (PState pstate,PubFunction_t *fn, int n)
{
  SoundlxListNode_t *node;
  int snd;

  FNPUB_CHECKOUT(pstate,n,0,LUXI_CLASS_SOUND,snd);

//  node = SoundList_addResource(snd);
  node = SoundlxListNode_new(ResData_getSound(snd));
  if (node == NULL)
    return FunctionPublish_returnError(pstate,"Couldn't initialize soundnode");

  Reference_makeVolatile(node->ref);
  return FunctionPublish_setRet(pstate,1,LUXI_CLASS_SOUNDNODE,REF2VOID(node->ref));
}

enum {
  SL_LISTENERPOS,
  SL_ORIENTATION,
  SL_VELOCITY,
  SL_VELOCITYSCALE,
  SL_POSITIONSCALE,
};

static int PubSoundListener_param (PState pstate,PubFunction_t *fn, int n) {
  float f[6];
  int read;
  SoundListener_t *l;

  read=FunctionPublish_getArg(pstate,6,FNPUB_TOVECTOR4(f),FNPUB_TOVECTOR2((&f[4])));

  l = SoundList_getListener();

  if (n==0)
    switch ((int)fn->upvalue) {
      case SL_LISTENERPOS:
        return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(l->position));
      break;
      case SL_ORIENTATION:
        return FunctionPublish_setRet(pstate,
          6,FNPUB_FROMVECTOR3(l->orientation),FNPUB_FROMVECTOR3((&l->orientation[3])));
      break;
      case SL_VELOCITY:
        return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(l->velocity));
      break;
      case SL_VELOCITYSCALE:
        return FunctionPublish_returnFloat(pstate,l->velocityScale);
      break;
      case SL_POSITIONSCALE:
        return FunctionPublish_returnFloat(pstate,l->positionScale);
      break;
    }
  else
    switch ((int)fn->upvalue) {
      case SL_LISTENERPOS:
        if (read<3) return FunctionPublish_returnError(pstate,"need 3 floats");
        lxVector3Copy(l->position,f);
      break;
      case SL_ORIENTATION:
        if (read<6) return FunctionPublish_returnError(pstate,"need 6 floats");
        lxVector3Copy(l->orientation,f);
        lxVector3Copy(&l->orientation[3],&f[3]);
      break;
      case SL_VELOCITY:
        if (read<3) return FunctionPublish_returnError(pstate,"need 3 floats");
        lxVector3Copy(l->velocity,f);
      break;
      case SL_VELOCITYSCALE:
        if (read<1) return FunctionPublish_returnError(pstate,"float required");
        l->velocityScale = f[0];
      break;
      case SL_POSITIONSCALE:
        if (read<1) return FunctionPublish_returnError(pstate,"float required");
        l->positionScale = f[0];
      break;
    }
  return 0;
}

enum {
  SP_PITCH,
  SP_GAIN,
  SP_LOOP,
  SP_PLAY,
  SP_STOP,
  SP_REWIND,
  SP_PAUSE,
  SP_REFDIST,
  SP_VELOCITY,
  SP_POSITION,
  SP_DELETE,
  SP_ISPLAYING,
  SP_AUTODELETE,
  SP_DELETEALL,
  SP_UPDATEPARAM,
};

static int PubSoundNode_param (PState pstate,PubFunction_t *fn, int n)
{
  SoundlxListNode_t *node;
  float f[4];
  Reference ref;
  int read;

  if ((int)fn->upvalue==SP_DELETEALL) {
    SoundList_reset(); return 0;
  }
  FNPUB_CHECKOUTREF(pstate,n,0,LUXI_CLASS_SOUNDNODE,ref,node);
  if ((n<1) || (read=FunctionPublish_getArg(pstate,5,LUXI_CLASS_SOUNDNODE,&ref,FNPUB_TOVECTOR4(f)))<1)
    return FunctionPublish_returnError(pstate,"Argument 1 must be soundnode");

  if (n == 1) {
    switch ((int)fn->upvalue) {
      case SP_DELETE: Reference_free(ref); return 0;
      case SP_PITCH: return FunctionPublish_returnFloat(pstate,SoundlxListNode_getPitch(node));
      case SP_GAIN: return FunctionPublish_returnFloat(pstate,SoundlxListNode_getGain(node));
      case SP_LOOP: return FunctionPublish_returnBool(pstate,SoundlxListNode_getLooping(node));
      case SP_STOP:   if (g_nosound)return 0; SoundlxListNode_stop(node); return 0;
      case SP_PLAY: if (g_nosound)return 0; return FunctionPublish_returnBool(pstate,SoundlxListNode_play(node));
      case SP_REWIND: if (g_nosound)return 0; SoundlxListNode_rewind(node); return 0;
      case SP_PAUSE:  if (g_nosound)return 0; SoundlxListNode_pause(node); return 0;
      case SP_ISPLAYING: return FunctionPublish_returnBool(pstate,SoundlxListNode_isPlaying(node));
      case SP_REFDIST: return FunctionPublish_returnFloat(pstate,SoundlxListNode_getReferenceDistance(node));
      case SP_AUTODELETE: return FunctionPublish_returnBool(pstate,node->autodelete);
      case SP_UPDATEPARAM: SoundlxListNode_update(node); return 0;
      case SP_VELOCITY:
        SoundlxListNode_getVelocity(node,f);
        return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(f));
      case SP_POSITION:
        SoundlxListNode_getPosition(node,f);
        return FunctionPublish_setRet(pstate,3,FNPUB_FROMVECTOR3(f));
    }
    return 0;
  }

  n = LUX_MIN(n,read);

  switch ((int)fn->upvalue) {
    case SP_AUTODELETE: if (n>1) {node->autodelete = (f[0]!=0); return 0; } break;
    case SP_PITCH: if (n>1) {SoundlxListNode_setPitch(node,f[0]); return 0; } break;
    case SP_GAIN: if (n>1) {SoundlxListNode_setGain(node,f[0]); return 0; } break;
    case SP_LOOP: if (n>1) {SoundlxListNode_setLooping(node,f[0]!=0); return 0; } break;
    case SP_REFDIST:if (n>1) {SoundlxListNode_setReferenceDistance(node,f[0]); return 0; } break;
    case SP_VELOCITY: if (n>3) {SoundlxListNode_setVelocity(node,f); return 0;} break;
    case SP_POSITION: if (n>3) {SoundlxListNode_setPosition(node,f); return 0;} break;
  }

  return FunctionPublish_returnError(pstate,"Invalid number or types of arguments");
}


enum {
  SC_DEVICELIST,
  SC_DEFAULTDEVICE,
  SC_OPENDEVICE,
  SC_STARTCAPTURE,
  SC_CHECKBUFFER,
  SC_STOPCAPTURE,
};
#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct tWAVEFORMATEX
{
    WORD    wFormatTag;
    WORD    nChannels;
    DWORD   nSamplesPerSec;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;
    WORD    wBitsPerSample;
    WORD    cbSize;
} WAVEFORMATEX;

#endif /* _WAVEFORMATEX_ */
typedef struct
{
  char      szRIFF[4];
  long      lRIFFSize;
  char      szWave[4];
  char      szFmt[4];
  long      lFmtSize;
  WAVEFORMATEX  wfex;
  char      szData[4];
  long      lDataSize;
} WAVEHEADER;

typedef struct {
  ALCdevice   *pDevice;
  ALCcontext    *pContext;
  ALCdevice   *pCaptureDevice;
  const ALCchar *szDefaultCaptureDevice;
  ALint     iSamplesAvailable;
  FILE        *pFile;
  int         iBuffersize;
  ALchar      *Buffer;
  WAVEHEADER    sWaveHeader;
  ALint     iDataSize;
  ALint     iSize;
  int       stop;
} SoundCapture;

void  ThreadedRecorder (void *arg)
{
  SoundCapture *sc = (SoundCapture*)arg;
  alcCaptureStart(sc->pCaptureDevice);
  while (!sc->stop)
  {
    g_LuxiniaWinMgr.luxiSleep(0.001);

    // Find out how many samples have been captured
    alcGetIntegerv(sc->pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &sc->iSamplesAvailable);


    // When we have enough data to fill our BUFFERSIZE byte buffer, grab the samples
    if (sc->iSamplesAvailable > (sc->iBuffersize / sc->sWaveHeader.wfex.nBlockAlign))
    {
      // Consume Samples
      alcCaptureSamples(sc->pCaptureDevice, sc->Buffer, sc->iBuffersize / sc->sWaveHeader.wfex.nBlockAlign);

      // Write the audio data to a file
      fwrite(sc->Buffer, sc->iBuffersize, 1, sc->pFile);

      // Record total amount of data recorded
      sc->iDataSize += sc->iBuffersize;
    }
  }
  alcCaptureStop(sc->pCaptureDevice);

  alcGetIntegerv(sc->pCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &sc->iSamplesAvailable);
  while (sc->iSamplesAvailable>0)
  {
    if (sc->iSamplesAvailable > (sc->iBuffersize / sc->sWaveHeader.wfex.nBlockAlign))
    {
      alcCaptureSamples(sc->pCaptureDevice, sc->Buffer, sc->iBuffersize / sc->sWaveHeader.wfex.nBlockAlign);
      fwrite(sc->Buffer, sc->iBuffersize, 1, sc->pFile);
      sc->iSamplesAvailable -= (sc->iBuffersize / sc->sWaveHeader.wfex.nBlockAlign);
      sc->iDataSize += sc->iBuffersize;
    }
    else
    {
      alcCaptureSamples(sc->pCaptureDevice, sc->Buffer, sc->iSamplesAvailable);
      fwrite(sc->Buffer, sc->iSamplesAvailable * sc->sWaveHeader.wfex.nBlockAlign, 1, sc->pFile);
      sc->iDataSize += sc->iSamplesAvailable * sc->sWaveHeader.wfex.nBlockAlign;
      sc->iSamplesAvailable = 0;
    }
  }

  // Fill in Size information in Wave Header
  fseek(sc->pFile, 4, SEEK_SET);
  sc->iSize = sc->iDataSize + sizeof(WAVEHEADER) - 8;
  fwrite(&sc->iSize, 4, 1, sc->pFile);
  fseek(sc->pFile, 42, SEEK_SET);
  fwrite(&sc->iDataSize, 4, 1, sc->pFile);

  fclose(sc->pFile);

  // Close the Capture Device
  alcCaptureCloseDevice(sc->pCaptureDevice);
  sc->pCaptureDevice = NULL;
  MEMORY_GLOBAL_FREE(sc->Buffer);
}

int PubClass_SoundCapture (PState pstate,PubFunction_t *fn, int n)
{
  static int init = 0;
  static int error = 1;
  static SoundCapture sc;
  static LUXIthread thread;

  if (init && error) {
    return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,0,LUXI_CLASS_STRING,"previous initialization failed");
  }
  if (!init) {
    init = 1;
    sc.pContext = alcGetCurrentContext();
    sc.pDevice = alcGetContextsDevice(sc.pContext);
    if (alcIsExtensionPresent(sc.pDevice, "ALC_EXT_CAPTURE") == AL_FALSE)
    {
      return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,0,LUXI_CLASS_STRING,
        "initialization failed, extension not supported");
    }
    sc.szDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
    error = 0;
    sc.pCaptureDevice = NULL;
    sc.Buffer = NULL;
    sc.iSamplesAvailable;
    sc.iDataSize = 0;
  }

  switch ((int)fn->upvalue) {
    case SC_DEFAULTDEVICE:
      return FunctionPublish_returnString(pstate,sc.szDefaultCaptureDevice);
    case SC_DEVICELIST: {
      const ALCchar *list = alcGetString(NULL,ALC_CAPTURE_DEVICE_SPECIFIER);
      // the list is specified: NULL termination, entry, double NULL, end of list
      int i=0,size=0;
      if (list == NULL) return 0;

      while (list[i]) {
        FunctionPublish_setNRet(pstate,size++,LUXI_CLASS_STRING,(void*)&list[i]);
        while (list[i++]);
      }
      return size;
    }
    case SC_OPENDEVICE: {
      int freq;
      int format;
      int iformat;
      int bsize;
      char *filename;
      char *capto = (char*)sc.szDefaultCaptureDevice;
      if (n!=4 || FunctionPublish_getArg(pstate,5,LUXI_CLASS_STRING,&filename,LUXI_CLASS_INT,
        &freq,LUXI_CLASS_INT,&format,LUXI_CLASS_INT,&bsize,LUXI_CLASS_STRING,&filename)<4)
        return FunctionPublish_returnError(pstate,"expected filename, samplefrequence, format and buffersize as 3 ints");
      if (sc.pCaptureDevice)
        return FunctionPublish_returnError(pstate,"only one open device per time allowed");
      switch (format) {
        case 0: iformat = AL_FORMAT_MONO8; break;
        case 1: iformat = AL_FORMAT_MONO16; break;
        case 2: iformat = AL_FORMAT_STEREO8; break;
        case 3: iformat = AL_FORMAT_STEREO16; break;
        default: return FunctionPublish_returnError(pstate,"invalid format number must be 0,1,2 or 3");
      }
      if (bsize<0)
        return FunctionPublish_returnError(pstate,"Invalid buffer size, must be >0");
      sc.pFile = fopen(filename, "wb");
      if (sc.pFile==NULL)
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,0,LUXI_CLASS_STRING,"could not open file");
      sc.iBuffersize = bsize;
      sc.Buffer = MEMORY_GLOBAL_MALLOC(bsize);
      if (sc.Buffer==NULL)
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,0,LUXI_CLASS_STRING,"memory allocation failed");

      sc.pCaptureDevice = alcCaptureOpenDevice(capto, freq, iformat, bsize);

      if (sc.pCaptureDevice == NULL) {
        MEMORY_GLOBAL_FREE(sc.Buffer);
        return FunctionPublish_setRet(pstate,2,LUXI_CLASS_BOOLEAN,0,LUXI_CLASS_STRING,"opening device failed");
      }

      sprintf(sc.sWaveHeader.szRIFF, "RIFF");
      sc.sWaveHeader.lRIFFSize = 0;
      sprintf(sc.sWaveHeader.szWave, "WAVE");
      sprintf(sc.sWaveHeader.szFmt, "fmt ");
      sc.sWaveHeader.lFmtSize = sizeof(WAVEFORMATEX);
      sc.sWaveHeader.wfex.nChannels = format/2+1;
      sc.sWaveHeader.wfex.wBitsPerSample = format*8+8;
      sc.sWaveHeader.wfex.wFormatTag = WAVE_FORMAT_PCM;
      sc.sWaveHeader.wfex.nSamplesPerSec = freq;
      sc.sWaveHeader.wfex.nBlockAlign = sc.sWaveHeader.wfex.nChannels * sc.sWaveHeader.wfex.wBitsPerSample / 8;
      sc.sWaveHeader.wfex.nAvgBytesPerSec = sc.sWaveHeader.wfex.nSamplesPerSec * sc.sWaveHeader.wfex.nBlockAlign;
      sc.sWaveHeader.wfex.cbSize = 0;
      sprintf(sc.sWaveHeader.szData, "data");
      sc.sWaveHeader.lDataSize = 0;

      fwrite(&sc.sWaveHeader, sizeof(WAVEHEADER), 1, sc.pFile);

      return FunctionPublish_returnBool(pstate,(char)(sc.pCaptureDevice!=NULL));
    }
    case SC_STARTCAPTURE:
      if (sc.pCaptureDevice==NULL)
        return FunctionPublish_returnError(pstate,"You need to open the capture device first");
      sc.stop = 0;
      thread = g_LuxiniaWinMgr.luxiCreateThread(ThreadedRecorder,&sc);
      return FunctionPublish_returnBool(pstate,thread>0);
    case SC_STOPCAPTURE:
      if (thread<1)
        return FunctionPublish_returnError(pstate,"No running recording");

      sc.stop = 1;
      g_LuxiniaWinMgr.luxiWaitThread( thread, LUXI_WAIT);

      return 0;
  }
  return 0;
}

enum {
  SNDI_DEVICELIST,
  SNDI_MONOCOUNT,
  SNDI_STEREOCOUNT,
  SNDI_SETDEVICE,
  SNDI_CURRENTDEVICE,
};

PubClass_Sound_getInfo (PState pstate,PubFunction_t *fn, int n)
{
  switch ((int)fn->upvalue) {
    case SNDI_SETDEVICE:
      {
        char *devname;
        if (n<1 || !FunctionPublish_getArg(pstate,1,LUXI_CLASS_STRING,&devname))
          SoundList_setDevice(NULL);
        else
          SoundList_setDevice(devname);
        return FunctionPublish_returnString(pstate,Sound_getCurrentDevice ());
      }
    case SNDI_CURRENTDEVICE:
      return FunctionPublish_returnString(pstate,Sound_getCurrentDevice ());
    case SNDI_MONOCOUNT:
      return FunctionPublish_returnInt(pstate,Sound_getMonoSources ());
    case SNDI_STEREOCOUNT:
      return FunctionPublish_returnInt(pstate,Sound_getStereoSources ());
    case SNDI_DEVICELIST: {
      const char * devs = Sound_getDevicelist ();
      n = 0;

      while (*devs) {
        FunctionPublish_setNRet(pstate,n, LUXI_CLASS_STRING, (void *)devs);
        n++;
        while (*devs) devs++;
        devs++;
      }
      return n;
    }
  }
  return 0;
}

int PubMusic_load (PState pstate,PubFunction_t *fn, int n)
{
  const char *file;
  if (n<1 || !FunctionPublish_getArg(pstate,1,LUXI_CLASS_STRING,&file))
    return FunctionPublish_returnError(pstate,"a string is required");

  Music_load(file);
  return 0;
}

int PubMusic_gain (PState pstate, PubFunction_t *fn, int n)
{
  float f;
  if (n<1 || !FunctionPublish_getArg(pstate,1,LUXI_CLASS_FLOAT,&f))
    return FunctionPublish_returnError(pstate,"A number is required");
  Music_setGain((double)f);
  return 0;
}

int PubMusic_play (PState pstate, PubFunction_t *fn, int n)
{
  Music_play();
  return 0;
}
int PubMusic_stop (PState pstate, PubFunction_t *fn, int n)
{
  Music_stop();
  return 0;
}

int PubMusic_time (PState pstate, PubFunction_t *fn, int n)
{
  float f;
  if (n<1 || !FunctionPublish_getArg(pstate,1,LUXI_CLASS_FLOAT,&f))
    return FunctionPublish_returnFloat(pstate,(float)Music_getTime());
  Music_setTime((double)f);
  return 0;
}

int PubMusic_isPlaying (PState pstate, PubFunction_t *fn, int n)
{
  return FunctionPublish_returnBool(pstate,Music_isPlaying());
}

int PubMusic_getcomment (PState pstate, PubFunction_t *fn, int n)
{
  const char *file;
  const char *list;
  PubArray_t  names;

  int i;
  if (n<1 || !FunctionPublish_getArg(pstate,1,LUXI_CLASS_STRING,&file))
    return FunctionPublish_returnError(pstate,"a string is required");

  n = Music_getFileInfo (file, &list);
  if (n<0) return 0;

  bufferclear();
  names.data.chars = buffermalloc(sizeof(char*)*n);
  names.length = n;
  LUX_PROFILING_OP_MAX(g_Profiling.global.memory.buffermax,buffergetinuse());

  for (i=0;i<n;i++) {
    names.data.chars[i] = list;
    list+=strlen(list)+1;
  }

  FunctionPublish_returnType(pstate,LUXI_CLASS_ARRAY_STRING,(void*)&names);
  bufferclear();
  return 1;
}


void PubClass_Sound_init()
{
  FunctionPublish_initClass(LUXI_CLASS_MUSIC,"music","Music playback, currently only for "
    "Ogg files. One file per time can be loaded and played. The playback is "
    "threaded and is only updated every 100ms. Thus, changes to the playback may lag "
    "behind for a small time period.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_MUSIC,PubMusic_load,NULL,"load",
    "():(string filename) - loads a musicfile");
  FunctionPublish_addFunction(LUXI_CLASS_MUSIC,PubMusic_play,NULL,"play",
    "():() - plays a loaded musicfile");
  FunctionPublish_addFunction(LUXI_CLASS_MUSIC,PubMusic_stop,NULL,"stop",
    "():() - stops a loaded musicfile");
  FunctionPublish_addFunction(LUXI_CLASS_MUSIC,PubMusic_time,NULL,"time",
    "([number time]):([number time]) - seeks a position in the musicfile");
  FunctionPublish_addFunction(LUXI_CLASS_MUSIC,PubMusic_gain,NULL,"gain",
    "():(number gain) - sets playback gain, must be set after loading a file again.");
  FunctionPublish_addFunction(LUXI_CLASS_MUSIC,PubMusic_getcomment,NULL,"getcomment",
    "([table string]):(string file) - reads the comments of a musicfile and returns a table that contains a list of it.");
  FunctionPublish_addFunction(LUXI_CLASS_MUSIC,PubMusic_isPlaying,NULL,"isplaying",
    "(boolean):() - returns true if the music is playing");

  FunctionPublish_initClass(LUXI_CLASS_SOUND,"sound","sound resources",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_SOUND,PubSound_load,NULL,"load",
    "(Sound snd):(string filename) - adds a soundfile to loaded resources");
  FunctionPublish_addFunction(LUXI_CLASS_SOUND,PubSound_play,NULL,"play",
    "(boolean):(Sound snd) - plays a soundfile. Returns true if it plays the sound.");
  FunctionPublish_addFunction(LUXI_CLASS_SOUND,PubClass_Sound_getInfo,(void*)SNDI_DEVICELIST,"devices",
    "(string ...):() - returns available sound devices");
  FunctionPublish_addFunction(LUXI_CLASS_SOUND,PubClass_Sound_getInfo,(void*)SNDI_STEREOCOUNT,"stereocount",
    "(int n):() - returns the maximum for stereo sources for the current device");
  FunctionPublish_addFunction(LUXI_CLASS_SOUND,PubClass_Sound_getInfo,(void*)SNDI_MONOCOUNT,"monocount",
    "(int n):() - returns the maximum for mono sources for the current device");
  FunctionPublish_addFunction(LUXI_CLASS_SOUND,PubClass_Sound_getInfo,(void*)SNDI_CURRENTDEVICE,"getdevicename",
    "(string name):() - returns the name of the current device");
  FunctionPublish_addFunction(LUXI_CLASS_SOUND,PubClass_Sound_getInfo,(void*)SNDI_SETDEVICE,"setdevice",
    "(string device):([string devicename]) - Trys to set the sounddevice according to a device with "
    "the given name or the default device if nil or no string is passed. If it fails, it "
    "will try to select the default device. Returns the selected device. Setting the current device "
    "will make all soundnodes invalid, which may result in a runtime error in lua when old (invalid) "
    "soundnodes are reused.");

  FunctionPublish_setParent(LUXI_CLASS_SOUND,LUXI_CLASS_RESOURCE);

  FunctionPublish_initClass(LUXI_CLASS_SOUNDCAPTURE,"soundcapture",
    "Capturing audio input from different sources. If the first call on any of the\
    functions of this class are done, the capturing is initialized. If the initialization fails, \
    false plus the eror is returned. Further calls on functions of this class \
    will be terminated too.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDCAPTURE,PubClass_SoundCapture,(void*)SC_DEVICELIST,
    "devicelist","([string,...]):() - returns a list of available devices");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDCAPTURE,PubClass_SoundCapture,(void*)SC_DEFAULTDEVICE,
    "defaultdevice","(string):() - returns default capture device");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDCAPTURE,PubClass_SoundCapture,(void*)SC_OPENDEVICE,
    "open","(boolean sucecss,[string error]):(string filename, int samplefrequence, \
    int format 0...3, int buffersize, [string device]) - \
    opens a device for recording. Only one open device is supported here. The \
    format is a number from 0 to 3 and represents MONO 8 bit, MONO 16 bit, STEREO \
    8 bit and STEREO 16 bit. If no device is given, the default device is used.");

  FunctionPublish_addFunction(LUXI_CLASS_SOUNDCAPTURE,PubClass_SoundCapture,(void*)SC_STOPCAPTURE,
    "stop","():() - Stops the recording and writes the buffer to the file and closes it.");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDCAPTURE,PubClass_SoundCapture,(void*)SC_STARTCAPTURE,
    "start","():() - Starts recording.");


  FunctionPublish_initClass(LUXI_CLASS_SOUNDLISTENER,"soundlistener",
    "The soundlistener represents the position, velocity and orientation \
    of the 'microphone' in space.",NULL,LUX_FALSE);
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDLISTENER,PubSoundListener_param,(void*)SL_LISTENERPOS,"pos",
    "([float x,y,z]):([float x,y,z]) - sets or gets position of listener");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDLISTENER,PubSoundListener_param,(void*)SL_VELOCITY,"vel",
    "([float x,y,z]):([float x,y,z]) - sets or gets velocity of listener");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDLISTENER,PubSoundListener_param,(void*)SL_ORIENTATION,"orientation",
    "([float x,y,z,upx,upy,upz]):([float x,y,z,upx,upy,upz]) - sets/gets orientation of listener");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDLISTENER,PubSoundListener_param,(void*)SL_VELOCITYSCALE,"velscale",
    "([float scale]):([float scale]) - scaling factor for velocity vector");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDLISTENER,PubSoundListener_param,(void*)SL_POSITIONSCALE,"posscale",
    "([float scale]):([float scale]) - scaling factor for position vector");

  FunctionPublish_initClass(LUXI_CLASS_SOUNDNODE,"soundnode","soundnodes are sound \
sources in space.",NULL,LUX_TRUE);
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundList_new,NULL,"new",
    "(soundnode node):(Sound snd) - new soundnode");
  //FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_DELETEALL,"deleteall",
  //  "():(soundnode self) - deletes all soundnodes");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_DELETE,"delete",
    "():(soundnode self) - deletes soundnode");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_UPDATEPARAM,"update",
    "():(soundnode self) - manualy updates the node parameters (gain, pitch, position...). This is \
done automaticly once per frame, use this function only if you want to make your changes work instantly. \
Remember that sound playback is threaded and you will instantly hear any changes to the soundplayback \
(unlike the graphics which is synchronized per frame).");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_PLAY,"play",
    "(boolean):(soundnode self) - plays soundnode. Make adjustments to pitch, gain, position, etc.\
before calling this function, otherwise you may hear not what you expect. These properties are \
only updated once per frame and when play is executed. Returns true if the sound is played, false \
if it couldn't play it (too many sounds playing) or nil if the sound playback is disabled.");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_ISPLAYING,"isplaying",
    "(boolean):(soundnode self) - true if soundnode is playing");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_PAUSE,"pause",
    "():(soundnode self) - pauses soundnode");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_STOP,"stop",
    "():(soundnode self) - stops soundnode");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_REWIND,"rewind",
    "():(soundnode self) - rewinds soundnode");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_PITCH,"pitch",
    "([float]):(soundnode self, [float]) - sets or gets pitch value");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_GAIN,"gain",
    "([float]):(soundnode self, [float]) - sets or gets gain value");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_REFDIST,"refdist",
    "([float]):(soundnode self, [float]) - sets or gets reference distance value");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_LOOP,"loop",
    "([boolean]):(soundnode self, [boolean]) - sets or gets looping value");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_AUTODELETE,"autodelete",
    "([boolean]):(soundnode self, [boolean]) - sets or gets autodelete value. If true, the \
    soundnode is automaticly delete when the soundplaying is stoped. This is the default.");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_VELOCITY,"velocity",
    "([float x,y,z]):(soundnode self, [float x,y,z]) - sets or gets velocity vector");
  FunctionPublish_addFunction(LUXI_CLASS_SOUNDNODE,PubSoundNode_param,(void*)SP_POSITION,"pos",
    "([float x,y,z]):(soundnode self, [float x,y,z]) - sets or gets position");
}
