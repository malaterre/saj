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

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <byteswap.h>

#include <simpleparser.h>

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
  { JP  , "JP  ", "JPEG 2000 signature box" },
  { FTYP, "FTYP", "File type box" },
  { JP2H, "JP2H", "JP2 header box (super-box)" },
  { JP2C, "JP2C", "Contiguous codestream box" },

  { 0, 0, 0 }
};

static const dictentry2 * getdictentry2frommarker( uint_fast32_t marker )
{
  static const size_t n = sizeof( dict2 ) / sizeof( dictentry2 ) - 1; /* remove sentinel */
  /* optimisation: binary search possible ? */
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
  { SOC, "SOC", "Start of codestream" },
  { SOT, "SOT", "Start of tile-part" },
  { SOD, "SOD", "Start of data" },
  { EOC, "EOC", "End of codestream" },
  { SIZ, "SIZ", "Image and tile size" },
  { COD, "COD", "Coding style default" },
  { COC, "COC", "Coding style component" },
  { RGN, "RGN", "Rgeion-of-interest" },
  { QCD, "QCD", "Quantization default" },
  { QCC, "QCC", "Quantization component" },
  { POC, "POC", "Progression order change" },
  { TLM, "TLM", "Tile-part lengths" },
  { PLM, "PLM", "Packet length, main header" },
  { PLT, "PLT", "Packet length, tile-part header" },
  { PPM, "PPM", "Packet packer headers, main header" },
  { PPT, "PPT", "Packet packer headers, tile-part header" },
  { SOP, "SOP", "Start of packet" },
  { EPH, "EPH", "End of packet header" },
  { CRG, "CRG", "Component registration" },
  { COM, "COM", "Comment (JPEG 2000)" },

  { 0, 0, 0 }
};

