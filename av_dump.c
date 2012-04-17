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
 * Let's try to produce something compatible with
 * jp2file.py / jp2codestream.py
 * from Algo Vision Technology
 *
 * Everything was based of the *.desc and *.txt as part of the zip file:
 * http://www.crc.ricoh.com/~gormish/jpeg2000conformance/j2kp4files_v1_5.zip
 */
#include <stdio.h>
#include <stdarg.h> /* va_list */
#include <assert.h>
#include <inttypes.h>
#include <byteswap.h>
#include <string.h>

#include <simpleparser.h>

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

typedef struct {
  uint16_t marker;
  const char* shortname;
  const char* longname;
} dictentry;

typedef struct {
  uint32_t marker;
  const char* shortname;
  const char* longname;
} dictentry2;

static const dictentry2 dict2[] = {
  { JP  , "jP  ", "JP2 Signature box" },
  { FTYP, "ftyp", "File Type box" },
  { JP2H, "jp2h", "JP2 Header box" },
  { JP2C, "jp2c", "Codestream box" },
  { XML , "xml ", "XML box" },
  { CDEF, "cdef", "Channel Definition box" },
  { CMAP, "cmap", "Component Mapping box" },
  { PCLR, "pclr", "Palette box" },
  { IHDR, "ihdr", "Image Header box" },
  { COLR, "colr", "Colour Specification box" },

  { 0, 0, 0 }
};

static const dictentry2 * getdictentry2frommarker( uint_fast32_t marker )
{
  static const size_t n = sizeof( dict2 ) / sizeof( dictentry2 ) - 1; /* remove sentinel */
  const dictentry2 * beg = dict2;
  const dictentry2 * end = dict2 + n;
  const dictentry2 * p = beg;
  for( ; p != end; ++p )
    {
    if( p->marker == marker ) return p;
    }
  assert( end->marker == 0 );
  return end;
}

static const dictentry dict[] = {
  { FF30, "0x30", "unknown segment-less" },
  { SOC,  "SOC",  "Start of codestream" },
  { SOT,  "SOT",  "Start of tile-part" },
  { SOD,  "SOD",  "Start of data" },
  { EOC,  "EOC",  "End of codestream" },
  { SIZ,  "SIZ",  "Image and tile size" },
  { COD,  "COD",  "Coding style default" },
  { COC,  "COC",  "Coding style component" },
  { RGN,  "RGN",  "Region-of-interest" },
  { QCD,  "QCD",  "Quantization default" },
  { QCC,  "QCC",  "Quantization component" },
  { POC,  "POC",  "Progression order change" },
  { TLM,  "TLM",  "Tile-part length" },
  { PLM,  "PLM",  "Packet length, main header" },
  { PLT,  "PLT",  "Packet length, tile-part header" },
  { PPM,  "PPM",  "Packet packer headers, main header" },
  { PPT,  "PPT",  "Packet packer headers, tile-part header" },
  { SOP,  "SOP",  "Start of packet" },
  { EPH,  "EPH",  "End of packet header" },
  { CRG,  "CRG",  "Component registration" },
  { COM,  "COM",  "Comment" },

  { 0, 0, 0 }
};

static const dictentry * getdictentryfrommarker( uint_fast16_t marker )
{
  static const size_t n = sizeof( dict ) / sizeof( dictentry ) - 1; /* remove sentinel */
  const dictentry * beg = dict;
  const dictentry * end = dict + n;
  const dictentry * p = beg;
  for( ; p != end; ++p )
    {
    if( p->marker == marker ) return p;
    }
  assert( end->marker == 0 );
  return end;
}

/* Table A.16 Progression order for the SGcod, SPcoc, and Ppoc parameters */
static const char *getDescriptionOfProgressionOrderString(uint8_t progressionOrder)
{
  const char *descriptionOfProgressionOrder = "Reserved";
  switch (progressionOrder) {
    case 0x00:
      descriptionOfProgressionOrder = "layer-resolution level-component-position";
      break;
    case 0x01:
      descriptionOfProgressionOrder = "resolution level-layer-component-position";
      break;
    case 0x02:
      descriptionOfProgressionOrder = "resolution level-position-component-layer";
      break;
    case 0x03:
      descriptionOfProgressionOrder = "position-component-resolution level-layer";
      break;
    case 0x04:
      descriptionOfProgressionOrder = "component-position-resolution level-layer";
      break;
  }
  return descriptionOfProgressionOrder;
}

