// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/common.h"
#include "../fileio/jpg.h"
#include "../fileio/filesystem.h"

#ifdef LUX_PLATFORM_WINDOWS
    #include <jpeglib/jpeglib.h>
    #include <jpeglib/jmemsource.h>
#else
    #include <jpeglib.h>
    #include <jpeglib/jmemsource.h>
#endif

/*
=================================================================
JPEG LOADING
DigiBen
http://www.gametutorials.com
=================================================================
*/


// DECODE JPG
static void DecodeJPG(struct jpeg_decompress_struct *cinfo, Texture_t *texture)
{
  unsigned int rowSpan;
  unsigned int i;
  unsigned char** rowPtr;
  int rowsRead;

  // Read in the header of the jpeg file
  jpeg_read_header(cinfo, LUX_TRUE);

  // Start to decompress the jpeg file with our compression info
  jpeg_start_decompress(cinfo);

  // Get the image dimensions and row span to read in the pixel data
  rowSpan = cinfo->image_width * cinfo->num_components;
  texture->width  = cinfo->image_width;
  texture->height   = cinfo->image_height;
  texture->depth = 1;

  // Allocate memory for the pixel buffer
  texture->imageData = lxMemGenZalloc(texture->height * rowSpan *sizeof(char));//new unsigned char[rowSpan * texture->height];

  // Here we use the library's state variable cinfo.output_scanline as the
  // loop counter, so that we don't have to keep track ourselves.

  // Create an array of row pointers
  rowPtr = (unsigned char**)lxMemGenAlloc(texture->height*sizeof(char*)); //new unsigned char*[texture->height];
  for (i = 0; i < (unsigned int)texture->height; i++)
    rowPtr[i] = &(texture->imageData[i*rowSpan]);

  // Now comes the juice of our work, here we extract all the pixel data
  rowsRead = 0;
  while (cinfo->output_scanline < cinfo->output_height)
  {
    // Read in the current row of pixels and increase the rowsRead count
    rowsRead += jpeg_read_scanlines(cinfo, &rowPtr[rowsRead], cinfo->output_height - rowsRead);
  }

  texture->components = cinfo->num_components;
  texture->bpp = cinfo->num_components * 8;

  // Delete the temporary row pointers
  lxMemGenFree(rowPtr,texture->height*sizeof(char*));

  // Finish decompressing the data
  jpeg_finish_decompress(cinfo);
}

int fileLoadJPG(const char *filename, Texture_t *texture, void *unused)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  char *scanline;
  unsigned int i;

  lxFSFile_t *file;
  char *buffer;

  // This is the only function you should care about.  You don't need to
  // really know what all of this does (since you can't cause it's a library!) :)
  // Just know that you need to pass in the jpeg file name, and get a pointer
  // to a tImageJPG structure which contains the width, height and pixel data.
  // Be sure to free the data after you are done with it, just like a bitmap.

  // Open a file pointer to the jpeg file and check if it was found and opened
  if((file = FS_open(filename)) == NULL)
  {
    lprintf("WARNING jpgload: "); // Display an error message
    lnofile(filename);
    return LUX_FALSE;                     // Exit function
  }

  buffer = lxFS_getContent(file);

  lprintf("Texture: \t%s\n",filename);
  // Create an error handler


  // Have our compression info object point to the error handler address
  cinfo.err = jpeg_std_error(&jerr);

  // Initialize the decompression object
  jpeg_create_decompress(&cinfo);

  // Specify the data source (Our file pointer)
  //jpeg_stdio_src(&cinfo, pFile);
  jpeg_memory_src(&cinfo, buffer, lxFS_getSize(file));

  // Decode the jpeg file and fill in the image data structure to pass back
  DecodeJPG(&cinfo, texture);

  // This releases all the stored memory for reading and decoding the jpeg
  jpeg_destroy_decompress(&cinfo);

  // Close the file pointer that opened the file
  FS_close(file);

  // flip scanlines from top half with bottom half
  scanline = lxMemGenAlloc(texture->width * 3);

  // swap top and bottom scanlines
  for(i=0; i < (unsigned int)texture->height/2; i++)
  {
    int linesize = texture->width*texture->bpp/8;
    int pos = i*linesize;
    int spos = (texture->height-i-1)*linesize;  // position to switch with
    if(spos != pos)
    {
      memcpy(scanline, &texture->imageData[pos], linesize);
      memcpy(&texture->imageData[pos], &texture->imageData[spos], linesize);
      memcpy(&texture->imageData[spos],scanline, linesize);
    }
  }
  lxMemGenFree(scanline,texture->width * 3);

  switch(texture->bpp) {
  case 8:
    texture->dataformat = GL_LUMINANCE;
    break;
  case 24:
    texture->dataformat = GL_RGB;
    break;
  default:
    return LUX_FALSE;
  }

  texture->datatype = GL_UNSIGNED_BYTE;
  texture->sctype = LUX_SCALAR_UINT8;

  return LUX_TRUE;
}


int fileSaveJPG
  (const char * filename, short int width, short int height, int channels, int quality,
    unsigned char *imageBuffer)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  int row_stride = width * channels; // actual rowlength in bytes (width * RGB)
  int size = width * height * channels;

  FILE *fJPG = fopen(filename, "wb");
  if(fJPG == NULL)
  {
    lprintf("WARNING jpgsave: ");
    lnofile(filename);
    return LUX_FALSE;
  }

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, fJPG);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = channels;
  cinfo.in_color_space = channels < 3 ? JCS_GRAYSCALE : JCS_RGB;
  jpeg_set_defaults(&cinfo);

  jpeg_set_quality(&cinfo, quality, LUX_TRUE);
  jpeg_start_compress(&cinfo, LUX_TRUE);

  size -= row_stride;
  while (cinfo.next_scanline < cinfo.image_height)
  {
    // flip the image horizontally
    row_pointer[0] = &imageBuffer[size - ((cinfo.next_scanline) * row_stride)];
    //row_pointer[0] = &imageBuffer[cinfo.next_scanline * row_stride]; // unflipped
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  fclose(fJPG);
  jpeg_destroy_compress(&cinfo);

  return LUX_TRUE;
}

