// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#ifndef __SOUNDLIST_H__
#define __SOUNDLIST_H__

#include <luxinia/luxcore/contmacrolinkedlist.h>
#include "../common/types.h"
#include "../resource/sound.h"
#include "../common/reflib.h"

typedef struct SoundlxListNode_s
{
  Sound_t   *sound;       // the sound object
  ALuint    source;       // this thingy emits the sound

  ALfloat   position[3];
  ALfloat   velocity[3];

  float   positionScale;    // scaling factor for position
  float   velocityScale;    // scaling factor for velocity

  ALfloat   pitch;
  ALfloat   gain;
  ALboolean looping;
  ALfloat   referencedistance;

  int     autodelete; // deletes soundsource if true (default)
  int     removed;

  Reference   ref;

  struct SoundlxListNode_s LUX_LISTNODEVARS; // this is for the new linked list
}SoundlxListNode_t;

typedef struct SoundListener_s
{
  ALfloat position[3];
  ALfloat velocity[3];
  ALfloat orientation[6];
  float velocityScale;
  float positionScale;
} SoundListener_t;


SoundListener_t *SoundList_getListener();

 // init soundsystem
void SoundList_init();

// delete all nodes and reset soundsystem
void SoundList_reset();

const char * Sound_getDevicelist ();
int Sound_getMonoSources ();
int Sound_getStereoSources ();
void SoundList_setDevice(char *devname);
const char * Sound_getCurrentDevice ();

 // deinit soundsystem
void SoundList_deinit(void);

void SoundList_update();

SoundlxListNode_t *SoundList_addNode(SoundlxListNode_t *node);

SoundlxListNode_t *SoundList_add(Sound_t *sound);

SoundlxListNode_t *SoundList_addResource(unsigned int id);

SoundlxListNode_t *SoundList_getNodeByRes(int res);

float *Sound_getListenerPosition(lxVector3 pos);

float *Sound_getListenerVelocity(lxVector3 vel);

float *Sound_getListenerOrientation(Vector6 ori);

float *Sound_getListenerOrientationAt(lxVector3 at);

float *Sound_getListenerOrientationUp(lxVector3 up);

void Sound_setListenerPosition(lxVector3 pos);

void Sound_setListenerVelocity(lxVector3 pos);

void Sound_setListenerOrientation(Vector6 pos);

void Sound_setListenerOrientationAt(lxVector3 pos);

void Sound_setListenerOrientationUp(lxVector3 pos);



SoundlxListNode_t*    SoundlxListNode_new(Sound_t *sound);
void          SoundlxListNode_free(SoundlxListNode_t* node);
int           SoundlxListNode_isPlaying(SoundlxListNode_t *sound);

int SoundlxListNode_play(SoundlxListNode_t *sound); // returns 1 on success

void SoundlxListNode_pause(SoundlxListNode_t *sound);

void SoundlxListNode_stop(SoundlxListNode_t *sound);

void SoundlxListNode_rewind(SoundlxListNode_t *sound);

void SoundlxListNode_setPitch(SoundlxListNode_t *sound, float pitch);

void SoundlxListNode_setGain(SoundlxListNode_t *sound, float gain);

void SoundlxListNode_setLooping(SoundlxListNode_t *sound, char looping);

void SoundlxListNode_setPosition(SoundlxListNode_t *sound, lxVector3 pos);

void SoundlxListNode_setVelocity(SoundlxListNode_t *sound, lxVector3 vel);

void SoundlxListNode_setReferenceDistance(SoundlxListNode_t *sound, float dist);

float SoundlxListNode_getPitch(SoundlxListNode_t *sound);

float SoundlxListNode_getGain(SoundlxListNode_t *sound);

char SoundlxListNode_getLooping(SoundlxListNode_t *sound);

float *SoundlxListNode_getPosition(SoundlxListNode_t *sound, lxVector3 pos);

float *SoundlxListNode_getVelocity(SoundlxListNode_t *sound, lxVector3 vel);

float SoundlxListNode_getReferenceDistance(SoundlxListNode_t *sound);

void      SoundlxListNode_update(SoundlxListNode_t *sound); // update changed params

void RSoundlxListNode_free (Reference ref);

#endif
