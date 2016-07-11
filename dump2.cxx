#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdint.h>
#include <byteswap.h>

uint32_t read32(std::istream &istr)
{
  union { uint32_t v; char bytes[4]; } u;
  istr.read(u.bytes,4);
  return bswap_32(u.v);
}

struct box
{
  uint32_t len;
  uint32_t type;
};

int main(int argc, char *argv[])
{
  if( argc < 2 ) return 1;

  const char *filename = argv[1];
  std::ifstream in( filename, std::ios::binary);

  uint32_t v1 = read32( in );
  uint32_t v2 = read32( in );
  uint32_t v3 = read32( in );
  std::cout << "\t" << std::hex << std::setw(4) << std::setfill('0') << v1 << std::endl;
  std::cout << "\t" << std::hex << std::setw(4) << std::setfill('0') << v2 << std::endl;
  std::cout << "\t" << std::hex << std::setw(4) << std::setfill('0') << v3 << std::endl;

  uint32_t v4 = read32( in );
  uint32_t v5 = read32( in );
  uint32_t v6 = read32( in );
  std::cout << "\t" << std::hex << std::setw(4) << std::setfill('0') << v4 << std::endl;
  std::cout << "\t" << std::hex << std::setw(4) << std::setfill('0') << v5 << std::endl;
  std::cout << "\t" << std::hex << std::setw(4) << std::setfill('0') << v6 << std::endl;
  
  return 0;
}
