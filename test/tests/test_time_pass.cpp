/*
  Timestamps passthrough test
*/

#include "auto_file.h"
#include "filters\proc.h"


int test_proc_time_pass()
{
  printf("\n* AudioProcessor timestamp passthrough test\n");

  AudioProcessor proc(2048);
  return 0;
}
