//
// Fl_GIF_Image routines.
//
// Copyright 1997-2021 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//

//
// Reference: GIF89a Specification (links valid as of Jan 05, 2019):
//
// "GRAPHICS INTERCHANGE FORMAT(sm), Version 89a" (authoritative):
// https://www.w3.org/Graphics/GIF/spec-gif89a.txt
//
// HTML version (non-authoritative):
// https://web.archive.org/web/20160304075538/http://qalle.net/gif89a.php
//

//
// Include necessary header files...
//

#include <FL/Fl.H>
#include <FL/Fl_GIF_Image.H>
#include "Fl_Image_Reader.h"
#include <FL/fl_utf8.h>
#include "flstring.h"

#include <stdio.h>
#include <stdlib.h>

// Read a .gif file and convert it to a "xpm" format (actually my
// modified one with compressed colormaps).

// Extensively modified from original code for gif2ras by
// Patrick J. Naughton of Sun Microsystems.  The original
// copyright notice follows:

/* gif2ras.c - Converts from a Compuserve GIF (tm) image to a Sun Raster image.
 *
 * Copyright (c) 1988 by Patrick J. Naughton
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the author:
 *
 *                     Patrick J. Naughton
 *                     Sun Microsystems, Inc.
 *                     2550 Garcia Ave, MS 14-40
 *                     Mountain View, CA 94043
 *                     (415) 336-1080
 */


/*
  This small helper function checks for read errors or end of file
  and does some cleanup if an error was found.
  It returns true (1) on error, false (0) otherwise.
*/
static int gif_error(Fl_Image_Reader &rdr, int line, uchar *Image) {
  if (rdr.error()) {
    if (Image)
      delete[] Image; // delete temporary image array

    Fl::error("[%d] Fl_GIF_Image: %s - unexpected EOF or read error at offset %ld",
              line, rdr.name(), rdr.tell());
    return 1;
  }
  return 0;
}

/*
  This macro is used to check for end of file (EOF) or other read errors.
  In case of a read error or EOF an error message is issued and the image
  loading is terminated with error code ERR_FORMAT.
  This calls gif_error (see above) to avoid code duplication.
*/
#define CHECK_ERROR \
  if (gif_error(rdr, __LINE__, Image)) { \
    ld(ERR_FORMAT); \
    return; \
  }

/**
  This constructor loads a GIF image from the given file.

  If a GIF image is animated, Fl_GIF_Image will only read and display the
  first frame of the animation.

  The destructor frees all memory and server resources that are used by
  the image.

  Use Fl_Image::fail() to check if Fl_GIF_Image failed to load. fail() returns
  ERR_FILE_ACCESS if the file could not be opened or read, ERR_FORMAT if the
  GIF format could not be decoded, and ERR_NO_IMAGE if the image could not
  be loaded for another reason.

  \param[in] filename a full path and name pointing to a GIF image file.

  \see Fl_GIF_Image::Fl_GIF_Image(const char *imagename, const unsigned char *data, const long length)
*/
Fl_GIF_Image::Fl_GIF_Image(const char *filename) :
  Fl_Pixmap((char *const*)0)
{
  Fl_Image_Reader rdr;
  if (rdr.open(filename) == -1) {
    Fl::error("Fl_GIF_Image: Unable to open %s!", filename);
    ld(ERR_FILE_ACCESS);
  } else {
    load_gif_(rdr);
  }
}

