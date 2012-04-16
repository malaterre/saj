#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <byteswap.h>

/* realpath */
#include <limits.h>
#include <stdlib.h>
/* strlen */
#include <string.h>

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
  { JP  , "jP"  , "Signature" },
  { FTYP, "ftyp", "File_Type" },
  { JP2H, "jp2h", "JP2_Header" },
  { JP2C, "jp2c", "Contiguous_Codestream" },
  { JP2 , "jp2" , "" },
  { IHDR, "ihdr", "Image_Header" },
  { COLR, "colr", "Colour_Specification" },

  { 0, 0, 0 }
};

static void print0a()
{
  const char c = 0x0a;
  printf("%c",c);
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
  { SOT, "SOT", "Start of tile-part" },
  { SOD, "SOD", "Start of data" },
  { EOC, "EOC", "End of codestream" },
  { SIZ, "SIZ", "Size" },
  { COD, "COD", "Coding_Style_Default" },
  { COC, "COC", "Coding style component" },
  { RGN, "RGN", "Rgeion-of-interest" },
  { QCD, "QCD", "Quantization_Default" },
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
  bool quant = (sqcd & 0x7f) == 0;
  printf( "\t\t\t\t/*\n");
  printf( "\t\t\t\t    Quantization:\n");
  printf( "\t\t\t\t      %s\n", quant ? "yes" : "None");
  printf( "\t\t\t\t*/\n");
  printf( "\t\t\t\tQuantization_Style = 0\n");
  printf( "\t\t\t\tTotal_Guard_Bits = 1\n");
  printf( "\t\t\t\t/*\n");
  printf( "\t\t\t\t    Reversible transform dynamic range exponent by sub-band.\n");
  printf( "\t\t\t\t*/\n");
  printf( "\t\t\t\tStep_Size = \n");
  printf( "\t\t\t\t	(" );
  size_t i;
  for( i = 0; i != len; ++i )
    {
    if( i ) printf( ", " );
    b = read8(stream, &sqcd); assert( b );
    printf("%u", sqcd);
    }
  printf(")\n");
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
  uint8_t *p = (uint8_t*)buffer;
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

#if 0
  /* Table A.13 Coding style parameter values for the Scod parameter */
  bool VariablePrecinctSize = (Scod & 0x01) != 0;
  bool SOPMarkerSegments    = (Scod & 0x02) != 0;
  bool EPHPMarkerSegments   = (Scod & 0x04) != 0;
#endif

  printf( "\t\t\t\t/*\n" );
#if 0
  printf("\t\t\t Precinct size %s\n", (VariablePrecinctSize ? "defined for each resolution level" : "PPx = 15 and PPy = 15") );
  printf("\t\t\t SOPMarkerSegments = %s used\n", (SOPMarkerSegments    ? "may be"   : "not") );
  printf("\t\t\t EPHPMarkerSegments = %s used\n", (EPHPMarkerSegments   ? "shall be" : "not") );
#endif

  printf( "\t\t\t\t    Entropy coder precincts:\n" );
  printf( "\t\t\t\t      Precinct size = %u x %u\n", 0 , 0 );
  printf( "\t\t\t\t      No SOP marker segments used\n" );
  printf( "\t\t\t\t      No EPH marker used\n" );
  printf( "\t\t\t\t*/\n" );
  printf( "\t\t\t\tCoding_Style = 16#%u#\n", Scod );

  printf( "\t\t\t\t/*\n" );
  printf( "\t\t\t\t    Progression order:\n" );
  printf( "\t\t\t\t      %s\n", sProgressionOrder );
  printf( "\t\t\t\t*/\n" );
  printf( "\t\t\t\tProgression_Order = %u\n", ProgressionOrder );
  printf( "\t\t\t\tTotal_Quality_Layers = %u\n", NumberOfLayers );
  printf( "\t\t\t\tMultiple_Component_Transform = %u\n", MultipleComponentTransformation );
  printf( "\t\t\t\tTotal_Resolution_Levels = %u\n", NumberOfDecompositionLevels + 1 );
  printf( "\t\t\t\t/*\n" );
  printf( "\t\t\t\t    Code-block width exponent offset %u.\n", CodeBlockWidth );
  printf( "\t\t\t\t*/\n" );
  printf( "\t\t\t\tCode_Block_Width = %u\n", CodeBlockWidth );
  printf( "\t\t\t\t/*\n" );
  printf( "\t\t\t\t    Code-block height exponent offset %u.\n", CodeBlockHeight );
  printf( "\t\t\t\t*/\n" );
  printf( "\t\t\t\tCode_Block_Height = %u\n", CodeBlockHeight );
  printf( "\t\t\t\t/*\n" );

#if 0
  /* Table A.19 - Code-block style for the SPcod and SPcoc parameters */
  bool SelectiveArithmeticCodingBypass                 = (CodeBlockStyle & 0x01) != 0;
  bool ResetContextProbabilitiesOnCodingPassBoundaries = (CodeBlockStyle & 0x02) != 0;
  bool TerminationOnEachCodingPass                     = (CodeBlockStyle & 0x04) != 0;
  bool VerticallyCausalContext                         = (CodeBlockStyle & 0x08) != 0;
  bool PredictableTermination                          = (CodeBlockStyle & 0x10) != 0;
  bool SegmentationSymbolsAreUsed                      = (CodeBlockStyle & 0x20) != 0;
#endif

  printf( "\t\t\t\t    Code-block style:\n" );
  printf( "\t\t\t\t      No selective arithmetic coding bypass.\n" );
  printf( "\t\t\t\t      No reset of context probabilities on coding pass boundaries.\n" );
  printf( "\t\t\t\t      No termination on each coding pass.\n" );
  printf( "\t\t\t\t      No verticallly causal context.\n" );
  printf( "\t\t\t\t      No predictable termination.\n" );
  printf( "\t\t\t\t      No segmentation symbols are used.\n" );
  printf( "\t\t\t\t*/\n" );
  printf( "\t\t\t\tCode_Block_Style = 16#%u#\n", CodeBlockStyle );

  printf( "\t\t\t\t/*\n" );
  printf( "\t\t\t\t    Wavelet transformation used:\n" );
  printf( "\t\t\t\t      %s.\n", sTransformation );
  printf( "\t\t\t\t*/\n" );
  printf( "\t\t\t\tTransform = %u\n", Transformation );

#if 0
  printf( "\t\tProgressionOrder = 0x%x (%s progression)\n", ProgressionOrder, sProgressionOrder );
  printf( "\t\tNumberOfLayers = %u\n", NumberOfLayers );
  printf( "\t\tMultipleComponentTransformation = 0x%x (%s)\n", MultipleComponentTransformation, sMultipleComponentTransformation );
  printf( "\t\tNumberOfDecompositionLevels = %u\n", NumberOfDecompositionLevels );
  printf( "\t\tCodeBlockWidth = 0x%x\n", CodeBlockWidth );
  printf( "\t\tCodeBlockHeight = 0x%x\n", CodeBlockHeight );
  printf( "\t\tCodeBlockStyle = 0x%x\n", CodeBlockStyle );

  printf( "\t\t\t %s arithmetic coding bypass\n", (SelectiveArithmeticCodingBypass                  ? "Selective"    : "No selective") );
  printf( "\t\t\t %s context probabilities on coding pass boundaries\n", (ResetContextProbabilitiesOnCodingPassBoundaries  ? "Reset"        : "No reset of"));
  printf( "\t\t\t %s on each coding pass\n", (TerminationOnEachCodingPass                      ? "Termination"  : "No termination"));
  printf( "\t\t\t %s causal context\n", (VerticallyCausalContext                          ? "Vertically"   : "No vertically"));
  printf( "\t\t\t %s termination\n", (PredictableTermination                           ? "Predictable"  : "No predictable"));
  printf( "\t\t\t %s symbols are used\n", (SegmentationSymbolsAreUsed                       ? "Segmentation" : "No segmentation"));
  printf( "\t\tWaveletTransformation = 0x%x (%s)\n", Transformation, sTransformation );
  printf( "\n" );
#endif
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

/* I.5.1 JPEG 2000 Signature box */
static void printsignature( FILE * stream )
{
  uint32_t s;
  bool b = read32(stream, &s);
  assert( b );
  printf("\t\tSignature = 16#%X#\n", s );
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
static void printfiletype( FILE * stream )
{
  uint32_t br;
  uint32_t minv;
  uint32_t cl;
  bool b = read32(stream, &br);
  assert( b );
  b = read32(stream, &minv);
  assert( b );
  b = read32(stream, &cl);
  assert( b );

  /* Table I.3 - Legal Brand values */
  const char *brand = getbrand( br );
  printstring("\t\tBrand = ", brand );
  printf("\t\tMinor_Version = %u\n", minv );
  printf("\t\tCompatibility = \n\t\t\t{\"%s \"}\n", brand );
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

  printf("\t\t\tHeight = %u <rows>\n", height );
  printf("\t\t\tWidth = %u <columns>\n", width);
  printf("\t\t\tTotal_Components = %u\n", nc);
  printf("\t\t\t/*\n\t\t\t    Negative bits indicate signed values of abs (bits);\n");
  printf("\t\t\t      Zero bits indicate variable number of bits.\n");
  printf("\t\t\t*/\n");
  printf("\t\t\tValue_Bits = %u\n", bpc + 1);
  printf("\t\t\t\t(%u)\n", bpc + 1);
  printf("\t\t\tCompression_Type = %u\n", c);
  printf("\t\t\tColorspace_Unknown = %s\n", Unk ? "true" : "false");
  printf("\t\t\tIntellectual_Property_Rights = %s\n", IPR ? "true" : "false");

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
  if( len == 7 )
    {
    b = read32(stream, &enumCS); assert( b );
    }
 
  printf("\t\t\tSpecification_Method = %u\n", meth);
  printf("\t\t\tPrecedence = %u\n", prec);
  printf("\t\t\tColourspace_Approximation = %u\n", approx);
  printf("\t\t\tEnumerated_Colourspace = %u\n", enumCS);
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
    printf("\t\t\tName = %s\n", d->longname );
    printf("\t\t\tType = 16#%X#\n", marker );
    printf("\t\t\t^Position = %td <byte offset>\n", offset - 8 );
    printf("\t\t\tLength = %u <bytes>\n", len );
    switch( marker )
      {
    case IHDR:
      printimageheaderbox( stream, len );
      break;
    case COLR:
      printcolourspec( stream, len );
      break;
    default:
      assert( 0 ); /* TODO */
      }
    printf("\t\tEND_GROUP\n" );
    }
}

static bool print2( uint_fast32_t marker, size_t len, FILE *stream )
{
  off_t offset = ftello(stream);
  const dictentry2 *d = getdictentry2frommarker( marker );
  printstring( "\tGROUP = ", d->shortname );
	printf("\t\tName = %s\n", d->longname );
	printf("\t\tType = 16#%X#\n", (uint32_t)marker );
	printf("\t\t^Position = %td <byte offset>\n", offset - 8 );
	printf("\t\tLength = %zd <bytes>\n", len );
  bool skip = false;
  switch( marker )
    {
  case JP:
    printsignature( stream );
    break;
  case FTYP:
    printfiletype( stream );
    break;
  case JP2H:
    assert( len >= 8 );
    printheaderbox( stream, len - 8 );
    break;
  case JP2C:
    printf("\t\t^Data_Position = %zd <byte offset>\n", len );
    printf("\t\tData_Length = %zd <bytes>\n", len );
    printf("\t\tGROUP = Codestream\n" );
  default:
    skip = true;
    }
  if( marker != JP2C )
    printf("\tEND_GROUP\n" );
  return skip;
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

  printf("\t\t\t\tCapability = 16#%X#\n", rsiz );
  printf( "\t\t\t\tReference_Grid_Width = %u\n", xsiz);
  printf( "\t\t\t\tReference_Grid_Height = %u\n", ysiz);
  printf( "\t\t\t\tHorizontal_Image_Offset = %u\n", xosiz);
  printf( "\t\t\t\tVertical_Image_Offset = %u\n", yosiz);
  printf( "\t\t\t\tTile_Width = %u\n",xtsiz);
  printf( "\t\t\t\tTile_Height = %u\n",ytsiz);
  printf( "\t\t\t\tHorizontal_Tile_Offset = %u\n",xtosiz);
  printf( "\t\t\t\tVertical_Tile_Offset = %u\n",ytosiz);
  printf( "\t\t\t\tTotal_Components = %u\n",csiz);
  printf( "\t\t\t\t/*\n");
  printf( "\t\t\t\t    Negative bits indicate signed values of abs (bits);\n");
  printf( "\t\t\t\t      Zero bits indicate variable number of bits.\n");
  printf( "\t\t\t\t*/\n");
  printf( "\t\t\t\tValue_Bits = \n");
  printf( "\t\t\t\t	(");
  uint_fast16_t i = 0;
  for( i = 0; i < csiz; ++i )
    {
    if( i ) printf( "," );
    b = read8(stream, &ssiz); assert( b );
    printf( "%u", ssiz);
    }
  printf(")\n");
  printf( "\t\t\t\tHorizontal_Sample_Spacing = \n");
  printf( "\t\t\t\t	(");
  for( i = 0; i < csiz; ++i )
    {
    if( i ) printf( "," );
    b = read8(stream, &xrsiz); assert( b );
    printf( "%u", xrsiz);
    }
  printf( ")\n");
  printf( "\t\t\t\tVertical_Sample_Spacing = \n");
  printf( "\t\t\t\t	(");
  for( i = 0; i < csiz; ++i )
    {
    if( i ) printf( "," );
    b = read8(stream, &yrsiz); assert( b );
    printf( "%u", yrsiz);
    }
  printf(")\n");
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
  printf("\t\t\t\tData_Type = %u\n", rcom );
  printf("\t\t\t\tText_Data = \"%s\"\n", buffer );
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

  printstring( "\t\t\tGROUP = ", d->longname );
	printf("\t\t\t\tMarker = 16#%X#\n", (uint16_t)marker );
	printf("\t\t\t\t^Position = %td <byte offset>\n", offset );
	printf("\t\t\t\tLength = %zd <bytes>\n", len );
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
  default:
    skip = true;
    }
	printf("\t\t\tEND_GROUP\n" );

  return skip;
}

int main(int argc, char *argv[])
{
  if( argc < 2 ) return 1;
  const char *filename = argv[1];

  uintmax_t size = getfilesize( filename );
  char * fullpath = realpath(filename, NULL);

  bool b;
  char c = 0x0a;
  bool isjp2 = isjp2file( filename );
  if( isjp2 )
    {
    printf("%c",c);
    printf(">>> WARNING <<< Incomplete JPEG2000 codestream.");
    printf("%c",c);
    printf("%c",c);
    printf("    End of Codestream marker not found.");
    printf("%c",c);
    printf("%c",c);
    }
  printf("GROUP = %s", fullpath);
  free( fullpath );
  printf("%c",c);
  if( isjp2 )
    {
    printf("	/*");
    printf("%c",c);
    printf("	    Total source file length.");
    printf("%c",c);
    printf("	*/");
    printf("%c",c);
    printf("	Data_Length = %td <bytes>", size);
    print0a();

    b = parsejp2( filename, &print2, &print1 );
    printf("\tEND_GROUP\n");
    }
  else
    {
    b = parsej2k( filename, &print1 );
    }
  if( !b ) return 1;
  printf("END_GROUP");

  return 0;
}
