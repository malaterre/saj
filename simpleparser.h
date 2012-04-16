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
  COLR = 0x636f6c72
} OtherType;

/* List of well known markers */
typedef enum {
  SOC = 0xFF4F,
  SIZ = 0xFF51,
  COD = 0xFF52,
  COC = 0xFF53,
  TLM = 0xFF55,
  QCD = 0xFF5C,
  QCC = 0xFF5D,
  RGN = 0xFF5E,
  POC = 0xFF5F,
  CRG = 0xFF63,
  COM = 0xFF64,
  SOT = 0xFF90,
  SOP = 0xFF91,
  SOD = 0xFF93,
  EOC = 0XFFD9, /* EOI in old jpeg */
  PLM = 0XFF57,
  PLT = 0XFF58,
  PPM = 0XFF60,
  PPT = 0XFF61,
  EPH = 0XFF92
} MarkerType;

/**
 * Function param that will be used to print an element (marker + len + data)
 * when this function return true this is the default behavior and the simple
 * parser will default to skipping `len` bytes. If returning false
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
 * Return whether or not a marker has a fixed length
 */
bool hasnolength( uint_fast16_t marker );

/* return true if JP2 file */
bool isjp2file( const char *filename );

/* return size of file */
uintmax_t getfilesize( const char *filename );

#endif
