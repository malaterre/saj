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

FILE * fout;
static int indentlevel = 0;

void print_with_indent(int indent, const char * format, ...)
{
  va_list arg;
  va_start(arg, format);
  if( indent )
  fprintf(fout,"%*s" "%s", indent, " ", "");
  vfprintf(fout,format, arg);
  va_end(arg);
}


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
  { PLM,  "PLM",  "Packed length, main header" },
  { PLT,  "PLT",  "Packed length, tile-part header" },
  { PPM,  "PPM",  "Packed packet headers, main header" },
  { PPT,  "PPT",  "Packed packet headers, tile-part header" },
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
  fprintf(fout,"\n");
#if 0
  off_t offset = ftello(stream);
  assert( offset + len == file_size ); /* file8.jp2 */
#else
  (void)stream;
  (void)len;
#endif
  fprintf(fout, "Size: %zu bytes\n", file_size );
  fprintf(fout, "Data Size: %zu bytes\n", data_size );
  assert( file_size >= data_size );
  uintmax_t overhead = file_size - data_size;
  const int ratio = 100 * overhead / file_size;
  fprintf(fout, "Overhead: %zu bytes (%u%%)\n", overhead , ratio );
}

static uint16_t csiz;

static void printrgn( FILE *stream, size_t len )
{
  bool b;
  uint16_t crgn;

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

  uint8_t srgn;
  uint8_t sprgn;
  b = read8(stream, &srgn); assert( b );
  b = read8(stream, &sprgn); assert( b );

  fprintf(fout,"\n");
  print_with_indent(indentlevel, "  Component          : %u\n", crgn);
  print_with_indent(indentlevel, "  Style              : %s\n", srgn ? "other" : "implicit" );
  print_with_indent(indentlevel, "  Implicit ROI Shift : %u\n", sprgn);

}

static void printpoc( FILE *stream, size_t len )
{
  char buffer[512];
  assert( len < 512 );
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  fprintf(fout, "\n" );
  const uint8_t *p = (const uint8_t*)buffer;
  const uint8_t *end = p + len;
  int i = 0;
  int number_progression_order_change = 0;
  if( csiz < 257 )
    {
    number_progression_order_change = ( len )/ 7;
    }
  else
    {
    number_progression_order_change = ( len )/ 9;
    }
  assert( number_progression_order_change == 2 || number_progression_order_change == 1 );
  for( ; i < number_progression_order_change; ++i )
    {
    uint8_t rspoc = *p++;
    uint16_t cspoc;
    if( csiz < 257 )
      {
      cspoc = *p++;
      }
    else
      {
      cread16((char*)p, &cspoc);
      p += 2;
      }
    uint16_t lyepoc;
    cread16((char*)p, &lyepoc);
    p += 2;
    uint8_t repoc = *p++;
    uint16_t cepoc;
    if( csiz < 257 )
      {
      cepoc = *p++;
      }
    else
      {
      cread16((char*)p, &cepoc);
      p += 2;
      }
    uint8_t ppoc = *p++;
    const char * sProgressionOrder = getDescriptionOfProgressionOrderString(ppoc);

    print_with_indent( indentlevel, "  Resolution Level Index #%d (Start) : %u\n", i, rspoc );
    print_with_indent( indentlevel, "  Component Index #%d (Start)        : %u\n", i, cspoc );
    print_with_indent( indentlevel, "  Layer Index #%d (End)              : %u\n", i, lyepoc );
    print_with_indent( indentlevel, "  Resolution Level Index #%d (End)   : %u\n", i, repoc );
    print_with_indent( indentlevel, "  Component Index #%d (End)          : %u\n", i, cepoc );
    print_with_indent( indentlevel, "  Progression Order #%d              : %s\n", i, sProgressionOrder);
    }

  assert( p == end );
}

