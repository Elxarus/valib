/*
  NullFilter noise passthrough test
*/

#include "log.h"
#include "filter.h"
#include "source\generator.h"
#include "common.h"

static const int seed = 234987;
static const int noise_size = 65536;

int test_null(Log *log)
{
  log->open_group("NullFilter noise passthrough test");

  Speakers spk;
  NoiseGen src;
  NoiseGen ref;

  NullFilter filter(-1);

  // Rawdata test
  log->msg("Rawdata test");
  spk.set(FORMAT_PCM16, MODE_STEREO, 48000);
  src.setup(spk, seed, noise_size);
  ref.setup(spk, seed, noise_size);

  if (filter.set_input(spk))
    compare(log, &src, &filter, &ref);
  else
    log->err("Init failed");

  // Linear test
  log->msg("Linear test");
  spk.set(FORMAT_LINEAR, MODE_STEREO, 48000);
  src.setup(spk, seed, noise_size);
  ref.setup(spk, seed, noise_size);

  if (filter.set_input(spk))
    compare(log, &src, &filter, &ref);
  else
    log->err("Init failed");

  return log->close_group();
}
