/*
  NullFilter noise passthrough test
*/

#include "log.h"
#include "filter.h"
#include "source\noise.h"
#include "common.h"

int test_null(Log *log)
{
  log->open_group("NullFilter noise passthrough test");

  Speakers spk;
  int seed = 2435346;
  Noise src;
  Noise ref;
  NullFilter filter(-1);

  // Rawdata test
  log->msg("Rawdata test");
  spk.set(FORMAT_PCM16, MODE_STEREO, 48000, 65536);

  src.set_seed(seed);
  ref.set_seed(seed);

  if (src.set_output(spk) &&
      ref.set_output(spk) &&
      filter.set_input(spk))
    compare(log, &src, &filter, &ref);
  else
    log->err("Init failed");

  // Linear test
  log->msg("Linear test");
  spk.set(FORMAT_LINEAR, MODE_STEREO, 48000, 1.0);

  src.set_seed(seed);
  ref.set_seed(seed);

  if (src.set_output(spk) &&
      ref.set_output(spk) &&
      filter.set_input(spk))
    compare(log, &src, &filter, &ref);
  else
    log->err("Init failed");

  return log->close_group();
}
