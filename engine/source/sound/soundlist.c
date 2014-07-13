// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include <al/alut.h>
#include <al/al.h>

#include "../resource/resmanager.h"
#include "../common/memorymanager.h"
#include "../common/3dmath.h"
#include "../main/main.h"
#include "soundlist.h"

extern void InitVorbisFile();
extern void ShutdownVorbisFile();


ALfloat DefaultSourcePos[] = { 0.0, 0.0, 0.0 };
ALfloat DefaultSourceVel[] = { 0.0, 0.0, 0.0 };

static SoundListener_t  l_SoundListener;
static SoundlxListNode_t *l_SoundListHead = NULL;
static int sourcelimit;

static int Sound_checkError(char *checker) {
  ALenum code;
  if((code = alGetError()) != AL_NO_ERROR) {
    const char *err = alGetString(code);
    LUX_PRINTF("WARNING OpenAL %s; %s\n",err,checker);
    return 1;
  } else return 0;
  return 0;
}

static ALCcontext *Context;
static ALCdevice *Device;
static char *l_preferreddevice = NULL;

SoundListener_t *SoundList_getListener() {
  return &l_SoundListener;
}

const char * Sound_getDevicelist () {
  if (g_nosound) return "<no sound>\0\0";
  return alcGetString(NULL, ALC_DEVICE_SPECIFIER);
}
const char * Sound_getCurrentDevice () {
  if (g_nosound) return "<no sound>";
  return alcGetString(Device,ALC_DEVICE_SPECIFIER);
}
int Sound_getStereoSources () {
  ALCint scnt[1] = {0};
  if (g_nosound) return 0;
  alcGetIntegerv(Device, ALC_STEREO_SOURCES, 1,scnt);
  return scnt[0];
}

int Sound_getMonoSources () {
  ALCint scnt[1] = {0};
  if (g_nosound) return 0;
  alcGetIntegerv(Device, ALC_MONO_SOURCES, 1,scnt);
  return scnt[0];
}
static void manualinit (char *devname)
{
  Device = alcOpenDevice((ALubyte*)devname);
  if (Device == NULL) // failed, maybe the default device can be inited
    Device = alcOpenDevice(NULL);
  if (Device == NULL)
  {
    bprintf("The openal device could not be opened\n");
    bprintf("ERROR OpenAL could not be initialized: %s\n",alutGetErrorString(alGetError()));
    exit(-1);
  }

  //Create context(s)
  Context=alcCreateContext(Device,NULL);
  if (Context==NULL) {
    bprintf("The openal context could not be opened\n");
    bprintf("ERROR OpenAL could not be initialized: %s\n",alutGetErrorString(alGetError()));
    exit(-1);
  }
  //Set active context
  alcMakeContextCurrent(Context);

  lprintf("  Available OpenAL Devices: \n");
  {
    const char *devicelist = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    while(*devicelist){
      lprintf("    %s\n",devicelist);
      devicelist += strlen(devicelist)+1;
    }
  }

  lprintf("  OpenAL Device specifier: \n    %s\n",
    alcGetString(Device, ALC_DEVICE_SPECIFIER));

  lprintf("  OpenAL Device extensions: \n    %s\n",
    alcGetString(Device, ALC_EXTENSIONS));
  {
    ALCint scnt[4] = {0,0,0,0};
    alcGetIntegerv(Device, ALC_STEREO_SOURCES, 4,scnt);
    lprintf("  Number of stereo sound sources: %i\n",scnt[0]);
    alcGetIntegerv(Device, ALC_MONO_SOURCES, 4,scnt);
    lprintf("  Number of mono sound sources: %i\n",scnt[0]);
    sourcelimit = scnt[0];
  }

  alGetError();
}
static int l_init = 0;
void SoundList_setDevice(char *devname)
{
  devname = devname && strcmp("<no sound>",devname)==0 ? "Generic Software" : devname;

  if (!l_init) {
    l_preferreddevice = devname;
    return;
  }
  if (g_nosound) return;
  if(l_SoundListHead)
    lxListNode_destroyList(l_SoundListHead, SoundlxListNode_t, SoundlxListNode_free);
  alcMakeContextCurrent(NULL);
  alcDestroyContext(Context);
  alcCloseDevice(Device);
  manualinit(devname);
}

