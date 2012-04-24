#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <byteswap.h>

/* realpath */
#include <limits.h>
#include <stdlib.h>
/* strlen */
#include <string.h>
#include <stdarg.h> /* va_list */

#include <simpleparser.h>

FILE * fout;
/*
Options -
  -[No_]Tiles
    Tiles segment parameters are to be included (or not).
    The default is not to include tile segment parameters.  */
static bool printtiles = false;

void print_with_indent(int indent, const char * format, ...)
{
  va_list arg;
  va_start(arg, format);
  fprintf(fout,"%*s" "%s", indent, " ", "");
  vfprintf(fout,format, arg);
  va_end(arg);
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
  { JP  , "jP"  , "Signature" },
  { FTYP, "ftyp", "File_Type" },
  { JP2H, "jp2h", "JP2_Header" },
  { JP2C, "jp2c", "Contiguous_Codestream" },
  { JP2 , "jp2" , "" },
  { XML , "xml", "XML" },
  { IHDR, "ihdr", "Image_Header" },
  { COLR, "colr", "Colour_Specification" },
  { CDEF, "cdef", "Channel_Definition" },
  { CMAP, "cmap", "Component_Mapping" },
  { PCLR, "pclr", "Palette" },
  { RES , "res", "Resolution" },
//  { IPTR, "iptr", "Unknown" }, /* FIXME */

  { 0, 0, 0 }
};

static void print0a()
{
  const char c = 0x0a;
  fprintf(fout,"%c",c);
}

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
  { SOC, "Codestream", "Start_of_Codestream" },
  { SOT, "SOT", "Start_of_Tile_Part" },
  { SOD, "SOD", "Start_of_Data" },
  { EOC, "EOC", "End of codestream" },
  { SIZ, "SIZ", "Size" },
  { COD, "COD", "Coding_Style_Default" },
  { COC, "COC", "Coding_Style_Component" },
  { RGN, "RGN", "Rgeion-of-interest" },
  { QCD, "QCD", "Quantization_Default" },
  { QCC, "QCC", "Quantization component" },
  { POC, "POC", "Progression order change" },
  { TLM, "TLM", "Tile-part lengths" },
  { PLM, "PLM", "Packet length, main header" },
  { PLT, "PLT", "Packet_Length_Tile" },
  { PPM, "PPM", "Packet packer headers, main header" },
  { PPT, "PPT", "Packet packer headers, tile-part header" },
  { SOP, "SOP", "Start of packet" },
  { EPH, "EPH", "End of packet header" },
  { CRG, "CRG", "Component registration" },
  { COM, "COM", "Comment" },

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

static bool read8(FILE *input, uint8_t * ret)
{
  union { uint8_t v; char bytes[1]; } u;
  size_t l = fread(u.bytes,sizeof(char),1,input);
  if( l != 1 || feof(input) ) return false;
  *ret = u.v;
  return true;
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
      descriptionOfProgressionOrder = "Layer-Resolution-Component-Position";
      break;
    case 0x01:
      descriptionOfProgressionOrder = "Resolution-Layer-Component-Position";
      break;
    case 0x02:
      descriptionOfProgressionOrder = "Resolution-Position-Component-Layer";
      break;
    case 0x03:
      descriptionOfProgressionOrder = "Position-component-resolution level-layer";
      break;
    case 0x04:
      descriptionOfProgressionOrder = "Component-Position-Resolution-Layer";
      break;
  }
  return descriptionOfProgressionOrder;
}

/* Table A.17 Multiple component transformation for the SGcod parameters */
#if 0
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
#endif

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