/**
  This constructor loads a GIF image from memory.

  Construct an image from a block of memory inside the application. Fluid offers
  "binary data" chunks as a great way to add image data into the C++ source code.
  \p imagename can be NULL. If a name is given, the image is added to the list of
  shared images and will be available by that name.

  If a GIF image is animated, Fl_GIF_Image will only read and display the
  first frame of the animation.

  The destructor frees all memory and server resources that are used by
  the image.

  The third parameter \p length is used to test for buffer overruns,
  i.e. truncated images.

  Use Fl_Image::fail() to check if Fl_GIF_Image failed to load. fail() returns
  ERR_FILE_ACCESS if the file could not be opened or read, ERR_FORMAT if the
  GIF format could not be decoded, and ERR_NO_IMAGE if the image could not
  be loaded for another reason.

  \param[in] imagename  A name given to this image or NULL
  \param[in] data       Pointer to the start of the GIF image in memory.
  \param[in] length     Length of the GIF image in memory.

  \see Fl_GIF_Image::Fl_GIF_Image(const char *filename)
  \see Fl_Shared_Image

  \since 1.4.0
*/
Fl_GIF_Image::Fl_GIF_Image(const char *imagename, const unsigned char *data, const size_t length) :
  Fl_Pixmap((char *const*)0)
{
  Fl_Image_Reader rdr;
  if (rdr.open(imagename, data, length) == -1) {
    ld(ERR_FILE_ACCESS);
  } else {
    load_gif_(rdr);
  }
}

/**
  This constructor loads a GIF image from memory (deprecated).

  \deprecated Please use
    Fl_GIF_Image(const char *imagename, const unsigned char *data, const size_t length)
    instead.

  \note Buffer overruns will not be checked.

  This constructor should not be used because the caller can't supply the
  memory size and the image reader can't check for "end of memory" errors.

  \note A new constructor with parameter \p length is available since FLTK 1.4.0.

  \param[in] imagename  A name given to this image or NULL
  \param[in] data       Pointer to the start of the GIF image in memory.

  \see Fl_GIF_Image(const char *filename)
  \see Fl_GIF_Image(const char *imagename, const unsigned char *data, const size_t length)
*/
Fl_GIF_Image::Fl_GIF_Image(const char *imagename, const unsigned char *data) :
  Fl_Pixmap((char *const*)0)
{
  Fl_Image_Reader rdr;
  if (rdr.open(imagename, data) == -1) {
    ld(ERR_FILE_ACCESS);
  } else {
    load_gif_(rdr);
  }
}

