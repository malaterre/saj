#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <byteswap.h>
/* stat */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* Part 1  Table A.2 List of markers and marker segments */
/* Part 10 Table A.1 – List of JP3D markers and marker segments */
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
  CAP = 0xFF50, /* ? */
  SIZ = 0xFF51,
  COD = 0xFF52,
  COC = 0xFF53,
  NSI = 0xFF54, /* T.801 | ISO/IEC 15444-2 */
  TLM = 0xFF55,
  PLM = 0XFF57,
  PLT = 0XFF58,
  QCD = 0xFF5C,
  QCC = 0xFF5D,
  RGN = 0xFF5E,
  POC = 0xFF5F,
  PPM = 0XFF60,
  PPT = 0XFF61,
  CRG = 0xFF63,
  COM = 0xFF64,
  DCO = 0xFF70, /* T.801 | ISO/IEC 15444-2 */
  MCT = 0xFF74, /* T.801 | ISO/IEC 15444-2 */
  MCC = 0xFF75, /* T.801 | ISO/IEC 15444-2 */
  NLT = 0xFF76, /* T.801 | ISO/IEC 15444-2 */
  MCO = 0xFF77, /* T.801 | ISO/IEC 15444-2 */
  CBD = 0xFF78, /* T.801 | ISO/IEC 15444-2 */
  ATK = 0xFF79, /* T.801 | ISO/IEC 15444-2 */
  SOT = 0xFF90,
  SOP = 0xFF91,
  EPH = 0XFF92,
  SOD = 0xFF93,
  EOC = 0XFFD9  /* EOI in old jpeg */
} MarkerType;

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
  IPTR = 0x69707472,
  /* MJ2 */
  MDAT = 0x6d646174,
  MOOV = 0x6d6f6f76,
  /* ? */
  ASOC = 0x61736f63,
  CIDX = 0x63696478,
  FIDX = 0x66696478,
  JPCH = 0x6a706368,
  JPLH = 0x6a706c68,
  LBL  = 0x6c626c20,
  RESC = 0x72657363,
  RESD = 0x72657364,
  RREQ = 0x72726571,
  UINF = 0x75696e66,
  ULST = 0x756c7374,
  URL  = 0x75726c20,
  UUID = 0x75756964
} OtherType;


static inline bool hasnolength( uint_fast16_t marker )
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


static inline bool read16(const char ** input, size_t * len, uint16_t * ret)
{
  if( *len >= 2 )
  {
  union { uint16_t v; char bytes[2]; } u;
  memcpy(u.bytes, *input, 2);
  *ret = bswap_16(u.v);
  *input += 2; *len -= 2;
  return true;
  }
  return false;
}


static inline bool read32(const char ** input, size_t * len, uint32_t * ret)
{
  if( *len >= 4 )
  {
  union { uint32_t v; char bytes[4]; } u;
  memcpy(u.bytes, *input, 4);
  *ret = bswap_32(u.v);
  *input += 4; *len -= 4;
  return true;
  }
  return false;
}

static inline bool read64(const char ** input, size_t * len, uint64_t * ret)
{
  if( *len >= 8 )
  {
  union { uint64_t v; char bytes[8]; } u;
  memcpy(u.bytes, *input, 8);
  *ret = bswap_64(u.v);
  *input += 8; *len -= 8;
  return true;
  }
  return false;
}


static bool parsej2k_imp( const char * const stream, const size_t file_size )
{
  uint16_t marker;
  uintmax_t sotlen = 0;
  size_t lenmarker;
  //size_t cur = 0;
  const char * cur = stream;
  size_t cur_size = file_size;
  while( read16(&cur, &cur_size, &marker) )
    {
    bool b;
    assert( marker ); /* debug */
    b = hasnolength( marker );
    if ( !b )
      {
      uint16_t l;
      bool r = read16( &cur, &cur_size, &l );
      if( !r ) return false;
      assert( l >= 2 );
      lenmarker = (size_t)l - 2;

      /* special book keeping */
      if( marker == COD )
        {
        const char * p = cur;
        uint8_t Scod = *p++;
        uint8_t ProgressionOrder = *p++;
        (char)*p++;
        (char)*p++;
        //uint16_t NumberOfLayers = bswap_16( u16.v );
        uint8_t MultipleComponentTransformation = *p++;
        uint8_t NumberOfDecompositionLevels = *p++;
        uint8_t CodeBlockWidth = *p++;
        uint8_t CodeBlockHeight = *p++;
        uint8_t CodeBlockStyle = *p++;
        uint8_t Transformation = *p++;
        printf( "%d\n", Transformation );
        }
      else if( marker == SOT )
        {
        break;
        }
      }
    else
      {
      lenmarker = 0;
      }
    printf( "%x %lu\n", marker, lenmarker );
    cur += lenmarker; cur_size -= lenmarker;
    }

  return true;
}

static bool parsejp2_imp( const char * const stream, const size_t file_size )
{
  uint32_t marker;
  uint64_t len64; /* ref */
  uint32_t len32; /* local 32bits op */
  const char * cur = stream;
  size_t cur_size = file_size;

  while( read32(&cur, &cur_size, &len32) )
    {
    bool b = read32(&cur, &cur_size, &marker);
    assert( b );
    len64 = len32;
    if( len32 == 1 ) /* 64bits ? */
      {
      bool b = read64(&cur, &cur_size, &len64);
      assert( b );
      len64 -= 8;
      }
    if( marker == JP2C )
      {
      const size_t start = cur - stream; //ftello(stream);
      if( !len64 )
        {
        len64 = (size_t)(file_size - start + 8);
        }
      assert( len64 >= 8 );
#if 0
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
#else
        bool bb = parsej2k_imp( cur, cur_size );
#endif
      /*const off_t end = ftello(stream);*/
      //assert( ftello(stream) - start == (off_t)(len64 - 8) );
      /* done with JP2C move on to remaining (trailing) stuff */
      break;
      }

#if 0
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
#endif
      const size_t lenmarker = len64 - 8;
  union { uint32_t v; char bytes[4]; } u;
     u.v = marker;
     char dest[5];
     memcpy(dest, u.bytes, 4 );
     dest[4] = 0;
	printf( "%s %x %d\n", dest, marker, lenmarker );
     cur += lenmarker;
    }
#if 0
  assert( feof(stream) );
{
  int v = fclose( stream );
  assert( !v );
}
#endif

  return true;
}


int main(int argc, char * argv[])
{
  if( argc < 2 ) return 1;
  const char * filename = argv[1];
  struct stat buf;
  if (stat(filename, &buf) != 0)
	  return 1;
  size_t flen = buf.st_size;

  FILE * in = fopen( filename, "rb" );

  char * mem = malloc( flen );
  fread(mem, 1, flen, in );
  bool b = (unsigned char)*mem == 0xFF ? false : true;
  if( b ) 
  parsejp2_imp( mem, flen);
  else
  parsej2k_imp( mem, flen);
  free(mem);

  fclose(in);
  return 0;
}