/* Table A.17 Multiple component transformation for the SGcod parameters */
static const char *getMultipleComponentTransformationString(uint8_t multipleComponentTransformation)
{
  const char *descriptionOfMultipleComponentTransformation = "Reserved";
  switch (multipleComponentTransformation)
    {
  case 0x00:
    descriptionOfMultipleComponentTransformation = "none";
    break;
  case 0x01:
    descriptionOfMultipleComponentTransformation = "components 0,1,2";
    break;
    }
  return descriptionOfMultipleComponentTransformation;
}

/* Table A.20 - Transformation for the SPcod and SPcoc parameters */
static const char *getDescriptionOfWaveletTransformationString(uint8_t waveletTransformation)
{
  const char *descriptionOfWaveletTransformation = "Reserved";
  switch (waveletTransformation) {
    case 0x00:
      descriptionOfWaveletTransformation = "9-7 irreversible";
      break;
    case 0x01:
      descriptionOfWaveletTransformation = "5-3 reversible";
      break;
  }
  return descriptionOfWaveletTransformation;
}

uintmax_t data_size;
uintmax_t file_size;

static void printeoc( FILE *stream, size_t len )
{
  printf("\n");

  off_t offset = ftello(stream);
  assert( offset + len == file_size );
  printf( "Size: %u bytes\n", file_size );
  printf( "Data Size: %u bytes\n", data_size );
  assert( file_size >= data_size );
  uintmax_t overhead = file_size - data_size;
  const int ratio = 100 * overhead / file_size;
  printf( "Overhead: %u bytes (%u%)\n", overhead , ratio );
}

static void printqcd( FILE *stream, size_t len )
{
  uint8_t sqcd;
  bool b;

  b = read8(stream, &sqcd); assert( b );
  --len;
  uint8_t quant = (sqcd & 0x1f);
  uint8_t nbits = (sqcd >> 5);
  size_t i;
  printf("\n");
  const char *s = "reserved";
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
  printf( "  Quantization Type : %s\n", s );
  printf( "  Guard Bits        : %u\n", nbits );

  if( quant == 0x0 )
    {
    size_t n = len;
    for( i = 0; i != n; ++i )
      {
      uint8_t val;
      b = read8(stream, &val); assert( b );
      const uint8_t exp = val >> 3;
      printf( "  Exponent #%-8u: %u\n", i, exp );
      }
    }
  else
    {
    size_t n = len / 2;
    for( i = 0; i != n; ++i )
      {
      uint16_t val;
      b = read16(stream, &val); assert( b );
      const uint16_t mant = val & 0x7ff;
      const uint16_t exp = val >> 11;

      printf( "  Mantissa #%-8u: %u\n", i, mant );
      printf( "  Exponent #%-8u: %u\n", i, exp );
      }
    }
}

static void printsod( FILE *stream, size_t len )
{
  int v = fseeko(stream, (off_t)len, SEEK_CUR);
  if( len )
    {
    printf("\n" );
    printf("Data : %u bytes\n", len );
    data_size += len;
    }
}

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
  printf("\n" );
  printf("  Tile        : %u\n", Isot );
  printf("  Length      : %u\n", Psot );
  printf("  Index       : %u\n", TPsot );
  if( TNsot )
    printf("  Tile-Parts: : %u\n", TNsot );
  else
    printf("  Tile-Parts: : unknown\n" );
}

static void printcomment( FILE *stream, size_t len )
{
  uint16_t rcom;
  bool b;
  b = read16(stream, &rcom); assert( b );
  len -= 2;
  printf("\n" );
  printf("  Registration : %s\n", rcom ? "ISO-8859-15" : "binary" );
  if( len < 512 )
    {
    char buffer[512];
    size_t l = fread(buffer,sizeof(char),len,stream);
    buffer[len] = 0;
    assert( l == len );
    printf("  Comment      : %s\n", buffer );
    }
  else
    {
    int v = fseeko(stream, (off_t)len, SEEK_CUR);
    printf("  Comment      : ...\n");
    }

}

