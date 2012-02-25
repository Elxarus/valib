#include <sstream>
#include "defs.h"
#include "version.h"

///////////////////////////////////////////////////////////////////////////////
// matrix_t

matrix_t &
matrix_t::operator =(const matrix_t &m)
{
  for (int i = 0; i < CH_NAMES; i++)
    for (int j = 0; j < CH_NAMES; j++)
      matrix[i][j] = m[i][j];
  return *this;
}

matrix_t &
matrix_t::zero()
{
  for (int i = 0; i < CH_NAMES; i++)
    for (int j = 0; j < CH_NAMES; j++)
      matrix[i][j] = 0;
  return *this;
}

matrix_t &
matrix_t::identity()
{
  zero();
  for (int i = 0; i < NCHANNELS; i++)
    matrix[i][i] = 1.0;
  return *this;
}

bool
matrix_t::operator ==(const matrix_t &m) const
{
  for (int i = 0; i < NCHANNELS; i++)
    for (int j = 0; j < NCHANNELS; j++)
      if (matrix[i][j] != m.matrix[i][j])
        return false;
  return true;
}

bool
matrix_t::operator !=(const matrix_t &m) const
{ return !(*this == m); }

///////////////////////////////////////////////////////////////////////////////
// Build info

const char *valib_build_info()
{
  static const char *info = 0;
  static std::string str;

  if (info) return info;

  std::stringstream stream;
  stream << "Compiler: " <<
  #if defined(_MSC_VER)
    "MSVC " << _MSC_VER << nl;
  #elif defined(__GNUC__)
    "GCC " << __VERSION__ << nl;
  #else
    "Unknown" << nl;
  #endif

  stream << "Build: " <<
  #ifdef _DEBUG
    "Debug" << nl;
  #else
    "Release" << nl;
  #endif

  #ifdef VALIB_HG_CHANGESET
    stream << "Revision: " << VALIB_HG_CHANGESET;
    if (VALIB_HG_LOCAL_MODIFICATIONS)
      stream << " (modified)";
    stream << nl;
  #endif
  stream << "Build date: " << __DATE__ " " __TIME__ << nl;
  stream << "Number of channels: " << NCHANNELS << nl;
  stream << "Sample format: " <<
  #ifdef FLOAT_SAMPLE
    "float" << nl;
  #else
    "double" << nl;
  #endif
  stream << "Sample size: " << sizeof(sample_t) << nl;

  str = stream.str();
  info = str.c_str();
  return info;
}

const char *valib_credits()
{
  return
    "libca (former libdts) - DTS decoder library\n"
    "http://developers.videolan.org/libdca.html\n"
    "\n"
    "ffmpeg - AC3 encoder\n"
    "http://ffmpeg.mplayerhq.hu\n"
    "\n"
    "FFT and Bessel code by Takuya OOURA\n"
    "http://www.kurims.kyoto-u.ac.jp/~ooura\n"
    "\n"
    "muxman - DVD authoring\n"
    "http://www.mpucoder.com/Muxman\n"
    "\n"
    "Media Player Classic - best media player\n"
    "http://sourceforge.net/projects/guliverkli\n";
}

const char *valib_revision()
{
#ifdef VALIB_HG_CHANGESET
  return VALIB_HG_CHANGESET;
#else
  return "";
#endif
}
