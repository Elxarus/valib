#include "log.h"
#include "test_source.h"
#include "all_filters.h"

#include "fir/param_fir.h"

int test_all_filters(Log *log);

int test_filter(Log *log, Filter *filter, const char *filter_name, 
  Speakers spk_supported1, const char *file_name1, 
  Speakers spk_supported2, const char *file_name2, 
  Speakers spk_unsupported);

int test_filter_format_change(Log *log, Filter *filter);

int test_filter_process(Log *log, Filter *filter,
  Speakers spk_supported, const char *filename, 
  size_t block_size);

int test_all_filters(Log *log)
{
  // Base filters
  Passthrough    pass;

  // Rawdata filters
  Converter      conv_ll(2048);
  Converter      conv_lr(2048);
  Converter      conv_rl(2048);

  AudioDecoder   dec_mpa;
  AudioDecoder   dec_mpa_mix;
  AudioDecoder   dec_ac3;
  AudioDecoder   dec_ac3_mix;
  AudioDecoder   dec_dts;
  AudioDecoder   dec_mix;

  Demux          demux;
  Spdifer        spdifer;
  Despdifer      despdifer;
  Detector       detector;

  // Processing filters
  AGC            agc(2048);
  Mixer          mixer_ip(2048); // inplace
  Mixer          mixer_ib(2048); // immediate
  Resample       resample_up;    // upsample
  Resample       resample_down;  // downsample
  ParamFIR       low_pass_ir(ParamFIR::low_pass, 0.5, 0.0, 0.1, 100, true);
  Convolver      convolver(&low_pass_ir);
  Delay          delay;
  BassRedir      bass_redir_ip; // inplace
  BassRedir      bass_redir_ib; // immediate
  Levels         levels;
  Dejitter       dejitter;

  // Aggregate filters
  DVDGraph       dvd;
  DVDGraph       dvd_spdif;

  AudioProcessor proc(2048);

  // Setup filters

  conv_ll.set_format(FORMAT_LINEAR);
  conv_lr.set_format(FORMAT_PCM16);
  conv_rl.set_format(FORMAT_LINEAR);
  mixer_ip.set_output(Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));
  mixer_ib.set_output(Speakers(FORMAT_LINEAR, MODE_5_1, 48000));
  resample_up.set_sample_rate(48000);
  resample_down.set_sample_rate(44100);
  bass_redir_ip.set_freq(120);
  bass_redir_ip.set_enabled(true);
  bass_redir_ib.set_freq(120);
  bass_redir_ib.set_enabled(true);
  proc.set_user(Speakers(FORMAT_PCM16, 0, 0));
  dvd_spdif.set_spdif(true, FORMAT_CLASS_SPDIFABLE, false, true, false);

  log->open_group("Test filters");

  // Base filters

  test_filter(log, &pass,   "Passthrough", 
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_RAWDATA, MODE_STEREO, 48000));

  // Rawdata filters

  test_filter(log, &conv_ll, "Converter linear->linear",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &conv_lr, "Converter linear->raw",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &conv_rl, "Converter raw->linear",
    Speakers(FORMAT_PCM16, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_PCM32, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  // Rawdata (ofdd) filters

  test_filter(log, &dec_mpa, "AudioDecoder (MPA)",
    Speakers(FORMAT_MPA, MODE_STEREO, 48000), "a.mp2.005.mp2",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.mp2.002.mp2",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &dec_mpa_mix, "AudioDecoder (MPA mix)",
    Speakers(FORMAT_MPA, 0, 48000), "a.mp2.mix.mp2",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.mp2.002.mp2",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &dec_ac3, "AudioDecoder (AC3)",
    Speakers(FORMAT_AC3, MODE_STEREO, 48000), "a.ac3.03f.ac3",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.ac3.005.ac3",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &dec_ac3_mix, "AudioDecoder (AC3 mix)",
    Speakers(FORMAT_AC3, 0, 48000), "a.ac3.mix.ac3",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.ac3.005.ac3",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &dec_dts, "AudioDecoder (DTS)",
    Speakers(FORMAT_DTS, MODE_5_1, 48000), "a.dts.03f.dts",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.dts.03f.dts",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &demux, "Demuxer",
    Speakers(FORMAT_PES, 0, 0), "a.madp.mix.pes",
    Speakers(FORMAT_PES, MODE_STEREO, 48000), "a.ac3.03f.pes",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &dec_mix, "AudioDecoder (mix)",
    Speakers(FORMAT_RAWDATA, 0, 48000), "a.mad.mix.mad",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.mad.mix.mad",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &spdifer, "Spdifer",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.mad.mix.mad",
    Speakers(FORMAT_AC3, 0, 0), "a.ac3.03f.ac3",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &despdifer, "Despdifer",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.mad.mix.spdif",
    Speakers(FORMAT_SPDIF, 0, 0), "a.ac3.03f.spdif",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  test_filter(log, &detector, "Detector",
    Speakers(FORMAT_RAWDATA, 0, 0), "a.madp.mix.madp",
    Speakers(FORMAT_SPDIF, 0, 0), "a.madp.mix.spdif",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000));

  // Linear format processing filters

  test_filter(log, &agc, "AGC",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &mixer_ip, "Mixer (inplace)",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));
    
  test_filter(log, &mixer_ib, "Mixer (immediate)",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &resample_up, "Resample (upsample)",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 44100), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 44100), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &resample_down, "Resample (downsmple)",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 48000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &convolver, "Convolver",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 48000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &delay, "Delay",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &bass_redir_ip, "BassRedir (inplace)",
    Speakers(FORMAT_LINEAR, MODE_5_1, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_STEREO, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));
    
  test_filter(log, &bass_redir_ib, "BassRedir (immediate)",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));
    
  test_filter(log, &levels, "Levels",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &dejitter, "Dejitter",
    Speakers(FORMAT_LINEAR, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  // Aggregate filters

  test_filter(log, &proc, "Processor",
    Speakers(FORMAT_PCM16, MODE_STEREO, 48000), 0,
    Speakers(FORMAT_LINEAR, MODE_5_1, 96000), 0,
    Speakers(FORMAT_AC3, MODE_STEREO, 48000));

  test_filter(log, &dvd, "DVDGraph",
    Speakers(FORMAT_PES, 0, 0), "a.madp.mix.pes",
    Speakers(FORMAT_AC3, 0, 0), "a.ac3.mix.ac3",
    spk_unknown);

  test_filter(log, &dvd_spdif, "DVDGraph (spdif)",
    Speakers(FORMAT_PES, 0, 0), "a.madp.mix.pes",
    Speakers(FORMAT_AC3, 0, 0), "a.ac3.mix.ac3",
    spk_unknown);

  return log->close_group();
}



int test_filter(Log *log, Filter *filter, const char *filter_name, 
  Speakers spk_supported, const char *filename, 
  Speakers spk_supported2, const char *filename2, 
  Speakers spk_unsupported)
{
  const size_t small_data_size = 5;
  const size_t large_data_size = 32768;

  log->open_group("Testing %s", filter_name);

  log->msg("Format change crash test");
  test_filter_format_change(log, filter);

  log->msg("Large buffer (%i)", large_data_size);
  test_filter_process(log, filter, 
    spk_supported, filename, 
    large_data_size);

  log->msg("Small buffer (%i)", small_data_size);
  test_filter_process(log, filter, 
    spk_supported, filename, 
    small_data_size);

  return log->close_group();
}


int test_filter_format_change(Log *log, Filter *filter)
{
  /////////////////////////////////////////////////////////
  // Format change crash test.
  // Try to set numerous formats. 

  static const int formats[] = 
  { 
    FORMAT_UNKNOWN, // unspecified format
    FORMAT_RAWDATA,
    FORMAT_LINEAR,
    FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32,
    FORMAT_PCM16_BE, FORMAT_PCM24_BE, FORMAT_PCM32_BE, 
    FORMAT_PCMFLOAT, FORMAT_PCMDOUBLE,
    FORMAT_PES, FORMAT_SPDIF,
    FORMAT_AC3, FORMAT_MPA, FORMAT_DTS,
    FORMAT_LPCM20, FORMAT_LPCM24
  };

  static const int modes[] = 
  {
    0, // unspecified mode
    MODE_1_0,     MODE_2_0,     MODE_3_0,     MODE_2_1,     MODE_3_1,     MODE_2_2,     MODE_3_2,
    MODE_1_0_LFE, MODE_2_0_LFE, MODE_3_0_LFE, MODE_2_1_LFE, MODE_3_1_LFE, MODE_2_2_LFE, MODE_3_2_LFE
  };

  static const int sample_rates[] = 
  {
    0, // unspecified sample rate
    192000, 96000, 48000, 24000, 12000,
    176400, 88200, 44100, 22050, 11025, 
    128000, 64000, 32000, 16000, 8000,
  };

  for (int i_format = 0; i_format < array_size(formats); i_format++)
    for (int i_mode = 0; i_mode < array_size(modes); i_mode++)
      for (int i_sample_rate = 0; i_sample_rate < array_size(sample_rates); i_sample_rate++)
        filter->open(Speakers(formats[i_format], modes[i_mode], sample_rates[i_sample_rate]));
  return 0;
}

int test_filter_process(Log *log, Filter *filter,
  Speakers spk_supported, const char *filename,
  size_t block_size)
{
  Chunk in, out;
  TestSource src;

  if (!src.open(spk_supported, filename, block_size))
    return log->err("Cannot open file %s", filename);

  if (!filter->open(spk_supported))
    return log->err("filter->open(%s %s %iHz) failed",
      spk_supported.format_text(), spk_supported.mode_text(), spk_supported.sample_rate);

  while (src.get_chunk(in))
  {
    while (filter->process(in, out))
      /*do nothing*/;
  }

  return 0;
}