static void printtlm( FILE *stream, size_t len )
{
  char buffer[512];
  assert( len < 512 );
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  printf( "\n" );
  const uint8_t *p = (const uint8_t*)buffer;
  const uint8_t *end = p + len;
  uint8_t Ztlm = *p++;
  printf( "  Index          : %u\n", Ztlm );
  uint8_t Stlm = *p++;
  uint8_t ST = ( Stlm >> 4 ) & 0x3;
  uint8_t SP = ( Stlm >> 6 ) & 0x1;

	int Ptlm_size = (SP + 1) * 2;
	int quotient = Ptlm_size + ST;
	size_t header_size = len - 2;

	int tot_num_tp = header_size / quotient;
	int tot_num_tp_remaining = header_size % quotient;

  int i;
  for (i = 0; i < tot_num_tp; ++i)
    {
    if( ST == 0 )
      {
      printf( "  Tile index #%-3d: in order\n", i );
      }
    else if( ST == 1 )
      {
      uint8_t v = *p++;
      printf( "  Tile index #%-3d: %u\n", i, v );
      }
    else if( ST == 2 )
      {
      uint16_t v;
      cread16(p, &v);
      printf( "  Tile index #%-2d: %u\n", i, v );
      }
    else
      {
      assert( 0 );
      }
    p += ST;
    if( Ptlm_size == 2 )
      {
      uint16_t v;
      cread16(p, &v);
      printf( "  Length #%-7d: %u\n",i, v );
      }
    if( Ptlm_size == 4 )
      {
      uint32_t v;
      cread32(p, &v);
      printf( "  Length #%-6d: %u\n",i, v );
      }
    else
      {
      assert( 0 );
      }
    p += Ptlm_size;
    }
  assert( p == end );
}

static uint16_t csiz;

static void printcoc( FILE *stream, size_t len )
{
  char buffer[512];
  assert( len < 512 );
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  const uint8_t *p = (const uint8_t*)buffer;
  const uint8_t *end = p + len;
  uint16_t comp;
  if( csiz < 257 )
    {
    comp = *p++;
    }
  else
    {
    cread16(p, &comp);
    p += 2;
    }
  uint8_t Scoc = *p++;
  uint8_t NumberOfDecompositionLevels = *p++;
  uint8_t CodeBlockWidth = *p++ & 0xf;
  uint8_t CodeBlockHeight = *p++ & 0xf;
  uint8_t xcb = CodeBlockWidth + 2;
  uint8_t ycb = CodeBlockHeight + 2;
  assert( xcb + ycb <= 12 );
  uint8_t CodeBlockStyle = *p++;
  uint8_t Transformation = *p++;
  assert( p == end );

  /* Table A.19 - Code-block style for the SPcod and SPcoc parameters */
  bool SelectiveArithmeticCodingBypass                 = (CodeBlockStyle & 0x01) != 0;
  bool ResetContextProbabilitiesOnCodingPassBoundaries = (CodeBlockStyle & 0x02) != 0;
  bool TerminationOnEachCodingPass                     = (CodeBlockStyle & 0x04) != 0;
  bool VerticallyCausalContext                         = (CodeBlockStyle & 0x08) != 0;
  bool PredictableTermination                          = (CodeBlockStyle & 0x10) != 0;
  bool SegmentationSymbolsAreUsed                      = (CodeBlockStyle & 0x20) != 0;
  const char * sTransformation = getDescriptionOfWaveletTransformationString(Transformation);

  printf( "\n");
  printf( "  Component                          : %u\n", comp);
  printf( "  Precincts                          : %s\n", Scoc == 0x0 ? "default" : "other" );
  printf( "  Decomposition Levels               : %u\n", NumberOfDecompositionLevels);
  printf( "  Code-block size                    : %ux%u\n", 1 << xcb, 1 << ycb);
  printf( "  Selective Arithmetic Coding Bypass : %s\n", SelectiveArithmeticCodingBypass ? "yes" : "no" );
  printf( "  Reset Context Probabilities        : %s\n", ResetContextProbabilitiesOnCodingPassBoundaries ? "yes" : "no" );
  printf( "  Termination on Each Coding Pass    : %s\n", TerminationOnEachCodingPass ? "yes" : "no" );
  printf( "  Vertically Causal Context          : %s\n", VerticallyCausalContext ? "yes" : "no" );
  printf( "  Predictable Termination            : %s\n", PredictableTermination ? "yes" : "no" );
  printf( "  Segmentation Symbols               : %s\n", SegmentationSymbolsAreUsed ? "yes" : "no" );
  printf( "  Wavelet Transformation             : %s\n", sTransformation );
}

