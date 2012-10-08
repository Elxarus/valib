#ifndef TEMP_FILENAME_H
#define TEMP_FILENAME_H

#include <string>

/*
  Temporary file name. Makes a temp file name at the temp folder and removes
  temp file on destruction.
*/

class TempFilename
{
public:
  TempFilename();
  ~TempFilename();

  std::string str() const
  { return fname; }

  const char *c_str() const
  { return fname.c_str(); }

protected:
  std::string fname;
};

#endif