void SoundList_init()
{
  ALenum error;

  l_init = 1;

  Reference_registerType(LUXI_CLASS_SOUNDNODE,"soundnode", RSoundlxListNode_free,NULL);

  lxVector3Set(l_SoundListener.position, 0.0, 0.0, 0.0 );
  lxVector3Set(l_SoundListener.velocity, 0.0, 0.0, 0.0 );
  LUX_ARRAY6SET(l_SoundListener.orientation, 0.0, 1.0, 0.0,  0.0, 0.0, 1.0 );

//  Variable_setSaveMode(
//    Environment_registerVar("system_nosound", PVariable_newBool((char*)&g_nosound)),1);

  if (g_nosound)
    return;

  manualinit(l_preferreddevice ? l_preferreddevice : "Generic Software");
  alutInitWithoutContext (NULL, NULL);


  if( (error=alutGetError()) != ALUT_ERROR_NO_ERROR){
    bprintf("ERROR OpenAL could not be initialized: %s\n",alutGetErrorString(error));
    g_nosound = LUX_TRUE;
    return;
  }

  alListenerfv(AL_POSITION,    l_SoundListener.position);
  alListenerfv(AL_VELOCITY,    l_SoundListener.velocity);
  alListenerfv(AL_ORIENTATION, l_SoundListener.orientation);

  l_SoundListener.positionScale = 1;
  l_SoundListener.velocityScale = 1;

  Sound_checkError("SoundList_init");

  InitVorbisFile();
}

void SoundList_deinit(void)
{
  if(l_SoundListHead)
    lxListNode_destroyList(l_SoundListHead, SoundlxListNode_t, SoundlxListNode_free);
  Sound_checkError("SoundList_deinit");
  if (!g_nosound)
    alutExit();
  ShutdownVorbisFile();
}

void SoundList_reset()
{
  lxVector3Set(l_SoundListener.position, 0.0, 0.0, 0.0 );
  lxVector3Set(l_SoundListener.velocity, 0.0, 0.0, 0.0 );
  LUX_ARRAY6SET(l_SoundListener.orientation, 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 );

  if (!g_nosound){
    alListenerfv(AL_POSITION,    l_SoundListener.position);
    alListenerfv(AL_VELOCITY,    l_SoundListener.velocity);
    alListenerfv(AL_ORIENTATION, l_SoundListener.orientation);
  }

  l_SoundListener.positionScale = 1;
  l_SoundListener.velocityScale = 1;

  if(l_SoundListHead)
    lxListNode_destroyList(l_SoundListHead, SoundlxListNode_t, SoundlxListNode_free);
}

static int sourcecount  =0;
static void SoundlxListNode_freeSource (SoundlxListNode_t *self)
{
  if (!self->source || g_nosound) return;
  sourcecount--;
  alSourceStop(self->source);
  alDeleteSources(1, &self->source);
  self->source = 0;
  Sound_checkError("SoundlxListNode_freeSource");
}

static int SoundlxListNode_allocSource (SoundlxListNode_t *self)
{
  //LUX_PRINTF("alloc %i %p %i?\n",sourcecount,self->source,g_nosound);
  if(Sound_checkError("SoundlxListNode_allocSource<precheck>")); // donothing

  if (self->source || g_nosound) return 1;
  if (sourcecount>=sourcelimit) return 0; //no need to try, it's useless

  alGenSources(1, &self->source);
  //LUX_PRINTF("is buffer: %i\n",alIsBuffer(self->sound->buffer));
  if(Sound_checkError("SoundlxListNode_allocSource<alloc>")) {
    printf("   Active sound sources: %d\n",sourcecount);
    return 0;
  }
  if(!self->source) {
    printf("   Source is null, strangly... active sound sources: %d\n",sourcecount);
    return 0;
  }
  //printf("   success, Active sound sources: %d\n",sourcecount);
  sourcecount++;
  //alSourceQueueBuffers(self->source,1,&self->sound->buffer);
  alSourcei (self->source, AL_BUFFER,   self->sound->buffer);
  alSourcef (self->source, AL_PITCH,    1.0);
  alSourcef (self->source, AL_GAIN,     1.0);
  alSourcefv(self->source, AL_POSITION, DefaultSourcePos);
  alSourcefv(self->source, AL_VELOCITY, DefaultSourceVel);
  alSourcei (self->source, AL_LOOPING,  AL_FALSE);
  if(Sound_checkError("SoundlxListNode_allocSource<setting>")) return 0;
  return 1;
}

