/*
  AudioProcessor PCM passthrough test
  AudioProcessor should not alter input stream with default settings

  Test all possible input formats
  Test standard speaker configs
  Use 48000 sample rate
*/

#include "source/raw_source.h"
#include "source/generator.h"
#include "filters/proc.h"
#include "common.h"

static const int seed = 445676;
static const int noise_samples = 128*1024;

// PCM passthrough test
// (we cannot test FORMAT_PCMFLOAT because noise generates
// floats > 1.0)

int test_proc(Log *log)
{
  log->open_group("AudioProcessor PCM Noise passthrough test");
  static const int formats[] = 
  { 
    FORMAT_LINEAR, 
    FORMAT_PCM16,    FORMAT_PCM24,    FORMAT_PCM32,
    FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE,
  };

  static const sample_t levels[] = 
  {
    1.0,
    32768.0, 8388608.0, 2147483648.0,
    32768.0, 8388608.0, 2147483648.0
  };

  static const int modes[] = 
  { 
    MODE_1_0,     MODE_2_0,     MODE_3_0,     MODE_2_1,     MODE_3_1,     MODE_2_2,
    MODE_1_0_LFE, MODE_2_0_LFE, MODE_3_0_LFE, MODE_2_1_LFE, MODE_3_1_LFE, MODE_2_2_LFE,
  };

  int iformat = 0;
  int imode = 0;

  Speakers spk;
  NoiseGen src;
  NoiseGen ref;
  AudioProcessor proc(2048);

  for (iformat = 0; iformat < array_size(formats); iformat++)
    for (imode = 0; imode < array_size(modes); imode++)
    {
      spk.set(formats[iformat], modes[imode], 48000, levels[iformat]);
      log->msg("Testing %s %s %iHz with %iK samples", spk.format_text(), spk.mode_text(), spk.sample_rate, noise_samples / 1024);

      if (spk.format == FORMAT_LINEAR)
      {
        src.setup(spk, seed, noise_samples);
        ref.setup(spk, seed, noise_samples);
      }
      else
      {
        src.setup(spk, seed, noise_samples * spk.nch() * spk.sample_size());
        ref.setup(spk, seed, noise_samples * spk.nch() * spk.sample_size());
      }

      if (src.get_output().is_unknown() || ref.get_output().is_unknown())
      {
        log->err("Cannot init noise source");
        continue;
      }

      if (!proc.set_input(spk) || !proc.set_user(spk))
      {
        log->err("Cannot init processor");
        continue;
      }

      compare(log, &src, &proc, &ref);
    }
  return log->close_group();
}