static void printqcd( FILE *stream, size_t len )
{
  uint8_t sqcd;
  bool b;
  assert( len >= 4 );
  len -= 4;

  b = read8(stream, &sqcd); assert( b );
  --len;
  bool bquant = (sqcd & 0x7f) == 0;
  uint8_t quant = (sqcd & 0x1f);
  uint8_t nbits = (sqcd >> 5);
  fprintf(fout, "\t\t\t\t/*\n");
  fprintf(fout, "\t\t\t\t    Quantization:\n");
  fprintf(fout, "\t\t\t\t      %s\n", bquant ? "yes" : "None");
  fprintf(fout, "\t\t\t\t*/\n");
  fprintf(fout, "\t\t\t\tQuantization_Style = %u\n", quant);
  fprintf(fout, "\t\t\t\tTotal_Guard_Bits = %u\n",nbits);
  fprintf(fout, "\t\t\t\t/*\n");
  fprintf(fout, "\t\t\t\t    Reversible transform dynamic range exponent by sub-band.\n");
  fprintf(fout, "\t\t\t\t*/\n");
  fprintf(fout, "\t\t\t\tStep_Size = \n");
  fprintf(fout, "\t\t\t\t	(" );
#if 0
  size_t i;
  for( i = 0; i != len; ++i )
    {
    if( i ) fprintf(fout, ", " );
    b = read8(stream, &sqcd); assert( b );
    fprintf(fout,"%u", sqcd);
    }
#endif
  int i;
  if( quant == 0x0 )
    {
    size_t n = len;
    for( i = 0; i != n; ++i )
      {
      uint8_t val;
      b = read8(stream, &val); assert( b );
      const uint8_t exp = val >> 3;
      if(i) fprintf(fout, ", " );
      fprintf(fout, "%u", exp );
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

      fprintf(fout, "\n  (%u, %u)", exp, mant );
      }
    }
  fprintf(fout,")\n");
}