static const dictentry * getdictentryfrommarker( uint_fast16_t marker )
{
  static const size_t n = sizeof( dict ) / sizeof( dictentry ) - 1; /* remove sentinel */
  /* optimisation: binary search possible ? */
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
      descriptionOfProgressionOrder = "Layer-resolution level-component-position";
      break;
    case 0x01:
      descriptionOfProgressionOrder = "Resolution level-layer-component-position";
      break;
    case 0x02:
      descriptionOfProgressionOrder = "Resolution level-position-component-layer";
      break;
    case 0x03:
      descriptionOfProgressionOrder = "Position-component-resolution level-layer";
      break;
    case 0x04:
      descriptionOfProgressionOrder = "Component-position-resolution level-layer";
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
    descriptionOfMultipleComponentTransformation = "None";
    break;
  case 0x01:
    descriptionOfMultipleComponentTransformation = "Part 1 Annex G2 transformation of components 0,1,2";
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
      descriptionOfWaveletTransformation = "9-7 irreversible filter";
      break;
    case 0x01:
      descriptionOfWaveletTransformation = "5-3 reversible filter";
      break;
  }
  return descriptionOfWaveletTransformation;
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
  uint8_t CodeBlockWidth = *p++;
  uint8_t CodeBlockHeight = *p++;
  uint8_t CodeBlockStyle = *p++;
  uint8_t Transformation = *p++;

  const char * sMultipleComponentTransformation = getMultipleComponentTransformationString(MultipleComponentTransformation);
  const char * sProgressionOrder = getDescriptionOfProgressionOrderString(ProgressionOrder);
  const char * sTransformation = getDescriptionOfWaveletTransformationString(Transformation);

  printf( "\tJPEG_COD_Parameters:\n" );
  printf( "\t\tScod = 0x%u\n", Scod );
  /* Table A.13 Coding style parameter values for the Scod parameter */
  bool VariablePrecinctSize = (Scod & 0x01) != 0;
  bool SOPMarkerSegments    = (Scod & 0x02) != 0;
  bool EPHPMarkerSegments   = (Scod & 0x04) != 0;
  printf("\t\t\t Precinct size %s\n", (VariablePrecinctSize ? "defined for each resolution level" : "PPx = 15 and PPy = 15") );
  printf("\t\t\t SOPMarkerSegments = %s used\n", (SOPMarkerSegments    ? "may be"   : "not") );
  printf("\t\t\t EPHPMarkerSegments = %s used\n", (EPHPMarkerSegments   ? "shall be" : "not") );
  printf( "\t\tProgressionOrder = 0x%x (%s progression)\n", ProgressionOrder, sProgressionOrder );
  printf( "\t\tNumberOfLayers = %u\n", NumberOfLayers );
  printf( "\t\tMultipleComponentTransformation = 0x%x (%s)\n", MultipleComponentTransformation, sMultipleComponentTransformation );
  printf( "\t\tNumberOfDecompositionLevels = %u\n", NumberOfDecompositionLevels );
  printf( "\t\tCodeBlockWidth = 0x%x\n", CodeBlockWidth );
  printf( "\t\tCodeBlockHeight = 0x%x\n", CodeBlockHeight );
  printf( "\t\tCodeBlockStyle = 0x%x\n", CodeBlockStyle );

  /* Table A.19 - Code-block style for the SPcod and SPcoc parameters */
  bool SelectiveArithmeticCodingBypass                 = (CodeBlockStyle & 0x01) != 0;
  bool ResetContextProbabilitiesOnCodingPassBoundaries = (CodeBlockStyle & 0x02) != 0;
  bool TerminationOnEachCodingPass                     = (CodeBlockStyle & 0x04) != 0;
  bool VerticallyCausalContext                         = (CodeBlockStyle & 0x08) != 0;
  bool PredictableTermination                          = (CodeBlockStyle & 0x10) != 0;
  bool SegmentationSymbolsAreUsed                      = (CodeBlockStyle & 0x20) != 0;
  printf( "\t\t\t %s arithmetic coding bypass\n", (SelectiveArithmeticCodingBypass                  ? "Selective"    : "No selective") );
  printf( "\t\t\t %s context probabilities on coding pass boundaries\n", (ResetContextProbabilitiesOnCodingPassBoundaries  ? "Reset"        : "No reset of"));
  printf( "\t\t\t %s on each coding pass\n", (TerminationOnEachCodingPass                      ? "Termination"  : "No termination"));
  printf( "\t\t\t %s causal context\n", (VerticallyCausalContext                          ? "Vertically"   : "No vertically"));
  printf( "\t\t\t %s termination\n", (PredictableTermination                           ? "Predictable"  : "No predictable"));
  printf( "\t\t\t %s symbols are used\n", (SegmentationSymbolsAreUsed                       ? "Segmentation" : "No segmentation"));
  printf( "\t\tWaveletTransformation = 0x%x (%s)\n", Transformation, sTransformation );
  printf( "\n" );
}

static bool print2( uint_fast32_t marker, size_t len, FILE *stream )
{
  off_t offset = ftello(stream);
  const dictentry2 *d = getdictentry2frommarker( marker );
  (void)len;
  printf( "Offset 0x%04tx Marker 0x%04x %s %s\n", offset, (uint32_t)marker, d->shortname, d->longname );

  switch( marker )
    {
  case JP2C:
    puts( "--" );
    break;
    }

  return true;
}

static bool print( uint_fast16_t marker, size_t len, FILE *stream )
{
  off_t offset = ftello(stream);
  if( hasnolength( marker ) )
    {
    offset -= 2; /* remove size of marker itself */
    }
  else
    {
    offset -= 4; /* remove size + len of marker itself */
    }
  const dictentry *d = getdictentryfrommarker( marker );
  assert( offset >= 0 );
  printf( "Offset 0x%04jx Marker 0x%04x %s %s", offset, (unsigned int)marker, d->shortname, d->longname );
  if( !hasnolength( marker ) )
    {
    printf( " length variable 0x%02zx", len + 2 );
    }
  printf("\n");
  switch( marker )
    {
  case EOC:
    puts( "End of file" );
    break;
  case COD:
    printcod( stream, len );
    return false; /* printcod internally read the stream */
    }
  return true;
}

int main(int argc, char *argv[])
{
  if( argc < 2 ) return 1;
  const char *filename = argv[1];

  bool b;
  if( isjp2file( filename ) )
    {
    b = parsejp2( filename, &print2, &print );
    }
  else
    {
    b = parsej2k( filename, &print );
    }
  if( !b ) return 1;

  return 0;
}
