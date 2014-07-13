// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "ma.h"
#include "../fileio/parser.h"
#include "../fileio/filesystem.h"

void  fileLoadMATrack();
void  AnimBuildFromBuffer(Anim_t *anim);
void  freeTracks(void);

// LOCALS
static struct MAData_s{
  lxFSFile_t  *file;
  Track_t   tracks[ANIM_MAX_TRACKS];
  int     tracksInBuffer;
}l_MAData;


/*
=================================================================
MA LOADING
  based on code provided by Diogo Teixeira
=================================================================
*/

int fileLoadMA(const char *filename,Anim_t *anim, void *unused)       // Load a MA FSFile_t
{
  lxFSFile_t * fMA;                       // FSFile_t pointer to model FSFile_t
  char buf[256];
  int version;
  float length;
  int _i = 0;

  fMA = FS_open(filename);                // Open FSFile_t for reading
  l_MAData.file = fMA;

  if(fMA == NULL)                       // If it didn't open....
  {
    lprintf("ERROR maload: ");
    lnofile(filename);
    return LUX_FALSE;                           // Exit function
  }

  lprintf("Anim: \t\t%s\n",filename);

  sscanf(&fMA->contents[fMA->index],"%s\n%n", buf,&_i);
  fMA->index+=_i;

  //FS_scanf(fMA, "%s\n", buf);
  if (strcmp(buf, MA_SIGNATURE) != 0)
  {
    // wrong signature
    lprintf("ERROR maload: invalid FSFile_t format\n");
    FS_close(fMA);
    return LUX_FALSE;
  }

  // read header
  lxFS_gets(buf, 255, fMA); // "Header{"

  sscanf(&fMA->contents[fMA->index]," Version: %d\n%n", &version,&_i);
  fMA->index+=_i;

  //FS_scanf(fMA, " Version: %d\n", &version);
  if (version < MA_VERSION)
  {
    // wrong version
    lprintf("ERROR maload: invalid FSFile_t format\n");
    FS_close(fMA);
    return LUX_FALSE;
  }
  lxFS_gets(buf, 255, fMA); // "Date: ..."
  lxFS_gets(buf, 255, fMA); // "Time: ..."

  // assign animation total length in ms
  sscanf(&fMA->contents[fMA->index]," Length: %f\n%n", &length,&_i);
  fMA->index+=_i;

  //FS_scanf(fMA, " Length: %f\n", &length); // "Length: ..."
  anim->length = (uint) length;
  dlprintf("\tLength: %d\n",anim->length);

  lxFS_gets(buf, 255, fMA); // }

  l_MAData.tracksInBuffer = 0;
  anim->numTracks = 0;

  while (!lxFS_eof(fMA))
  {
    // read block identification
    if (lxFS_gets(buf, 255, fMA) == NULL)
    {
      // reached end of FSFile_t
      break;
    }

    // compare blocks
    if (strstr(buf, "Track{"))
    {
      if(l_MAData.tracksInBuffer < ANIM_MAX_TRACKS){
        fileLoadMATrack();
        anim->numTracks++;
        l_MAData.tracksInBuffer++;
      }
      else lprintf("WARNING maload: tracksbuffer full\n");
    }
    else
    {
      if(lxFS_eof(fMA))
        break;

      // skip block
      while (strstr(buf, "}") == NULL){
        if (lxFS_gets(buf, 255, fMA) == NULL)
          break;
      };
    }
  }

  AnimBuildFromBuffer(anim);

  FS_close(fMA);
  freeTracks();
  return LUX_TRUE;        // All went well
}

// LOADS A TRACK IN BUFFER
//-------------------------
void  fileLoadMATrack()
{
  lxFSFile_t *fMA = l_MAData.file;
  char  buf[256];
  char  name[256];
  int   i;
  int _i = 0;
  Track_t *track;

  track = &l_MAData.tracks[l_MAData.tracksInBuffer];

  // Name:
  lxFS_gets(buf, 255, fMA);
  lxStrReadInQuotes(buf, name, 255);
  //lprintf("Track:%s",name);

  track->name = (char *)reszalloc(sizeof(char) * (strlen(name)+1));
  strcpy(track->name,name);
  name[0] = 0;


  // Keys
  sscanf(&fMA->contents[fMA->index]," Keys: %d\n%n", &track->numKeys,&_i);
  fMA->index+=_i;

  //FS_scanf(fMA, " Keys: %d\n", &track->numKeys);  // Triangle count
  //lprintf("\tKeyCnt: %d\n",track->numKeys);

  track->keys= (PRS_t *)reszallocaligned((track->numKeys)*(sizeof(PRS_t)),16);

  for (i = 0; i < track->numKeys; i++)
  {
    PRS_t *key = &track->keys[i];

    sscanf(&fMA->contents[fMA->index]," %f %f %f %f %f %f %f %f %f %f %f\n%n",
      &key->time,
      &key->pos[0], &key->pos[1], &key->pos[2],
      &key->rot[0], &key->rot[1], &key->rot[2], &key->rot[3],
      &key->scale[0], &key->scale[1], &key->scale[2],
      &_i);
    key->usedflags = PRS_ALL;
    fMA->index+=_i;

    /*FS_scanf(fMA, " %f %f %f %f %f %f %f %f %f %f %f\n",
      &time,
      &key->pos[0], &key->pos[1], &key->pos[2],
      &key->rot[0], &key->rot[1], &key->rot[2], &key->rot[3],
      &key->scale[0], &key->scale[1], &key->scale[2]);
      // time, pos, rot, scale*/
    // assign time
  }

  lxFS_gets(buf, 255, fMA);
}


// BUILDS AN ANIMATION FROM BUFFER
//---------------------------------
// very important this actually builds our data
// reallocs and so on
void  AnimBuildFromBuffer(Anim_t *anim)
{
  // not much to do
  anim->tracks = reszalloc( anim->numTracks * ( sizeof(Track_t) ) );
  memcpy(anim->tracks, l_MAData.tracks,(sizeof(Track_t) * anim->numTracks) );

  AnimLoader_applyScale(anim);

  dlprintf("\tTrackCnt: %d\n\n",anim->numTracks);
}

void freeTracks()
{
  clearArray(l_MAData.tracks,ANIM_MAX_TRACKS);

  l_MAData.tracksInBuffer = 0;
}

