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

#include "simpleparser.h"

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <byteswap.h>
#include <string.h>

/* stat */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* HACK */
uintmax_t file_size;

bool hasnolength( uint_fast16_t marker )
{
  switch( marker )
    {
  case SOC:
  case SOD:
  case EOC:
    return true;
    }
  return false;
}

static bool read16(FILE *input, uint16_t * ret)
{
  union { uint16_t v; char bytes[2]; } u;
  size_t l = fread(u.bytes,sizeof(char),2,input);
  if( l != 2 || feof(input) ) return false;
  /* At least on debian this generated a warning
   * http://bugs.debian.org/561249#29
   */
  *ret = bswap_16(u.v);
  return true;
}

static bool read32(FILE *input, uint32_t * ret)
{
  union { uint32_t v; char bytes[4]; } u;
  size_t l = fread(u.bytes,sizeof(char),4,input);
  if( l != 4 || feof(input) ) return false;
  *ret = bswap_32(u.v);
  return true;
}

typedef struct
{
  uint16_t Isot; /* 16   0 to 65534 */
  uint32_t Psot; /* 32   0, or 14 to (2^32 - 1) */
  uint8_t TPsot; /*  8   0 to 254 */
  uint8_t TNsot; /*  8   Table A.6 */
}  __attribute__((packed)) sot;

static uint32_t readsot( const char *a, size_t l )
{
  sot s;
  const size_t ref = sizeof( s );
  assert( l == ref );
  memcpy( &s, a, sizeof(s) );
  s.Isot = bswap_16(s.Isot);
  s.Psot = bswap_32(s.Psot);
  assert( s.TPsot < 255 );
  return s.Psot;
}

static bool parsej2k_imp( FILE *stream, PrintFunctionJ2K printfun )
{
  uint16_t marker;
  uint32_t sotlen = 1;
  size_t lenmarker;
  while( read16(stream, &marker) )
    {
    bool b = hasnolength( marker );
    if ( !b )
      {
      uint16_t l;
      int r = read16( stream, &l );
      assert( r );
      assert( l >= 2 );
      lenmarker = (size_t)l - 2;

      /* special book keeping */
      if( marker == SOT )
        {
        int v;
        char b[8];
        size_t lr = fread( b, sizeof(char), sizeof(b), stream);
        assert( lr == 8 );
        sotlen = readsot( b, sizeof(b) );
        if( !sotlen )
          {
          /* Only the last tile-part in the codestream may contain a 0 for
           * Psot. If the Psot is 0, this tile-part is assumed to contain
           * all data until the EOC marker.
           */
          off_t offset = ftello(stream);
          assert( file_size >= (uintmax_t)offset + 10 );
          assert( file_size < SIZE_MAX );
          assert( file_size < UINT32_MAX );
          sotlen = (uint32_t)(file_size - (uintmax_t)offset + 10);
          }
        v = fseeko( stream, -8, SEEK_CUR );
        assert( v == 0 );
        }
      else
        {
        sotlen -= (lenmarker+4);
        }
      }
    else
      {
      lenmarker = 0;
      /* marker has no lenght but we know how much to skip */
      if( marker == SOD )
        {
        assert( sotlen > 14 );
        lenmarker = sotlen - 14;
        }
      }
    if( printfun( marker, lenmarker, stream ) )
      {
      int v = fseeko(stream, (off_t)lenmarker, SEEK_CUR);
      assert( v == 0 );
      }
    }
  assert( feof(stream) );

  fclose( stream );
  return true;
}

bool parsejp2( const char *filename, PrintFunctionJP2 printfun2, PrintFunctionJ2K printfun )
{
  FILE *stream = fopen( filename, "rb" );

  uint32_t marker;
  uint32_t len;
  assert( stream );
  while( read32(stream, &len) )
    {
    bool b = read32(stream, &marker);
    assert( b );
    if( marker == JP2C )
      {
      assert( len == 0 );
      if( printfun2( marker, len, stream ) )
        {
        bool bb = parsej2k_imp( stream, printfun );
        assert ( bb );
        return bb;
        }
      }

    assert( len > 8 );
    if( printfun2( marker, len, stream ) )
      {
      const size_t lenmarker = len - 4 - 4;
      int v = fseeko(stream, (off_t)lenmarker, SEEK_CUR);
      assert( v == 0 );
      }
    }
  assert( feof(stream) );

  return true;
}


bool parsej2k( const char *filename, PrintFunctionJP2 printfun )
{
  FILE *stream;

  file_size = getfilesize( filename );
  stream = fopen( filename, "rb" );
  assert( stream );

  return parsej2k_imp( stream, printfun );
}

bool isjp2file( const char *filename )
{
  FILE *stream = fopen( filename, "rb" );
  int c = fgetc(stream);

  /* TODO use code from openjpeg */
  bool b = c == 0xFF ? false : true;

  int v = fclose( stream );
  assert( v == 0 );

  return b;
}

uintmax_t getfilesize( const char *filename )
{
  struct stat buf;
  if (stat(filename, &buf) != 0)
    {
    return 0;
    }
  return (uintmax_t)buf.st_size;
}