static void printcod( FILE *stream, size_t len )
{
  assert( len >= 4 );
  len -= 4;

  char buffer[512];
  assert( len < 512 );
  size_t r = fread( buffer, sizeof(char), len, stream);
  assert( r == len );

  union { uint16_t v; char bytes[2]; } u16;

/* Table A.12 - Coding style default parameter values */
  const uint8_t *p = (uint8_t*)buffer;
  const uint8_t *end = p + len;
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

  /*const char * sMultipleComponentTransformation = getMultipleComponentTransformationString(MultipleComponentTransformation);*/
  const char * sProgressionOrder = getDescriptionOfProgressionOrderString(ProgressionOrder);
  const char * sTransformation = getDescriptionOfWaveletTransformationString(Transformation);

#if 1
  /* Table A.13 Coding style parameter values for the Scod parameter */
  bool VariablePrecinctSize = (Scod & 0x01) != 0;
  bool SOPMarkerSegments    = (Scod & 0x02) != 0;
  bool EPHPMarkerSegments   = (Scod & 0x04) != 0;
#endif

  fprintf(fout, "\t\t\t\t/*\n" );
#if 0
  fprintf(fout,"\t\t\t Precinct size %s\n", (VariablePrecinctSize ? "defined for each resolution level" : "PPx = 15 and PPy = 15") );
  fprintf(fout,"\t\t\t SOPMarkerSegments = %s used\n", (SOPMarkerSegments    ? "may be"   : "not") );
  fprintf(fout,"\t\t\t EPHPMarkerSegments = %s used\n", (EPHPMarkerSegments   ? "shall be" : "not") );
#endif

  fprintf(fout, "\t\t\t\t    Entropy coder precincts:\n" );
  if( VariablePrecinctSize )
    fprintf(fout, "\t\t\t\t      Precinct size defined in the Precinct_Size parameter.\n" );
  else
    fprintf(fout, "\t\t\t\t      Precinct size = %u x %u\n", 0 , 0 );
  fprintf(fout, "\t\t\t\t      No SOP marker segments used\n" );
  fprintf(fout, "\t\t\t\t      No EPH marker used\n" );
  fprintf(fout, "\t\t\t\t*/\n" );
  fprintf(fout, "\t\t\t\tCoding_Style = 16#%u#\n", Scod );

  fprintf(fout, "\t\t\t\t/*\n" );
  fprintf(fout, "\t\t\t\t    Progression order:\n" );
  fprintf(fout, "\t\t\t\t      %s\n", sProgressionOrder );
  fprintf(fout, "\t\t\t\t*/\n" );
  fprintf(fout, "\t\t\t\tProgression_Order = %u\n", ProgressionOrder );
  fprintf(fout, "\t\t\t\tTotal_Quality_Layers = %u\n", NumberOfLayers );
  fprintf(fout, "\t\t\t\tMultiple_Component_Transform = %u\n", MultipleComponentTransformation );
  fprintf(fout, "\t\t\t\tTotal_Resolution_Levels = %u\n", NumberOfDecompositionLevels + 1 );
  fprintf(fout, "\t\t\t\t/*\n" );
  fprintf(fout, "\t\t\t\t    Code-block width exponent offset %u.\n", CodeBlockWidth );
  fprintf(fout, "\t\t\t\t*/\n" );
  fprintf(fout, "\t\t\t\tCode_Block_Width = %u\n", 1 << CodeBlockWidth+2 );
  fprintf(fout, "\t\t\t\t/*\n" );
  fprintf(fout, "\t\t\t\t    Code-block height exponent offset %u.\n", CodeBlockHeight );
  fprintf(fout, "\t\t\t\t*/\n" );
  fprintf(fout, "\t\t\t\tCode_Block_Height = %u\n", 1 << CodeBlockHeight+2 );
  fprintf(fout, "\t\t\t\t/*\n" );

#if 0
  /* Table A.19 - Code-block style for the SPcod and SPcoc parameters */
  bool SelectiveArithmeticCodingBypass                 = (CodeBlockStyle & 0x01) != 0;
  bool ResetContextProbabilitiesOnCodingPassBoundaries = (CodeBlockStyle & 0x02) != 0;
  bool TerminationOnEachCodingPass                     = (CodeBlockStyle & 0x04) != 0;
  bool VerticallyCausalContext                         = (CodeBlockStyle & 0x08) != 0;
  bool PredictableTermination                          = (CodeBlockStyle & 0x10) != 0;
  bool SegmentationSymbolsAreUsed                      = (CodeBlockStyle & 0x20) != 0;
#endif

  fprintf(fout, "\t\t\t\t    Code-block style:\n" );
  fprintf(fout, "\t\t\t\t      No selective arithmetic coding bypass.\n" );
  fprintf(fout, "\t\t\t\t      No reset of context probabilities on coding pass boundaries.\n" );
  fprintf(fout, "\t\t\t\t      No termination on each coding pass.\n" );
  fprintf(fout, "\t\t\t\t      No verticallly causal context.\n" );
  fprintf(fout, "\t\t\t\t      No predictable termination.\n" );
  fprintf(fout, "\t\t\t\t      No segmentation symbols are used.\n" );
  fprintf(fout, "\t\t\t\t*/\n" );
  fprintf(fout, "\t\t\t\tCode_Block_Style = 16#%u#\n", CodeBlockStyle );

  fprintf(fout, "\t\t\t\t/*\n" );
  fprintf(fout, "\t\t\t\t    Wavelet transformation used:\n" );
  fprintf(fout, "\t\t\t\t      %s.\n", sTransformation );
  fprintf(fout, "\t\t\t\t*/\n" );
  fprintf(fout, "\t\t\t\tTransform = %u\n", Transformation );

  if( VariablePrecinctSize )
    {
    uint8_t N = *p++;
    uint_fast8_t i;
    fprintf(fout, "\t\t\t\t/*\n" );
    fprintf(fout, "\t\t\t\t    Precinct (width, height) by resolution level.\n" );
    fprintf(fout, "\t\t\t\t*/\n" );
    fprintf(fout, "\t\t\t\tPrecinct_Size = \n", ProgressionOrder, sProgressionOrder );
    fprintf(fout, "\t\t\t\t\t(\n" );
    for( i = 0; i < NumberOfDecompositionLevels; ++i )
      {
      uint8_t val = *p++;
      /* Table A.21 - Precinct width and height for the SPcod and SPcoc parameters */
      uint8_t width = val & 0x0f;
      uint8_t height = val >> 4;
      fprintf(fout, "\t\t\t\t\t\t(%u, %u)", 1 << width, 1 << height );
      if( i == NumberOfDecompositionLevels - 1 )
        fprintf(fout, ")\n" );
      else
        fprintf(fout, ",\n" );
      }
    }

#if 0
  fprintf(fout, "\t\tProgressionOrder = 0x%x (%s progression)\n", ProgressionOrder, sProgressionOrder );
  fprintf(fout, "\t\tNumberOfLayers = %u\n", NumberOfLayers );
  fprintf(fout, "\t\tMultipleComponentTransformation = 0x%x (%s)\n", MultipleComponentTransformation, sMultipleComponentTransformation );
  fprintf(fout, "\t\tNumberOfDecompositionLevels = %u\n", NumberOfDecompositionLevels );
  fprintf(fout, "\t\tCodeBlockWidth = 0x%x\n", CodeBlockWidth );
  fprintf(fout, "\t\tCodeBlockHeight = 0x%x\n", CodeBlockHeight );
  fprintf(fout, "\t\tCodeBlockStyle = 0x%x\n", CodeBlockStyle );

  fprintf(fout, "\t\t\t %s arithmetic coding bypass\n", (SelectiveArithmeticCodingBypass                  ? "Selective"    : "No selective") );
  fprintf(fout, "\t\t\t %s context probabilities on coding pass boundaries\n", (ResetContextProbabilitiesOnCodingPassBoundaries  ? "Reset"        : "No reset of"));
  fprintf(fout, "\t\t\t %s on each coding pass\n", (TerminationOnEachCodingPass                      ? "Termination"  : "No termination"));
  fprintf(fout, "\t\t\t %s causal context\n", (VerticallyCausalContext                          ? "Vertically"   : "No vertically"));
  fprintf(fout, "\t\t\t %s termination\n", (PredictableTermination                           ? "Predictable"  : "No predictable"));
  fprintf(fout, "\t\t\t %s symbols are used\n", (SegmentationSymbolsAreUsed                       ? "Segmentation" : "No segmentation"));
  fprintf(fout, "\t\tWaveletTransformation = 0x%x (%s)\n", Transformation, sTransformation );
  fprintf(fout, "\n" );
#endif
  assert( p == end );
}

