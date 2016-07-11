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

/*
 * kdu_expand -i input.jp2 -o output.rawl -record r.txt
 */
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <byteswap.h>
#include <string.h>

#include <simpleparser.h>

FILE * fout;

const char *sprofile;

static bool read8(FILE *input, uint8_t * ret)
{
  union { uint8_t v; char bytes[1]; } u;
  size_t l = fread(u.bytes,sizeof(char),1,input);
  if( l != 1 || feof(input) ) return false;
  *ret = u.v;
  return true;
}

static void cread16(const char *input, uint16_t * ret)
{
  union { uint16_t v; char bytes[2]; } u;
  memcpy(u.bytes,input,2);
  *ret = bswap_16(u.v);
}

static void cread32(const char *input, uint32_t * ret)
{
  union { uint32_t v; char bytes[4]; } u;
  memcpy(u.bytes,input,4);
  *ret = bswap_32(u.v);
}

static bool read16(FILE *input, uint16_t * ret)
{
  union { uint16_t v; char bytes[2]; } u;
  size_t l = fread(u.bytes,sizeof(char),2,input);
  if( l != 2 || feof(input) ) return false;
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


/* Table A.16 Progression order for the SGcod, SPcoc, and Ppoc parameters */
static const char *getDescriptionOfProgressionOrderString(uint8_t progressionOrder)
{
  const char *descriptionOfProgressionOrder = "Reserved";
  switch (progressionOrder) {
    case 0x00:
      descriptionOfProgressionOrder = "LRCP";
      break;
    case 0x01:
      descriptionOfProgressionOrder = "RLCP";
      break;
    case 0x02:
      descriptionOfProgressionOrder = "RPCL";
      break;
    case 0x03:
      descriptionOfProgressionOrder = "PCRL";
      break;
    case 0x04:
      descriptionOfProgressionOrder = "CPRL";
      break;
  }
  return descriptionOfProgressionOrder;
}

/* Table A.20 - Transformation for the SPcod and SPcoc parameters */
static const char *getDescriptionOfWaveletTransformationString(uint8_t waveletTransformation)
{
  const char *descriptionOfWaveletTransformation = "Reserved";
  switch (waveletTransformation) {
    case 0x00:
      descriptionOfWaveletTransformation = "W9X7";
      break;
    case 0x01:
      descriptionOfWaveletTransformation = "W5X3";
      break;
  }
  return descriptionOfWaveletTransformation;
}

uintmax_t data_size;
uintmax_t file_size;

static void printeoc( FILE *stream, size_t len )
{
  (void)stream;
  (void)len;
}

static uint16_t csiz;

static void printrgn( FILE *stream, size_t len )
{
  bool b;
  uint16_t crgn;
  uint8_t srgn;
  uint8_t sprgn;

  if( csiz < 257 )
    {
    uint8_t crgn8;
    b = read8(stream, &crgn8); assert( b );
    --len;
    crgn = crgn8;
    }
  else
    {
    b = read16(stream, &crgn); assert( b );
    --len;
    --len;
    }

  b = read8(stream, &srgn); assert( b );
  b = read8(stream, &sprgn); assert( b );
}

static void printpoc( FILE *stream, size_t len )
{
  char buffer[512];
  size_t r = fread( buffer, sizeof(char), len, stream);

  const uint8_t *p = (const uint8_t*)buffer;
  const uint8_t *end = p + len;
  int i = 0;
  int number_progression_order_change = 0;
  assert( len < 512 );
  assert( r == len );
  if( csiz < 257 )
    {
    number_progression_order_change = ( len )/ 7;
    }
  else
    {
    number_progression_order_change = ( len )/ 9;
    }
  for( ; i < number_progression_order_change; ++i )
    {
    uint8_t rspoc = *p++;
    uint16_t cspoc;
    uint16_t lyepoc;
    uint16_t cepoc;
    uint8_t repoc;
    (void)rspoc;
    if( csiz < 257 )
      {
      cspoc = *p++;
      }
    else
      {
      cread16((char*)p, &cspoc);
      p += 2;
      }
    cread16((char*)p, &lyepoc);
    p += 2;
    repoc = *p++;
    if( csiz < 257 )
      {
      cepoc = *p++;
      }
    else
      {
      cread16((char*)p, &cepoc);
      p += 2;
      }
    p++;
    }

  assert( p == end );
}

static void printqcc( FILE *stream, size_t len )
{
  bool b;
  uint16_t cqcc;
  size_t i;
  const char *s = "reserved";
  uint8_t quant;
  uint8_t nbits;
  uint8_t sqcc;

  if( csiz < 257 )
    {
    uint8_t cqcc8;
    b = read8(stream, &cqcc8); assert( b );
    --len;
    cqcc = cqcc8;
    }
  else
    {
    b = read16(stream, &cqcc); assert( b );
    --len;
    --len;
    }

  b = read8(stream, &sqcc); assert( b );
  --len;
  quant = (sqcc & 0x1f);
  nbits = (sqcc >> 5);
  switch( quant )
    {
  case 0x0:
    s = "none";
    break;
  case 0x1:
    s = "scalar derived";
    break;
  case 0x2:
    s = "scalar expounded";
    break;
    }

  if( quant == 0x0 )
    {
    size_t n = len;
    for( i = 0; i != n; ++i )
      {
      uint8_t val;
      bool b = read8(stream, &val);
      assert( b );
      }
    }
  else
    {
    size_t n = len / 2;
    for( i = 0; i != n; ++i )
      {
      uint16_t val;
      b = read16(stream, &val); assert( b );
      }
    }

}

uint8_t quant  ;
uint8_t nbits ;
size_t abs_ranges;
uint8_t *exps = NULL;
static void printqcd( FILE *stream, size_t len )
{
  bool b;
  uint8_t sqcd;
  size_t i;
  const char *s = "reserved";

  b = read8(stream, &sqcd); assert( b );
  --len;
  quant = (sqcd & 0x1f);
  nbits = (sqcd >> 5);
  switch( quant )
    {
  case 0x0:
    s = "none";
    break;
  case 0x1:
    s = "scalar derived";
    break;
  case 0x2:
    s = "scalar expounded";
    break;
    }

  if( quant == 0x0 )
    {
    size_t n = len;
    abs_ranges = n;
    exps = malloc( len * sizeof(char) );
    for( i = 0; i != n; ++i )
      {
      uint8_t val;
      bool b = read8(stream, &val);
      const uint8_t exp = val >> 3;
      assert( b );
      exps[i] = exp;
      }
    }
  else
    {
    size_t n = len / 2;
    assert( len == n * 2 );
    for( i = 0; i != n; ++i )
      {
      uint16_t val;
      b = read16(stream, &val); assert( b );
      }
    }
}

static void printeph( FILE *stream, size_t len )
{
  int c =0;
  while( fgetc( stream ) != 0xFF )
    {
    ++c;
    }

  if( c )
    {
    }
  fseeko(stream, -1, SEEK_CUR);
  (void)len;
}

static void printsop( FILE *stream, size_t len )
{
  uint16_t Nsop;
  bool b;
  int c =0;
  assert( len == 2 );
  b = read16(stream, &Nsop); assert( b );

  while( fgetc( stream ) != 0xFF )
    {
    ++c;
    }

  data_size += c;
  fseeko(stream, -1, SEEK_CUR);
}

static void printsod( FILE *stream, size_t len )
{
  fseeko(stream, (off_t)len, SEEK_CUR);
}

size_t ntiles;
static void printsot( FILE *stream, size_t len )
{
  uint16_t Isot;
  uint32_t Psot;
  uint8_t  TPsot;
  uint8_t  TNsot;
  bool b;
  b = read16(stream, &Isot); assert( b );
  b = read32(stream, &Psot); assert( b );
  b = read8(stream, &TPsot); assert( b );
  b = read8(stream, &TNsot); assert( b );
  ++ntiles;
  (void)len;
}

static void printcomment( FILE *stream, size_t len )
{
  uint16_t rcom;
  bool b;
  b = read16(stream, &rcom); assert( b );
  len -= 2;
  if( len < 512 )
    {
    char buffer[512];
    size_t l = fread(buffer,sizeof(char),len,stream);
    buffer[len] = 0;
    assert( l == len );
    }
  else
    {
    fseeko(stream, (off_t)len, SEEK_CUR);
    }

}

static void printtlm( FILE *stream, size_t len )
{
  char buffer[512];
  fread( buffer, sizeof(char), len, stream);
}


static void printcoc( FILE *stream, size_t len )
{
  char buffer[512];
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( len < 512 );
  assert( r == len );
}

  uint8_t MultipleComponentTransformation;
  uint16_t NumberOfLayers  ;
  bool SOPMarkerSegments   ;
  bool EPHMarkerSegments   ;
  const char * sProgressionOrder = NULL;
  uint8_t NumberOfDecompositionLevels ;
  uint8_t xcb  ;
  uint8_t ycb  ;
  uint8_t Transformation ;
  const char * sTransformation  ;
  bool VariablePrecinctSize ;
  uint8_t CodeBlockStyle ;

typedef enum 
{
  BYPASS = 0x01,
  RESET = 0x2,
  RESTART = 0x4,
  CAUSAL = 0x8,
  ERTERM = 0x10,
  SEGMARK = 0x20
} modes;

uint8_t *precintsize = NULL;

static void printcod( FILE *stream, size_t len )
{
  union { uint16_t v; char bytes[2]; } u16;
  char buffer[512];
  size_t r = fread( buffer, sizeof(char), len, stream);

/*  union { uint32_t v; char bytes[4]; } u32; */

/* Table A.12 - Coding style default parameter values */
  const uint8_t *p = (const uint8_t*)buffer;
  const uint8_t *end = p + len;
  uint8_t Scod = *p++;
  uint8_t ProgressionOrder = *p++;
  assert( len < 512 );
  assert( r == len );
  u16.bytes[0] = (char)*p++;
  u16.bytes[1] = (char)*p++;
  NumberOfLayers = bswap_16( u16.v );
  MultipleComponentTransformation = *p++;
  NumberOfDecompositionLevels = *p++;
{
  uint8_t CodeBlockWidth = *p++ & 0xf;
  uint8_t CodeBlockHeight = *p++ & 0xf;
  xcb = CodeBlockWidth + 2;
  ycb = CodeBlockHeight + 2;
  assert( xcb + ycb <= 12 );
}
  CodeBlockStyle = *p++;
  Transformation = *p++;

  sProgressionOrder = getDescriptionOfProgressionOrderString(ProgressionOrder);
  sTransformation = getDescriptionOfWaveletTransformationString(Transformation);
  /* Table A.13 Coding style parameter values for the Scod parameter */
  VariablePrecinctSize = (Scod & 0x01) != 0;
  SOPMarkerSegments    = (Scod & 0x02) != 0;
  EPHMarkerSegments   = (Scod & 0x04) != 0;

  if( VariablePrecinctSize )
    {
    uint_fast8_t i;
    precintsize = malloc( (NumberOfDecompositionLevels + 1) * sizeof(char) );
    for( i = 0; i <= NumberOfDecompositionLevels; ++i )
      {
      uint8_t val = *p++;
      /* Table A.21 - Precinct width and height for the SPcod and SPcoc parameters */
      precintsize[i] = val;
      }
    }

  assert( p == end );
}

/* I.5.1 JPEG 2000 Signature box */
static void printsignature( FILE * stream )
{
  uint32_t s;
  bool b = read32(stream, &s);
  assert( b );
}

static const char * getbrand( const char br[] )
{
  const char ref[] = "jp2\040";
  if( strncmp( br, ref, 4 ) == 0 )
    {
    return "JP2";
    }
  return NULL;
}

/* I.7.1 XML boxes */
static void printxml( FILE * stream, size_t len )
{
  for( ; len != 0; --len )
    {
    fgetc(stream);
    }
}

/* I.5.2 File Type box */
static void printfiletype( FILE * stream, size_t len )
{
  char br[4+1];
  uint32_t minv;
  int i;
  bool b= true;
  int n = (len - 8 ) / 4;
  const char *brand;
  br[4] = 0;
  fread(br,4,1,stream); assert( b );
  b = read32(stream, &minv); assert( b );
  /* The number of CLi fields is determined by the length of this box. */

  /* Table I.3 - Legal Brand values */
  brand = getbrand( br );
  if( brand )
    {
    }
  else
    {
    uint32_t v;
    memcpy( &v, br, 4 );
    }
  for (i = 0; i < n; ++i )
    {
    const char *brand;
    fread(br,4,1,stream); assert( b );
    brand = getbrand( br );
    if( brand )
      {
      }
    else
      {
      uint32_t v;
      memcpy( &v, br, 4 );
      }
    }
}


/* I.5.3.1 Image Header box */
static void printimageheaderbox( FILE * stream , size_t fulllen )
{
  uint32_t height;
  uint32_t width;
  uint16_t nc;

  /* \precondition */
  uint8_t  bpc;
  uint8_t  c;
  uint8_t  Unk;
  uint8_t  IPR;
  bool b;
  assert( fulllen - 8 == 14 );
  b = read32(stream, &height); assert( b );
  b = read32(stream, &width); assert( b );
  b = read16(stream, &nc); assert( b );
  b = read8(stream, &bpc); assert( b );
  b = read8(stream, &c); assert( b );
  b = read8(stream, &Unk); assert( b );
  b = read8(stream, &IPR); assert( b );

}

static void printcmap( FILE *stream, size_t len )
{
  int n = len / 4;
  bool b;
  uint16_t CMPi;
  uint8_t MTYPi;
  uint8_t PCOLi;
  int i;
  len -= 8;
  for( i = 0; i < n; ++i )
    {
    b = read16(stream, &CMPi); assert( b );
    len--;
    len--;
    b = read8(stream, &MTYPi); assert( b );
    len--;
    b = read8(stream, &PCOLi); assert( b );
    len--;
    }
  assert( len == 0 );
}

static void printpclr( FILE *stream, size_t len )
{

  uint16_t NE;
  uint8_t NPC;
  uint8_t Bi;
  uint8_t Cij;
  uint_fast16_t i;
  uint_fast8_t j;

  bool b;
  len -= 8;
  b = read16(stream, &NE); assert( b );
  len--;
  len--;
  b = read8(stream, &NPC); assert( b );
  len--;
  for( j = 0; j < NPC; ++j )
    {
    b = read8(stream, &Bi); assert( b );
    len--;
    }
  for( i = 0; i < NE; ++i )
    {
    for( j = 0; j < NPC; ++j )
      {
      b = read8(stream, &Cij); assert( b );
      len--;
      }
    }
  assert( len == 0 );
}

static void printcdef( FILE *stream, size_t len )
{
  uint16_t N;
  uint_fast16_t i;
  uint16_t cni;
  uint16_t typi;
  uint16_t asoci;
  bool b;

  len -= 8;
  b = read16(stream, &N); assert( b );

  for (i = 0; i < N; ++i )
    {
    b = read16(stream, &cni); assert( b );
    b = read16(stream, &typi); assert( b );
    b = read16(stream, &asoci); assert( b );
    }
}

/* I.5.3.3 Colour Specification box */
static void printcolourspec( FILE *stream, size_t len )
{

  uint8_t meth;
  uint8_t prec;
  uint8_t approx;
  uint32_t enumCS = 0;
  bool b;
  const char *smeth = "reserved";
  const char *senumCS = "reserved";
  char buffer[512];

  len -= 8;

  b = read8(stream, &meth); assert( b );
  b = read8(stream, &prec); assert( b );
  b = read8(stream, &approx); assert( b );
  assert( meth == 1 || meth == 2 );
  if( meth == 1 )
    {
    assert( len == 7 );
    b = read32(stream, &enumCS); assert( b );
    len-=4;
    }
  len--;
  len--;
  len--;

  switch( meth )
    {
    case 0x1:
      smeth = "enumerated colourspace";
      break;
    case 0x2:
      smeth = "restricted ICC profile";
      break;
    }
  switch( enumCS )
    {
  case 16:
    senumCS = "sRGB";
    break;
  case 17:
    senumCS = "greyscale";
    break;
  case 18:
    senumCS = "YCC";
    break;
  default:
    sprintf(buffer, "unknown (%d)", enumCS );
    senumCS = buffer;
    }

  if( meth == 1 )
    {
    assert( len == 0);
    }
  else if( meth == 2 )
    {
    size_t i;
    assert( len != 0 );
    for( i = 0; i < len; ++i )
      {
      fgetc(stream);
      }
    }
}

/* I.5.3 JP2 Header box (superbox) */
static void printheaderbox( FILE * stream , size_t fulllen )
{
  while( fulllen )
    {
    off_t offset;

    uint32_t len;
    uint32_t marker;
    bool b = read32(stream, &len);
    assert( b );
    assert( fulllen >= len );
    fulllen -= len;
    b = read32(stream, &marker);
    assert( b );

    offset = ftello(stream);
    switch( marker )
      {
    case IHDR:
      printimageheaderbox( stream, len );
      break;
    case CMAP:
      printcmap( stream, len );
      break;
    case CDEF:
      printcdef( stream, len );
      break;
    case COLR:
      printcolourspec( stream, len );
      break;
    case PCLR:
      printpclr( stream, len );
      break;
    default:
      assert( 0 ); /* TODO */
      }
    }
}

static bool print2( uint_fast32_t marker, size_t len, FILE *stream )
{
  bool skip = false;
  (void)len;
  if( true )
    {
    }
  else
    {
    char buffer[4+1];
    uint32_t swap = bswap_32( marker );
    memcpy( buffer, &swap, 4);
    buffer[4] = 0;
    len -= 8;
    for( ; len != 0; --len )
      {
      fgetc(stream);
      }
    return false;
    }

  assert( len >= 8 || (marker == JP2C && len == 0 ) );
  switch( marker )
    {
  case JP:
    printsignature( stream );
    break;
  case FTYP:
    printfiletype( stream, len - 8 );
    break;
  case XML:
    printxml( stream, len - 8 );
    break;
  case JP2H:
    printheaderbox( stream, len - 8 );
    break;
  default:
    skip = true;
    }

  return skip;
}

static uint32_t xsiz;
static uint32_t ysiz;
static uint32_t xosiz;
static uint32_t yosiz;
static uint32_t xtsiz;
static uint32_t ytsiz;
static uint32_t xtosiz;
static uint32_t ytosiz;
static uint8_t *start = NULL;

static void printsiz( FILE *stream, size_t len )
{
  uint16_t rsiz;
  uint8_t ssiz;
  uint8_t xrsiz;
  uint8_t yrsiz;
  bool b = true;
  const char *s = "Reserved";

  const char *p, *end;
  char buffer[1024];
  size_t r;
  assert( len < 1024 );
  r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  p = buffer;
  end = p + len;

  cread16(p, &rsiz); assert( b );
  p+=2;
  cread32(p, &xsiz); assert( b );
  p+=4;
  cread32(p, &ysiz); assert( b );
  p+=4;
  cread32(p, &xosiz); assert( b );
  p+=4;
  cread32(p, &yosiz); assert( b );
  p+=4;
  cread32(p, &xtsiz); assert( b );
  p+=4;
  cread32(p, &ytsiz); assert( b );
  p+=4;
  cread32(p, &xtosiz); assert( b );
  p+=4;
  cread32(p, &ytosiz); assert( b );
  p+=4;
  cread16(p, &csiz); assert( b );
  p+=2;

  switch( rsiz )
    {
  case 0x0:
    s = "PROFILE2";
    break;
  case 0x1:
    s = "PROFILE0";
    break;
  case 0x2:
    s = "PROFILE1";
    break;
    }
  sprofile = s;

  start = malloc( (end - p) * sizeof(char) );
  memcpy( start, p, end - p );
{
  uint_fast16_t i = 0;
  for( i = 0; i < csiz; ++i )
    {
    ssiz = *p++; /* int8_t ? */
    xrsiz = *p++;
    yrsiz = *p++; 
    }
}

  assert( p == end );
}

static bool print1( uint_fast16_t marker, size_t len, FILE *stream )
{
  off_t offset = ftello(stream);
  static int init = 0;
  static off_t rel_offset;
  if( hasnolength( marker ) )
    {
    offset -= 2; /* remove size of marker itself */
    }
  else
    {
    offset -= 4; /* remove size + len of marker itself */
    }
  if( !init )
    {
    rel_offset = offset;
    ++init;
    }
  assert( offset >= 0 );
  switch( marker )
    {
  case EOC:
    printeoc( stream, len );
    break;
  case RGN:
    printrgn( stream, len );
    return false;
  case POC:
    printpoc( stream, len );
    return false;
  case QCC:
    printqcc( stream, len );
    return false;
  case QCD:
    printqcd( stream, len );
    return false;
  case SIZ:
    printsiz( stream, len );
    return false;
  case EPH:
    printeph( stream, len );
    return false;
  case SOP:
    printsop( stream, len );
    return false;
  case SOD:
    printsod( stream, len );
    return false;
  case SOT:
    printsot( stream, len );
    return false;
  case COM:
    printcomment( stream, len );
    return false;
  case TLM:
    printtlm( stream, len );
    return false;
  case COC:
    printcoc( stream, len );
    return false;
  case COD:
    printcod( stream, len );
    return false;
    }
  return true;
}

int main(int argc, char *argv[])
{
  bool b;
  uint_fast16_t i;
  const char *filename;
  if( argc < 2 ) return 1;
  filename = argv[1];

  if( argc > 2 )
    {
    const char *outfilename = argv[2];
    fout = fopen( outfilename, "w" );
    }
  else
    {
    fout = stdout;
    }

  ntiles = 0;
  if( isjp2file( filename ) )
    {
    b = parsejp2( filename, &print2, &print1 );
    }
  else
    {
    b = parsej2k( filename, &print1 );
    }

  fprintf(fout, "Sprofile=%s\n", sprofile);
  fprintf(fout, "Scap=no\n" );
  fprintf(fout, "Sextensions=0\n" );
  fprintf(fout, "Ssize={%u,%u}\n", ysiz, xsiz);
  fprintf(fout, "Sorigin={%u,%u}\n", yosiz, xosiz );
  fprintf(fout, "Stiles={%u,%u}\n", ytsiz, xtsiz );
  fprintf(fout, "Stile_origin={%u,%u}\n", ytosiz, xtosiz );
  fprintf(fout, "Scomponents=%u\n", csiz );
  fprintf(fout, "Ssigned=" );
  for( i = 0; i < csiz; ++i )
    {
    uint8_t ssiz  = start[ i * 3 + 0 ];
    const bool sign = ssiz >> 7;
    if( i ) fprintf(fout, "," );
    fprintf(fout, "%s", sign ? "yes" : "no" );
    }
  fprintf(fout, "\n" );
  fprintf(fout, "Sprecision=" );
  for( i = 0; i < csiz; ++i )
    {
  uint8_t ssiz;
    ssiz  = start[ i * 3 + 0 ];
    if( i ) fprintf(fout, "," );
    fprintf(fout, "%u", (ssiz & 0x7f) + 1 );
    }
  fprintf(fout, "\n" );
  fprintf(fout, "Ssampling=" );
  for( i = 0; i < csiz; ++i )
    {
  uint8_t xrsiz;
  uint8_t yrsiz;

    xrsiz = start[ i * 3 + 1 ];
    yrsiz = start[ i * 3 + 2 ]; 
    if( i ) fprintf(fout, "," );
    fprintf(fout, "{%u,%u}", yrsiz, xrsiz );
    }
  fprintf(fout, "\n" );
  fprintf(fout, "Sdims=" );
  for( i = 0; i < csiz; ++i )
    {
  uint8_t xrsiz;
  uint8_t yrsiz;

    xrsiz = start[ i * 3 + 1 ];
    yrsiz = start[ i * 3 + 2 ]; 
    if( i ) fprintf(fout, "," );
    fprintf(fout, "{%u,%u}", (int)((double)(ysiz - yosiz) / yrsiz + 0.5), (int)((double)(xsiz - xosiz ) / xrsiz + 0.5));
    }
  fprintf(fout, "\n" );
  if( csiz == 3 )
    fprintf(fout, "Cycc=yes\n" );
  else
    fprintf(fout, "Cycc=no\n" );
  fprintf(fout, "Cmct=%u\n", MultipleComponentTransformation );
  fprintf(fout, "Clayers=%u\n", NumberOfLayers );
  fprintf(fout, "Cuse_sop=%s\n", SOPMarkerSegments ? "yes" : "no" );
  fprintf(fout, "Cuse_eph=%s\n", EPHMarkerSegments ? "yes" : "no" );
  fprintf(fout, "Corder=%s\n", sProgressionOrder );
  fprintf(fout, "Calign_blk_last={no,no}\n" );
  fprintf(fout, "Clevels=%u\n", NumberOfDecompositionLevels );
  fprintf(fout, "Cads=0\n" ); /*  Arbitrary Downsampling Style information */
  fprintf(fout, "Cdfs=0\n" ); /* Downsampling Factor Style */
  fprintf(fout, "Cdecomp=B(-:-:-)\n" );
  fprintf(fout, "Creversible=%s\n", Transformation ? "yes" : "no" );
  fprintf(fout, "Ckernels=%s\n", sTransformation );
  fprintf(fout, "Catk=0\n" );
  fprintf(fout, "Cuse_precincts=%s\n", VariablePrecinctSize ? "yes" : "no" );

  if( precintsize )
    {
    uint_fast8_t i;
    fprintf(fout, "Cprecincts=%s", VariablePrecinctSize ? "yes" : "no" );
    for( i = 0; i <= NumberOfDecompositionLevels; ++i )
      {
      const uint8_t val = precintsize[NumberOfDecompositionLevels - i];
      const uint8_t width = val & 0x0f;
      const uint8_t height = val >> 4;
      if( i ) fprintf(fout, "," );
      fprintf(fout, "{%u,%u}", width+1, height+1 );
      }
    fprintf(fout, "\n" );
    }

  fprintf(fout, "Cblk={%u,%u}\n", 1 << ycb, 1 << xcb );

  fprintf(fout, "Cmodes=" );
  if(CodeBlockStyle)
    {
    int mask = 1;
    while(CodeBlockStyle)
      {
      switch(CodeBlockStyle & mask)
        {
      case BYPASS:
        fprintf(fout, "BYPASS" );
        break;
      case RESET:
        fprintf(fout, "RESET" );
        break;
      case RESTART:
        fprintf(fout, "RESTART" );
        break;
      case CAUSAL:
        fprintf(fout, "CAUSAL" );
        break;
      case ERTERM:
        fprintf(fout, "ERTERM" );
        break;
      case SEGMARK:
        fprintf(fout, "SEGMARK" );
        break;
        }
      if( CodeBlockStyle & ~mask )
        if(CodeBlockStyle & mask)
          fprintf(fout, "," );
      CodeBlockStyle &= ~mask;
      mask <<= 1;
      }
    }
  else
    {
    fprintf(fout, "0" );
    }
  fprintf(fout, "\n" );
  fprintf(fout, "Qguard=%u\n", nbits );
  if( quant == 0x0 )
    {
    size_t n = abs_ranges;
    fprintf(fout, "Qabs_ranges=" );
    for( i = 0; i != n; ++i )
      {
      if( i ) fprintf(fout, "," );
      fprintf(fout, "%u", exps[i] );
      }
    }
  fprintf(fout, "\n");
  for( i = 0; i < ntiles; ++i )
    {
    fprintf(fout, "\n");
    fprintf(fout, ">> New attributes for tile %u:\n", (unsigned int)i);
    }

  free(start);
  free(exps);
  free(precintsize);

  if( argc > 2 )
    {
    fclose( fout );
    }

  if( !b ) return 1;

  return 0;
}
