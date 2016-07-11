#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <byteswap.h>

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
  SIZ = 0xFF51,
  COD = 0xFF52,
  COC = 0xFF53,
  TLM = 0xFF55,
  QCD = 0xFF5C,
  QCC = 0xFF5D,
  RGN = 0xFF5E,
  POC = 0xFF5F,
  CRG = 0xFF63,
  COM = 0xFF64,
  SOT = 0xFF90,
  SOP = 0xFF91,
  SOD = 0xFF93,
  EOI = 0XFFD9,
} MarkerType;

const char *getmarkerstring( MarkerType mt )
{
  switch( mt )
    {
  case FF30:
    return "Reserved";
  case SOC:
    return "SOC Start of codestream";
  case SIZ:
    return "SIZ Image and tile size";
  case COD:
    return "COD Coding style default";
  case COC:
    return "COC Coding style component";
  case TLM:
    return "TLM Tile-part lengths";
  case QCD:
    return "QCD Quantization default";
  case QCC:
    return "QCC Quantization component";
  case RGN:
    return "RGN";
  case POC:
    return "POC";
  case CRG:
    return "CRG";
  case COM:
    return "COM Comment (JPEG 2000)";
  case SOT:
    return "SOT Start of tile-part";
  case SOP:
    return "SOP";
  case SOD:
    return "SOD Start of data";
  case EOI:
    return "EOI End of Image (JPEG 2000 EOC End of codestream)";
    }

  assert( 0 );
  return 0;
}

std::istream & read16(std::istream &istr, uint16_t & ret)
{
  union { uint16_t v; char bytes[2]; } u;
  istr.read(u.bytes,2);
  std::swap(u.bytes[0], u.bytes[1]);
  ret = u.v;
  return istr;
}

void swapByteOrder(uint16_t& us)
{
    us = (us >> 8) |
         (us << 8);
}

void swapByteOrder(uint32_t& ui)
{
    ui = (ui >> 24) |
         ((ui<<8) & 0x00FF0000) |
         ((ui>>8) & 0x0000FF00) |
         (ui << 24);
}

uint32_t read32(std::istream &istr)
{
  union { uint32_t v; char bytes[4]; } u;
  istr.read(u.bytes,4);
//  std::swap(u.bytes[0], u.bytes[1]);
//  std::swap(u.bytes[2], u.bytes[3]);
  swapByteOrder(u.v);
  return u.v;
}

void print16(std::ostream &out, uint16_t v)
{
  out << "Ox";
  out << std::hex << std::setw( 4 ) << std::setfill( '0' ) << v;
}

#if 0
// Table A.27 - Quantization default parameter values
struct qcd
{
  uint16_t Lqcd; // 16 4 to 197
  uint8_t Sqcd; // 8 Table A.28
  uint16_t SPgcd; // variable
  void print( std::ostream & os )
    {
    os << "\n";
    os << "\t" << std::hex << std::setw(4) << std::setfill('0') << Lqcd << std::endl;
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)Sqcd << std::endl;
    }
  void read( std::istream & in )
    {
    Lqcd = read16( in );
    Sqcd = read16( in );
    }
};

// Table A.15 - Coding style parameter values of the SPcod and SPcoc parameters

// Table A.17 - Multiple component transformation for the SGcod parameters
static const char *MultipleCompStrings[] = {
"No multiple component transformation specified",
"Component transformation used on components 0, 1, 2 for coding efficiency (see G.2). Irreversible component transformation used with the 9-7 irreversible filter. Reversible component transformation used with the 5-3 reversible filter",
"All other values reserved"
};

const char *getmultiplecompstring( uint_fast8_t multiplecomp )
{
  if( multiplecomp > 0x03 ) multiplecomp = 0x3;
  return MultipleCompStrings[ multiplecomp ];
}

// Table A.16 - Progression order for the SGcod, SPcoc, and Ppoc parameters
static const char *ProgressionOrderStrings[] = {
"Layer-resolution level-component-position progression",
"Resolution level-layer-component-position progression",
"Resolution level-position-component-layer progression",
"Position-component-resolution level-layer progression",
"Component-position-resolution level-layer progression",
"All other values reserved"
};

const char *getprogressionorderstring( uint_fast8_t progressionOrder)
{
  if( progressionOrder > 0x05 ) progressionOrder = 0x5;
  return ProgressionOrderStrings[ progressionOrder ];
}