/*
  This method reads GIF image data and creates an RGB or RGBA image. The GIF
  format supports only 1 bit for alpha. The final image data is stored in
  a modified XPM format (Fl_GIF_Image is a subclass of Fl_Pixmap).
  To avoid code duplication, we use an Fl_Image_Reader that reads data from
  either a file or from memory.
*/
void Fl_GIF_Image::load_gif_(Fl_Image_Reader &rdr)
{
  char **new_data;      // Data array
  uchar *Image = 0L;    // internal temporary image data array
  w(0); h(0);

  // printf("\nFl_GIF_Image::load_gif_ : %s\n", rdr.name());

  {char b[6] = { 0 };
    for (int i=0; i<6; ++i) b[i] = rdr.read_byte();
    if (b[0]!='G' || b[1]!='I' || b[2] != 'F') {
      Fl::error("Fl_GIF_Image: %s is not a GIF file.\n", rdr.name());
      ld(ERR_FORMAT);
      return;
    }
    if (b[3]!='8' || b[4]>'9' || b[5]!= 'a')
      Fl::warning("%s is version %c%c%c.",rdr.name(),b[3],b[4],b[5]);
  }

  int Width = rdr.read_word();
  int Height = rdr.read_word();

  uchar ch = rdr.read_byte();
  CHECK_ERROR
  char HasColormap = ((ch & 0x80) != 0);
  int BitsPerPixel = (ch & 7) + 1;
  int ColorMapSize;
  if (HasColormap) {
    ColorMapSize = 2 << (ch & 7);
  } else {
    ColorMapSize = 0;
  }
  // int OriginalResolution = ((ch>>4)&7)+1;
  // int SortedTable = (ch&8)!=0;
  ch = rdr.read_byte(); // Background Color index
  ch = rdr.read_byte(); // Aspect ratio is N/64
  CHECK_ERROR

  // Read in global colormap:
  uchar transparent_pixel = 0;
  char has_transparent = 0;
  uchar Red[256], Green[256], Blue[256]; /* color map */
  if (HasColormap) {
    for (int i=0; i < ColorMapSize; i++) {
      Red[i] = rdr.read_byte();
      Green[i] = rdr.read_byte();
      Blue[i] = rdr.read_byte();
    }
  }
  CHECK_ERROR

  int CodeSize;         /* Code size, init from GIF header, increases... */
  char Interlace;

  // Main parser loop: parse "blocks" until an image is found or error

  for (;;) {

    int i = rdr.read_byte();
    CHECK_ERROR
    int blocklen;

    if (i == 0x21) {                          // a "gif extension"
      ch = rdr.read_byte();                   // extension type
      blocklen = rdr.read_byte();
      CHECK_ERROR

      if (ch == 0xF9 && blocklen == 4) {      // Graphic Control Extension
        // printf("Graphic Control Extension at offset %ld\n", rdr.tell()-2);
        char bits = rdr.read_byte();          // Packed Fields
        rdr.read_word();                      // Delay Time
        transparent_pixel = rdr.read_byte();  // Transparent Color Index
        blocklen = rdr.read_byte();           // Block Terminator (must be zero)
        CHECK_ERROR
        if (bits & 1) has_transparent = 1;
      }
      else if (ch == 0xFF) {                  // Application Extension
        // printf("Application Extension at offset %ld, length = %d\n", rdr.tell()-3, blocklen);
        ; // skip data
      }
      else if (ch == 0xFE) {                  // Comment Extension
        // printf("Comment Extension at offset %ld, length = %d\n", rdr.tell()-3, blocklen);
        ; // skip data
      }
      else if (ch == 0x01) {                  // Plain Text Extension
        // printf("Plain Text Extension at offset %ld, length = %d\n", rdr.tell()-3, blocklen);
        ; // skip data
      }
      else {
        Fl::warning("%s: unknown GIF extension 0x%02x at offset %ld, length = %d",
                    rdr.name(), ch, rdr.tell()-3, blocklen);
        ; // skip data
      }
    } else if (i == 0x2c) {       // an image: Image Descriptor follows
      // printf("Image Descriptor at offset %ld\n", rdr.tell());
      rdr.read_word();          // Image Left Position
      rdr.read_word();          // Image Top Position
      Width = rdr.read_word();  // Image Width
      Height = rdr.read_word(); // Image Height
      ch = rdr.read_byte();     // Packed Fields
      CHECK_ERROR
      Interlace = ((ch & 0x40) != 0);
      if (ch & 0x80) {          // image has local color table
        // printf("Local Color Table at offset %ld\n", rdr.tell());
        BitsPerPixel = (ch & 7) + 1;
        ColorMapSize = 2 << (ch & 7);
        for (i=0; i < ColorMapSize; i++) {
          Red[i] = rdr.read_byte();
          Green[i] = rdr.read_byte();
          Blue[i] = rdr.read_byte();
        }
      }
      CHECK_ERROR
      break; // okay, this is the image we want
    } else if (i == 0x3b) {       // Trailer (end of GIF data)
      // printf("Trailer found at offset %ld\n", rdr.tell());
      Fl::error("%s: no image data found.", rdr.name());
      ld(ERR_NO_IMAGE); // this GIF file is "empty" (no image)
      return;           // terminate
    } else {
      Fl::error("%s: unknown GIF code 0x%02x at offset %ld", rdr.name(), i, rdr.tell()-1);
      ld(ERR_FORMAT); // broken file
      return;         // terminate
    }
    CHECK_ERROR

    // skip all data (sub)blocks:
    while (blocklen > 0) {
      rdr.skip(blocklen);
      blocklen = rdr.read_byte();
    }
    // printf("End of data (sub)blocks at offset %ld\n", rdr.tell());
  }

  // read image data

  // printf("Image Data at offset %ld\n", rdr.tell());

  CodeSize = rdr.read_byte() + 1; // LZW Minimum Code Size
  CHECK_ERROR

  if (BitsPerPixel >= CodeSize) { // Workaround for broken GIF files...
    BitsPerPixel = CodeSize - 1;
    ColorMapSize = 1 << BitsPerPixel;
  }

  // Fix images w/o color table. The standard allows this and lets the
  // decoder choose a default color table. The standard recommends the
  // first two color table entries should be black and white.

  if (ColorMapSize == 0) { // no global and no local color table
    Fl::warning("%s does not have a color table, using default.\n", rdr.name());
    BitsPerPixel = CodeSize - 1;
    ColorMapSize = 1 << BitsPerPixel;
    Red[0] = Green[0] = Blue[0] = 0;    // black
    Red[1] = Green[1] = Blue[1] = 255;  // white
    for (int i = 2; i < ColorMapSize; i++) {
      Red[i] = Green[i] = Blue[i] = (uchar)(255 * i / (ColorMapSize - 1));
    }
  }

  // Fix transparent pixel index outside ColorMap (Issue #271)
  if (has_transparent && transparent_pixel >= ColorMapSize) {
    for (int k = ColorMapSize; k <= transparent_pixel; k++)
      Red[k] = Green[k] = Blue[k] = 0xff; // white (color is irrelevant)
    ColorMapSize = transparent_pixel + 1;
  }

#if (0) // TEST/DEBUG: fill color table to maximum size
  for (int i = ColorMapSize; i < 256; i++) {
    Red[i] = Green[i] = Blue[i] = 0; // black
  }
#endif

  CHECK_ERROR

  // now read the LZW compressed image data

  Image = new uchar[Width*Height];

  int YC = 0, Pass = 0; /* Used to de-interlace the picture */
  uchar *p = Image;
  uchar *eol = p+Width;

  int InitCodeSize = CodeSize;
  int ClearCode = (1 << (CodeSize-1));
  int EOFCode = ClearCode + 1;
  int FirstFree = ClearCode + 2;
  int FinChar = 0;
  int ReadMask = (1<<CodeSize) - 1;
  int FreeCode = FirstFree;
  int OldCode = ClearCode;

  // tables used by LZW decompressor:
  short int Prefix[4096];
  uchar Suffix[4096];

  int blocklen = rdr.read_byte();
  uchar thisbyte = rdr.read_byte(); blocklen--;
  CHECK_ERROR
  int frombit = 0;

  // loop to read LZW compressed image data

  for (;;) {

    /* Fetch the next code from the raster data stream.  The codes can be
     * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
     * maintain our location as a pointer and a bit offset.
     * In addition, GIF adds totally useless and annoying block counts
     * that must be correctly skipped over. */
    int CurCode = thisbyte;
    if (frombit+CodeSize > 7) {
      if (blocklen <= 0) {
        blocklen = rdr.read_byte();
        CHECK_ERROR
        if (blocklen <= 0) break;
      }
      thisbyte = rdr.read_byte(); blocklen--;
      CHECK_ERROR
      CurCode |= thisbyte<<8;
    }
    if (frombit+CodeSize > 15) {
      if (blocklen <= 0) {
        blocklen = rdr.read_byte();
        CHECK_ERROR
        if (blocklen <= 0) break;
      }
      thisbyte = rdr.read_byte(); blocklen--;
      CHECK_ERROR
      CurCode |= thisbyte<<16;
    }
    CurCode = (CurCode>>frombit)&ReadMask;
    frombit = (frombit+CodeSize)%8;

    if (CurCode == ClearCode) {
      CodeSize = InitCodeSize;
      ReadMask = (1<<CodeSize) - 1;
      FreeCode = FirstFree;
      OldCode = ClearCode;
      continue;
    }

    if (CurCode == EOFCode)
      break;

    uchar OutCode[4097]; // temporary array for reversing codes
    uchar *tp = OutCode;
    int i;
    if (CurCode < FreeCode) {
      i = CurCode;
    } else if (CurCode == FreeCode) {
      *tp++ = (uchar)FinChar;
      i = OldCode;
    } else {
      Fl::error("Fl_GIF_Image: %s - LZW Barf at offset %ld", rdr.name(), rdr.tell());
      break;
    }

    while (i >= ColorMapSize) {
      if (i < FreeCode) {
        *tp++ = Suffix[i];
        i = Prefix[i];
      } else { // FIXME - should never happen (?)
        Fl::error("Fl_GIF_Image: %s - i(%d) >= FreeCode (%d) at offset %ld",
                  rdr.name(), i, FreeCode, rdr.tell());
        // NOTREACHED
        i = FreeCode - 1; // fix broken index ???
        break;
      }
    }
    *tp++ = FinChar = i;
    do {
      *p++ = *--tp;
      if (p >= eol) {
        if (!Interlace) YC++;
        else switch (Pass) {
          case 0: YC += 8; if (YC >= Height) {Pass++; YC = 4;} break;
          case 1: YC += 8; if (YC >= Height) {Pass++; YC = 2;} break;
          case 2: YC += 4; if (YC >= Height) {Pass++; YC = 1;} break;
          case 3: YC += 2; break;
        }
        if (YC>=Height) YC=0; /* cheap bug fix when excess data */
        p = Image + YC*Width;
        eol = p+Width;
      }
    } while (tp > OutCode);

    if (OldCode != ClearCode) {
      if (FreeCode < 4096) {
        Prefix[FreeCode] = (short)OldCode;
        Suffix[FreeCode] = FinChar;
        FreeCode++;
      }
      if (FreeCode > ReadMask) {
        if (CodeSize < 12) {
          CodeSize++;
          ReadMask = (1 << CodeSize) - 1;
        }
      }
    }
    OldCode = CurCode;
  }

  // We are done reading the image, now convert to xpm

  w(Width);
  h(Height);
  d(1);

  // allocate line pointer arrays:
  new_data = new char*[Height+2];

  // transparent pixel must be zero, swap if it isn't:
  if (has_transparent && transparent_pixel != 0) {
    // swap transparent pixel with zero
    p = Image+Width*Height;
    while (p-- > Image) {
      if (*p==transparent_pixel) *p = 0;
      else if (!*p) *p = transparent_pixel;
    }
    uchar t;
    t                        = Red[0];
    Red[0]                   = Red[transparent_pixel];
    Red[transparent_pixel]   = t;

    t                        = Green[0];
    Green[0]                 = Green[transparent_pixel];
    Green[transparent_pixel] = t;

    t                        = Blue[0];
    Blue[0]                  = Blue[transparent_pixel];
    Blue[transparent_pixel]  = t;
  }

  // find out what colors are actually used:
  uchar used[256]; uchar remap[256];
  int i;
  for (i = 0; i < ColorMapSize; i++) used[i] = 0;
  p = Image+Width*Height;
  while (p-- > Image) used[*p] = 1;

  // remap them to start with printing characters:
  int base = has_transparent && used[0] ? ' ' : ' '+1;
  int numcolors = 0;
  for (i = 0; i < ColorMapSize; i++) if (used[i]) {
    remap[i] = (uchar)(base++);
    numcolors++;
  }

  // write the first line of xpm data (use suffix as temp array):
  int length = snprintf((char*)(Suffix), sizeof(Suffix),
                       "%d %d %d %d",Width,Height,-numcolors,1);
  new_data[0] = new char[length+1];
  strcpy(new_data[0], (char*)Suffix);

  // write the colormap
  new_data[1] = (char*)(p = new uchar[4*numcolors]);
  for (i = 0; i < ColorMapSize; i++) if (used[i]) {
    *p++ = remap[i];
    *p++ = Red[i];
    *p++ = Green[i];
    *p++ = Blue[i];
  }

  // remap the image data:
  p = Image+Width*Height;
  while (p-- > Image) *p = remap[*p];

  // split the image data into lines:
  for (i=0; i<Height; i++) {
    new_data[i+2] = new char[Width+1];
    memcpy(new_data[i + 2], (char*)(Image + i*Width), Width);
    new_data[i + 2][Width] = 0;
  }

  data((const char **)new_data, Height + 2);
  alloc_data = 1;

  delete[] Image;

} // load_gif_()
