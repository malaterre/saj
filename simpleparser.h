/*
 * Copyright (c) 2012, Mathieu Malaterre <mathieu.malaterre@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef simpleparser_h
#define simpleparser_h

#include <stdint.h>
#include <stdlib.h> /* size_t */
#include <stdbool.h>
#include <stdio.h> /* FILE */

typedef enum {
  JP   = 0x6a502020,
  FTYP = 0x66747970,
  JP2H = 0x6a703268,
  JP2C = 0x6a703263,
  JP2  = 0x6a703220,
  IHDR = 0x69686472,
  COLR = 0x636f6c72,
  XML  = 0x786d6c20,
  CDEF = 0x63646566,
  CMAP = 0x636D6170,
  PCLR = 0x70636c72,
  RES  = 0x72657320,
  /* JPIP */
  IPTR = 0x69707472
} OtherType;

/* Table A.2 List of markers and marker segments */
typedef enum {
  FF30 = 0xFF30,
  FF31 = 0xFF31,
  FF32 = 0xFF32,
  FF33 = 0xFF33,
  FF34 = 0xFF34,
  FF35 = 0xFF35,
  FF36 = 0xFF36,
  FF37 = 0xFF37,
  FF38 = 0xFF38,
  FF39 = 0xFF39,
  FF3A = 0xFF3A,
  FF3B = 0xFF3B,
  FF3C = 0xFF3C,
  FF3D = 0xFF3D,
  FF3E = 0xFF3E,
  FF3F = 0xFF3F,
  SOC = 0xFF4F,
  SIZ = 0xFF51,
  COD = 0xFF52,
  COC = 0xFF53,
  TLM = 0xFF55,
  PLM = 0xFF57,
  PLT = 0xFF58,
  QCD = 0xFF5C,
  QCC = 0xFF5D,
  RGN = 0xFF5E,
  POC = 0xFF5F,
  PPM = 0xFF60,
  PPT = 0xFF61,
  CRG = 0xFF63,
  COM = 0xFF64,
  SOT = 0xFF90,
  SOP = 0xFF91,
  EPH = 0xFF92,
  SOD = 0xFF93,
  EOC = 0xFFD9  /* EOI in old jpeg */
} MarkerType;

/**
 * Function param that will be used to print an element (marker + len + data)
 * when this function return true this is the default behavior and the simple
 * parser will default to skipping `len` bytes. If returning false
 * The file `stream` is left open right after the length of the marker (or the
 * maker itself when no length).
 *
 * The function does not take as input a MarkerType, but a uint16_t as we could
 * see application specific marker.
 */
typedef bool (*PrintFunctionJ2K)( uint_fast16_t marker, size_t len, FILE* stream );

/**
 * Function param to print a JP2 marker
 */
typedef bool (*PrintFunctionJP2)( uint_fast32_t marker, size_t len, FILE* stream );

/**
 * Main entry point
 * Use to parse a J2K (JPEG 2000 Codestream)
 */
bool parsej2k( const char *filename, PrintFunctionJ2K fj2k );

/**
 * Main entry point
 * Use to parse a JP2 file (JPEG 2000 File)
 */
bool parsejp2( const char *filename, PrintFunctionJP2 fjp2, PrintFunctionJ2K fj2k );

/**
 * Return whether or not a marker has no length
 */
bool hasnolength( uint_fast16_t marker );

/* return true if JP2 file */
bool isjp2file( const char *filename );

/* return size of file */
uintmax_t getfilesize( const char *filename );

/**
 * return 0, when EOC is at end of file,
 * return 1, when EOC is one byte before end
 * ...
 */
uintmax_t geteocposition( const char *filename );

#endif