static void printstring( const char *in, const char *ref )
{
  assert( in );
  assert( ref );
  size_t len = strlen( ref );
  fprintf(fout,"%s", in);
  if( len < 4 )
    {
    int l;
    fprintf(fout,"\"");
    fprintf(fout,"%s", ref);
    for( l = 0; l < 4 - (int)len; ++l )
      {
      fprintf(fout," ");
      }
    fprintf(fout,"\"");
    }
  else
    {
    fprintf(fout,"%s", ref);
    }
  fprintf(fout,"\n");
}

/* I.5.1 JPEG 2000 Signature box */
static void printsignature( FILE * stream )
{
  uint32_t s;
  bool b = read32(stream, &s);
  assert( b );
  fprintf(fout,"\t\tSignature = 16#%X#\n", s );
}

static const char * getbrand( uint_fast32_t br )
{
  switch( br )
    {
  case JP2:
      {
      const dictentry2 *d = getdictentry2frommarker( br );
      return d->shortname;
      }
    }
  return NULL;
}

/* I.5.2 File Type box */
static void printfiletype( FILE * stream, size_t len )
{
  char br[4+1];
  br[4] = 0;

  uint32_t minv;
  uint32_t cl;
  bool b = true;
  fread(br,4,1,stream); assert( b );
  b = read32(stream, &minv); assert( b );
  /* The number of CLi fields is determined by the length of this box. */
  int n = (len - 8 ) / 4;

  /* Table I.3 - Legal Brand values */
  const char *brand = br;
  if( brand )
    printstring("\t\tBrand = ", brand );
  fprintf(fout,"\t\tMinor_Version = %u\n", minv );
  int i;
  for (i = 0; i < n; ++i )
    {
    b = read32(stream, &cl); assert( b );
    fprintf(fout,"\t\tCompatibility = \n\t\t\t{\"%s \"}\n", brand );
    }
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

  fprintf(fout,"\t\t\tHeight = %u <rows>\n", height );
  fprintf(fout,"\t\t\tWidth = %u <columns>\n", width);
  fprintf(fout,"\t\t\tTotal_Components = %u\n", nc);
  fprintf(fout,"\t\t\t/*\n\t\t\t    Negative bits indicate signed values of abs (bits);\n");
  fprintf(fout,"\t\t\t      Zero bits indicate variable number of bits.\n");
  fprintf(fout,"\t\t\t*/\n");
  fprintf(fout,"\t\t\tValue_Bits = \n" );
  fprintf(fout,"\t\t\t\t(%u)\n", bpc + 1);
  fprintf(fout,"\t\t\tCompression_Type = %u\n", c);
  fprintf(fout,"\t\t\tColorspace_Unknown = %s\n", Unk ? "true" : "false");
  fprintf(fout,"\t\t\tIntellectual_Property_Rights = %s\n", IPR ? "true" : "false");

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
  fprintf( fout, "\t\t\tEntries = %x\n", N );
  fprintf( fout, "\t\t\t/*\n" );
  fprintf( fout, "\t\t\t\t\tChannel definition entries:\n" );
  fprintf( fout, "\t\t\t\t\t\tChannel index,\n" );
  fprintf( fout, "\t\t\t\t\t\tChannel type,\n" );
  fprintf( fout, "\t\t\t\t\t\tChannel association\n" );
  fprintf( fout, "\t\t\t*/\n" );
  fprintf( fout, "\t\t\tMap = \n" );
  fprintf( fout, "\t\t\t\t(\n" );
  for (i = 0; i < N; ++i )
    {
    b = read16(stream, &cni); assert( b );
    b = read16(stream, &typi); assert( b );
    b = read16(stream, &asoci); assert( b );
    fprintf(fout,"\t\t\t\t\t(%x, ",  cni );
    fprintf(fout,"%x, ",  typi );
    fprintf(fout,"%x)",  asoci );
    if( i != N - 1 ) fprintf(fout,"," );
    else fprintf(fout,")" );
    fprintf(fout,"\n" );
    }
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
  fprintf( fout, "\t\t\t/*\n" );
  fprintf( fout, "\t\t\t\t\tMap entries:\n" );
  fprintf( fout, "\t\t\t\t\t\tComponent index,\n" );
  fprintf( fout, "\t\t\t\t\t\tMap type,\n" );
  fprintf( fout, "\t\t\t\t\t\tPalette index\n" );
  fprintf( fout, "\t\t\t*/\n" );
  fprintf( fout, "\t\t\tMap = \n" );
  fprintf( fout, "\t\t\t\t(\n" );
  for( i = 0; i < n; ++i )
    {
    b = read16(stream, &CMPi); assert( b );
    len--;
    len--;
    b = read8(stream, &MTYPi); assert( b );
    len--;
    b = read8(stream, &PCOLi); assert( b );
    len--;
    //fprintf(fout, "  Component      #%d: %u\n", i, CMPi );
    //fprintf(fout, "  Mapping Type   #%d: %s\n", i, MTYPi ? "palette mapping" : "wazzza" );
    //fprintf(fout, "  Palette Column #%d: %u\n", i, PCOLi );

    fprintf(fout,"\t\t\t\t\t(%x, ",  CMPi );
    fprintf(fout,"%x, ",  MTYPi );
    fprintf(fout,"%x)",  PCOLi );
    if( i != n - 1 ) fprintf(fout,"," );
    else fprintf(fout,")" );
    fprintf(fout,"\n" );

    }
  assert( len == 0 );
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
 
  fprintf(fout,"\t\t\tSpecification_Method = %u\n", meth);
  fprintf(fout,"\t\t\tPrecedence = %u\n", prec);
  fprintf(fout,"\t\t\tColourspace_Approximation = %u\n", approx);
  fprintf(fout,"\t\t\tEnumerated_Colourspace = %u\n", enumCS);
}