static void printcod( FILE *stream, size_t len )
{
  char buffer[512];
  assert( len < 512 );
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  union { uint16_t v; char bytes[2]; } u16;
/*  union { uint32_t v; char bytes[4]; } u32; */

/* Table A.12 - Coding style default parameter values */
  const uint8_t *p = (const uint8_t*)buffer;
  uint8_t Scod = *p++;
  uint8_t ProgressionOrder = *p++;
  u16.bytes[0] = (char)*p++;
  u16.bytes[1] = (char)*p++;
  uint16_t NumberOfLayers = bswap_16( u16.v );
  uint8_t MultipleComponentTransformation = *p++;
  uint8_t NumberOfDecompositionLevels = *p++;
  uint8_t CodeBlockWidth = *p++ & 0xf;
  uint8_t CodeBlockHeight = *p++ & 0xf;
  uint8_t xcb = CodeBlockWidth + 2;
  uint8_t ycb = CodeBlockHeight + 2;
  assert( xcb + ycb <= 12 );
  uint8_t CodeBlockStyle = *p++;
  uint8_t Transformation = *p++;

  const char * sMultipleComponentTransformation = getMultipleComponentTransformationString(MultipleComponentTransformation);
  const char * sProgressionOrder = getDescriptionOfProgressionOrderString(ProgressionOrder);
  const char * sTransformation = getDescriptionOfWaveletTransformationString(Transformation);

  /* Table A.13 Coding style parameter values for the Scod parameter */
  bool VariablePrecinctSize = (Scod & 0x01) != 0;
  bool SOPMarkerSegments    = (Scod & 0x02) != 0;
  bool EPHPMarkerSegments   = (Scod & 0x04) != 0;

  /* Table A.19 - Code-block style for the SPcod and SPcoc parameters */
  bool SelectiveArithmeticCodingBypass                 = (CodeBlockStyle & 0x01) != 0;
  bool ResetContextProbabilitiesOnCodingPassBoundaries = (CodeBlockStyle & 0x02) != 0;
  bool TerminationOnEachCodingPass                     = (CodeBlockStyle & 0x04) != 0;
  bool VerticallyCausalContext                         = (CodeBlockStyle & 0x08) != 0;
  bool PredictableTermination                          = (CodeBlockStyle & 0x10) != 0;
  bool SegmentationSymbolsAreUsed                      = (CodeBlockStyle & 0x20) != 0;
  printf( "\n" );

  printf( "  Default Precincts of 2^15x2^15     : %s\n" , VariablePrecinctSize ? "no" : "yes" );
  printf( "  SOP Marker Segments                : %s\n" , SOPMarkerSegments ? "yes" : "no" );
  printf( "  EPH Marker Segments                : %s\n" , EPHPMarkerSegments ? "yes" : "no" );
  printf( "  All Flags                          : %08u\n", Scod );
  printf( "  Progression Order                  : %s\n", sProgressionOrder );
  printf( "  Layers                             : %u\n", NumberOfLayers );
  printf( "  Multiple Component Transformation  : %s\n", sMultipleComponentTransformation );
  printf( "  Decomposition Levels               : %u\n", NumberOfDecompositionLevels );
  printf( "  Code-block size                    : %ux%u\n", 1 << xcb, 1 << ycb );
  printf( "  Selective Arithmetic Coding Bypass : %s\n", SelectiveArithmeticCodingBypass ? "yes" : "no" );
  printf( "  Reset Context Probabilities        : %s\n", ResetContextProbabilitiesOnCodingPassBoundaries ? "yes" : "no" );
  printf( "  Termination on Each Coding Pass    : %s\n", TerminationOnEachCodingPass ? "yes" : "no" );
  printf( "  Vertically Causal Context          : %s\n", VerticallyCausalContext ? "yes" : "no" );
  printf( "  Predictable Termination            : %s\n", PredictableTermination ? "yes" : "no" );
  printf( "  Segmentation Symbols               : %s\n", SegmentationSymbolsAreUsed ? "yes" : "no" );
  printf( "  Wavelet Transformation             : %s\n", sTransformation );

}

/* I.5.1 JPEG 2000 Signature box */
static void printsignature( FILE * stream )
{
  uint32_t s;
  const uint32_t sig = 0xd0a870a;
  bool b = read32(stream, &s);
  assert( b );
  printf("\n" );
  printf("  Corrupted: %s\n", s == sig ? "no" : "yes" );
}