void RSoundlxListNode_free (Reference ref)
{
  SoundlxListNode_t *node = (SoundlxListNode_t*)Reference_value(ref);
  SoundlxListNode_freeSource(node);
  if (l_SoundListHead!=NULL && !node->removed)
    lxListNode_remove(l_SoundListHead,node);
  lxMemGenFree(node,sizeof(SoundlxListNode_t));
}

SoundlxListNode_t* SoundlxListNode_new(Sound_t *sound)
{
  SoundlxListNode_t *node;

  node = lxMemGenZalloc(sizeof(SoundlxListNode_t));
  node->sound = sound;

  node->pitch = 1;
  node->gain = 1;
  node->autodelete = 1;
  node->removed = 1;
  node->positionScale = 1;
  node->velocityScale = 1;
  node->referencedistance = 1;
  node->source = 0;

  lxListNode_init(node);
  node->ref = Reference_new(LUXI_CLASS_SOUNDNODE,node);

  return node;
}

void SoundlxListNode_free(SoundlxListNode_t* node)
{
  Reference_free(node->ref);
}

SoundlxListNode_t *SoundList_getNodeByRes(int res)
{
  SoundlxListNode_t *browse;

  lxListNode_forEach(l_SoundListHead, browse)
  {
    if(ResData_getSound(res) == browse->sound)
    {
      return browse;
    }
  }lxListNode_forEachEnd(l_SoundListHead, browse);

  return NULL;
}

SoundlxListNode_t *SoundList_addNode(SoundlxListNode_t *node)
{
  if (lxListNode_next(node)!=node)
    return NULL; // already added

  lxListNode_addLast(l_SoundListHead, node);
  node->removed = 0;

  return node;
}

SoundlxListNode_t *SoundList_add(Sound_t *sound)
{
  // zur liste hinzufügen
  // und zurückgeben
  SoundlxListNode_t *node;

  if(sound == NULL)
    return NULL;

  node = SoundlxListNode_new(sound);
  if (node == NULL)
    return NULL;

  return SoundList_addNode(node);
}

SoundlxListNode_t *SoundList_addResource(unsigned int id)
{
  if(id != -1)
    return SoundList_add(ResData_getSound(id));

  return NULL;
}

