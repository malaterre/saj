#include <stdio.h>
#include <stdarg.h> /* va_list */
#include <assert.h>
#include <inttypes.h>
#include <byteswap.h>
#include <string.h>
#include <math.h>

#include <simpleparser.h>

static bool read8(FILE *input, uint8_t * ret)
{
  union { uint8_t v; char bytes[1]; } u;
  size_t l = fread(u.bytes,sizeof(char),1,input);
  if( l != 1 || feof(input) ) return false;
  *ret = u.v;
  return true;
}
static bool write8(FILE *output, const uint8_t ret)
{
  union { uint8_t v; char bytes[1]; } u;
  u.v = ret;
  size_t l = fwrite(u.bytes,sizeof(char),1,output);
  if( l != 1 || feof(output) ) return false;
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
static bool write16(FILE *output, const uint16_t ret)
{
  union { uint16_t v; char bytes[2]; } u;
  u.v = bswap_16(ret);
  size_t l = fwrite(u.bytes,sizeof(char),2,output);
  if( l != 2 || feof(output) ) return false;
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
static bool write32(FILE *output, const uint32_t ret)
{
  union { uint32_t v; char bytes[4]; } u;
  u.v = bswap_32(ret);
  size_t l = fwrite(u.bytes,sizeof(char),4,output);
  if( l != 4 || feof(output) ) return false;
  return true;
}

FILE *fout;

static void write_marker( FILE *out, uint_fast16_t marker, size_t len )
{
  union { uint16_t v; char bytes[2]; } u;
  u.v = bswap_16(marker);
  size_t s = fwrite(u.bytes, 1, 2, out);
  assert( s == 2 );
  // length
  const bool nolen = hasnolength( marker );
  if(!nolen)
    {
    u.v = bswap_16((uint16_t)len+2);
    s = fwrite(u.bytes, 1, 2, out);
    assert( s == 2 );
    }

}

static int extract_tile = 720;
static int current_tile = -1;

static void fixsiz( FILE *out, uint_fast16_t marker, size_t len,  FILE *stream )
{
  write_marker( out, marker, len );
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

  const uint32_t ntilesX = (xsiz + xtsiz - 1) / xtsiz;
  const uint32_t ntilesY = (ysiz + ytsiz - 1) / ytsiz;
  const uint32_t t1 = extract_tile % ntilesX;
  const uint32_t t2 = extract_tile / ntilesX;
  const tposx = t1 * xtsiz;
  assert( tposx < xsiz );
  const tposy = t2 * ytsiz;
  assert( tposy < ysiz );
  const uint32_t newxtsiz = xsiz > (tposx + xtsiz) ? xtsiz : (xsiz - tposx);
  assert( newxtsiz <= xtsiz );
  const uint32_t newytsiz = ysiz > (tposy + ytsiz) ? ytsiz : (ysiz - tposy );
  assert( newytsiz <= ytsiz );

  b = write16(out, rsiz); assert( b );
  b = write32(out, newxtsiz); assert( b ); // Tile size
  b = write32(out, newytsiz); assert( b ); // Tile size
  b = write32(out, xosiz); assert( b );
  b = write32(out, yosiz); assert( b );
  b = write32(out, -1 /*xtsiz*/); assert( b );
  b = write32(out, -1 /*ytsiz*/); assert( b );
  b = write32(out, xtosiz); assert( b );
  b = write32(out, ytosiz); assert( b );
  b = write16(out, csiz); assert( b );

  uint_fast16_t i = 0;
  for( i = 0; i < csiz; ++i )
    {
    b = read8(stream, &ssiz); assert( b ); /* int8_t ? */
    b = read8(stream, &xrsiz); assert( b );
    b = read8(stream, &yrsiz); assert( b );

    b = write8(out, ssiz);  assert( b ); /* int8_t ? */
    b = write8(out, xrsiz); assert( b );
    b = write8(out, yrsiz); assert( b );
    }
}

static void fixnsi( FILE *out, uint_fast16_t marker, size_t len,  FILE *stream )
{
  write_marker( out, marker, len );
  uint8_t ndims;
  uint32_t zsiz;
  uint32_t zosiz;
  uint32_t ztsiz;
  uint32_t ztosiz;
  uint8_t zrsiz;
  bool b;
  b = read8 (stream, &ndims); assert(b);
  b = read32(stream, &zsiz);  assert(b);
  b = read32(stream, &zosiz); assert(b);
  b = read32(stream, &ztsiz); assert(b);
  b = read32(stream, &ztosiz);assert(b);
  b = read8 (stream, &zrsiz); assert(b);

  b = write8 (out, ndims); assert(b);
  b = write32(out, ztsiz); assert(b); // Tile size
  b = write32(out, zosiz); assert(b);
  b = write32(out, ztsiz); assert(b);
  b = write32(out, ztosiz);assert(b);
  b = write8 (out, zrsiz); assert(b);
}

static void simple_copy( FILE *out, uint_fast16_t marker, size_t len,  FILE *in )
{
  // marker
  write_marker( out, marker, len );
  while( len-- )
    {
    int in_eof = fgetc(in);
    assert( in_eof != EOF );
    int out_eof = fputc(in_eof, out);
    assert( out_eof != EOF );
    }
}

static void processsot( FILE *out, uint_fast16_t marker, size_t len, FILE *in, int * curtile )
{
  uint16_t Isot;
  uint32_t Psot;
  uint8_t  TPsot;
  uint8_t  TNsot;
  bool b;
  assert( len == 8 ); // 2 + 4 + 1 + 1
  b = read16(in, &Isot); assert( b );
  b = read32(in, &Psot); assert( b );
  b = read8 (in, &TPsot); assert( b );
  b = read8 (in, &TNsot); assert( b );

  *curtile = Isot;
  if( *curtile == extract_tile )
    {
    write_marker( out, marker, len );
    b = write16(out, 0/*Isot*/); assert( b );
    b = write32(out, Psot); assert( b );
    b = write8 (out, TPsot); assert( b );
    b = write8 (out, TNsot); assert( b );
    }
}

static bool copy_tile( uint_fast16_t marker, size_t len, FILE *stream )
{
  bool skip = false;
  switch( marker )
    {
  case SIZ:
    fixsiz( fout, marker, len, stream );
    break;
  case NSI:
    fixnsi( fout, marker, len, stream );
    break;
  case SOT:
    processsot( fout, marker, len, stream, &current_tile );
    break;
  case SOD:
    if( current_tile == extract_tile )
      {
      simple_copy( fout, marker, len, stream );
      }
    else
      {
      skip = true;
      }
    break;
  default:
    simple_copy( fout, marker, len, stream );
    }
  return skip;
}

int main(int argc, char *argv[])
{
  if( argc < 3 ) return 1;
  const char *filename = argv[1];
  const char *outfilename = argv[2];

  if( argc >= 3 )
    {
    extract_tile = atoi( argv[3] );
    }

  fout = fopen( outfilename, "wb");
  if( !fout ) return 1;

  bool b = parsej2k( filename, &copy_tile );
  if( !b ) return 1;

  return 0;
}