static void printstring( const char *in, const char *ref )
{
  assert( in );
  assert( ref );
  size_t len = strlen( ref );
  printf("%s", in);
  if( len < 4 )
    {
    int l;
    printf("\"");
    printf("%s", ref);
    for( l = 0; l < 4 - (int)len; ++l )
      {
      printf(" ");
      }
    printf("\"");
    }
  else
    {
    printf("%s", ref);
    }
    printf("\n");
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
  printf( "\n" );
  printf( "Data:\n" );
  for( ; len != 0; --len )
    {
    int val = fgetc(stream);
    //if( isascii( val ) )
    /* remove ^M */
    /*if( !iscntrl( val ) ) */
    /* it does not look like the tool is consistant BTW */
    if( val != 0xd )
      printf( "%c", val );
    }
  printf( "\n");
  printf( "\n");
  printf( "\n");
}

/* I.5.2 File Type box */
static void printfiletype( FILE * stream, size_t len )
{
  char br[4+1];
  br[4] = 0;
  uint32_t minv;
  uint32_t cl;
  bool b= true;
  fread(br,4,1,stream); assert( b );
  b = read32(stream, &minv); assert( b );
  /* The number of CLi fields is determined by the length of this box. */
  int n = (len - 8 ) / 4;

  /* Table I.3 - Legal Brand values */
  const char *brand = getbrand( br );
  printf("\n" );
  if( brand )
    {
    printf("  Brand: %s\n", brand );
    }
  else
    {
    uint32_t v;
    memcpy( &v, br, 4 );
    printf("  Brand: 0x%08x\n", bswap_32(v) );
    }
  printf("  Minor version: %u\n", minv );
  int i;
  printf("  Compatibility: " );
  for (i = 0; i < n; ++i )
    {
    if( i ) printf( " " );
    fread(br,4,1,stream); assert( b );
    const char *brand = getbrand( br );
    if( brand )
      {
      printf("%s", brand );
      }
    else
      {
      uint32_t v;
      memcpy( &v, br, 4 );
      printf("0x%08x", bswap_32(v) );
      }
    }
    printf( "\n" );
}

/* I.5.3.1 Image Header box */
static void printimageheaderbox( FILE * stream , size_t fulllen )
{
  /* \precondition */
  assert( fulllen - 8 == 14 );
  uint32_t height;
  uint32_t width;
  uint16_t nc;
  uint8_t  bpc;
  uint8_t  c;
  uint8_t  Unk;
  uint8_t  IPR;
  bool b;
  b = read32(stream, &height); assert( b );
  b = read32(stream, &width); assert( b );
  b = read16(stream, &nc); assert( b );
  b = read8(stream, &bpc); assert( b );
  b = read8(stream, &c); assert( b );
  b = read8(stream, &Unk); assert( b );
  b = read8(stream, &IPR); assert( b );

  printf("\n" );
  printf("  Height: %u\n", height );
  printf("  Width: %u\n", width);
  printf("  Components: %u\n", nc);
  printf("  Bits Per Component: %u\n", (bpc & 0x7f) + 1);
  const bool sign = bpc >> 7;
  printf("  Signed Components: %s\n", sign ? "yes" : "no" );
  printf("  Compression Type: %s\n", c == 7 ? "wavelet" : "holly crap");
  printf("  Unknown Colourspace: %s\n", Unk ? "yes" : "no");
  printf("  Intellectual Property: %s\n", IPR ? "yes" : "no");
}

static void printcmap( FILE *stream, size_t len )
{
  len -= 8;
  int n = len / 4;
  bool b;
  uint16_t CMPi;
  uint8_t MTYPi;
  uint8_t PCOLi;
  int i;
  printf( "\n" );
  for( i = 0; i < n; ++i )
    {
    b = read16(stream, &CMPi); assert( b );
    len--;
    len--;
    b = read8(stream, &MTYPi); assert( b );
    len--;
    b = read8(stream, &PCOLi); assert( b );
    len--;
    printf( "  Component      #%d: %u\n", i, CMPi );
    printf( "  Mapping Type   #%d: %s\n", i, MTYPi ? "palette mapping" : "wazzza" );
    printf( "  Palette Column #%d: %u\n", i, PCOLi );
    }
  assert( len == 0 );
}

