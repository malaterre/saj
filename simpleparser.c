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

bool hasnolength( uint_fast16_t marker )
{
  switch( marker )
    {
  case FF30:
  case FF31:
  case FF32:
  case FF33:
  case FF34:
  case FF35:
  case FF36:
  case FF37:
  case FF38:
  case FF39:
  case FF3A:
  case FF3B:
  case FF3C:
  case FF3D:
  case FF3E:
  case FF3F:
  case SOC:
  case SOD:
  case EOC:
  case EPH:
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

static bool read64(FILE *input, uint64_t * ret)
{
  union { uint64_t v; char bytes[8]; } u;
  size_t l = fread(u.bytes,sizeof(char),8,input);
  if( l != 8 || feof(input) ) return false;
  *ret = bswap_64(u.v);
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

/* Take as input an open FILE* stream
 * it will not close it.
 */
static bool parsej2k_imp( FILE *stream, PrintFunctionJ2K printfun, const uintmax_t file_size )
{
  uint16_t marker;
  uintmax_t sotlen = 0;
  size_t lenmarker;
  const off_t start = ftello( stream );
  while( ftello( stream ) < (off_t)(start + file_size) && read16(stream, &marker) )
    {
    bool b;
    assert( marker ); /* debug */
    b = hasnolength( marker );
    if ( !b )
      {
      uint16_t l;
      int r = read16( stream, &l );
      if( !r ) return false;
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
          /*
           * However this is not as simple as that what if the codestream contains
           * -say- an extra zero how can we tell that ?
           */
          off_t offset = ftello(stream);
          assert( offset < SIZE_MAX );
          assert( file_size >= (uintmax_t)offset + 10 );
          assert( file_size < SIZE_MAX );
          sotlen = file_size - (uintmax_t)offset + 10;
          //fprintf( stderr, "sotlen was computed to: %d\n",  sotlen );
          }
        v = fseeko( stream, -8, SEEK_CUR );
        assert( v == 0 );
        }
      else if( sotlen )
        {
        assert( sotlen > 4 );
        /* remove size of -say- qcd item for our book keeping */
        sotlen -= (lenmarker+4);
        }
      }
    else
      {
      lenmarker = 0;
      /* marker has no lenght but we know how much to skip */
      if( marker == SOD )
        {
        assert( sotlen >= 14 );
        assert( sotlen < SIZE_MAX );
        lenmarker = sotlen - 14;
        }
      }
    if( printfun( marker, lenmarker, stream ) )
      {
      int v = fseeko(stream, (off_t)lenmarker, SEEK_CUR);
      assert( v == 0 );
      }
    }

  return true;
}

bool parsejp2( const char *filename, PrintFunctionJP2 printfun2, PrintFunctionJ2K printfun )
{
  uintmax_t file_size = getfilesize( filename );
  FILE *stream = fopen( filename, "rb" );

  uint32_t marker;
  uint64_t len64; /* ref */
  uint32_t len32; /* local 32bits op */
  assert( stream );
  while( read32(stream, &len32) )
    {
    bool b = read32(stream, &marker);
    assert( b );
    len64 = len32;
    if( len32 == 1 ) /* 64bits ? */
      {
      bool b = read64(stream, &len64);
      assert( b );
      len64 -= 8;
      }
    if( marker == JP2C )
      {
      const off_t start = ftello(stream);
      if( !len64 )
        {
        len64 = (uint64_t)(file_size - (uintmax_t)start + 8);
        }
      assert( len64 >= 8 );
      if( printfun2( marker, len64, stream ) )
        {
        bool bb;
        assert( len64 - 8 < file_size ); /* jpeg codestream cant be longer than jp2 file */
        bb = parsej2k_imp( stream, printfun, len64 - 8 /*file_size*/ );
        if( !bb )
          {
          fprintf( stderr, "*** unexpected end of codestream\n" );
          return false;
          }
        assert ( bb );
        }
      /*const off_t end = ftello(stream);*/
      assert( ftello(stream) - start == (off_t)(len64 - 8) );
      /* done with JP2C move on to remaining (trailing) stuff */
      continue;
      }

    assert( len64 >= 8 );
    if( !(len64 - 8 < file_size) ) /* jpeg codestream cant be longer than jp2 file */
      {
      return false;
      }
    if( printfun2( marker, len64, stream ) )
      {
      const size_t lenmarker = len64 - 8;
      int v = fseeko(stream, (off_t)lenmarker, SEEK_CUR);
      assert( v == 0 );
      }
    }
  assert( feof(stream) );
{
  int v = fclose( stream );
  assert( !v );
}

  return true;
}


bool parsej2k( const char *filename, PrintFunctionJP2 printfun )
{
  bool b;
  FILE *stream;
  uintmax_t eocpos = geteocposition( filename );
  uintmax_t file_size = getfilesize( filename );

  stream = fopen( filename, "rb" );
  assert( stream );

  b = parsej2k_imp( stream, printfun, file_size - eocpos );
{
  int v = fclose( stream );
  assert( !v );
}

  return b;
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

uintmax_t geteocposition( const char *filename )
{
  size_t res;
  int c = 0;
  char buf[2];
  static const char sig[2] = { (char)0xFF, (char)0xd9 };
  FILE *stream = fopen( filename, "rb" );
  int v = fseeko( stream, -2, SEEK_END );
  assert( v == 0 );

  res = fread(buf, sizeof(*buf), sizeof(buf), stream);
  assert( res == 2 );

  while( memcmp( buf, sig, 2 ) != 0 )
    {
    c++;
    v = fseeko( stream, -3, SEEK_CUR );
    assert( v == 0 );
    res = fread(buf, sizeof(char), sizeof(buf), stream);
    assert( res == 2 );
    }

  v = fclose( stream );
  assert( v == 0 );

  return c;
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