/* I.7.1 XML boxes */
static void printxml( FILE * stream, size_t len )
{
  fprintf(fout, "\t\tText = \"" );
  for( ; len != 0; --len )
    {
    int val = fgetc(stream);
    if( val == '"' || val == '\'' ) fprintf(fout, "\\" );
    if( val == 0xd ) /* cr */
      {
      fprintf(fout, "\\r" );
      }
    else if( val == 0x9 ) /* ht */
      {
      fprintf(fout, "\\t" );
      }
    else
      {
      fprintf(fout, "%c", val );
      }
    }
  fprintf(fout, "\"\n");
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
    printstring( "\t\tGROUP = ", d->shortname );
    fprintf(fout,"\t\t\tName = %s\n", d->longname );
    fprintf(fout,"\t\t\tType = 16#%X#\n", marker );
    fprintf(fout,"\t\t\t^Position = %td <byte offset>\n", offset - 8 );
    fprintf(fout,"\t\t\tLength = %u <bytes>\n", len );
    switch( marker )
      {
    case IHDR:
      printimageheaderbox( stream, len );
      break;
    case COLR:
      printcolourspec( stream, len );
      break;
    case CDEF:
      printcdef( stream, len );
      break;
    case CMAP:
      printcmap( stream, len );
      break;
    case RES:
    case PCLR:
      fseeko( stream, len - 8, SEEK_CUR );
      break;
    default:
      assert( 0 ); /* TODO */
      }
    fprintf(fout,"\t\tEND_GROUP\n" );
    }
}