static void printqcc( FILE *stream, size_t len )
{
  bool b;
  uint16_t cqcc;

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
  uint8_t sqcc;

  b = read8(stream, &sqcc); assert( b );
  --len;
  uint8_t quant = (sqcc & 0x1f);
  uint8_t nbits = (sqcc >> 5);
  size_t i;
  fprintf(fout,"\n");
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

  print_with_indent(indentlevel, "  Index             : %u\n", cqcc );
  print_with_indent(indentlevel, "  Quantization Type : %s\n", s );
  print_with_indent(indentlevel, "  Guard Bits        : %u\n", nbits );

  if( quant == 0x0 )
    {
    size_t n = len;
    for( i = 0; i != n; ++i )
      {
      uint8_t val;
      b = read8(stream, &val); assert( b );
      const uint8_t exp = val >> 3;
      print_with_indent(indentlevel, "  Exponent #%-8u: %u\n", i, exp );
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

      print_with_indent(indentlevel, "  Mantissa #%-8u: %u\n", i, mant );
      print_with_indent(indentlevel, "  Exponent #%-8u: %u\n", i, exp );
      }
    }

}

static void printqcd( FILE *stream, size_t len )
{
  bool b;
  uint8_t sqcd;

  b = read8(stream, &sqcd); assert( b );
  --len;
  uint8_t quant = (sqcd & 0x1f);
  uint8_t nbits = (sqcd >> 5);
  size_t i;
  fprintf(fout,"\n");
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
  print_with_indent(indentlevel, "  Quantization Type : %s\n", s );
  print_with_indent(indentlevel, "  Guard Bits        : %u\n", nbits );

  if( quant == 0x0 )
    {
    size_t n = len;
    for( i = 0; i != n; ++i )
      {
      uint8_t val;
      b = read8(stream, &val); assert( b );
      const uint8_t exp = val >> 3;
      print_with_indent(indentlevel, "  Exponent #%-8u: %u\n", i, exp );
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
      const uint16_t mant = val & 0x7ff;
      const uint16_t exp = val >> 11;

      print_with_indent(indentlevel, "  Mantissa #%-8u: %u\n", i, mant );
      print_with_indent(indentlevel, "  Exponent #%-8u: %u\n", i, exp );
      }
    }
}

static void printeph( FILE *stream, size_t len )
{
  int c = 0;
  //assert( len == 2 );
  while( fgetc( stream ) != 0xFF )
    {
    ++c;
    }

  if( c )
    {
    fprintf(fout,"\n" );
    fprintf(fout,"Data : %u bytes\n", c );
    }
    data_size += c;
  (void)fseeko(stream, -1, SEEK_CUR);
}

static void printsop( FILE *stream, size_t len )
{
  assert( len == 2 );
  uint16_t Nsop;
  bool b;
  b = read16(stream, &Nsop); assert( b );

  fprintf(fout,"\n" );
  fprintf(fout,"  Sequence : %u\n", Nsop );
  fprintf(fout,"\n" );
  int c = 0;
  int k;
  char buf[64];
  while( (k = fgetc( stream )) != 0xFF )
    {
    buf[c] = k;
    ++c;
    }

  data_size += c;
  fprintf(fout,"Data : %u bytes\n", c );
  (void)fseeko(stream, -1, SEEK_CUR);
  buf[c] = 0;
  assert( 0 );
}

