/*
  Basic data types, constants and utility finctions
*/

#ifndef DEFS_H
#define DEFS_H


///////////////////////////////////////////////////////////////////////////////
// Basic constants
///////////////////////////////////////////////////////////////////////////////

#define NCHANNELS          6

#define SPDIF_HEADER_SIZE  8
#define SPDIF_FRAME_SIZE   0x1800

#ifndef M_PI
  #define M_PI 3.1415926535897932384626433832795029
#endif


///////////////////////////////////////////////////////////////////////////////
// Level multipliers
///////////////////////////////////////////////////////////////////////////////

#define LEVEL_PLUS6DB 2.0
#define LEVEL_PLUS3DB 1.4142135623730951
#define LEVEL_3DB     0.7071067811865476
#define LEVEL_45DB    0.5946035575013605
#define LEVEL_6DB     0.5


///////////////////////////////////////////////////////////////////////////////
// Audio sample
///////////////////////////////////////////////////////////////////////////////

#ifndef FLOAT_SAMPLE

  typedef double   sample_t;

#else

  typedef float    sample_t;

  #if _MSC_VER >= 1200
    // warning C4244: '+=' : conversion from 'double' to 'float', possible loss of data
    // warning C4305: 'initializing' : truncation from 'const double' to 'const float'
    #pragma warning (disable: 4244 4305)
  #endif

#endif

#define SAMPLE_THRESHOLD (1e-10)
#define EQUAL_SAMPLES(s1, s2) (fabs(s1 - s2) < SAMPLE_THRESHOLD)

///////////////////////////////////////////////////////////////////////////////
// Base word types
///////////////////////////////////////////////////////////////////////////////

typedef signed char      int8_t;
typedef signed short     int16_t;
typedef signed int       int32_t;
typedef signed __int64   int64_t;
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
typedef unsigned __int64 uint64_t;

#pragma pack(push, 1)   // do not justify following structure
struct int24_t 
{
  uint16_t low; 
  int8_t   high;
  int24_t(int32_t i)
  {
    low  = uint16_t(i & 0xFFFF);
    high = int8_t(i >> 16);
  }
  int24_t(double d)
  {
    int i = int(d);
    low  = uint16_t(i & 0xFFFF);
    high = int8_t(i >> 16);
  }
  operator int32_t()
  {
    return (high << 16) + low;
  }
};
#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// Other type definitions
///////////////////////////////////////////////////////////////////////////////

typedef double time_t;
typedef sample_t matrix_t[NCHANNELS][NCHANNELS];

///////////////////////////////////////////////////////////////////////////////
// Some utilities
///////////////////////////////////////////////////////////////////////////////

#ifndef MIN
  #define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
  #define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#define value2db(value) ((value > 0)? log10(value)*20.0: 0)
#define db2value(db)    pow(10.0, db/20.0)

#define array_size(array) (sizeof(array) / sizeof(array[0]))

///////////////////////////////////////////////////////////////////////////////
// Byteorder
///////////////////////////////////////////////////////////////////////////////

#if defined(_M_IX86)

  #pragma warning(push)
  #pragma warning(disable: 4035) 
  inline uint32_t swab32(uint32_t x) 
  {
    __asm mov eax, x
    __asm bswap eax
  }
  #pragma warning(pop)

#else

  inline uint32_t swab32(uint32_t i) { return (i >> 24) | (i >> 8) & 0xff00 | (i << 8) & 0xff0000 | (i << 24); }

#endif

inline uint16_t swab16(uint16_t i) { return (i << 8) | (i >> 8); };
inline int24_t  swab24(int24_t i)  { return (int24_t)(int32_t)(swab32(i) >> 8); }

#endif