static bool print2( uint_fast32_t marker, size_t len, FILE *stream )
{
  off_t offset = ftello(stream);
  const dictentry2 *d = getdictentry2frommarker( marker );
  if( d->shortname )
    {
    printstring( "\tGROUP = ", d->shortname );
    fprintf(fout,"\t\tName = %s\n", d->longname );
    }
  else
    {
    char buffer[4+1];
    uint32_t swap = bswap_32( marker );
    memcpy( buffer, &swap, 4);
    buffer[4] = 0;
    printstring( "\tGROUP = ", buffer );
    fprintf(fout,"\t\tName = %s\n", "Unknown" );

    }
	fprintf(fout,"\t\tType = 16#%X#\n", (uint32_t)marker );
	fprintf(fout,"\t\t^Position = %td <byte offset>\n", offset - 8 );
	fprintf(fout,"\t\tLength = %zd <bytes>\n", len );
  if( !d->shortname )
    {
    fprintf(fout,"\t\t^Data_Position = %zd <byte offset>\n", offset );
    fprintf(fout,"\t\tData_Length = %zd <bytes>\n", len - 8 );
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
  case JP2H:
    printheaderbox( stream, len - 8 );
    break;
  case XML:
    printxml( stream, len - 8 );
    break;
  case JP2C:
    fprintf(fout,"\t\t^Data_Position = %zd <byte offset>\n", offset );
    fprintf(fout,"\t\tData_Length = %zd <bytes>\n", len - 8 );
    fprintf(fout,"\t\tGROUP = Codestream\n" );
  default:
    skip = true;
    }
  if( marker != JP2C )
    fprintf(fout,"\tEND_GROUP\n" );
  return skip;
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
  fprintf(fout,"\t\t\t\tTile_Index = %u\n", Isot );
  fprintf(fout,"\t\t\t\tTile_Part_Length = %u <bytes>\n", Psot );
  fprintf(fout,"\t\t\t\tTile_Part_Index = %u\n", TPsot );
  if( TNsot || printtiles )
    fprintf(fout,"\t\t\t\tTotal_Tile_Parts = %u\n", TNsot );
  else
    fprintf(fout,"\t\t\t\tTotal_Tile_Parts  unknown\n" );
}

static void printsize( FILE *stream, size_t len )
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
  uint16_t csiz;
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

  fprintf(fout,"\t\t\t\tCapability = 16#%X#\n", rsiz );
  fprintf(fout, "\t\t\t\tReference_Grid_Width = %u\n", xsiz);
  fprintf(fout, "\t\t\t\tReference_Grid_Height = %u\n", ysiz);
  fprintf(fout, "\t\t\t\tHorizontal_Image_Offset = %u\n", xosiz);
  fprintf(fout, "\t\t\t\tVertical_Image_Offset = %u\n", yosiz);
  fprintf(fout, "\t\t\t\tTile_Width = %u\n",xtsiz);
  fprintf(fout, "\t\t\t\tTile_Height = %u\n",ytsiz);
  fprintf(fout, "\t\t\t\tHorizontal_Tile_Offset = %u\n",xtosiz);
  fprintf(fout, "\t\t\t\tVertical_Tile_Offset = %u\n",ytosiz);
  fprintf(fout, "\t\t\t\tTotal_Components = %u\n",csiz);
  fprintf(fout, "\t\t\t\t/*\n");
  fprintf(fout, "\t\t\t\t    Negative bits indicate signed values of abs (bits);\n");
  fprintf(fout, "\t\t\t\t      Zero bits indicate variable number of bits.\n");
  fprintf(fout, "\t\t\t\t*/\n");
  uint_fast16_t i = 0;
  assert( csiz < 4 );
  uint8_t vb[3];
  uint8_t hss[3];
  uint8_t vss[3];
  /* read all values */
  for( i = 0; i < csiz; ++i )
    {
    uint8_t *ssiz  = vb+i;
    uint8_t *xrsiz = hss+i;
    uint8_t *yrsiz = vss+i;
    b = read8(stream, ssiz); assert( b );
    b = read8(stream, xrsiz); assert( b );
    b = read8(stream, yrsiz); assert( b );
    }
  /* dump out */
  fprintf(fout, "\t\t\t\tValue_Bits = \n");
  fprintf(fout, "\t\t\t\t	(");
  for( i = 0; i < csiz; ++i )
    {
    if( i ) fprintf(fout,", ");
    fprintf(fout,"%u", vb[i] + 1);
    }
  fprintf(fout,")\n");

  fprintf(fout, "\t\t\t\tHorizontal_Sample_Spacing = \n");
  fprintf(fout, "\t\t\t\t	(");
  for( i = 0; i < csiz; ++i )
    {
    if( i ) fprintf(fout,", ");
    fprintf(fout, "%u", hss[i]);
    }
  fprintf(fout, ")\n");
  fprintf(fout, "\t\t\t\tVertical_Sample_Spacing = \n");
  fprintf(fout, "\t\t\t\t	(");
  for( i = 0; i < csiz; ++i )
    {
    if( i ) fprintf(fout,", ");
    fprintf(fout, "%u", vss[i]);
    }
  fprintf(fout,")\n");
}