struct SPcod_
{
  uint8_t NumberOfDecompositionLevels;
  uint8_t CodeBlockWidth;
  uint8_t CodeBlockHeight;
  uint8_t CodeBlockStyle;
  uint8_t Transformation;
  void print( std::ostream & os )
    {
    os << "\n";
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)NumberOfDecompositionLevels << std::endl;
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)CodeBlockWidth << std::endl;
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)CodeBlockHeight << std::endl;
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)CodeBlockStyle << std::endl;
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)Transformation << std::endl;
    }
  void read( std::istream & in )
    {
    NumberOfDecompositionLevels = in.get();
    CodeBlockWidth = in.get();
    CodeBlockHeight = in.get();
    CodeBlockStyle = in.get();
    Transformation = in.get();
    }
};

// Table A.12 - Coding style default parameter values
struct cod
{
//  uint16_t COD; // 16 0xFF52
  uint16_t Lcod; // 16 12 to 45
  uint8_t Scod; // 8 Table A.13
  uint32_t SGcod; // 32 Table A.14
  SPcod_ SPcod; // variable Table A.15
  void print( std::ostream & os )
    {
    os << "\n";
    os << "\t" << std::hex << std::setw(4) << std::setfill('0') << Lcod << std::endl;
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)Scod << std::endl;
    printsgcod( os );
    SPcod.print( os );
    }
  void read( std::istream & in )
    {
    Lcod = read16( in );
    assert( Lcod >= 12 && Lcod <= 45 );
    Scod = in.get();
    //SGcod = read32( in );
    in.read( (char*)&SGcod, sizeof(SGcod) );
    // FIXME SGcod is not byte swapped ?
    SPcod.read( in );
    }
private:
  void printsgcod( std::ostream & os )
    {
    union { uint32_t v; char bytes[4]; } u;
    u.v = SGcod;
    uint_fast8_t ProgressionOrder = u.bytes[0];
    uint_fast16_t NumberOfLayers = ( u.bytes[1] << 8 ) + u.bytes[2];
    uint_fast8_t MultipleComponentTransformation = u.bytes[3];
    const char *s = getprogressionorderstring( ProgressionOrder);
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)ProgressionOrder << " " << s << " " << std::endl;
    os << "\t" << std::hex << std::setw(4) << std::setfill('0') << NumberOfLayers << std::endl;
    const char *ss = getmultiplecompstring( MultipleComponentTransformation );
    os << "\t" << std::hex << std::setw(2) << std::setfill('0') << (int)MultipleComponentTransformation << " " << ss << " " << std::endl;
    }
};

// Table A.15  Coding style parameter values of the SPcod and SPcoc parameters
struct bla
{
};

// Table A.9  Image and tile size parameter values
struct siz
{
  // SIZ 16 0xFF51
  uint16_t Lsiz; // 16 41 to 49 190
  uint16_t Rsiz; // 16 Table A.10
  uint32_t Xsiz; // 32 1 to (232  1)
  uint32_t Ysiz; // 32 1 to (232  1)
  uint32_t XOsiz; // 32 0 to (232  2)
  uint32_t YOsiz; // 32 0 to (232  2)
  uint32_t XTsiz; // 32 1 to (232  1)
  uint32_t YTsiz; // 32 1 to (232  1)
  uint32_t XTOsiz; // 32 0 to (232  2)
  uint32_t YTOsiz; // 32 0 to (232  2)
  uint16_t Csiz;   // 16 1 to 16 384
//  uint16_t Ssizi;  // 8 Table A.11
//  uint16_t XRsizi; // 8 1 to 255
//  uint16_t YRsizi; // 8 1 to 255
  uint8_t *vals;
  void print( std::ostream & os )
    {
    os << "\n";
    os << "\t" << std::hex << std::setw(4) << std::setfill('0') << Lsiz << std::endl;
    os << "\t" << std::hex << std::setw(4) << std::setfill('0') << Rsiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << Xsiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << Ysiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << XOsiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << YOsiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << XTsiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << YTsiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << XTOsiz << std::endl;
    os << "\t" << std::dec << std::setw(8) << std::setfill('0') << YTOsiz << std::endl;
    os << "\t" << std::dec << std::setw(4) << std::setfill('0') << Csiz << std::endl;
    for( uint_fast16_t i = 0; i < Csiz; ++i )
      {
      os << "\t" << std::dec << std::setw(2) << std::setfill('0') << (int)vals[ i * 3 + 0 ] << std::endl;
      os << "\t" << std::dec << std::setw(2) << std::setfill('0') << (int)vals[ i * 3 + 1 ] << std::endl;
      os << "\t" << std::dec << std::setw(2) << std::setfill('0') << (int)vals[ i * 3 + 2 ] << std::endl;
      }
    }
  siz()
    {
    vals = 0;
    }
  ~siz()
    {
    delete [] vals;
    }
  void read( std::istream & in )
    {
    Lsiz = read16( in );
    assert( Lsiz >= 41 && Lsiz <= 49190 );
    assert( (Lsiz - 38) % 3 == 0 ); // Lsiz = 38 + 3 * Csiz
    Rsiz = read16( in );
    assert( Rsiz == 0 );
    Xsiz = read32( in );
    Ysiz = read32( in );
    XOsiz = read32( in );
    YOsiz = read32( in );
    XTsiz = read32( in );
    YTsiz = read32( in );
    XTOsiz = read32( in );
    YTOsiz = read32( in );
    Csiz = read16( in );
    assert( Csiz == 1 );
    assert( vals == 0 );
    vals = new uint8_t[ Csiz ];
    for( uint_fast16_t i = 0; i < Csiz; ++i )
      {
      vals[ i * 3 + 0 ] = in.get();
      vals[ i * 3 + 1 ] = in.get();
      assert( vals[ i * 3 + 1 ] >= 1 );
      vals[ i * 3 + 2 ] = in.get();
      assert( vals[ i * 3 + 2 ] >= 1 );
      }
    }
};
#endif