// removes stopped sounds
void SoundList_update()
{
  // foreach sound : if sound is stopped : remove
  SoundlxListNode_t *browse,*rem;
  ALint state;
  float v[3];
  int cnt;

  if (g_nosound)
    return;


  Sound_checkError("SoundList_update<init:1>");

  if (!g_nosound){
    alListenerfv(AL_ORIENTATION, l_SoundListener.orientation);

    lxVector3Scale(v,l_SoundListener.velocity,l_SoundListener.velocityScale);
    alListenerfv(AL_VELOCITY,  v);

    //Vector3Scale(v,l_SoundListener.position,10); <-- wtf?
    lxVector3Scale(v,l_SoundListener.position,l_SoundListener.positionScale);
    alListenerfv(AL_POSITION,  v);
    //LUX_PRINTF("lis %f %f\n",l_SoundListener.position[0],v[0]);
  }
  Sound_checkError("SoundList_update<init:2>");

  browse = l_SoundListHead;
  cnt = 0;
  if (browse!=NULL)
  do
  {
    if (browse->source != 0) {
      alGetSourcei(browse->source, AL_SOURCE_STATE, &state);
      Sound_checkError("SoundList_update<browse>");
    } else state = -123;
    if(state != AL_PLAYING)
    {
//      if (browse==l_SoundListHead) {
//        l_SoundListHead = l_SoundListHead->prev;
//      }
      rem = browse;
      browse = browse->next;
      lxListNode_remove(l_SoundListHead, rem);
//LUX_PRINTF("removing\n");
      SoundlxListNode_freeSource(rem);
      rem->removed = 1;
      lxListNode_init(rem);
      if (rem->autodelete) {
        SoundlxListNode_free(rem);
      }
      if (rem == browse) break;
      continue;
      //destroySoundNode(browse);
    } else SoundlxListNode_update(browse);
    browse = browse->next;
    cnt++;
  } while (l_SoundListHead != browse && l_SoundListHead!=NULL);

//  LUX_PRINTF("%i\n",cnt);
  Sound_checkError("SoundList_update<finish>");
}

// Listener Settings

float *Sound_getListenerPosition(lxVector3 pos)
{
  lxVector3Copy(pos, l_SoundListener.position);
  return pos;
}

float *Sound_getListenerVelocity(lxVector3 vel)
{
  lxVector3Copy(vel, l_SoundListener.velocity);
  return vel;
}

float *Sound_getListenerOrientation(Vector6 ori)
{
  lxVector3Copy(ori, l_SoundListener.orientation);
  return ori;
}

float *Sound_getListenerOrientationAt(lxVector3 at)
{
  lxVector3Copy(at, l_SoundListener.orientation);
  return at;
}

float *Sound_getListenerOrientationUp(lxVector3 up)
{
  lxVector3Copy(up, (&(l_SoundListener.orientation[3])));
  return up;
}

void Sound_setListenerPosition(lxVector3 pos)
{
  lxVector3Copy(l_SoundListener.position, pos);
  alListenerfv(AL_POSITION,    l_SoundListener.position);

  //LUX_PRINTF("lis %f %f %f \n",l_SoundListener.position[1],l_SoundListener.position[2],l_SoundListener.position[3]);
}

void Sound_setListenerVelocity(lxVector3 vel)
{
  lxVector3Copy(l_SoundListener.velocity, vel);
  alListenerfv(AL_VELOCITY,    l_SoundListener.velocity);
}

void Sound_setListenerOrientation(Vector6 ori)
{
  Sound_setListenerOrientationAt(&ori[0]);
  Sound_setListenerOrientationUp(&ori[3]);
  alListenerfv(AL_ORIENTATION, l_SoundListener.orientation);
}

void Sound_setListenerOrientationAt(lxVector3 at)
{
  lxVector3Copy(&l_SoundListener.orientation[0], at);
  alListenerfv(AL_ORIENTATION, l_SoundListener.orientation);
}

void Sound_setListenerOrientationUp(lxVector3 up)
{
  lxVector3Copy(&l_SoundListener.orientation[3], up);
  alListenerfv(AL_ORIENTATION, l_SoundListener.orientation);
}

// Sound Settings

void SoundlxListNode_update(SoundlxListNode_t *sound) {
  float v[3];

  if (sound->source == 0 || g_nosound) return;
  if (sound->pitch>0) alSourcef (sound->source, AL_PITCH,   sound->pitch);
  if (sound->gain>=0) alSourcef (sound->source, AL_GAIN,    sound->gain);
  if (sound->referencedistance>=0)
          alSourcef (sound->source, AL_REFERENCE_DISTANCE, sound->referencedistance);

  alSourcei (sound->source, AL_LOOPING,   sound->looping);
  //LUX_PRINTF("upd: %f %f %f %i\n",sound->pitch,sound->gain,sound->referencedistance,sound->looping);
  lxVector3Scale(v,sound->position,sound->positionScale);
  alSourcefv(sound->source, AL_POSITION,  v);
  //LUX_PRINTF("pos %p: %f %f %f \n",sound->source,v[0],v[1],v[2]);

  lxVector3Scale(v,sound->velocity,sound->velocityScale);
  alSourcefv(sound->source, AL_VELOCITY,  v);
  //LUX_PRINTF("vec: %f %f %f \n",v[0],v[1],v[2]);

  if (Sound_checkError("SoundlxListNode_update<finish>"));// LUX_PRINTF("%i\n",sound->source);
}