static void printcomment( FILE *stream, size_t len )
{
  assert( len >= 4 );
  len -= 4;
  uint16_t rcom;
  bool b;
  b = read16(stream, &rcom); assert( b );
  len -= 2;
  char buffer[512];
  assert( len < 512 );
  size_t l = fread(buffer,sizeof(char),len,stream);
  buffer[len] = 0;
  assert( l == len );
  fprintf(fout,"\t\t\t\tData_Type = %u\n", rcom );
  fprintf(fout,"\t\t\t\tText_Data = \"%s\"\n", buffer );
}

static bool print1( uint_fast16_t marker, size_t len, FILE *stream )
{
  off_t offset = ftello(stream);
  if( hasnolength( marker ) )
    {
    offset -= 2; /* remove size of marker itself */
    len += 2;
    }
  else
    {
    offset -= 4; /* remove size + len of marker itself */
    len += 4;
    }
  const dictentry *d = getdictentryfrommarker( marker );
  assert( offset >= 0 );

  if( d->longname )
    {
    printstring( "\t\t\tGROUP = ", d->longname );
    }
  else
    {
    char buffer[4+1];
    uint32_t swap = bswap_32( marker );
    memcpy( buffer, &swap, 4);
    buffer[4] = 0;
    printstring( "\t\t\tGROUP = ", buffer );
    }
	fprintf(fout,"\t\t\t\tMarker = 16#%X#\n", (uint16_t)marker );
	fprintf(fout,"\t\t\t\t^Position = %td <byte offset>\n", offset );
	fprintf(fout,"\t\t\t\tLength = %zd <bytes>\n", len );
  bool skip = false;
  switch( marker )
    {
  case SIZ:
    printsize( stream, len );
    break;
  case COM:
    printcomment( stream, len );
    break;
  case QCD:
    printqcd( stream, len );
    break;
  case COD:
    printcod( stream, len );
    break;
  case SOT:
    printsot( stream, len );
    break;
  case PLT:
    fprintf(fout,"\t\t\t\tIndex = %zd <bytes>\n", len );
    fprintf(fout,"\t\t\t\tPacket_Length = %zd <bytes>\n", len );
    fprintf(fout,"\t\t\t\t()\n" );
  case EOC:
  default:
    skip = true;
    }
  if( marker != SOT )
    fprintf(fout,"\t\t\tEND_GROUP\n" );

  return skip;
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

  if( argc == 4 )
    {
    printtiles = true;
    }

  uintmax_t size = getfilesize( filename );
  char * fullpath = realpath(filename, NULL);

  bool b;
  char c = 0x0a;
  bool isjp2 = isjp2file( filename );
  if( isjp2 )
    {
/* Compat with PIRL 2.3.4 */
/*    fprintf(fout,"%c",c);
    fprintf(fout,">>> WARNING <<< Incomplete JPEG2000 codestream.");
    fprintf(fout,"%c",c);
    fprintf(fout,"%c",c);
    fprintf(fout,"    End of Codestream marker not found.");
    fprintf(fout,"%c",c);
    fprintf(fout,"%c",c);
*/
    }
  fprintf(fout,"GROUP = %s", fullpath);
  free( fullpath );
  fprintf(fout,"%c",c);
  if( isjp2 )
    {
    fprintf(fout,"	/*");
    fprintf(fout,"%c",c);
    fprintf(fout,"	    Total source file length.");
    fprintf(fout,"%c",c);
    fprintf(fout,"	*/");
    fprintf(fout,"%c",c);
    fprintf(fout,"	Data_Length = %td <bytes>", size);
    print0a();

    b = parsejp2( filename, &print2, &print1 );
    fprintf(fout,"\tEND_GROUP\n");
    }
  else
    {
    b = parsej2k( filename, &print1 );
    }
  if( !b ) return 1;
  fprintf(fout,"END_GROUP");

  return 0;
}