bool isfixedlength( uint_fast16_t marker )
{
  switch( marker )
    {
  case SOC:
  case SOD:
  case SOT:
  case EOI:
    return true;
    }
  return false;
}

struct sot
{
  uint16_t Lsot; //16 10
  uint16_t Isot; //16 0 to 65 534
  uint32_t Psot; //32 0, or 14 to (232 - 1)
  uint8_t TPsot; //8 0 to 254
  uint8_t TNsot; //8 Table A.6
}  __attribute__((packed));

uint32_t readsot( const char *a, size_t l )
{
  sot s;
  const size_t ref = sizeof( s );
  assert( l == sizeof(s) );
  memcpy( &s, a, sizeof(s) );
  s.Lsot = bswap_16(s.Lsot);
  s.Isot = bswap_16(s.Isot);
  s.Psot = bswap_32(s.Psot);
  assert( s.Lsot == 10 );
  assert( s.TPsot < 255 );
  return s.Psot;
}


int main(int argc, char *argv[])
{
  if( argc < 2 ) return 1;
  const char *filename = argv[1];
  std::ifstream in( filename, std::ios::binary );
  if( !in ) return 1;

//  siz s;
//  cod c;
//  qcd q;
  uint16_t m;
  uint32_t sotlen = 1; // impossible value
  while( read16(in, m) )
    {
#if 0
    switch( m )
      {
    case SOC:
      // A.4.1 Start of codestream (SOC)
      std::cout << "SOC";
      break;
    case SIZ:
      // A.5.1 Image and tile size (SIZ)
      std::cout << "SIZ";
      s.read( in );
      s.print( std::cout );
      break;
    case COD:
      std::cout << "COD";
      c.read( in );
      c.print( std::cout );
      break;
    case QCD:
      std::cout << "QCD";
      q.read( in );
      q.print( std::cout );
      break;
    default:
      print16( std::cout, m );
      }
#endif
    std::cout << "Offset 0x" << std::hex << std::setw(4) << std::setfill('0') << ( (uint64_t)in.tellg() - 2 ); // tellg is 32 bits ?
    std::cout << " Marker 0x" << std::hex << std::setw(4) << std::setfill('0') << m;
    const char * ms = getmarkerstring( (MarkerType)m );
    std::cout << " " << ms;
    bool b = isfixedlength( m );
    if ( !b )
      {
      uint16_t l;
      read16( in, l );
      std::cout << " length variable 0x" << std::hex << std::setw(2) << std::setfill('0') << l;
      in.seekg( l - 2 , std::ios_base::cur );
      }
    else
      {
      if( m == SOT )
        {
        char b[10];
        in.read( b, sizeof(b) );
        sotlen = readsot( b, sizeof(b) );
        std::cout << " fixed length 0x" << std::hex << std::setw(2) << std::setfill('0') << 10;
        }
      else if( m == SOD )
        {
        std::cout << " (skipping Psot length: 0x" << std::hex << std::setw(4) << std::setfill('0') << sotlen - 12 << ")";
        in.seekg( sotlen - 14 , std::ios_base::cur );
        }
      }
      std::cout << std::endl;
    }
  assert( in.eof() );
      std::cout << "End of file" << std::endl;

  return 0;
}
