// Copyright (C) 2004-2011 Christoph Kubisch & Eike Decker
// This file is part of the "Luxinia Engine".
// For conditions of distribution and use, see luxinia.h


#include "../common/vid.h"
#include "../common/common.h"
#include "../fileio/png.h"
#include "../fileio/filesystem.h"


#include <libpng/png.h>



//PNG Loading
// Based on IrrLicht's implementation
// http://www.irrlicht.org


// semi global buffer for reading in row data

unsigned char *l_png_load_buffer;   // 32768

static const uchar *readRow(void *row_ptr)
{
  png_read_row((png_structp)row_ptr, (png_bytep)l_png_load_buffer, 0);
  return (const unsigned char*)l_png_load_buffer;
}

static void png_cpexcept_error(png_structp png_ptr, png_const_charp msg)
{
  if(png_ptr)
  {
    lprintf("WARNING pngload: error - %s\n",msg);
  }
}

// PNG function for file reading
void user_read_data_fcn(png_structp png_ptr,png_bytep data, png_size_t length)
{
  png_size_t check;


  // changed by zola {
  lxFSFile_t* file=(lxFSFile_t*)png_ptr->io_ptr;
  check=(png_size_t) lxFS_read((void*)data,1,length,file);
  // }

  if (check != length)
    png_error(png_ptr, "Read Error");
}

int fileLoadPNG(const char *filename, Texture_t *texture, void *unused)
{
  //some variables
  unsigned int width;
  unsigned int height;
  int bitdepth;
  int colortype;
  int interlace;
  int compression;
  int filter;
  int bytes_per_row;
  int alphaSupport = LUX_TRUE;

  char lbuf[128];

  png_structp png_ptr;
  png_infop info_ptr;

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
    lprintf("WARNING pngload: "); // Display an error message
    lnofile(filename);
    return LUX_FALSE;                     // Exit function
  }

  buffer = lxFS_getContent(file);

  lprintf("Texture: \t%s\n",filename);
  lxFS_read(lbuf,sizeof(uchar),8,file);

  // CHeck if it really is a PNG file
  if( !png_check_sig((png_bytep)lbuf,8) )
  {
    lprintf("WARNING pngload: wrong header %s\n",filename);
    FS_close(file);
    return LUX_FALSE;
  }

  // Allocate the png read struct
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,(png_error_ptr)png_cpexcept_error, (png_error_ptr)0 );
  if (!png_ptr)
  {
    lprintf("WARNING pngload: Internal PNG create read struct failure %s\n",filename);
    FS_close(file);
    return LUX_FALSE;
  }

  // Allocate the png info struct
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    lprintf("WARNING pngload: Internal PNG create info struct failure %s\n",filename);
    png_destroy_read_struct(&png_ptr, 0, 0);
    FS_close(file);
    return LUX_FALSE;
  }

  //png_init_io(png_ptr,fp);   // Init png
  // changed by zola so we don't need to have public FILE pointers
  png_set_read_fn(png_ptr, file, user_read_data_fcn);

  png_set_sig_bytes(png_ptr, 8);   // Tell png that we read the signature

  png_read_info(png_ptr, info_ptr);   // Read the info section of the png file

  png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width,
    (png_uint_32*)&height, &bitdepth, &colortype, &interlace,
    &compression, &filter);   // Extract info

  if ( bitdepth != 8)
  {
    lprintf("WARNING pngload: Failure - Only 8 bits per color supported %s\n",filename);
    FS_close(file);
    if(png_ptr)
      png_destroy_read_struct(&png_ptr,&info_ptr, 0);   // Clean up memory
    return LUX_FALSE;
  }

  if (colortype==PNG_COLOR_TYPE_RGBA)
    alphaSupport = LUX_TRUE;
  else if (colortype==PNG_COLOR_TYPE_RGB)
    alphaSupport = LUX_FALSE;
  else
  {
    lprintf("WARNING pngload: Failure - Format not supported - must be 24 or 32 bits per pixel %s\n",filename);
    FS_close(file);
    if(png_ptr)
      png_destroy_read_struct(&png_ptr,&info_ptr, 0);   // Clean up memory
    return LUX_FALSE;
  }

  if ( interlace!= PNG_INTERLACE_NONE)
  {
    lprintf("WARNING pngload: Failure - Format not supported - must be non-interlaced %s\n",filename);
    FS_close(file);
    if(png_ptr)
      png_destroy_read_struct(&png_ptr,&info_ptr, 0);   // Clean up memory
    return LUX_FALSE;
  }

  // Update the changes
  png_read_update_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr,
    (png_uint_32*)&width, (png_uint_32*)&height,
    &bitdepth,&colortype, &interlace, &compression,
    &filter);   // Extract info

  // Check the number of bytes per row
  bytes_per_row = png_get_rowbytes(png_ptr, info_ptr);
  l_png_load_buffer = (char*)lxMemGenAlloc(bytes_per_row);
  /*if( bytes_per_row >= (int)sizeof( l_png_load_buffer ) )
  {
    lprintf("WARNING pngload: Failure - Format not supported %s\n",filename);
    FS_close(file);
    if(png_ptr)
      png_destroy_read_struct(&png_ptr,&info_ptr, 0);   // Clean up memory
    return FALSE;
  }*/


  //now read the png stucture into a simple bitmap array
  texture->width = width;
  texture->height = height;
  texture->depth = 1;

  texture->datatype = GL_UNSIGNED_BYTE;
  texture->sctype = LUX_SCALAR_UINT8;

  if (alphaSupport){
    texture->bpp = 32;
    texture->components = 4;
    texture->dataformat = GL_RGBA;
    texture->imageData = lxMemGenZalloc(sizeof(uchar)*4*width*height);
  }
  else {
    texture->bpp = 24;
    texture->components = 3;
    texture->dataformat = GL_RGB;
    texture->imageData = lxMemGenZalloc(sizeof(uchar)*3*width*height);
  }


  {
    const unsigned char* pSrc;
    const unsigned char inc = alphaSupport ? 4 : 3;
    unsigned char* data = texture->imageData;
    int y,x;

    for ( y = (int)texture->height-1; y >= 0; y-- )
    {
      //read in a row of pixels
      pSrc = readRow(png_ptr);

      for ( x = 0; x < texture->width; x++ )
      {
        //loop through the row of pixels
        if (!alphaSupport)
        {
          data[y*width*inc + x*inc] = *(pSrc); //red
          data[y*width*inc + x*inc + 1] = *(pSrc+1); //green
          data[y*width*inc + x*inc + 2] = *(pSrc+2); //blue
        }
        else
        {
          data[y*width*inc + x*inc] = *(pSrc); //red
          data[y*width*inc + x*inc + 1] = *(pSrc+1); //green
          data[y*width*inc + x*inc + 2] = *(pSrc+2); //blue
          data[y*width*inc + x*inc + 3] = *(pSrc+3); //alpha
        }

        pSrc+=inc; //move to next pixel (24 or 32 bits - depends on alpha)
      }
    }

  }
  if (png_ptr)
    png_destroy_read_struct(&png_ptr,&info_ptr, 0);   // Clean up memory
  FS_close(file);

  lxMemGenFree(l_png_load_buffer,bytes_per_row);
  return LUX_TRUE;

}