int SoundlxListNode_isPlaying(SoundlxListNode_t *sound)
{
  int state;
  if (sound->source==0 || g_nosound) return 0;

  SoundlxListNode_update(sound);
  alGetSourcei(sound->source, AL_SOURCE_STATE, &state);

  return (state==AL_PLAYING);
}

int SoundlxListNode_play(SoundlxListNode_t *node)
{
  static int suc;
  if (node!=NULL) {
    if (!SoundlxListNode_allocSource(node)) {
      if (node->autodelete) SoundlxListNode_free(node);
      return 0;
    }
    SoundlxListNode_update(node);
    alSourcePlay(node->source);
    suc = !(Sound_checkError("SoundlxListNode_play"));
    if (node->removed) {
      node->removed = 0;
      lxListNode_addLast(l_SoundListHead, node);
    }
  }
  return suc;
}

void SoundlxListNode_pause(SoundlxListNode_t *sound)
{
  if (sound->source)  alSourcePause(sound->source);
}

void SoundlxListNode_stop(SoundlxListNode_t *sound)
{
  SoundlxListNode_freeSource(sound);
}

void SoundlxListNode_rewind(SoundlxListNode_t *sound)
{
  if (sound->source) alSourceRewind(sound->source);
}

void SoundlxListNode_setPitch(SoundlxListNode_t *sound, float pitch)
{
  if (pitch<0.1f) pitch = 0.1f;
  if (pitch>10) pitch = 10;
  sound->pitch = pitch;
  if (sound->source)
    alSourcef (sound->source, AL_PITCH, pitch);
}

void SoundlxListNode_setGain(SoundlxListNode_t *sound, float gain)
{
  sound->gain = gain;
  if (sound->source)
    alSourcef (sound->source, AL_GAIN, gain);
}

void SoundlxListNode_setLooping(SoundlxListNode_t *sound, char looping)
{
  sound->looping = looping;
  if (sound->source)
    alSourcei (sound->source, AL_LOOPING, looping);
}

void SoundlxListNode_setPosition(SoundlxListNode_t *sound, lxVector3 pos)
{
  lxVector3Copy(sound->position, pos);
  if (sound->source)
    alSourcefv(sound->source, AL_POSITION, pos);
}

void SoundlxListNode_setVelocity(SoundlxListNode_t *sound, lxVector3 vel)
{
  lxVector3Copy(sound->velocity, vel);
  if (sound->source)
    alSourcefv(sound->source, AL_VELOCITY, vel);
}

void SoundlxListNode_setReferenceDistance(SoundlxListNode_t *sound, float dist)
{
  sound->referencedistance = dist;
  if (sound->source)
    alSourcef(sound->source, AL_REFERENCE_DISTANCE, dist);
}

float SoundlxListNode_getPitch(SoundlxListNode_t *sound)
{
  return sound->pitch;
}

float SoundlxListNode_getGain(SoundlxListNode_t *sound)
{
  return sound->gain;
}

char SoundlxListNode_getLooping(SoundlxListNode_t *sound)
{
  return sound->looping;
}

float *SoundlxListNode_getPosition(SoundlxListNode_t *sound, float *pos)
{
  lxVector3Copy(pos, sound->position);
  return pos;
}

float *SoundlxListNode_getVelocity(SoundlxListNode_t *sound, float *vel)
{
  lxVector3Copy(vel, sound->velocity);
  return vel;
}

float SoundlxListNode_getReferenceDistance(SoundlxListNode_t *sound)
{
  return sound->referencedistance;
}

