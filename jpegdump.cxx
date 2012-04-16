/*
Copyright (c) 1993-2010, David A. Clunie DBA PixelMed Publishing. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimers.

2. Redistributions in binary form must reproduce the above copyright notice, this list of
   conditions and the following disclaimers in the documentation and/or other materials
   provided with the distribution.

3. Neither the name of PixelMed Publishing nor the names of its contributors may
   be used to endorse or promote products derived from this software.

This software is provided by the copyright holders and contributors "as is" and any
express or implied warranties, including, but not limited to, the implied warranties
of merchantability and fitness for a particular purpose are disclaimed. In no event
shall the copyright owner or contributors be liable for any direct, indirect, incidental,
special, exemplary, or consequential damages (including, but not limited to, procurement
of substitute goods or services; loss of use, data or profits; or business interruption)
however caused and on any theory of liability, whether in contract, strict liability, or
tort (including negligence or otherwise) arising in any way out of the use of this software,
even if advised of the possibility of such damage.

This software has neither been tested nor approved for clinical use or for incorporation in
a medical device. It is the redistributor's or user's responsibility to comply with any
applicable local, state, national or international regulations.
*/

/*
 * Copyright (c) 2012, Mathieu Malaterre <mathieu.malaterre@gmail.com>. All rights reserved.
 */

