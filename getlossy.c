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


static bool parsej2k_imp( const char * const stream, const size_t file_size, bool * lossless )
{
  uint16_t marker;
  uintmax_t sotlen = 0;
  size_t lenmarker;
  const char * cur = stream;
  size_t cur_size = file_size;
  *lossless = false; // default init
  while( read16(&cur, &cur_size, &marker) )
    {
    if ( !hasnolength( marker ) )
      {
      uint16_t l;
      bool r = read16( &cur, &cur_size, &l );
      if( !r || l < 2 )
{
        printf( "  break %lu %d %d\n", cur_size, r, l);
        break;
}
      lenmarker = (size_t)l - 2;

      if( marker == COD )
        {
        const uint8_t Transformation = *(cur+9);
        printf( "%d\n", Transformation );
        if( Transformation == 0x0 ) { *lossless = false; return true; }
        else if( Transformation == 0x1 ) *lossless = true;
        else return false;
        }
      else if( marker == SOT )
        {
        }
      printf( "  %d %ud\n", lenmarker, cur_size );
      cur += lenmarker; cur_size -= lenmarker;
      }
      else if( marker == SOD )
        return true;
    }

  return false;
}

static bool parsejp2_imp( const char * const stream, const size_t file_size, bool * lossless )
{
  uint32_t marker;
  uint64_t len64; /* ref */
  uint32_t len32; /* local 32bits op */
  const char * cur = stream;
  size_t cur_size = file_size;

  while( read32(&cur, &cur_size, &len32) )
    {
    bool b = read32(&cur, &cur_size, &marker);
    if( !b ) break;
    len64 = len32;
    if( len32 == 1 ) /* 64bits ? */
      {
      bool b = read64(&cur, &cur_size, &len64);
      assert( b );
      len64 -= 8;
      }
    if( marker == JP2C )
      {
      const size_t start = cur - stream;
      if( !len64 )
        {
        len64 = (size_t)(file_size - start + 8);
        }
      assert( len64 >= 8 );
      printf( "debug %d\n", len64 - 8 );
      return parsej2k_imp( cur, len64 - 8 /*cur_size*/, lossless );
      }

      const size_t lenmarker = len64 - 8;
#if 1
      union { uint32_t v; char bytes[4]; } u;
      u.v = marker;
      char dest[5];
      memcpy(dest, u.bytes, 4 );
      dest[4] = 0;
      printf( "%s %x %d\n", dest, marker, lenmarker );
#endif
      cur += lenmarker;
    }

  return false;
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
  bool lossless;
  if( b ) 
  b = parsejp2_imp( mem, flen, &lossless);
  else
  b = parsej2k_imp( mem, flen, &lossless);
  if(!b) printf("failed to parse\n");
  printf("lossless: %d\n", lossless);
  free(mem);

  fclose(in);
  return 0;
}