static void printpclr( FILE *stream, size_t len )
{
  len -= 8;

  uint16_t NE;
  uint8_t NPC;
  uint8_t Bi;
  uint8_t Cij;
  uint_fast16_t i;
  uint_fast8_t j;

  bool b;
  b = read16(stream, &NE); assert( b );
  len--;
  len--;
  b = read8(stream, &NPC); assert( b );
  len--;
  printf("\n" );
  printf("  Entries: %u\n", NE );
  printf("  Created Channels: %u\n", NPC  );
  for( j = 0; j < NPC; ++j )
    {
    b = read8(stream, &Bi); assert( b );
    len--;
    const bool sign = Bi >> 7;
    printf("  Depth  #%u: %u\n", j, (Bi & 0x7f) + 1 );
    printf("  Signed #%u: %s\n", j, sign ? "yes" : "no" );
    }
  for( i = 0; i < NE; ++i )
    {
      printf("  Entry #%03u: ", i );
    for( j = 0; j < NPC; ++j )
      {
      b = read8(stream, &Cij); assert( b );
      len--;
      if( j ) printf(" ");
      printf("0x%010x", Cij );
      }
    printf("\n" );
    }
  assert( len == 0 );
}

static void printcdef( FILE *stream, size_t len )
{
  len -= 8;
  uint16_t N;

  bool b;
  b = read16(stream, &N); assert( b );

  uint_fast16_t i;
  uint16_t cni;
  uint16_t typi;
  uint16_t asoci;
    printf("\n" );
  for (i = 0; i < N; ++i )
    {
    b = read16(stream, &cni); assert( b );
    b = read16(stream, &typi); assert( b );
    b = read16(stream, &asoci); assert( b );
    printf("  Channel     #%u: %x\n", i, cni );
    printf("  Type        #%u: %s\n", i, typi == 0 ? "color" : "donno" );
    printf("  Association #%u: %x\n", i, asoci );
    }
}

/* I.5.3.3 Colour Specification box */
static void printcolourspec( FILE *stream, size_t len )
{
  len -= 8;

  uint8_t meth;
  uint8_t prec;
  uint8_t approx;
  uint32_t enumCS = 0;
  bool b;
  b = read8(stream, &meth); assert( b );
  b = read8(stream, &prec); assert( b );
  b = read8(stream, &approx); assert( b );
  if( meth != 2 )
    {
    assert( len == 7 );
    b = read32(stream, &enumCS); assert( b );
    len-=4;
    }
  len--;
  len--;
  len--;
  if( len != 0 )
    {
    int v = fseeko(stream, (off_t)len, SEEK_CUR);
    }

  printf("\n");
  const char *smeth = "reserved";
  switch( meth )
    {
    case 0x1:
      smeth = "enumerated colourspace";
      break;
    case 0x2:
      smeth = "restricted ICC profile";
      break;
    }
  const char *senumCS = "reserved";
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
    }
  printf("  Colour Specification Method: %s\n", smeth);
  printf("  Precedence: %u\n", prec);
  printf("  Approximation: %u\n", approx);
  printf("  Colourspace: %s\n", senumCS);
}