/*
 * This code was lifted from the whole dicom3tools archive and adapted for help in implementing
 * the simple parser for JP2 file (Simple API for JPEG 2000)
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdint.h>
#include <cassert>
#include <cstring>

const uint16_t JPEG_MARKER_APP0 = 0xffe0;
const uint16_t JPEG_MARKER_APP1 = 0xffe1;
const uint16_t JPEG_MARKER_APP2 = 0xffe2;
const uint16_t JPEG_MARKER_APP3 = 0xffe3;
const uint16_t JPEG_MARKER_APP4 = 0xffe4;
const uint16_t JPEG_MARKER_APP5 = 0xffe5;
const uint16_t JPEG_MARKER_APP6 = 0xffe6;
const uint16_t JPEG_MARKER_APP7 = 0xffe7;
const uint16_t JPEG_MARKER_APP8 = 0xffe8;
const uint16_t JPEG_MARKER_APP9 = 0xffe9;
const uint16_t JPEG_MARKER_APPA = 0xffea;
const uint16_t JPEG_MARKER_APPB = 0xffeb;
const uint16_t JPEG_MARKER_APPC = 0xffec;
const uint16_t JPEG_MARKER_APPD = 0xffed;
const uint16_t JPEG_MARKER_APPE = 0xffee;
const uint16_t JPEG_MARKER_APPF = 0xffef;

const uint16_t JPEG_MARKER_COM = 0xfffe;
const uint16_t JPEG_MARKER_DAC = 0xffcc;
const uint16_t JPEG_MARKER_DHP = 0xffde;
const uint16_t JPEG_MARKER_DHT = 0xffc4;
const uint16_t JPEG_MARKER_DNL = 0xffdc;
const uint16_t JPEG_MARKER_DQT = 0xffdb;
const uint16_t JPEG_MARKER_DRI = 0xffdd;
const uint16_t JPEG_MARKER_EOI = 0xffd9;          // also JPEG 2000 "EOC"
const uint16_t JPEG_MARKER_EXP = 0xffdf;

const uint16_t JPEG_MARKER_JPG = 0xffc8;

// left out reserved JPGn and RES
// (especially those with first bit (not just byte) zero ... new LS 0 stuffing)

const uint16_t JPEG_MARKER_RST0 = 0xffd0;
const uint16_t JPEG_MARKER_RST1 = 0xffd1;
const uint16_t JPEG_MARKER_RST2 = 0xffd2;
const uint16_t JPEG_MARKER_RST3 = 0xffd3;
const uint16_t JPEG_MARKER_RST4 = 0xffd4;
const uint16_t JPEG_MARKER_RST5 = 0xffd5;
const uint16_t JPEG_MARKER_RST6 = 0xffd6;
const uint16_t JPEG_MARKER_RST7 = 0xffd7;

const uint16_t JPEG_MARKER_SOF0 = 0xffc0;
const uint16_t JPEG_MARKER_SOF1 = 0xffc1;
const uint16_t JPEG_MARKER_SOF2 = 0xffc2;
const uint16_t JPEG_MARKER_SOF3 = 0xffc3;

const uint16_t JPEG_MARKER_SOF5 = 0xffc5;
const uint16_t JPEG_MARKER_SOF6 = 0xffc6;
const uint16_t JPEG_MARKER_SOF7 = 0xffc7;

const uint16_t JPEG_MARKER_SOF9 = 0xffc9;
const uint16_t JPEG_MARKER_SOFA = 0xffca;
const uint16_t JPEG_MARKER_SOFB = 0xffcb;

const uint16_t JPEG_MARKER_SOFD = 0xffcd;
const uint16_t JPEG_MARKER_SOFE = 0xffce;
const uint16_t JPEG_MARKER_SOFF = 0xffcf;

const uint16_t JPEG_MARKER_SOI = 0xffd8;
const uint16_t JPEG_MARKER_SOS = 0xffda;
const uint16_t JPEG_MARKER_TEM = 0xff01;

// New for JPEG-LS (14495-1:1997)

const uint16_t JPEG_MARKER_SOF55 = 0xfff7;
const uint16_t JPEG_MARKER_LSE   = 0xfff8;

const unsigned char JPEG_LSE_ID_L1   = 0x01;
const unsigned char JPEG_LSE_ID_L2   = 0x02;
const unsigned char JPEG_LSE_ID_L3   = 0x03;
const unsigned char JPEG_LSE_ID_L4   = 0x04;

// New for JPEG 2000 (15444-1:2000)

const uint16_t JPEG_MARKER_SOC = 0xff4f;
const uint16_t JPEG_MARKER_SOT = 0xff90;
const uint16_t JPEG_MARKER_SOD = 0xff93;
//const uint16_t JPEG_MARKER_EOC = 0xffd9;        // same as JPEG EOI
const uint16_t JPEG_MARKER_SIZ = 0xff51;
const uint16_t JPEG_MARKER_COD = 0xff52;
const uint16_t JPEG_MARKER_COC = 0xff53;
const uint16_t JPEG_MARKER_RGN = 0xff5e;
const uint16_t JPEG_MARKER_QCD = 0xff5c;
const uint16_t JPEG_MARKER_QCC = 0xff5d;
const uint16_t JPEG_MARKER_POC = 0xff5f;
const uint16_t JPEG_MARKER_TLM = 0xff55;
const uint16_t JPEG_MARKER_PLM = 0xff57;
const uint16_t JPEG_MARKER_PLT = 0xff58;
const uint16_t JPEG_MARKER_PPM = 0xff60;
const uint16_t JPEG_MARKER_PPT = 0xff61;
const uint16_t JPEG_MARKER_SOP = 0xff91;
const uint16_t JPEG_MARKER_EPH = 0xff92;
const uint16_t JPEG_MARKER_CRG = 0xff63;
const uint16_t JPEG_MARKER_COM2K = 0xff64;

const uint16_t JPEG_MARKER_FF30 = 0xff30;
const uint16_t JPEG_MARKER_FF31 = 0xff31;
const uint16_t JPEG_MARKER_FF32 = 0xff32;
const uint16_t JPEG_MARKER_FF33 = 0xff33;
const uint16_t JPEG_MARKER_FF34 = 0xff34;
const uint16_t JPEG_MARKER_FF35 = 0xff35;
const uint16_t JPEG_MARKER_FF36 = 0xff36;
const uint16_t JPEG_MARKER_FF37 = 0xff37;
const uint16_t JPEG_MARKER_FF38 = 0xff38;
const uint16_t JPEG_MARKER_FF39 = 0xff39;
const uint16_t JPEG_MARKER_FF3A = 0xff3a;
const uint16_t JPEG_MARKER_FF3B = 0xff3b;
const uint16_t JPEG_MARKER_FF3C = 0xff3c;
const uint16_t JPEG_MARKER_FF3D = 0xff3d;
const uint16_t JPEG_MARKER_FF3E = 0xff3e;
const uint16_t JPEG_MARKER_FF3F = 0xff3f;

void writeZeroPaddedHexNumber(std::ostream &out, const unsigned long & a, const int & b)
{
  out << "0x";
  out << std::hex << std::setw( b ) << std::setfill( '0' ) << a;
  out << std::dec;
}

uint16_t isFixedLengthJPEGSegment(uint16_t marker)
{
  uint16_t length;
  switch (marker) {
  case JPEG_MARKER_EXP:
    length=3; break;
  default:
    length=0; break;
  }
  return length;
}

bool isNoLengthJPEGSegment(uint16_t marker)
{
  bool nolength;
  switch (marker) {
  case JPEG_MARKER_SOI:
  case JPEG_MARKER_EOI:
  case JPEG_MARKER_TEM:
  case JPEG_MARKER_RST0:
  case JPEG_MARKER_RST1:
  case JPEG_MARKER_RST2:
  case JPEG_MARKER_RST3:
  case JPEG_MARKER_RST4:
  case JPEG_MARKER_RST5:
  case JPEG_MARKER_RST6:
  case JPEG_MARKER_RST7:
  case JPEG_MARKER_FF30:
  case JPEG_MARKER_FF31:
  case JPEG_MARKER_FF32:
  case JPEG_MARKER_FF33:
  case JPEG_MARKER_FF34:
  case JPEG_MARKER_FF35:
  case JPEG_MARKER_FF36:
  case JPEG_MARKER_FF37:
  case JPEG_MARKER_FF38:
  case JPEG_MARKER_FF39:
  case JPEG_MARKER_FF3A:
  case JPEG_MARKER_FF3B:
  case JPEG_MARKER_FF3C:
  case JPEG_MARKER_FF3D:
  case JPEG_MARKER_FF3E:
  case JPEG_MARKER_FF3F:
  case JPEG_MARKER_SOC:
  case JPEG_MARKER_SOD:
    //case JPEG_MARKER_EOC:         // same as JPEG EOI
  case JPEG_MARKER_EPH:
    nolength=true; break;
  default:
    nolength=false; break;
  }
  return nolength;
}

bool isVariableLengthJPEGSegment(uint16_t marker)
{
  return !isNoLengthJPEGSegment(marker) && !isFixedLengthJPEGSegment(marker);
}

struct JPEGMarkerDictionaryEntry {
  uint16_t markercode;
  const char *abbreviation;
  const char *description;
} JPEGMarkerDictionaryTable[] = {
  JPEG_MARKER_APP0, "APP0", "Reserved for Application Use",
  JPEG_MARKER_APP1, "APP1", "Reserved for Application Use",
  JPEG_MARKER_APP2, "APP2", "Reserved for Application Use",
  JPEG_MARKER_APP3, "APP3", "Reserved for Application Use",
  JPEG_MARKER_APP4, "APP4", "Reserved for Application Use",
  JPEG_MARKER_APP5, "APP5", "Reserved for Application Use",
  JPEG_MARKER_APP6, "APP6", "Reserved for Application Use",
  JPEG_MARKER_APP7, "APP7", "Reserved for Application Use",
  JPEG_MARKER_APP8, "APP8", "Reserved for Application Use",
  JPEG_MARKER_APP9, "APP9", "Reserved for Application Use",
  JPEG_MARKER_APPA, "APPA", "Reserved for Application Use",
  JPEG_MARKER_APPB, "APPB", "Reserved for Application Use",
  JPEG_MARKER_APPC, "APPC", "Reserved for Application Use",
  JPEG_MARKER_APPD, "APPD", "Reserved for Application Use",
  JPEG_MARKER_APPE, "APPE", "Reserved for Application Use",
  JPEG_MARKER_APPF, "APPF", "Reserved for Application Use",

  JPEG_MARKER_COM, "COM", "Comment",
  JPEG_MARKER_DAC, "DAC", "Define Arithmetic Conditioning Table(s)",
  JPEG_MARKER_DHP, "DHP", "Define Hierarchical Progression",
  JPEG_MARKER_DHT, "DHT", "Define Huffman Table(s)",
  JPEG_MARKER_DNL, "DNL", "Define Number of Lines",
  JPEG_MARKER_DQT, "DQT", "Define Quantization Table(s)",
  JPEG_MARKER_DRI, "DRI", "Define Restart Interval",
  JPEG_MARKER_EOI, "EOI", "End of Image (JPEG 2000 EOC End of codestream)",
  JPEG_MARKER_EXP, "EXP", "Expand Reference Image(s)",

  JPEG_MARKER_JPG, "JPG", "Reserved for JPEG extensions",

  JPEG_MARKER_RST0, "RST0", "Restart with modulo 8 counter 0",
  JPEG_MARKER_RST1, "RST1", "Restart with modulo 8 counter 1",
  JPEG_MARKER_RST2, "RST2", "Restart with modulo 8 counter 2",
  JPEG_MARKER_RST3, "RST3", "Restart with modulo 8 counter 3",
  JPEG_MARKER_RST4, "RST4", "Restart with modulo 8 counter 4",
  JPEG_MARKER_RST5, "RST5", "Restart with modulo 8 counter 5",
  JPEG_MARKER_RST6, "RST6", "Restart with modulo 8 counter 6",
  JPEG_MARKER_RST7, "RST7", "Restart with modulo 8 counter 7",

  JPEG_MARKER_SOF0, "SOF0", "Huffman Baseline DCT",
  JPEG_MARKER_SOF1, "SOF1", "Huffman Extended Sequential DCT",
  JPEG_MARKER_SOF2, "SOF2", "Huffman Progressive DCT",
  JPEG_MARKER_SOF3, "SOF3", "Huffman Lossless Sequential",
  JPEG_MARKER_SOF5, "SOF5", "Huffman Differential Sequential DCT",
  JPEG_MARKER_SOF6, "SOF6", "Huffman Differential Progressive DCT",
  JPEG_MARKER_SOF7, "SOF7", "Huffman Differential Lossless",
  JPEG_MARKER_SOF9, "SOF9", "Arithmetic Extended Sequential DCT",
  JPEG_MARKER_SOFA, "SOFA", "Arithmetic Progressive DCT",
  JPEG_MARKER_SOFB, "SOFB", "Arithmetic Lossless Sequential",
  JPEG_MARKER_SOFD, "SOFD", "Arithmetic Differential Sequential DCT",
  JPEG_MARKER_SOFE, "SOFE", "Arithmetic Differential Progressive DCT",
  JPEG_MARKER_SOFF, "SOFF", "Arithmetic Differential Lossless",

  JPEG_MARKER_SOF55, "SOF55", "LS",

  JPEG_MARKER_SOI, "SOI", "Start of Image",
  JPEG_MARKER_SOS, "SOS", "Start of Scan",
  JPEG_MARKER_TEM, "TEM", "Temporary use with Arithmetic Encoding",

  JPEG_MARKER_SOC, "SOC", "Start of codestream",
  JPEG_MARKER_SOT, "SOT", "Start of tile-part",
  JPEG_MARKER_SOD, "SOD", "Start of data",
  //JPEG_MARKER_EOC, "EOC", "End of codestream",          // same as JPEG EOI
  JPEG_MARKER_SIZ, "SIZ", "Image and tile size",
  JPEG_MARKER_COD, "COD", "Coding style default",
  JPEG_MARKER_COC, "COC", "Coding style component",
  JPEG_MARKER_RGN, "RGN", "Rgeion-of-interest",
  JPEG_MARKER_QCD, "QCD", "Quantization default",
  JPEG_MARKER_QCC, "QCC", "Quantization component",
  JPEG_MARKER_POC, "POC", "Progression order change",
  JPEG_MARKER_TLM, "TLM", "Tile-part lengths",
  JPEG_MARKER_PLM, "PLM", "Packet length, main header",
  JPEG_MARKER_PLT, "PLT", "Packet length, tile-part header",
  JPEG_MARKER_PPM, "PPM", "Packet packer headers, main header",
  JPEG_MARKER_PPT, "PPT", "Packet packer headers, tile-part header",
  JPEG_MARKER_SOP, "SOP", "Start of packet",
  JPEG_MARKER_EPH, "EPH", "End of packet header",
  JPEG_MARKER_CRG, "CRG", "Component registration",
  JPEG_MARKER_COM2K, "COM", "Comment (JPEG 2000)",

  JPEG_MARKER_FF30, "FF30", "Reserved",
  JPEG_MARKER_FF31, "FF31", "Reserved",
  JPEG_MARKER_FF32, "FF32", "Reserved",
  JPEG_MARKER_FF33, "FF33", "Reserved",
  JPEG_MARKER_FF34, "FF34", "Reserved",
  JPEG_MARKER_FF35, "FF35", "Reserved",
  JPEG_MARKER_FF36, "FF36", "Reserved",
  JPEG_MARKER_FF37, "FF37", "Reserved",
  JPEG_MARKER_FF38, "FF38", "Reserved",
  JPEG_MARKER_FF39, "FF39", "Reserved",
  JPEG_MARKER_FF3A, "FF3A", "Reserved",
  JPEG_MARKER_FF3B, "FF3B", "Reserved",
  JPEG_MARKER_FF3C, "FF3C", "Reserved",
  JPEG_MARKER_FF3D, "FF3D", "Reserved",
  JPEG_MARKER_FF3E, "FF3E", "Reserved",
  JPEG_MARKER_FF3F, "FF3F", "Reserved",

  0, 0, 0
};

class JPEGMarkerDictionary {
public:
  JPEGMarkerDictionary() {}

  bool getEntry(uint16_t code,const char * &abbreviation,const char * &description) const
    {
      JPEGMarkerDictionaryEntry *ptr=JPEGMarkerDictionaryTable;
      while (ptr && ptr->abbreviation) {
        if (code == ptr->markercode) {
          abbreviation=ptr->abbreviation;
          description =ptr->description;
          return true;
        }
        ++ptr;
      }
      return false;
    }
};



class JPEG_APP0_JFIF_Parameters {
  unsigned short version;
  unsigned short units;
  unsigned short Xdensity;
  unsigned short Ydensity;
  unsigned short Xthumbnail;
  unsigned short Ythumbnail;
public:
  JPEG_APP0_JFIF_Parameters(const unsigned char *buffer,size_t length)
    {
      // identifier is 4 bytes plus a zero byte
      version=(buffer[5]<<8)+buffer[6];
      units=buffer[7];
      Xdensity=(buffer[8]<<8)+buffer[9];
      Ydensity=(buffer[10]<<8)+buffer[11];
      Xthumbnail=buffer[12];
      Ythumbnail=buffer[13];

      dump(std::cout);
    }

  void dump(std::ostream &out) const
    {
      out << std::endl << "\tJPEG_APP0_JFIF_Parameters:" << std::endl;
      out << "\t\t Version = ";
      writeZeroPaddedHexNumber(out,version,2);
      out << std::endl;
      out << "\t\t Units for the X and Y densities = " << units << std::endl;
      out << "\t\t Horizontal pixel density = " << Xdensity << std::endl;
      out << "\t\t Vertical pixel density = " << Ydensity << std::endl;
      out << "\t\t Thumbnail horizontal pixel count = " << Xthumbnail << std::endl;
      out << "\t\t Thumbnail vertical pixel count = " << Ythumbnail << std::endl;
    }
};

class JPEG_SOS_Parameters {
  unsigned  nComponentsPerScan;
  unsigned *ScanComponentSelector;
  unsigned *DCEntropyCodingTableSelector;
  unsigned *ACEntropyCodingTableSelector;
  unsigned *MappingTableSelector;      // LS
  unsigned StartOfSpectralOrPredictorSelection;
  unsigned EndOfSpectralSelection;
  unsigned SuccessiveApproximationBitPositionHigh;
  unsigned SuccessiveApproximationBitPositionLowOrPointTransform;
public:
  JPEG_SOS_Parameters(const unsigned char *buffer,size_t length)
    {
      nComponentsPerScan=buffer[0];
      assert(length == 1+nComponentsPerScan*2+3);
      ScanComponentSelector       =new unsigned[nComponentsPerScan];
      DCEntropyCodingTableSelector=new unsigned[nComponentsPerScan];
      ACEntropyCodingTableSelector=new unsigned[nComponentsPerScan];
      MappingTableSelector        =new unsigned[nComponentsPerScan];  // LS
      unsigned short i;
      for (i=0; i<nComponentsPerScan; ++i) {
        ScanComponentSelector[i]       =buffer[1+i*2];
        DCEntropyCodingTableSelector[i]=buffer[1+i*2+1] >> 4;
        ACEntropyCodingTableSelector[i]=buffer[1+i*2+1] & 0x0f;
        MappingTableSelector[i]=buffer[1+i*2+1];  // LS
      }
      StartOfSpectralOrPredictorSelection                  =buffer[1+nComponentsPerScan*2];
      EndOfSpectralSelection                               =buffer[1+nComponentsPerScan*2+1];
      SuccessiveApproximationBitPositionHigh               =buffer[1+nComponentsPerScan*2+2] >> 4;
      SuccessiveApproximationBitPositionLowOrPointTransform=buffer[1+nComponentsPerScan*2+2] & 0x0f;

      dump(std::cout);
    }

  ~JPEG_SOS_Parameters()
    {
      if (nComponentsPerScan && ScanComponentSelector)        delete[] ScanComponentSelector;
      if (nComponentsPerScan && DCEntropyCodingTableSelector) delete[] DCEntropyCodingTableSelector;
      if (nComponentsPerScan && ACEntropyCodingTableSelector) delete[] ACEntropyCodingTableSelector;
      if (nComponentsPerScan && MappingTableSelector)   delete[] MappingTableSelector;  // LS
    }

  void dump(std::ostream &out) const
    {
      out << std::endl << "\tJPEG_SOS_Parameters:" << std::endl;
      out << "\t\t nComponentsPerScan = " << nComponentsPerScan << std::endl;
      unsigned short i;
      for (i=0; i<nComponentsPerScan; ++i) {
        out << "\t\t component " << i << std::endl;
        out << "\t\t\t ScanComponentSelector = " << ScanComponentSelector[i] << std::endl;
        out << "\t\t\t DCEntropyCodingTableSelector = " << DCEntropyCodingTableSelector[i] << std::endl;
        out << "\t\t\t ACEntropyCodingTableSelector = " << ACEntropyCodingTableSelector[i] << std::endl;
        out << "\t\t\t MappingTableSelector(LS) = " << MappingTableSelector[i] << std::endl;  // LS
      }

      out << "\t\t StartOfSpectralOrPredictorSelection/NearLosslessDifferenceBound(LS) = " << StartOfSpectralOrPredictorSelection << std::endl;
      out << "\t\t EndOfSpectralSelection/InterleaveMode(LS) = " << EndOfSpectralSelection << std::endl;
      out << "\t\t SuccessiveApproximationBitPositionHigh = " << SuccessiveApproximationBitPositionHigh << std::endl;
      out << "\t\t SuccessiveApproximationBitPositionLowOrPointTransform = " << SuccessiveApproximationBitPositionLowOrPointTransform;
    }
};

class JPEG_SOF_Parameters {
  unsigned  SamplePrecision;
  unsigned  nLines;
  unsigned  nSamplesPerLine;
  unsigned  nComponentsInFrame;
  unsigned *ComponentIdentifier;
  unsigned *HorizontalSamplingFactor;
  unsigned *VerticalSamplingFactor;
  unsigned *QuantizationTableDestinationSelector;
public:
  JPEG_SOF_Parameters(const unsigned char *buffer,size_t length)
    {
      SamplePrecision    = buffer[0];
      nLines             =(buffer[1]<<8)+buffer[2];
      nSamplesPerLine    =(buffer[3]<<8)+buffer[4];
      nComponentsInFrame = buffer[5];
      assert(length == 6+nComponentsInFrame*3);
      ComponentIdentifier                 = new unsigned[nComponentsInFrame];
      HorizontalSamplingFactor            = new unsigned[nComponentsInFrame];
      VerticalSamplingFactor              = new unsigned[nComponentsInFrame];
      QuantizationTableDestinationSelector= new unsigned[nComponentsInFrame];
      unsigned short i;
      for (i=0; i<nComponentsInFrame; ++i) {
        ComponentIdentifier[i]                  = buffer[6+i*3];
        HorizontalSamplingFactor[i]             = buffer[6+i*3+1] >> 4;
        VerticalSamplingFactor[i]               = buffer[6+i*3+1] & 0x0f;
        QuantizationTableDestinationSelector[i] = buffer[6+i*3+2];
      }

      dump(std::cout);
    }

  ~JPEG_SOF_Parameters()
    {
      if (nComponentsInFrame && ComponentIdentifier)                  delete[] ComponentIdentifier;
      if (nComponentsInFrame && HorizontalSamplingFactor)             delete[] HorizontalSamplingFactor;
      if (nComponentsInFrame && VerticalSamplingFactor)               delete[] VerticalSamplingFactor;
      if (nComponentsInFrame && QuantizationTableDestinationSelector) delete[] QuantizationTableDestinationSelector;
    }

  void dump(std::ostream &out) const
    {
      out << std::endl << "\tJPEG_SOF_Parameters:" << std::endl;
      out << "\t\t SamplePrecision = "  << SamplePrecision << std::endl;
      out << "\t\t nLines = "             << nLines << std::endl;
      out << "\t\t nSamplesPerLine = "    << nSamplesPerLine << std::endl;
      out << "\t\t nComponentsInFrame = " << nComponentsInFrame << std::endl;
      unsigned short i;
      for (i=0; i<nComponentsInFrame; ++i) {
        out << "\t\t component " << i << std::endl;
        out << "\t\t\t ComponentIdentifier = "                  << ComponentIdentifier[i] << std::endl;
        out << "\t\t\t HorizontalSamplingFactor = "             << HorizontalSamplingFactor[i] << std::endl;
        out << "\t\t\t VerticalSamplingFactor = "               << VerticalSamplingFactor[i] << std::endl;
        out << "\t\t\t QuantizationTableDestinationSelector = " << QuantizationTableDestinationSelector[i] << std::endl;
      }
    }
};

class JPEG_DHT_Parameters {
  unsigned  nTables;

  unsigned   *TableClass;
  unsigned   *HuffmanTableIdentifier;
  unsigned  **nHuffmanCodesOfLengthI;
  unsigned ***ValueOfHuffmanCodeIJ;
public:
  JPEG_DHT_Parameters(const unsigned char *buffer,size_t length)
    {
      TableClass             = new unsigned   [4];
      HuffmanTableIdentifier = new unsigned   [4];
      nHuffmanCodesOfLengthI = new unsigned  *[4];
      ValueOfHuffmanCodeIJ   = new unsigned **[4];

      assert(TableClass);
      assert(HuffmanTableIdentifier);
      assert(nHuffmanCodesOfLengthI);
      assert(ValueOfHuffmanCodeIJ);

      nTables=0;
      while (length > 0) {
        assert(nTables<4);
        TableClass[nTables]             = buffer[0] >> 4;
        HuffmanTableIdentifier[nTables] = buffer[0] & 0x0f;
        nHuffmanCodesOfLengthI[nTables] = new unsigned[16];
        assert(nHuffmanCodesOfLengthI[nTables]);
        ++buffer; --length;
        unsigned i;
        for (i=0; i<16; ++i) {
          assert(length > 0);
          nHuffmanCodesOfLengthI[nTables][i] = *buffer++; --length;
        }
        ValueOfHuffmanCodeIJ[nTables] = new unsigned *[16];
        assert(nHuffmanCodesOfLengthI[nTables]);
        for (i=0; i<16; ++i) {
          ValueOfHuffmanCodeIJ[nTables][i] = new unsigned[nHuffmanCodesOfLengthI[nTables][i]];
          assert(ValueOfHuffmanCodeIJ[nTables][i]);
          unsigned j;
          for (j=0; j<nHuffmanCodesOfLengthI[nTables][i]; ++j) {
            assert(length > 0);
            ValueOfHuffmanCodeIJ[nTables][i][j] = *buffer++; --length;
          }
        }
        ++nTables;
      }
      assert(length == 0);

      dump(std::cout);
    }

  ~JPEG_DHT_Parameters()
    {
      if (TableClass)             delete[] TableClass;
      if (HuffmanTableIdentifier) delete[] HuffmanTableIdentifier;

      if (nHuffmanCodesOfLengthI) {
        unsigned t;
        for (t=0; t<nTables; ++t) {
          if (nHuffmanCodesOfLengthI[t]) delete[] nHuffmanCodesOfLengthI[t];
        }
        delete[] nHuffmanCodesOfLengthI;
      }

      if (ValueOfHuffmanCodeIJ) {
        unsigned t;
        for (t=0; t<nTables; ++t) {
          unsigned i;
          for (i=0; i<16; ++i) {
            if (ValueOfHuffmanCodeIJ[t][i]) delete[] ValueOfHuffmanCodeIJ[t][i];
          }
          if (ValueOfHuffmanCodeIJ[t]) delete[] ValueOfHuffmanCodeIJ[t];
        }
        delete[] ValueOfHuffmanCodeIJ;
      }
    }

  void dump(std::ostream &out) const
    {
      out << std::endl << "\tJPEG_DHT_Parameters:" << std::endl;
      unsigned t;
      for (t=0; t<nTables; ++t) {
        out << "\t\t TableClass = "             << TableClass[t] << std::endl;
        out << "\t\t HuffmanTableIdentifier = " << HuffmanTableIdentifier[t] << std::endl;
        unsigned i;
        for (i=0; i<16; ++i) {
          out << "\t\t\t nHuffmanCodesOfLength " << i << " = " << nHuffmanCodesOfLengthI[t][i] << std::endl;
          unsigned j;
          for (j=0; j<nHuffmanCodesOfLengthI[t][i];++j) {
            out << "\t\t\t\t ValueOfHuffmanCode " << j << " = " << ValueOfHuffmanCodeIJ[t][i][j] << std::endl;
          }
        }
      }
    }
};

class JPEG_DQT_Parameters {
  unsigned  nTables;

  unsigned   *QuantizationTableElementPrecision;
  unsigned   *QuantizationTableIdentifier;
  unsigned  **QuantizationTableElement;
public:
  JPEG_DQT_Parameters(const unsigned char *buffer,size_t length)
    {
      QuantizationTableElementPrecision = new unsigned   [4];
      QuantizationTableIdentifier       = new unsigned   [4];
      QuantizationTableElement          = new unsigned  *[4];

      assert(QuantizationTableElementPrecision);
      assert(QuantizationTableIdentifier);
      assert(QuantizationTableElement);

      nTables=0;
      while (length > 0) {
        assert(nTables<4);
        QuantizationTableElementPrecision[nTables] = buffer[0] >> 4;
        QuantizationTableIdentifier[nTables]       = buffer[0] & 0x0f;
        QuantizationTableElement[nTables]          = new unsigned[64];
        assert(QuantizationTableElement[nTables]);
        ++buffer; --length;

        unsigned i;
        for (i=0; i<64; ++i) {
          if (QuantizationTableElementPrecision[nTables]) {
            assert(length > 1);
            QuantizationTableElement[nTables][i] = (buffer[0]<<8)+buffer[1];
            buffer+=2; length-=2;
          }
          else {
            assert(length > 0);
            QuantizationTableElement[nTables][i] = *buffer++; --length;
          }
        }
        ++nTables;
      }
      assert(length == 0);

      dump(std::cout);
    }

  ~JPEG_DQT_Parameters()
    {
      if (QuantizationTableElementPrecision) delete[] QuantizationTableElementPrecision;
      if (QuantizationTableIdentifier)       delete[] QuantizationTableIdentifier;

      if (QuantizationTableElement) {
        unsigned t;
        for (t=0; t<nTables; ++t) {
          if (QuantizationTableElement[t]) delete[] QuantizationTableElement[t];
        }
        delete[] QuantizationTableElement;
      }
    }

  void dump(std::ostream &out) const
    {
      out << std::endl << "\tJPEG_DQT_Parameters:" << std::endl;
      unsigned t;
      for (t=0; t<nTables; ++t) {
        out << "\t\t QuantizationTableElementPrecision = " << QuantizationTableElementPrecision[t] << std::endl;
        out << "\t\t QuantizationTableIdentifier = "       << QuantizationTableIdentifier[t] << std::endl;
        unsigned i;
        for (i=0; i<64; ++i) {
          out << "\t\t\t QuantizationTableElement " << i << " = " << QuantizationTableElement[t][i] << std::endl;
        }
      }
    }
};

class JPEG_LSE_Parameters {
  unsigned char id;

  uint16_t Maxval;
  uint16_t T1;
  uint16_t T2;
  uint16_t T3;
  uint16_t Reset;
public:
  JPEG_LSE_Parameters(const unsigned char *buffer,size_t length)
    {
      // L1 parameter (the length has already been read

      id=*buffer++; --length;
      switch (id) {
        case JPEG_LSE_ID_L1:  // Coding parameters
          assert(length == 10);
          Maxval=(buffer[0]<<8)+buffer[1];
          T1    =(buffer[2]<<8)+buffer[3];
          T2    =(buffer[4]<<8)+buffer[5];
          T3    =(buffer[6]<<8)+buffer[7];
          Reset =(buffer[8]<<8)+buffer[9];
          break;
        case JPEG_LSE_ID_L2:  // Mapping table
          break;
        case JPEG_LSE_ID_L3:  // Mapping table continuation
          break;
        case JPEG_LSE_ID_L4:  // > 16 bit X and Y parameters
          break;
        default:
          assert(0);
          break;
      }

      dump(std::cout);
    }

  ~JPEG_LSE_Parameters()
    {
    }

  void dump(std::ostream &out) const
    {
      out << std::endl << "\tJPEG_LSE_Parameters - ID " << std::ios::dec << (unsigned)id << " ";
      switch (id) {
        case JPEG_LSE_ID_L1:
          out << "Coding parameters";
          break;
        case JPEG_LSE_ID_L2:
          out << "Mapping table";
          break;
        case JPEG_LSE_ID_L3:
          out << "Mapping table continuation";
          break;
        case JPEG_LSE_ID_L4:
          out << "> 16 bit X and Y parameters";
          break;
        default:
          assert(0);
          break;
      }
      out << ":" << std::endl;

      switch (id) {
        case JPEG_LSE_ID_L1:  // Coding parameters
          out << "\t\t Maxval = " << Maxval << std::endl;
          out << "\t\t T1 = " << T1 << std::endl;
          out << "\t\t T2 = " << T2 << std::endl;
          out << "\t\t T3 = " << T3 << std::endl;
          out << "\t\t Reset = " << Reset << std::endl;
          break;
        case JPEG_LSE_ID_L2:  // Mapping table
          break;
        case JPEG_LSE_ID_L3:  // Mapping table continuation
          break;
        case JPEG_LSE_ID_L4:  // > 16 bit X and Y parameters
          break;
        default:
          assert(0);
      }
    }
};

// ISO/IEC 15444-1:2002 Table A.16 Progression order for the SGcod, SPcoc, and Ppoc parameters
const char *getDescriptionOfProgressionOrder(unsigned char progressionOrder) {
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

// ISO/IEC 15444-1:2002 Table A.17 Multiple component transformation for the SGcod parameters
const char *getDescriptionOfMultipleComponentTransformation(unsigned char multipleComponentTransformation) {
  const char *descriptionOfMultipleComponentTransformation = "Reserved";
  switch (multipleComponentTransformation) {
    case 0x00:
      descriptionOfMultipleComponentTransformation = "None";
      break;
    case 0x01:
      descriptionOfMultipleComponentTransformation = "Part 1 Annex G2 transformation of components 0,1,2";
      break;
  }
  return descriptionOfMultipleComponentTransformation;
}

// ISO/IEC 15444-1:2002 Table A.20 - Transformation for the SPcod and SPcoc parameters
const char *getDescriptionOfWaveletTransformation(unsigned char waveletTransformation) {
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

class JPEG_COD_Parameters {
  unsigned char Scod;
  bool VariablePrecinctSize;
  bool SOPMarkerSegments;
  bool EPHPMarkerSegments;
  
  unsigned char ProgressionOrder;
  const char *descriptionOfProgressionOrder;
  
  unsigned NumberOfLayers;

  unsigned char MultipleComponentTransformation;
  const char *descriptionOfMultipleComponentTransformation;
  
  unsigned NumberOfDecompositionLevels;

  unsigned char CodeBlockWidth;
  unsigned char CodeBlockHeight;
  
  unsigned char CodeBlockStyle;
  bool SelectiveArithmeticCodingBypass;
  bool ResetContextProbabilitiesOnCodingPassBoundaries;
  bool TerminationOnEachCodingPass;
  bool VerticallyCausalContext;
  bool PredictableTermination;
  bool SegmentationSymbolsAreUsed;
  
  unsigned char WaveletTransformation;
  const char *descriptionOfWaveletTransformation;

public:
  JPEG_COD_Parameters(const unsigned char *buffer,size_t length)
    {
      Scod = buffer[0];
      VariablePrecinctSize = (Scod & 0x01) != 0;
      SOPMarkerSegments    = (Scod & 0x02) != 0;
      EPHPMarkerSegments   = (Scod & 0x04) != 0;

      ProgressionOrder = buffer[1];
      descriptionOfProgressionOrder = getDescriptionOfProgressionOrder(ProgressionOrder);
      
      NumberOfLayers  = (buffer[2]<<8)+buffer[3];

      MultipleComponentTransformation = buffer[4];
      descriptionOfMultipleComponentTransformation = getDescriptionOfMultipleComponentTransformation(MultipleComponentTransformation);
      
      NumberOfDecompositionLevels = buffer[5];
      CodeBlockWidth              = buffer[6];
      CodeBlockHeight             = buffer[7];
      
      CodeBlockStyle        = buffer[8];
      // Table A.19 - Code-block style for the SPcod and SPcoc parameters 
      SelectiveArithmeticCodingBypass                 = (Scod & 0x01) != 0;
      ResetContextProbabilitiesOnCodingPassBoundaries = (Scod & 0x02) != 0;
      TerminationOnEachCodingPass                     = (Scod & 0x04) != 0;
      VerticallyCausalContext                         = (Scod & 0x08) != 0;
      PredictableTermination                          = (Scod & 0x10) != 0;
      SegmentationSymbolsAreUsed                      = (Scod & 0x20) != 0;

      WaveletTransformation = buffer[9];
      descriptionOfWaveletTransformation = getDescriptionOfWaveletTransformation(WaveletTransformation);
      
      // if (VariablePrecinctSize) { ... } should extract precint size table

      dump(std::cout);
    }

  ~JPEG_COD_Parameters()
    {
    }

  void dump(std::ostream &out) const
    {
      out << std::endl << "\tJPEG_COD_Parameters:" << std::endl;
      out << "\t\t Scod = "; writeZeroPaddedHexNumber(out,Scod,1); out << std::endl;

      out << "\t\t\t Precinct size "          << (VariablePrecinctSize ? "defined for each resolution level" : "PPx = 15 and PPy = 15") << std::endl;
      out << "\t\t\t SOPMarkerSegments = "  << (SOPMarkerSegments    ? "may be"   : "not") << " used" << std::endl;
      out << "\t\t\t EPHPMarkerSegments = "  << (EPHPMarkerSegments   ? "shall be" : "not") << " used" << std::endl;

      out << "\t\t ProgressionOrder = "; writeZeroPaddedHexNumber(out,ProgressionOrder,1); out << " (" << descriptionOfProgressionOrder << " progression)" << std::endl;
      out << "\t\t NumberOfLayers = "      << NumberOfLayers << std::endl;

      out << "\t\t MultipleComponentTransformation = "; writeZeroPaddedHexNumber(out,MultipleComponentTransformation,1); out << " (" << descriptionOfMultipleComponentTransformation << ")" << std::endl;

      out << "\t\t NumberOfDecompositionLevels = "      << NumberOfDecompositionLevels << std::endl;

      out << "\t\t CodeBlockWidth = ";  writeZeroPaddedHexNumber(out,CodeBlockWidth,1);  out << std::endl;
      out << "\t\t CodeBlockHeight = "; writeZeroPaddedHexNumber(out,CodeBlockHeight,1); out << std::endl;

      out << "\t\t CodeBlockStyle = "; writeZeroPaddedHexNumber(out,CodeBlockStyle,1); out << std::endl;
      out << "\t\t\t " << (SelectiveArithmeticCodingBypass                  ? "Selective"    : "No selective")    << " arithmetic coding bypass" << std::endl;
      out << "\t\t\t " << (ResetContextProbabilitiesOnCodingPassBoundaries  ? "Reset"        : "No reset of")     << " context probabilities on coding pass boundaries" << std::endl;
      out << "\t\t\t " << (TerminationOnEachCodingPass                      ? "Termination"  : "No termination")  << " on each coding pass" << std::endl;
      out << "\t\t\t " << (VerticallyCausalContext                          ? "Vertically"   : "No vertically")   << " causal context" << std::endl;
      out << "\t\t\t " << (PredictableTermination                           ? "Predictable"  : "No predictable")  << " termination" << std::endl;
      out << "\t\t\t " << (SegmentationSymbolsAreUsed                       ? "Segmentation" : "No segmentation") << " symbols are used" << std::endl;

      out << "\t\t WaveletTransformation = "; writeZeroPaddedHexNumber(out,WaveletTransformation,1); out << " (" << descriptionOfWaveletTransformation << ")" << std::endl;
      
      // if (VariablePrecinctSize) { ... } should dump precint size table
    }
};

uint16_t read8(std::istream &istr)
{
  unsigned char b;
  istr.read((char *)&b,1);
  return (uint16_t)b;
}

uint16_t read16(std::istream &istr)  // big-endian
{
  uint16_t u;
  unsigned char b[2];
  istr.read((char *)b,2);
  u =  (uint16_t)b[0];
  u <<= 8;
  u |= (uint16_t)b[1];
  return u;
}

int main(int argc, char *argv[])
{
  if( argc < 2 ) return 1;
  std::ifstream cin( argv[1], std::ios::binary );
  JPEGMarkerDictionary dict;

  bool doing_jpegls=false;
  bool doing_jpeg2k_tilepart=false;

  unsigned long offset=0;
  uint16_t markerprefix=read8(cin);
  while (1) {
    if (!cin) {
      std::cout << "End of file" << std::endl;
      break;
    }
    if (markerprefix != 0xff) {    // byte of entropy-coded segment
      ++offset;
      markerprefix=read8(cin);
      continue;
    }
    uint16_t marker=read8(cin);
    if (!cin) {
      std::cout << "End of file immediately after marker flag 0xff ... presumably was padding" << std::endl;
      break;
    }
    else if (marker == 0xff) {  // 0xff byte of padding
      std::cout << "Offset ";
      writeZeroPaddedHexNumber(std::cout,offset,4);
      std::cout << " "
        << "Fill byte 0xfff"
        << std::endl;
      ++offset;
      markerprefix=marker;  // the first 0xff is padding, the 2nd may be the start of a marker
      continue;
    }
    else if (doing_jpeg2k_tilepart) {
      marker|=0xff00;
      if (marker == JPEG_MARKER_EOI /* J2K EOC */ || marker == JPEG_MARKER_SOT) {
        doing_jpeg2k_tilepart=false;
        // fall through
      }
      else if (marker == JPEG_MARKER_SOP || marker == JPEG_MARKER_EPH) {
        // fall through
      }
      else {
        ++offset;               // not a marker in tile-part
        markerprefix=marker;
        continue;
      }
    }
    else if (marker == 0) {    // 0xff byte of entropy-coded segment ... ignore following zero byte
      std::cout << "Offset ";
      writeZeroPaddedHexNumber(std::cout,offset,4);
      std::cout << " "
        << "Encoded 0xff in entropy-coded segment followed by stuffed zero byte"
        << std::endl;
      markerprefix=read8(cin);
      offset+=2;
      continue;
    }

    if (doing_jpegls && (marker & 0x80) == 0) {
      // 0xff byte of JPEG-LS entropy-coded segment ... ignore following zero BIT (not byte)
      // but don't activate this mode until sure we are doing JPEG-LS, else
      // will not pick up JPEG 2000 markers, which don't have high bit set
      std::cout << "Offset ";
      writeZeroPaddedHexNumber(std::cout,offset,4);
      std::cout << " "
        << "Encoded 0xff in entropy-coded segment followed by stuffed zero bit (JPEG-LS)"
        << std::endl;
      markerprefix=read8(cin);
      offset+=2;    // the dump doesn't need to look at the remaining 7 entropy coded segment bits
      continue;
    }

    // Definitely have a marker ...

    marker|=0xff00;      // convention is to express them with the leading ff

    if (marker == JPEG_MARKER_SOF55) doing_jpegls=true;
    else if (marker == JPEG_MARKER_SOD) doing_jpeg2k_tilepart=true;

    std::cout << "Offset ";
    writeZeroPaddedHexNumber(std::cout,offset,4);
    std::cout << " ";
    offset+=2;
    std::cout << "Marker ";
    writeZeroPaddedHexNumber(std::cout,marker,2);
    std::cout << " ";
    const char *desc;
    const char *abbrev;
    if (dict.getEntry(marker,abbrev,desc)) {
      std::cout << abbrev << " " << desc << " ";
    }
    if (isVariableLengthJPEGSegment(marker)) {
      uint16_t length=read16(cin);
      if (cin) {
        offset+=2;
        std::cout << "length variable ";
        writeZeroPaddedHexNumber(std::cout,length,2);
        std::cout << " ";
      }
      else {
        std::cout << "Error - variable length marker without length" << std::endl;
        return 1;
      }

      // NB. the length includes itself (but not the marker)

      if (length > 2) {
        unsigned char *buffer=new unsigned char[length-2];
        cin.read((char *)buffer,length-2);
        if (!cin || cin.gcount() != length-2) {
          std::cout << "Error - couldn't read variable length parameter sequence" << std::endl;
          return 1;
        }
        else {
          switch (marker) {
          case JPEG_MARKER_SOS:
            (void)new JPEG_SOS_Parameters(buffer,length-2);
            break;
          case JPEG_MARKER_SOF0:
          case JPEG_MARKER_SOF1:
          case JPEG_MARKER_SOF2:
          case JPEG_MARKER_SOF3:
          case JPEG_MARKER_SOF5:
          case JPEG_MARKER_SOF6:
          case JPEG_MARKER_SOF7:
          case JPEG_MARKER_SOF9:
          case JPEG_MARKER_SOFA:
          case JPEG_MARKER_SOFB:
          case JPEG_MARKER_SOFD:
          case JPEG_MARKER_SOFE:
          case JPEG_MARKER_SOFF:
          case JPEG_MARKER_SOF55:
            (void)new JPEG_SOF_Parameters(buffer,length-2);
            break;
          case JPEG_MARKER_DHT:
            (void)new JPEG_DHT_Parameters(buffer,length-2);
            break;
          case JPEG_MARKER_DQT:
            (void)new JPEG_DQT_Parameters(buffer,length-2);
            break;
          case JPEG_MARKER_LSE:            // LS
            (void)new JPEG_LSE_Parameters(buffer,length-2);
            break;
          case JPEG_MARKER_DRI:
            unsigned long restartinterval;
            if (length == 4) {
              restartinterval=((unsigned long)buffer[0]<<8)+buffer[1];
            }
            else if (length == 5) {
              restartinterval=((unsigned long)buffer[0]<<16)
                +((unsigned long)buffer[1]<<8)
                +buffer[2];
            }
            else if (length == 6) {
              restartinterval=((unsigned long)buffer[0]<<24)
                +((unsigned long)buffer[1]<<16)
                +((unsigned long)buffer[2]<<8)
                +buffer[3];
            }
            else {
              assert(0);
            }
            std::cout << std::endl << "\tJPEG_DRI_Parameters - Define Restart Interval = ";
            writeZeroPaddedHexNumber(std::cout,restartinterval,4);
            std::cout << std::endl;
            break;
          case JPEG_MARKER_DNL:
            unsigned long numberoflines;
            if (length == 4) {
              numberoflines=((unsigned long)buffer[0]<<8)+buffer[1];
            }
            else if (length == 5) {
              numberoflines=((unsigned long)buffer[0]<<16)
                +((unsigned long)buffer[1]<<8)
                +buffer[2];
            }
            else if (length == 6) {
              numberoflines=((unsigned long)buffer[0]<<24)
                +((unsigned long)buffer[1]<<16)
                +((unsigned long)buffer[2]<<8)
                +buffer[3];
            }
            else {
              assert(0);
            }
            std::cout << std::endl << "\tJPEG_DNL_Parameters - Define Number of Lines = ";
            writeZeroPaddedHexNumber(std::cout,numberoflines,4);
            std::cout << std::endl;
            break;
          case JPEG_MARKER_COD:
            (void)new JPEG_COD_Parameters(buffer,length-2);
            break;
          case JPEG_MARKER_APP0:
            if (length >= 16 && strncmp((char*)(buffer),"JFIF",4) == 0) {
              (void)new JPEG_APP0_JFIF_Parameters(buffer,length-2);
              break;
            }
          }
        }
      }
      else {
        std::cout << "Warning - variable length marker without \"zero\" length (really 2)";
      }
      offset+=(length-2);
    }
    else {
      uint16_t length=isFixedLengthJPEGSegment(marker);
      switch (length) {
      case 0:
        break;
      case 3: {
        unsigned char value;
        cin.read((char *)&value,1);
        if (cin) {
          offset+=1;
          std::cout << "length fixed 3 value ";
          writeZeroPaddedHexNumber(std::cout,(uint16_t)value,2);
          std::cout << " ";
        }
        else {
          std::cout << "Error - fixed length 3 marker without value" << std::endl;
          return 1;
        }
      }
        break;
      case 4: {
        uint16_t value=read16(cin);
        if (cin) {
          offset+=2;
          std::cout << "length fixed 3 value ";
          writeZeroPaddedHexNumber(std::cout,value,2);
          std::cout << " ";
        }
        else {
          std::cout << "Error - fixed length 4 marker without value" << std::endl;
          return 1;
        }
      }
        break;
      default:
        assert(0);
        break;
      }
    }
    std::cout << std::endl;
    markerprefix=read8(cin);
  }

  return 0;
}

