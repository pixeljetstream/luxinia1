// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h" // needed for GL_RGB and GL_RGBA
#include "../common/common.h"
#include "../fileio/tga.h"
#include "../fileio/filesystem.h"


// TGA
// Nehe's OpenGL tutorials - lesson 33  (http://nehe.gamedev.net)


#pragma pack(1)
typedef struct TGAHeader_s
{
   char  idlength;
   char  colourmaptype;
   char  datatypecode;      // UNCOMPR: 1 indexed 2 BGR(A) COMPR: 9 indexed 10 BGR(A)
   short int colourmaporigin;
   short int colourmaplength;
   char  colourmapdepth;
   short int x_origin;
   short int y_origin;
   short width;
   short height;
   char  bitsperpixel;
   char  imagedescriptor;
} TGAHeader_t;

int fileLoadUncompressedTGA(Texture_t *texture, char *data, unsigned long datasize);  // Load an Uncompressed file
int fileLoadCompressedTGA(Texture_t *texture, char *data, unsigned long datasize);    // Load a Compressed file

int fileLoadTGA(const char * filename,Texture_t *texture, void *unused)       // Load a TGA file
{
  lxFSFile_t *fTGA;
  char *data;
  long datalength;
  int success;
  TGAHeader_t *tgaheader;

  fTGA = FS_open(filename);
  if(fTGA == NULL)                      // If it didn't open....
  {
    lprintf("ERROR tgaload: ");
    lnofile(filename);
    return LUX_FALSE;                           // Exit function
  }

  datalength = lxFS_getSize(fTGA);
  data = lxFS_getContent(fTGA);

  lprintf("Texture: \t%s\n",filename);

  tgaheader = (TGAHeader_t*)data;     // pointer to the beginning of the file

  if(datalength < sizeof(TGAHeader_t))
  {
    lprintf("ERROR tgaload: invalid file format\n");    // If it fails, display an error message

    FS_close(fTGA);
    return LUX_FALSE;                           // Exit function
  }

  texture->width  = tgaheader->width;
  texture->height = tgaheader->height;
  texture->depth = 1;
  texture->bpp  = tgaheader->bitsperpixel;
  texture->components = texture->bpp/8;

  if(tgaheader->idlength      !=0 ||
    tgaheader->colourmaporigin  !=0 ||
    (tgaheader->colourmaplength != 0 && tgaheader->colourmaplength != 256) ||
    (tgaheader->colourmapdepth  != 0 && tgaheader->colourmapdepth != 24) ||
    tgaheader->x_origin     !=0 ||
    tgaheader->y_origin     !=0)
  {
    lprintf("ERROR tgaload: illegal format, no offsets, no id, only 24bpp (256) palette allowed:\"%s\"\n");
    FS_close(fTGA);
    return LUX_FALSE;
  }

  if(tgaheader->width <= 0 ||
    tgaheader->height <= 0 ||
    (tgaheader->bitsperpixel != 24 && tgaheader->bitsperpixel !=32 && tgaheader->bitsperpixel !=8))
  {
    lprintf("Error loading TGA: Header is corrupted\n");
    FS_close(fTGA);
    return LUX_FALSE;
  }

  if(texture->bpp == 24)
    texture->dataformat = GL_RGB;
  else if (texture->bpp == 32)
    texture->dataformat = GL_RGBA;
  else if (texture->bpp == 8)
    texture->dataformat = GL_LUMINANCE;

  texture->datatype = GL_UNSIGNED_BYTE;
  texture->sctype = LUX_SCALAR_UINT8;

  switch(tgaheader->datatypecode)
  {
    case 1:
    case 2:
    case 3:
      success = fileLoadUncompressedTGA(texture, data, datalength);
      break;
    case 9:
    case 10:
      success = fileLoadCompressedTGA(texture, data, datalength);
      break;
    default:
      success = LUX_FALSE;
      bprintf("ERROR tgaload: unsupported datatype\n");
  }

  if (tgaheader->bitsperpixel == 8 && tgaheader->colourmaplength && tgaheader->datatypecode==1){
    char *out;
    char *palette = data + sizeof(TGAHeader_t);
    char *curpal;
    int size = texture->width * texture->height;
    int i;
    out = texture->imageData;
    for (i = 0; i < size; i++){
      curpal = &palette[out[0]*3];
      out[0] = ((uint)curpal[0]+(uint)curpal[1]+(uint)curpal[2])/3;
    }
  }

  FS_close(fTGA);
  return success;                               // Exit function
}

