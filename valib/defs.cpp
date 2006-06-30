#include <stdio.h>
#include "defs.h"

static char info_str[1024];

const char *valib_build_info()
{
  static bool init = false;
  if (!init)
  {
    int ptr = 0;
    ptr += sprintf(info_str + ptr, "Compiler: "
    #if defined(_MSC_VER)
      "MSVC %i\n", _MSC_VER);
    #elif defined(__GNUC__)
      "GCC %s\n", __VERSION__);
    #else
      "Unknown%s\n",
    #endif

    ptr += sprintf(info_str + ptr, "Debug/Release: "
    #ifdef _DEBUG
      "Debug\n");
    #else
      "Release\n");
    #endif

    ptr += sprintf(info_str + ptr, "Build date: " __TIMESTAMP__ "\n");
    ptr += sprintf(info_str + ptr, "Number of channels: %i\n", NCHANNELS);
    ptr += sprintf(info_str + ptr, "Sample format: "
    #ifdef FLOAT_SAMPLE
      "float\n");
    #else
      "double\n");
    #endif
    ptr += sprintf(info_str + ptr, "Sample size: %i\n", sizeof(sample_t));
    init = true;
  }

  return info_str;
}
