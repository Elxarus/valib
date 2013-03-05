#include "stdio.h"
#include <boost/filesystem.hpp>
#include "temp_filename.h"

TempFilename::TempFilename()
{
  boost::filesystem::path temp_dir = boost::filesystem::temp_directory_path();
  boost::filesystem::path full_path;
  do {
    full_path = temp_dir / boost::filesystem::unique_path();
  } while (boost::filesystem::exists(full_path));
  fname = full_path.string();
}

TempFilename::~TempFilename()
{
  remove(fname.c_str());
}