int fileLoadUncompressedTGA(Texture_t *texture, char *data, unsigned long datalength)
{                                     // TGA Loading code nehe.gamedev.net)
  uint cswap;
  TGAHeader_t *tgaheader;
  char *tgadata;
  int bytesPerPixel;
  int imageSize;

  tgaheader = (TGAHeader_t*)data;     // pointer to the beginning of the file
  tgadata   = data + sizeof(TGAHeader_t) + (tgaheader->colourmaplength*(tgaheader->colourmapdepth/8));  // pointer to the image data

  bytesPerPixel   = tgaheader->bitsperpixel / 8;
  imageSize     = bytesPerPixel * tgaheader->width * tgaheader->height;
  texture->imageData  = (uchar *)lxMemGenZalloc(imageSize);         // Allocate that much memory

  if(texture->imageData == NULL)                      // If no space was allocated
  {
    lprintf("ERROR tgaload: Could not allocate memory for image\n");  // Display Error
    return LUX_FALSE;                           // Return failed
  }

  if(datalength < sizeof(TGAHeader_t)+imageSize)
  {
    lprintf("ERROR tgaload: invalid file format\n");
    return LUX_FALSE;
  }

  memcpy(texture->imageData, tgadata, imageSize);

  // byte Swapping Optimized By Steve Thomas
  if (bytesPerPixel!=1){
  for(cswap = 0; cswap < (unsigned int)imageSize; cswap += bytesPerPixel)
  {
      int sw = texture->imageData[cswap];
      texture->imageData[cswap] = texture->imageData[cswap+2];
      texture->imageData[cswap+2] = sw;
    //texture->imageData[cswap] ^= texture->imageData[cswap+2] ^=
    //texture->imageData[cswap] ^= texture->imageData[cswap+2];
  }
  }
  return LUX_TRUE;
}

int fileLoadCompressedTGA(Texture_t *texture, char *data, unsigned long datasize)
{
  uint    pixelcount, currentpixel, currentbyte;
  uchar   *colorbuffer;
  short   counter;
  TGAHeader_t *tgaheader;
  char    *tgadata;
  int     bytesPerPixel;
  int     imageSize;
  uchar   *chunkheader;

  tgaheader = (TGAHeader_t*)data;     // pointer to the beginning of the file
  tgadata   = data + sizeof(TGAHeader_t) + (tgaheader->colourmaplength*(tgaheader->colourmapdepth/8));; // pointer to the image data

  bytesPerPixel   = tgaheader->bitsperpixel / 8;
  imageSize     = bytesPerPixel * tgaheader->width * tgaheader->height;
  texture->imageData  = (uchar *)lxMemGenZalloc(imageSize);

  if(texture->imageData == NULL)                      // If it wasnt allocated correctly..
  {
    lprintf("ERROR tgaload: Could not allocate memory for image\n");  // Display Error
    return LUX_FALSE;                           // Return failed
  }

  pixelcount    = tgaheader->height * tgaheader->width;
  currentpixel  = 0;
  currentbyte   = 0;

  do
  {
    chunkheader = (uchar*)tgadata;
    tgadata++;
    if(chunkheader > (unsigned char*)tgaheader+datasize)
    {
      lprintf("ERROR tgaload: Could not read RLE header\n");  // Display Error
      if(texture->imageData != NULL)                  // If there is stored image data
      {
        lxMemGenFree(texture->imageData,imageSize);                 // Delete image data
      }
      return LUX_FALSE;                         // Return failed
    }

    if(*chunkheader < 128)                        // If the ehader is < 128, it means the that is the number of RAW color packets minus 1
    {                                 // that follow the header
      (*chunkheader)++;                         // add 1 to get number of following color values
      for(counter = 0; counter < *chunkheader; counter++)   // Read RAW color values
      {
        colorbuffer = (uchar*)tgadata;
        tgadata += bytesPerPixel;
        if(colorbuffer > (unsigned char*)tgaheader+datasize)
        {
          lprintf("ERROR tgaload: Could not read image data\n");    // IF we cant, display an error

          if(texture->imageData != NULL)                    // See if there is stored Image data
          {
            lxMemGenFree(texture->imageData,imageSize);                 // If so, delete it too
          }

          return LUX_FALSE;                           // Return failed
        }
                                            // write to memory
        texture->imageData[currentbyte    ] = colorbuffer[2];           // Flip R and B vcolor values around in the process
        texture->imageData[currentbyte + 1  ] = colorbuffer[1];
        texture->imageData[currentbyte + 2  ] = colorbuffer[0];

        if(bytesPerPixel == 4)                        // if its a 32 bpp image
        {
          texture->imageData[currentbyte + 3] = colorbuffer[3];       // copy the 4th byte
        }

        currentbyte += bytesPerPixel;                   // Increase thecurrent byte by the number of bytes per pixel
        currentpixel++;                             // Increase current pixel by 1

        if(currentpixel > pixelcount)                     // Make sure we havent read too many pixels
        {
          lprintf("ERROR tgaload: Too many pixels read\n");     // if there is too many... Display an error!

          if(texture->imageData != NULL)                    // If there is Image data
          {
            lxMemGenFree(texture->imageData,imageSize);                   // delete it
          }

          return LUX_FALSE;                           // Return failed
        }
      }
    }
    else                                      // chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
    {
      (*chunkheader) -= 127;                              // Subteact 127 to get rid of the ID bit
      colorbuffer = (uchar*)tgadata;
      tgadata += bytesPerPixel;
      if(colorbuffer > (unsigned char*)tgaheader+datasize)
      {
        lprintf("ERROR tgaload: Could not read from file\n");     // If attempt fails.. Display error (again)
        if(texture->imageData != NULL)                      // If thereis image data
        {
          lxMemGenFree(texture->imageData,imageSize);                     // delete it
        }

        return LUX_FALSE;                             // return failed
      }

      for(counter = 0; counter < *chunkheader; counter++)         // copy the color into the image data as many times as dictated
      {                                     // by the header
        texture->imageData[currentbyte    ] = colorbuffer[2];         // switch R and B bytes areound while copying
        texture->imageData[currentbyte + 1  ] = colorbuffer[1];
        texture->imageData[currentbyte + 2  ] = colorbuffer[0];

        if(bytesPerPixel == 4)                        // If TGA images is 32 bpp
        {
          texture->imageData[currentbyte + 3] = colorbuffer[3];       // Copy 4th byte
        }

        currentbyte += bytesPerPixel;                   // Increase current byte by the number of bytes per pixel
        currentpixel++;                             // Increase pixel count by 1

        if(currentpixel > pixelcount)                     // Make sure we havent written too many pixels
        {
          lprintf("ERROR tgaload: Too many pixels read\n");     // if there is too many... Display an error!

          if(texture->imageData != NULL)                    // If there is Image data
          {
            lxMemGenFree(texture->imageData,imageSize);                   // delete it
          }

          return LUX_FALSE;                           // Return failed
        }
      }
    }
  }while(currentpixel < pixelcount);                          // Loop while there are still pixels left

  return LUX_TRUE;                                    // return success
}