/* I.5.3 JP2 Header box (superbox) */
static void printheaderbox( FILE * stream , size_t fulllen )
{
  while( fulllen )
    {
    uint32_t len;
    bool b = read32(stream, &len);
    assert( b );
    assert( fulllen >= len );
    fulllen -= len;
    uint32_t marker;
    b = read32(stream, &marker);
    assert( b );

    off_t offset = ftello(stream);
    const dictentry2 *d = getdictentry2frommarker( marker );
    printf( "\n" );
    printf( "  Sub box: \"%s\" (%s)\n", d->shortname, d->longname );
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
  off_t offset = ftello(stream);
  const dictentry2 *d = getdictentry2frommarker( marker );
  (void)len;
  printf( "\n" );
  if( d->shortname )
    printf( "New box: \"%s\" (%s)\n", d->shortname, d->longname );
  else
    {
    char buffer[4+1];
    uint32_t swap = bswap_32( marker );
    memcpy( buffer, &swap, 4);
    buffer[4] = 0;
    printf( "New box: \"%s\" (unknown box)\n", buffer );
    puts( "\n" );
    }

  bool skip = false;
  assert( len >= 8 );
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

static void printsiz( FILE *stream, size_t len )
{
  assert( len >= 4 );
  len -= 4;

  uint16_t rsiz;
  uint32_t xsiz;
  uint32_t ysiz;
  uint32_t xosiz;
  uint32_t yosiz;
  uint32_t xtsiz;
  uint32_t ytsiz;
  uint32_t xtosiz;
  uint32_t ytosiz;
  uint8_t ssiz;
  uint8_t xrsiz;
  uint8_t yrsiz;
  bool b;
  b = read16(stream, &rsiz); assert( b );
  b = read32(stream, &xsiz); assert( b );
  b = read32(stream, &ysiz); assert( b );
  b = read32(stream, &xosiz); assert( b );
  b = read32(stream, &yosiz); assert( b );
  b = read32(stream, &xtsiz); assert( b );
  b = read32(stream, &ytsiz); assert( b );
  b = read32(stream, &xtosiz); assert( b );
  b = read32(stream, &ytosiz); assert( b );
  b = read16(stream, &csiz); assert( b );

  const char *s = "Reserved";
  switch( rsiz )
    {
  case 0x0:
    s = "JPEG2000 full standard";
    break;
  case 0x1:
    s = "JPEG2000 profile 0";
    break;
  case 0x2:
    s = "JPEG2000 profile 1";
    break;
    }
  printf( "\n" );
  printf( "  Required Capabilities          : %s\n", s );
  printf( "  Reference Grid Size            : %ux%u\n", xsiz, ysiz );
  printf( "  Image Offset                   : %ux%u\n", xosiz, yosiz );
  printf( "  Reference Tile Size            : %ux%u\n", xtsiz, ytsiz );
  printf( "  Reference Tile Offset          : %ux%u\n", xtosiz, ytosiz );
  printf( "  Components                     : %u\n", csiz );

  uint_fast16_t i = 0;
  for( i = 0; i < csiz; ++i )
    {
    b = read8(stream, &ssiz); assert( b ); /* int8_t ? */
    const bool sign = ssiz >> 7;
    printf( "  Component #%u Depth             : %u\n", i, (ssiz & 0x7f) + 1 );
    printf( "  Component #%u Signed            : %s\n", i, sign ? "yes" : "no" );
    b = read8(stream, &xrsiz); assert( b );
    b = read8(stream, &yrsiz); assert( b );
    printf( "  Component #%u Sample Separation : %ux%u\n", i, xrsiz, yrsiz );
    }
}

void print_with_indent(int indent, const char * format, ...)
{
  va_list arg;
  va_start(arg, format);
  printf("%*s" "%s", indent, " ", "");
  vprintf(format, arg);
  va_end(arg);
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
  static int nspaces = 0;
  if( !init )
    {
    if( offset ) nspaces = 2;
    rel_offset = offset;
    ++init;
    }
  const dictentry *d = getdictentryfrommarker( marker );
  assert( offset >= 0 );
  assert( d->shortname );
  printf( "\n" );
  print_with_indent( nspaces, "%-8u: New marker: %s (%s)", offset - rel_offset, d->shortname, d->longname );
  printf("\n");
  switch( marker )
    {
  case EOC:
    printeoc( stream, len );
    break;
  case QCD:
    printqcd( stream, len );
    return false;
  case SIZ:
    printsiz( stream, len );
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
  if( argc < 2 ) return 1;
  const char *filename = argv[1];

  bool b;
  data_size = 0;
  file_size = getfilesize( filename );
  if( isjp2file( filename ) )
    {
    printf("###############################################################\n" );
    printf("# JP2 file format log file generated by jp2file.py            #\n" );
    printf("# jp2file.py is copyrighted (c) 2001,2002                     #\n" );
    printf("# by Algo Vision Technology GmbH, All Rights Reserved         #\n" );
    printf("#                                                             #\n" );
    printf("# http://www.av-technology.de/ jpeg2000@av-technology.de      #\n" );
    printf("###############################################################\n" );

    b = parsejp2( filename, &print2, &print1 );
    }
  else
    {
    printf( "###############################################################\n" );
    printf( "# JP2 codestream log file generated by jp2codestream.py       #\n" );
    printf( "# jp2codestream.py is copyrighted (c) 2001,2002               #\n" );
    printf( "# by Algo Vision Technology GmbH, All Rights Reserved         #\n" );
    printf( "#                                                             #\n" );
    printf( "# http://www.av-technology.de/ jpeg2000@av-technology.de      #\n" );
    printf( "###############################################################\n" );

    b = parsej2k( filename, &print1 );
    }
  if( !b ) return 1;

  return 0;
}