static void printsod( FILE *stream, size_t len )
{
  (void)fseeko(stream, (off_t)len, SEEK_CUR);
  if( len )
    {
    fprintf(fout,"\n" );
    fprintf(fout,"Data : %zu bytes\n", len );
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
  assert( len == 8 ); // 2 + 4 + 1 + 1
  b = read16(stream, &Isot); assert( b );
  b = read32(stream, &Psot); assert( b );
  b = read8(stream, &TPsot); assert( b );
  b = read8(stream, &TNsot); assert( b );
  fprintf(fout,"\n" );
  fprintf(fout,"  Tile        : %u\n", Isot );
  fprintf(fout,"  Length      : %u\n", Psot );
  fprintf(fout,"  Index       : %u\n", TPsot );
  if( TNsot )
    fprintf(fout,"  Tile-Parts: : %u\n", TNsot );
  else
    fprintf(fout,"  Tile-Parts: : unknown\n" );
}

static void printcomment( FILE *stream, size_t len )
{
  uint16_t rcom;
  bool b;
  b = read16(stream, &rcom); assert( b );
  len -= 2;
  fprintf(fout,"\n" );
  fprintf(fout,"  Registration : %s\n", rcom ? "ISO-8859-15" : "binary" );
  if( len < 512 )
    {
    char buffer[512];
    size_t l = fread(buffer,sizeof(char),len,stream);
    buffer[len] = 0;
    assert( l == len );
    fprintf(fout,"  Comment      : %s\n", buffer );
    }
  else
    {
    (void)fseeko(stream, (off_t)len, SEEK_CUR);
    fprintf(fout,"  Comment      : ...\n");
    }

}

static void printtlm( FILE *stream, size_t len )
{
  char buffer[512];
  assert( len < 512 );
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  fprintf(fout, "\n" );
  const uint8_t *p = (const uint8_t*)buffer;
  const uint8_t *end = p + len;
  uint8_t Ztlm = *p++;
  fprintf(fout, "  Index          : %u\n", Ztlm );
  uint8_t Stlm = *p++;
  uint8_t ST = ( Stlm >> 4 ) & 0x3;
  uint8_t SP = ( Stlm >> 6 ) & 0x1;

	int Ptlm_size = (SP + 1) * 2;
	int quotient = Ptlm_size + ST;
	size_t header_size = len - 2;

	int tot_num_tp = header_size / quotient;

  int i;
  for (i = 0; i < tot_num_tp; ++i)
    {
    if( ST == 0 )
      {
      fprintf(fout, "  Tile index #%-3d: in order\n", i );
      }
    else if( ST == 1 )
      {
      uint8_t v = *p;
      fprintf(fout, "  Tile index #%-3d: %u\n", i, v );
      }
    else if( ST == 2 )
      {
      uint16_t v;
      cread16((char*)p, &v);
      fprintf(fout, "  Tile index #%-2d: %u\n", i, v );
      }
    else
      {
      assert( 0 );
      }
    p += ST;
    if( Ptlm_size == 2 )
      {
      uint16_t v;
      cread16((char*)p, &v);
      fprintf(fout, "  Length #%-7d: %u\n",i, v );
      }
    if( Ptlm_size == 4 )
      {
      uint32_t v;
      cread32((char*)p, &v);
      fprintf(fout, "  Length #%-6d: %u\n",i, v );
      }
    else
      {
      assert( 0 );
      }
    p += Ptlm_size;
    }
  assert( p == end );
}


static void printcoc( FILE *stream, size_t len )
{
  char buffer[512];
  assert( len < 512 );
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  const uint8_t *p = (const uint8_t*)buffer;
  const uint8_t *end = p + len;
  uint16_t ccoc;
  if( csiz < 257 )
    {
    ccoc = *p++;
    }
  else
    {
    cread16((char*)p, &ccoc);
    p += 2;
    }
  uint8_t Scoc = *p++;
  assert( Scoc == 0x0 || Scoc == 0x1 );
  uint8_t NumberOfDecompositionLevels = *p++;
  uint8_t CodeBlockWidth = *p++ & 0xf;
  uint8_t CodeBlockHeight = *p++ & 0xf;
  uint8_t xcb = CodeBlockWidth + 2;
  uint8_t ycb = CodeBlockHeight + 2;
  assert( xcb + ycb <= 12 );
  uint8_t CodeBlockStyle = *p++ & 0x3f;
  uint8_t Transformation = *p++;

#if 0
  if( Scoc )
    {
    /* Table A.21 - Precinct width and height for the SPcod and SPcoc parameters */
    uint_fast8_t i;
    uint8_t n = *p++;
    for( i = 0; i < NumberOfDecompositionLevels; ++i )
      {
      *p++;
      }
    }
    // p1_03.j2k ???
    //assert( !NumberOfDecompositionLevels );
  assert( p == end );
#endif
  bool ResetContextProbabilitiesOnCodingPassBoundaries = (CodeBlockStyle & 0x02) != 0;
  bool TerminationOnEachCodingPass                     = (CodeBlockStyle & 0x04) != 0;
  bool VerticallyCausalContext                         = (CodeBlockStyle & 0x08) != 0;
  bool PredictableTermination                          = (CodeBlockStyle & 0x10) != 0;
  bool SegmentationSymbolsAreUsed                      = (CodeBlockStyle & 0x20) != 0;
  const char * sTransformation = getDescriptionOfWaveletTransformationString(Transformation);

  fprintf(fout, "\n");
  fprintf(fout, "  Component                          : %u\n", ccoc);
  fprintf(fout, "  Precincts                          : %s\n", Scoc == 0x0 ? "default" : "custom" );
  fprintf(fout, "  Decomposition Levels               : %u\n", NumberOfDecompositionLevels);
  fprintf(fout, "  Code-block size                    : %ux%u\n", 1 << xcb, 1 << ycb);
  fprintf(fout, "  Selective Arithmetic Coding Bypass : %s\n", Scoc ? "yes" : "no" );
  fprintf(fout, "  Reset Context Probabilities        : %s\n", ResetContextProbabilitiesOnCodingPassBoundaries ? "yes" : "no" );
  fprintf(fout, "  Termination on Each Coding Pass    : %s\n", TerminationOnEachCodingPass ? "yes" : "no" );
  fprintf(fout, "  Vertically Causal Context          : %s\n", VerticallyCausalContext ? "yes" : "no" );
  fprintf(fout, "  Predictable Termination            : %s\n", PredictableTermination ? "yes" : "no" );
  fprintf(fout, "  Segmentation Symbols               : %s\n", SegmentationSymbolsAreUsed ? "yes" : "no" );
  fprintf(fout, "  Wavelet Transformation             : %s\n", sTransformation );

  if( Scoc )
    {
    //uint8_t N = *p++;
    uint_fast8_t i;
    for( i = 0; i <= NumberOfDecompositionLevels; ++i )
      {
      uint8_t val = *p++;
      /* Table A.21 - Precinct width and height for the SPcod and SPcoc parameters */
      uint8_t width = val & 0x0f;
      uint8_t height = val >> 4;
      print_with_indent(indentlevel, "  Precinct #%u Size Exponents         : %ux%u\n", i, width, height );
      }
    }
  assert( p == end );
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
  const uint8_t *end = p + len;
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
  bool EPHMarkerSegments   = (Scod & 0x04) != 0;

  /* Table A.19 - Code-block style for the SPcod and SPcoc parameters */
  bool SelectiveArithmeticCodingBypass                 = (CodeBlockStyle & 0x01) != 0;
  bool ResetContextProbabilitiesOnCodingPassBoundaries = (CodeBlockStyle & 0x02) != 0;
  bool TerminationOnEachCodingPass                     = (CodeBlockStyle & 0x04) != 0;
  bool VerticallyCausalContext                         = (CodeBlockStyle & 0x08) != 0;
  bool PredictableTermination                          = (CodeBlockStyle & 0x10) != 0;
  bool SegmentationSymbolsAreUsed                      = (CodeBlockStyle & 0x20) != 0;
  fprintf(fout, "\n" );

  print_with_indent(indentlevel, "  Default Precincts of 2^15x2^15     : %s\n" , VariablePrecinctSize ? "no" : "yes" );
  print_with_indent(indentlevel, "  SOP Marker Segments                : %s\n" , SOPMarkerSegments ? "yes" : "no" );
  print_with_indent(indentlevel, "  EPH Marker Segments                : %s\n" , EPHMarkerSegments ? "yes" : "no" );
  print_with_indent(indentlevel, "  All Flags                          : %08u\n", Scod );
  print_with_indent(indentlevel, "  Progression Order                  : %s\n", sProgressionOrder );
  print_with_indent(indentlevel, "  Layers                             : %u\n", NumberOfLayers );
  print_with_indent(indentlevel, "  Multiple Component Transformation  : %s\n", sMultipleComponentTransformation );
  print_with_indent(indentlevel, "  Decomposition Levels               : %u\n", NumberOfDecompositionLevels );
  print_with_indent(indentlevel, "  Code-block size                    : %ux%u\n", 1 << xcb, 1 << ycb );
  print_with_indent(indentlevel, "  Selective Arithmetic Coding Bypass : %s\n", SelectiveArithmeticCodingBypass ? "yes" : "no" );
  print_with_indent(indentlevel, "  Reset Context Probabilities        : %s\n", ResetContextProbabilitiesOnCodingPassBoundaries ? "yes" : "no" );
  print_with_indent(indentlevel, "  Termination on Each Coding Pass    : %s\n", TerminationOnEachCodingPass ? "yes" : "no" );
  print_with_indent(indentlevel, "  Vertically Causal Context          : %s\n", VerticallyCausalContext ? "yes" : "no" );
  print_with_indent(indentlevel, "  Predictable Termination            : %s\n", PredictableTermination ? "yes" : "no" );
  print_with_indent(indentlevel, "  Segmentation Symbols               : %s\n", SegmentationSymbolsAreUsed ? "yes" : "no" );
  print_with_indent(indentlevel, "  Wavelet Transformation             : %s\n", sTransformation );

  if( VariablePrecinctSize )
    {
    //uint8_t N = *p++;
    uint_fast8_t i;
    for( i = 0; i <= NumberOfDecompositionLevels; ++i )
      {
      uint8_t val = *p++;
      /* Table A.21 - Precinct width and height for the SPcod and SPcoc parameters */
      uint8_t width = val & 0x0f;
      uint8_t height = val >> 4;
      print_with_indent(indentlevel, "  Precinct #%u Size Exponents         : %ux%u\n", i, width, height );
      }
    }

  assert( p == end );
}

/* I.5.1 JPEG 2000 Signature box */
static void printsignature( FILE * stream )
{
  uint32_t s;
  const uint32_t sig = 0xd0a870a;
  bool b = read32(stream, &s);
  assert( b );
  fprintf(fout,"\n" );
  fprintf(fout,"  Corrupted: %s\n", s == sig ? "no" : "yes" );
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
  fprintf(fout, "\n" );
  fprintf(fout, "Data:\n" );
  for( ; len != 0; --len )
    {
    int val = fgetc(stream);
    //if( isascii( val ) )
    /* remove ^M */
    /*if( !iscntrl( val ) ) */
    /* it does not look like the tool is consistant BTW */
    if( val != 0xd )
      fprintf(fout, "%c", val );
    }
  fprintf(fout, "\n");
  fprintf(fout, "\n");
  fprintf(fout, "\n");
}

/* I.5.2 File Type box */
static void printfiletype( FILE * stream, size_t len )
{
  char br[4+1];
  br[4] = 0;
  uint32_t minv;
  bool b= true;
  fread(br,4,1,stream); assert( b );
  b = read32(stream, &minv); assert( b );
  /* The number of CLi fields is determined by the length of this box. */
  int n = (len - 8 ) / 4;

  /* Table I.3 - Legal Brand values */
  const char *brand = getbrand( br );
  fprintf(fout,"\n" );
  if( brand )
    {
    fprintf(fout,"  Brand: %s\n", brand );
    }
  else
    {
    uint32_t v;
    memcpy( &v, br, 4 );
    fprintf(fout,"  Brand: 0x%08x\n", bswap_32(v) );
    }
  fprintf(fout,"  Minor version: %u\n", minv );
  int i;
  fprintf(fout,"  Compatibility: " );
  for (i = 0; i < n; ++i )
    {
    if( i ) fprintf(fout, " " );
    fread(br,4,1,stream); assert( b );
    const char *brand = getbrand( br );
    if( brand )
      {
      fprintf(fout,"%s", brand );
      }
    else
      {
      uint32_t v;
      memcpy( &v, br, 4 );
      fprintf(fout,"0x%08x", bswap_32(v) );
      }
    }
    fprintf(fout, "\n" );
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

  const bool sign = bpc >> 7;
  fprintf(fout,"\n" );
  print_with_indent(indentlevel, "  Height: %u\n", height );
  print_with_indent(indentlevel, "  Width: %u\n", width);
  print_with_indent(indentlevel, "  Components: %u\n", nc);
  print_with_indent(indentlevel, "  Bits Per Component: %u\n", (bpc & 0x7f) + 1);
  print_with_indent(indentlevel, "  Signed Components: %s\n", sign ? "yes" : "no" );
  print_with_indent(indentlevel, "  Compression Type: %s\n", c == 7 ? "wavelet" : "holly crap");
  print_with_indent(indentlevel, "  Unknown Colourspace: %s\n", Unk ? "yes" : "no");
  print_with_indent(indentlevel, "  Intellectual Property: %s\n", IPR ? "yes" : "no");
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
  fprintf(fout, "\n" );
  for( i = 0; i < n; ++i )
    {
    b = read16(stream, &CMPi); assert( b );
    len--;
    len--;
    b = read8(stream, &MTYPi); assert( b );
    len--;
    b = read8(stream, &PCOLi); assert( b );
    len--;
    fprintf(fout, "  Component      #%d: %u\n", i, CMPi );
    fprintf(fout, "  Mapping Type   #%d: %s\n", i, MTYPi ? "palette mapping" : "wazzza" );
    fprintf(fout, "  Palette Column #%d: %u\n", i, PCOLi );
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
  fprintf(fout,"\n" );
  fprintf(fout,"  Entries: %u\n", NE );
  fprintf(fout,"  Created Channels: %u\n", NPC  );
  for( j = 0; j < NPC; ++j )
    {
    b = read8(stream, &Bi); assert( b );
    len--;
    const bool sign = Bi >> 7;
    fprintf(fout,"  Depth  #%u: %u\n", j, (Bi & 0x7f) + 1 );
    fprintf(fout,"  Signed #%u: %s\n", j, sign ? "yes" : "no" );
    }
  for( i = 0; i < NE; ++i )
    {
    fprintf(fout,"  Entry #%03u: ", (unsigned)i );
    for( j = 0; j < NPC; ++j )
      {
      b = read8(stream, &Cij); assert( b );
      len--;
      if( j ) fprintf(fout," ");
      fprintf(fout,"0x%010x", Cij );
      }
    fprintf(fout,"\n" );
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
  fprintf(fout,"\n" );
  for (i = 0; i < N; ++i )
    {
    b = read16(stream, &cni); assert( b );
    b = read16(stream, &typi); assert( b );
    b = read16(stream, &asoci); assert( b );
    print_with_indent(indentlevel,"  Channel     #%u: %x\n", i, cni );
    print_with_indent(indentlevel,"  Type        #%u: %s\n", i, typi == 0 ? "color" : "donno" );
    print_with_indent(indentlevel,"  Association #%u: %x\n", i, asoci );
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

  fprintf(fout,"\n");
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
  char buffer[512];
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
  print_with_indent(indentlevel,"  Colour Specification Method: %s\n", smeth);
  print_with_indent(indentlevel,"  Precedence: %u\n", prec);
  print_with_indent(indentlevel,"  Approximation: %u\n", approx);
  if( meth == 1 )
    {
    print_with_indent(indentlevel,"  Colourspace: %s\n", senumCS);
    assert( len == 0);
    }
  else if( meth == 2 )
    {
    fprintf(fout,"  ICC Colour Profile:" );
    assert( len != 0 );
    size_t i;
    for( i = 0; i < len; ++i )
      {
      int val = fgetc(stream);
      if( i % 16 == 0 ) fprintf (fout, "\n    " );
      fprintf(fout, "%02x ", val );
      }
    }
  fprintf (fout, "\n" );
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

    const dictentry2 *d = getdictentry2frommarker( marker );
    fprintf(fout, "\n" );
    fprintf(fout, "  Sub box: \"%s\" (%s)\n", d->shortname, d->longname );
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
  const dictentry2 *d = getdictentry2frommarker( marker );
  (void)len;
  fprintf(fout, "\n" );
  if( d->shortname )
    fprintf(fout, "New box: \"%s\" (%s)\n", d->shortname, d->longname );
  else
    {
    char buffer[4+1];
    uint32_t swap = bswap_32( marker );
    memcpy( buffer, &swap, 4);
    buffer[4] = 0;
    fprintf(fout, "New box: \"%s\" (unknown box)\n", buffer );
    fprintf(fout, "\n  " );
    len -= 8;
    for( ; len != 0; --len )
      {
      int val = fgetc(stream);
      fprintf(fout, "%02x ", val );
      }
      fprintf(fout, "\n" );
    return false;
    }

  bool skip = false;
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
  fprintf(fout, "\n" );
  const char t1[] = "Required Capabilities";
  const char t2[] = "Reference Grid Size";
  const char t3[] = "Image Offset";
  const char t4[] = "Reference Tile Size";
  const char t5[] = "Reference Tile Offset";
  const char t6[] = "Components";
  print_with_indent(indentlevel, "  %-32s : %s\n"   , t1, s );
  print_with_indent(indentlevel, "  %-32s : %ux%u\n", t2, xsiz, ysiz );
  print_with_indent(indentlevel, "  %-32s : %ux%u\n", t3, xosiz, yosiz );
  print_with_indent(indentlevel, "  %-32s : %ux%u\n", t4, xtsiz, ytsiz );
  print_with_indent(indentlevel, "  %-32s : %ux%u\n", t5, xtosiz, ytosiz );
  print_with_indent(indentlevel, "  %-32s : %u\n"   , t6, csiz );

  const char t7[] = "Depth";
  const char t8[] = "Signed";
  const char t9[] = "Sample Separation";

  uint_fast16_t i = 0;
  for( i = 0; i < csiz; ++i )
    {
    b = read8(stream, &ssiz); assert( b ); /* int8_t ? */
    const bool sign = ssiz >> 7;
    b = read8(stream, &xrsiz); assert( b );
    b = read8(stream, &yrsiz); assert( b );
    char buffer[50];
    sprintf( buffer, "Component #%u %s", (unsigned)i, t7 );
    print_with_indent(indentlevel, "  %-33s: %u\n"   , buffer, (ssiz & 0x7f) + 1 );
    sprintf( buffer, "Component #%u %s", (unsigned)i, t8 );
    print_with_indent(indentlevel, "  %-33s: %s\n"   , buffer, sign ? "yes" : "no" );
    sprintf( buffer, "Component #%u %s", (unsigned)i, t9 );
    print_with_indent(indentlevel, "  %-33s: %ux%u\n", buffer, xrsiz, yrsiz );
    }
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
  const dictentry *d = getdictentryfrommarker( marker );
  assert( offset >= 0 );
  assert( d->shortname );
  fprintf(fout, "\n" );
  print_with_indent( indentlevel, "%-8u: New marker: %s (%s)",
    offset - rel_offset, d->shortname, d->longname );
  fprintf(fout,"\n");
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
  if( argc < 2 ) return 1;
  const char *filename = argv[1];

  if( argc > 2 )
    {
    const char *outfilename = argv[2];
    fout = fopen( outfilename, "w" );
    }
  else
    {
    fout = stdout;
    }

  bool b;
  data_size = 0;
  file_size = getfilesize( filename );
  if( isjp2file( filename ) )
    {
    fprintf(fout,"###############################################################\n" );
    fprintf(fout,"# JP2 file format log file generated by jp2file.py            #\n" );
    fprintf(fout,"# jp2file.py is copyrighted (c) 2001,2002                     #\n" );
    fprintf(fout,"# by Algo Vision Technology GmbH, All Rights Reserved         #\n" );
    fprintf(fout,"#                                                             #\n" );
    fprintf(fout,"# http://www.av-technology.de/ jpeg2000@av-technology.de      #\n" );
    fprintf(fout,"###############################################################\n" );

    indentlevel = 2;
    b = parsejp2( filename, &print2, &print1 );
    }
  else
    {
    fprintf(fout, "###############################################################\n" );
    fprintf(fout, "# JP2 codestream log file generated by jp2codestream.py       #\n" );
    fprintf(fout, "# jp2codestream.py is copyrighted (c) 2001,2002               #\n" );
    fprintf(fout, "# by Algo Vision Technology GmbH, All Rights Reserved         #\n" );
    fprintf(fout, "#                                                             #\n" );
    fprintf(fout, "# http://www.av-technology.de/ jpeg2000@av-technology.de      #\n" );
    fprintf(fout, "###############################################################\n" );

    b = parsej2k( filename, &print1 );
    }

  if( argc > 2 )
    {
    fclose( fout );
    }

  if( !b ) return 1;

  return 0;
}
