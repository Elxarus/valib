/*
  Basic data types, constants and utility finctions
*/

#ifndef DEFS_H
#define DEFS_H


///////////////////////////////////////////////////////////////////////////////
// Basic constants
///////////////////////////////////////////////////////////////////////////////

#define NCHANNELS          6

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

// todo: Add intxxbe_t and intxxle_t types and conversion functions between 
// all this types. This conversion functions should be created according to 
// current machine architecture so we have no need to take it into account 
// anymore (and we may forgot about swab functions)...

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

  int24_t() {}
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

typedef double vtime_t;
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

#if defined(_DEBUG)

  // do not use inline functions for debug version (faaaaster)
  #define swab_u32(i) uint32_t((uint32_t(i) >> 24) | (uint32_t(i) >> 8) & 0xff00 | (uint32_t(i) << 8) & 0xff0000 | (uint32_t(i) << 24))
  #define swab_s32(i) int32_t(swab_u32(i))
  #define swab_u16(i) uint16_t((i << 8) | (i >> 8))
  #define swab_s16(i) int16_t(swab_u16(i))
  inline int32_t  swab_s24(int24_t i)  { return swab_s32(i) >> 8; }

#elif defined(_M_IX86)

  // use asm inline functions
  #pragma warning(push)
  #pragma warning(disable: 4035) 
  inline uint32_t swab_u32(uint32_t x) 
  {
    __asm mov eax, x
    __asm bswap eax
  }
  inline int32_t swab_s32(int32_t x) 
  {
    __asm mov eax, x
    __asm bswap eax
  }
  inline uint16_t swab_u16(uint16_t x) 
  {
    __asm mov ax, x
    __asm bswap eax
    __asm shr eax, 16
  }
  inline int16_t swab_s16(int16_t x) 
  {
    __asm mov ax, x
    __asm bswap eax
    __asm shr eax, 16
  }
  inline int32_t swab_s24(int24_t x)
  {
    __asm mov eax, x
    __asm bswap eax
    __asm sar eax, 8
  }
/*
  inline float swab_float(float x)
  {
    __asm mov eax, x
    __asm bswap eax
  }
*/
  #pragma warning(pop)

#else

  // use general inline functions
  inline uint32_t swab_u32(uint32_t i) { return (i >> 24) | (i >> 8) & 0xff00 | (i << 8) & 0xff0000 | (i << 24); }
  inline int32_t  swab_s32(int32_t i)  { return (int32_t)swab_u32((uint32_t)i); }
  inline uint16_t swab_u16(uint16_t i) { return (i << 8) | (i >> 8); };
  inline int16_t  swab_s16(int16_t i)  { return (int16_t)swab_u16((uint16_t)i); };
  inline int32_t  swab_s24(int24_t i)  { return swab_s32(i) >> 8; }
#endif

inline float    swab_float(float f) { uint32_t i = swab_u32(*(uint32_t *)&f); return *(float *)&i; };


#endif