int fileSaveTGA(const char * filename, short int width, short int height, int channels, unsigned char *imageBuffer, int isbgra)
{
  int i;
  uchar *in,*out;
  FILE *fTGA = fopen(filename, "wb");
  int bpp  = channels*8, spec = 0;
  int imageSize = width * height * channels; // we have 3 bytes per pixel (24 bit)
  uchar *temp;
  TGAHeader_t tgaheader;
  static const uchar colormap[3*256] = {
    0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5,5,6,6,6,7,7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,12,13,13,13,14,14,14,15,15,15,16,16,16,
    17,17,17,18,18,18,19,19,19,20,20,20,21,21,21,22,22,22,23,23,23,24,24,24,25,25,25,26,26,26,27,27,27,28,28,28,29,29,29,30,30,30,31,31,31,32,32,32,
    33,33,33,34,34,34,35,35,35,36,36,36,37,37,37,38,38,38,39,39,39,40,40,40,41,41,41,42,42,42,43,43,43,44,44,44,45,45,45,46,46,46,47,47,47,48,48,48,
    49,49,49,50,50,50,51,51,51,52,52,52,53,53,53,54,54,54,55,55,55,56,56,56,57,57,57,58,58,58,59,59,59,60,60,60,61,61,61,62,62,62,63,63,63,64,64,64,
    65,65,65,66,66,66,67,67,67,68,68,68,69,69,69,70,70,70,71,71,71,72,72,72,73,73,73,74,74,74,75,75,75,76,76,76,77,77,77,78,78,78,79,79,79,80,80,80,
    81,81,81,82,82,82,83,83,83,84,84,84,85,85,85,86,86,86,87,87,87,88,88,88,89,89,89,90,90,90,91,91,91,92,92,92,93,93,93,94,94,94,95,95,95,96,96,96,
    97,97,97,98,98,98,99,99,99,100,100,100,101,101,101,102,102,102,103,103,103,104,104,104,105,105,105,106,106,106,107,107,107,108,108,108,109,109,109,110,110,110,111,111,111,112,112,112,
    113,113,113,114,114,114,115,115,115,116,116,116,117,117,117,118,118,118,119,119,119,120,120,120,121,121,121,122,122,122,123,123,123,124,124,124,125,125,125,126,126,126,127,127,127,128,128,128,
    129,129,129,130,130,130,131,131,131,132,132,132,133,133,133,134,134,134,135,135,135,136,136,136,137,137,137,138,138,138,139,139,139,140,140,140,141,141,141,142,142,142,143,143,143,144,144,144,
    145,145,145,146,146,146,147,147,147,148,148,148,149,149,149,150,150,150,151,151,151,152,152,152,153,153,153,154,154,154,155,155,155,156,156,156,157,157,157,158,158,158,159,159,159,160,160,160,
    161,161,161,162,162,162,163,163,163,164,164,164,165,165,165,166,166,166,167,167,167,168,168,168,169,169,169,170,170,170,171,171,171,172,172,172,173,173,173,174,174,174,175,175,175,176,176,176,
    177,177,177,178,178,178,179,179,179,180,180,180,181,181,181,182,182,182,183,183,183,184,184,184,185,185,185,186,186,186,187,187,187,188,188,188,189,189,189,190,190,190,191,191,191,192,192,192,
    193,193,193,194,194,194,195,195,195,196,196,196,197,197,197,198,198,198,199,199,199,200,200,200,201,201,201,202,202,202,203,203,203,204,204,204,205,205,205,206,206,206,207,207,207,208,208,208,
    209,209,209,210,210,210,211,211,211,212,212,212,213,213,213,214,214,214,215,215,215,216,216,216,217,217,217,218,218,218,219,219,219,220,220,220,221,221,221,222,222,222,223,223,223,224,224,224,
    225,225,225,226,226,226,227,227,227,228,228,228,229,229,229,230,230,230,231,231,231,232,232,232,233,233,233,234,234,234,235,235,235,236,236,236,237,237,237,238,238,238,239,239,239,240,240,240,
    241,241,241,242,242,242,243,243,243,244,244,244,245,245,245,246,246,246,247,247,247,248,248,248,249,249,249,250,250,250,251,251,251,252,252,252,253,253,253,254,254,254,255,255,255,
  };


  if(!fTGA)
  {
    lprintf("ERROR tgasave: could not open file for writing \"%s\"\n", filename);
    return LUX_FALSE;
  }


  tgaheader.idlength = 0;
  tgaheader.colourmaptype = (channels == 1);
  tgaheader.datatypecode = (channels == 1) ? 1 : 2;


  if (channels == 1){
    tgaheader.colourmaporigin = 0;
    tgaheader.colourmaplength = 256;
    tgaheader.colourmapdepth = 24;

  }
  else{
    tgaheader.colourmaporigin = 0;
    tgaheader.colourmaplength = 0;
    tgaheader.colourmapdepth = 0;
  }


  tgaheader.x_origin = 0;
  tgaheader.y_origin = 0;

  tgaheader.height = height;
  tgaheader.width = width;
  tgaheader.bitsperpixel = bpp;
  tgaheader.imagedescriptor = 0;


  fwrite(&tgaheader, sizeof(TGAHeader_t),1,fTGA);

  if (channels == 1){
    fwrite(colormap,sizeof(uchar),3*256,fTGA);
  }


  if (isbgra){
    fwrite(imageBuffer, sizeof(unsigned char), imageSize, fTGA);
  }
  else{
    out = temp = lxMemGenZalloc(sizeof(uchar)*imageSize);
    in = imageBuffer;


    // we have the pixels in rgb format tga wants em in bgr, so we gotta swap em
    switch(channels){
      case 1:
        for(i=0; i<imageSize; i+=channels,in+=channels,out+=channels)
        {
          out[0] = in[0];
        }
        break;
      case 3:
        for(i=0; i<imageSize; i+=channels,in+=channels,out+=channels)
        {
          out[2] = in[0];
          out[1] = in[1];
          out[0] = in[2];
        }
        break;
      case 4:
        for(i=0; i<imageSize; i+=channels,in+=channels,out+=channels)
        {
          out[2] = in[0];
          out[1] = in[1];
          out[0] = in[2];
          out[3] = in[3];
        }
        break;

    }
    fwrite(temp, sizeof(unsigned char), imageSize, fTGA);
    lxMemGenFree(temp,sizeof(uchar)*imageSize);
  }

  fclose(fTGA);



  return LUX_TRUE;
}
