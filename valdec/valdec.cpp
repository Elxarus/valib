#include <stdio.h>
#include <conio.h>
#include <math.h>
    
#include "parsers\ac3\ac3_parser.h"
#include "parsers\dts\dts_parser.h"
#include "parsers\mpa\mpa_parser.h"
#include "parsers\file_parser.h"

#include "filters\proc.h"
#include "filters\spdifer.h"

#include "sink.h"
#include "sink\sink_raw.h"
#include "sink\sink_dsound.h"

#include "win32\cpu.h"


#define bool2str(v) ((v)? "true": "false")
#define bool2str1(v) ((v)? "+": "-")


const char *_ch_names[NCHANNELS] = { "Left", "Center", "Right", "Left surround", "Right surround", "LFE" };


const int mask_tbl[7] =
{
  0,
  MODE_MONO,
  MODE_STEREO,
  MODE_3_0,
  MODE_2_2,
  MODE_3_2,
  MODE_5_1
};

const int format_tbl[4] = 
{
  FORMAT_PCM16,
  FORMAT_PCM24,
  FORMAT_PCM32,
  FORMAT_PCMFLOAT,
};

enum arg_type { argt_exist, argt_bool, argt_num };

bool is_arg(char *arg, const char *name, arg_type type)
{
  if (arg[0] != '-') return false;
  arg++;

  while (*name)
    if (*name && *arg != *name) 
      return false;
    else
      name++, arg++;

  if (type == argt_exist && *arg == '\0') return true;
  if (type == argt_bool && (*arg == '\0' || *arg == '+' || *arg == '-')) return true;
  if (type == argt_num && (*arg == ':' || *arg == '=')) return true;

  return false;
}

bool arg_bool(char *arg)
{
  arg += strlen(arg) - 1;
  if (*arg == '-') return false;
  return true;
}

double arg_num(char *arg)
{
  arg += strlen(arg);
  while (*arg != ':' && *arg != '=')
    arg--;
  arg++;
  return atof(arg);
}


int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("VALib DECoder (valdec)\n"
           "Copyright (c) 2004 by Alexander Vigovsky\n"
           "\n"
           "Advanced audio decoder/processor/player utility\n"
           "This utility is part of AC3Filter project (VALib subproject)\n"
           "http://ac3filter.sourceforge.net\n"
           "\n"
           "Usage:\n"
           "  valdec some_file [options]\n"
           "\n"
           "Options:\n"
           "  (*)  - default value\n"
           "  {ch} - channel name (l, c, r, sl, sr)\n"
           "\n"
           "  output mode:\n"
           "    -d[ecode] - just decode (used for testing and performance measurements)\n"
           "    -p[lay]   - play file (*)\n"
           "    -r[aw]    - decode to RAW file\n"
           "  \n"
           "  output options:\n"
           "    -spdif - spdif output (no other options will work in this mode)\n"
           "    -spk:n - set number of output channels:\n"
           "          0 - from file     4 - 2/2 (quadro)\n" 
           "          1 - 1/0 (mono)    5 - 3/2 (5 ch)\n"
           "      (*) 2 - 2/0 (stereo)  6 - 3/2+SW (5.1)\n"
           "          3 - 3/0 (surround)\n"
           "    -fmt:n - set sample format:\n"
           "      (*) 0 - PCM 16        2 - PCM 32\n"
           "          1 - PCM 24        3 - PCM Float \n" 
           "  \n"
           "  format selection:\n"
           "    -ac3 - force ac3 (do not autodetect format)\n"
           "    -dts - force dts (do not autodetect format)\n"
           "    -mpa - force mpa (do not autodetect format)\n"
           "  \n"
           "  info:\n"
           "    -i    - print bitstream info\n"
//             "  -opt  - print decoding options\n"
           "    -hist - print levels histogram\n"
           "  \n"
           "  mixer options:\n"
           "    -auto_matrix[+|-] - automatic matrix calculation on(*)/off\n"
           "    -normalize_matrix[+|-] - normalize matrix on(*)/off\n"
           "    -voice_control[+|-] - voice control on(*)/off\n"
           "    -expand_stereo[+|-] - expand stereo on(*)/off\n"
           "    -clev:N - center mix level (dB)\n"
           "    -slev:N - surround mix level (dB)\n"
           "    -lfelev:N - lfe mix level (dB)\n"
           "    -gain:N - master gain (dB)\n"
           "    -gain_{ch}:N - output channel gain (dB)\n"
           "  \n"
           "  automatic gain control options:\n"
           "    -limiter[+|-] - limiter on(*)/off\n"
           "    -normalize[+|-] - one-pass normalize on/off(*)\n"
           "    -drc[+|-] - dynamic range compression on/off(*)\n"
           "    -drc_power:N - dynamic range compression level (dB)\n"
           "    -release:N - release speed (dB/s)\n"
           "  \n"
           "  delay options:\n"
           "    -delay_units:n - units in wich delay values are given\n"
           "          0 - samples (*) 2 - meters      4 - feet   \n"
           "          1 - ms          3 - cm          5 - inches \n"
           "    -delay_{ch}:N - delay for channel {ch} (in samples by default)\n" 
           );
    return 1;
  }

  const char *input_filename = argv[1];
  enum { mode_nothing, mode_play, mode_raw, mode_decode } mode = mode_nothing;

  bool spdif = false;
  int mask   = MODE_STEREO;
  int format = FORMAT_PCM16;

  bool print_info = false;
  bool print_opt  = false;
  bool print_hist = false;

  int delay_units = DELAY_SP;
  float delays[NCHANNELS];
  memset(delays, 0, sizeof(delays));

  sample_t gains[NCHANNELS];
  gains[0] = 1.0;
  gains[1] = 1.0;
  gains[2] = 1.0;
  gains[3] = 1.0;
  gains[4] = 1.0;
  gains[5] = 1.0;

  /////////////////////////////////////////////////////////
  // Parsers
  /////////////////////////////////////////////////////////

  Parser *parser = 0;

  AC3Parser ac3;
  DTSParser dts;
  MPAParser mpa;

  FileParser file;

  /////////////////////////////////////////////////////////
  // Sinks
  /////////////////////////////////////////////////////////

  RAWRenderer    raw;
  DSRenderer     dsound(0);
  NullRenderer   null;
  AudioRenderer *sink = &dsound;

  /////////////////////////////////////////////////////////
  // Filters
  /////////////////////////////////////////////////////////

  Filter *filter;

  AudioProcessor proc(2048);
  Spdifer spdifer;

  /////////////////////////////////////////////////////////
  // Parse arguments
  /////////////////////////////////////////////////////////

  for (int iarg = 2; iarg < argc; iarg++)
  {
    ///////////////////////////////////////////////////////
    // Parsers
    ///////////////////////////////////////////////////////

    // -ac3 - force ac3 parser (do not autodetect format)
    if (is_arg(argv[iarg], "ac3", argt_exist))
    {
      if (parser)
      {
        printf("-ac3 : ambigous parser\n");
        return 1;
      }

      parser = &ac3;
      continue;
    }

    // -dts - force dts parser (do not autodetect format)
    if (is_arg(argv[iarg], "dts", argt_exist))
    {
      if (parser)
      {
        printf("-dts : ambigous parser\n");
        return 1;
      }

      parser = &dts;
      continue;
    }

    // -mpa - force mpa parser (do not autodetect format)
    if (is_arg(argv[iarg], "mpa", argt_exist))
    {
      if (parser)
      {
        printf("-mpa : ambigous parser\n");
        return 1;
      }

      parser = &mpa;
      continue;
    }

    ///////////////////////////////////////////////////////
    // Output mode
    ///////////////////////////////////////////////////////

    // -d[ecode] - decode
    if (is_arg(argv[iarg], "d", argt_exist) || 
        is_arg(argv[iarg], "decode", argt_exist))
    {
      if (mode != mode_nothing)
      {
        printf("-decode : ambigous output mode\n");
        return 1;
      }

      sink = &null;
      mode = mode_decode;
      continue;
    }

    // -p[lay] - play
    if (is_arg(argv[iarg], "p", argt_exist) || 
        is_arg(argv[iarg], "play", argt_exist))
    {
      if (mode != mode_nothing)
      {
        printf("-play : ambigous output mode\n");
        return 1;
      }

      sink = &dsound;
      mode = mode_play;
      continue;
    }
    
    // -r[aw] - RAW output
    if (is_arg(argv[iarg], "r", argt_exist) ||
        is_arg(argv[iarg], "raw", argt_exist))
    {
      if (argc - iarg < 1)
      {
        printf("-raw : specify a file name\n");
        return 1;
      }
      if (mode != mode_nothing)
      {
        printf("-raw : ambigous output mode\n");
        return 1;
      }

      const char *filename = argv[++iarg];
      if (!raw.open_file(filename))
      {
        printf("Error: failed to open file '%s'", filename);
        return 1;
      }

      sink = &raw;
      mode = mode_raw;
      continue;
    }

    ///////////////////////////////////////////////////////
    // Output options
    ///////////////////////////////////////////////////////

    // -spdif - enable SPDIF output
    if (is_arg(argv[iarg], "spdif", argt_exist))
    {
      spdif = true;
      continue;
    }

    // -spk - number of speakers
    if (is_arg(argv[iarg], "spk", argt_num))
    {
      mask = int(arg_num(argv[iarg]));

      if (mask < 0 || mask > 6)
      {
        printf("-spk : incorrect speaker configuration\n");
        return 1;
      }

      mask = mask_tbl[mask];
      continue;
    }

    // -fmt - sample format
    if (is_arg(argv[iarg], "fmt", argt_num))
    {
      format = int(arg_num(argv[iarg]));
      if (format < 0 || format > 4)
      {
        printf("-fmt : incorrect sample format");
        return 1;
      }
      format = format_tbl[format];
      continue;
    }

    ///////////////////////////////////////////////////////
    // Info
    ///////////////////////////////////////////////////////

    // -i - print bitstream info
    if (is_arg(argv[iarg], "i", argt_exist))
    {
      print_info = true;
      continue;
    }

    // -opt - print decoding options
    if (is_arg(argv[iarg], "opt", argt_exist))
    {
      print_opt = true;
      continue;
    }

    // -hist - print levels histogram
    if (is_arg(argv[iarg], "hist", argt_exist))
    {
      print_hist = true;
      continue;
    }

    ///////////////////////////////////////////////////////
    // Mixer options
    ///////////////////////////////////////////////////////

    // -auto_matrix
    if (is_arg(argv[iarg], "auto_matrix", argt_bool))
    {
      proc.set_auto_matrix(arg_bool(argv[iarg]));
      continue;
    }

    // -normalize_matrix
    if (is_arg(argv[iarg], "normalize_matrix", argt_bool))
    {
      proc.set_normalize_matrix(arg_bool(argv[iarg]));
      continue;
    }

    // -voice_control
    if (is_arg(argv[iarg], "voice_control", argt_bool))
    {
      proc.set_voice_control(arg_bool(argv[iarg]));
      continue;
    }

    // -expand_stereo
    if (is_arg(argv[iarg], "expand_stereo", argt_bool))
    {
      proc.set_expand_stereo(arg_bool(argv[iarg]));
      continue;
    }

    // -clev
    if (is_arg(argv[iarg], "clev", argt_num))
    {
      proc.set_clev(db2value(arg_num(argv[iarg])));
      continue;
    }

    // -slev
    if (is_arg(argv[iarg], "slev", argt_num))
    {
      proc.set_slev(db2value(arg_num(argv[iarg])));
      continue;
    }

    // -lfelev
    if (is_arg(argv[iarg], "lfelev", argt_num))
    {
      proc.set_lfelev(db2value(arg_num(argv[iarg])));
      continue;
    }

    // -gain
    if (is_arg(argv[iarg], "gain", argt_num))
    {
      proc.set_master(db2value(arg_num(argv[iarg])));
      continue;
    }

    // -gain_l
    if (is_arg(argv[iarg], "gain_l", argt_num))
    {
      gains[CH_L] = db2value(arg_num(argv[iarg]));
      continue;
    }
    // -gain_c
    if (is_arg(argv[iarg], "gain_c", argt_num))
    {
      gains[CH_C] = db2value(arg_num(argv[iarg]));
      continue;
    }
    // -gain_r
    if (is_arg(argv[iarg], "gain_r", argt_num))
    {
      gains[CH_R] = db2value(arg_num(argv[iarg]));
      continue;
    }
    // -gain_sl
    if (is_arg(argv[iarg], "gain_sl", argt_num))
    {
      gains[CH_SL] = db2value(arg_num(argv[iarg]));
      continue;
    }
    // -gain_sr
    if (is_arg(argv[iarg], "gain_sr", argt_num))
    {
      gains[CH_SR] = db2value(arg_num(argv[iarg]));
      continue;
    }
    // -gain_lfe
    if (is_arg(argv[iarg], "gain_lfe", argt_num))
    {
      gains[CH_LFE] = db2value(arg_num(argv[iarg]));
      continue;
    }

    ///////////////////////////////////////////////////////
    // Auto gain control options
    ///////////////////////////////////////////////////////

    // -normalize
    if (is_arg(argv[iarg], "normalize", argt_bool))
    {
      proc.set_normalize(arg_bool(argv[iarg]));
      continue;
    }

    // -drc
    if (is_arg(argv[iarg], "drc", argt_bool))
    {
      proc.set_drc(arg_bool(argv[iarg]));
      continue;
    }

    // -drc_power
    if (is_arg(argv[iarg], "drc_power", argt_num))
    {
      proc.set_drc(true);
      proc.set_drc_power(arg_num(argv[iarg]));
      continue;
    }

    // -release
    if (is_arg(argv[iarg], "release", argt_num))
    {
      proc.set_release(db2value(arg_num(argv[iarg])));
      continue;
    }

    ///////////////////////////////////////////////////////
    // Delay options
    ///////////////////////////////////////////////////////

    // -delay
    if (is_arg(argv[iarg], "delay", argt_bool))
    {
      proc.set_delay(arg_bool(argv[iarg]));
      continue;
    }

    // -delay_units
    if (is_arg(argv[iarg], "delay_units", argt_num))
    {
      switch (int(arg_num(argv[iarg])))
      {
        case 0: delay_units = DELAY_SP;
        case 1: delay_units = DELAY_MS;
        case 2: delay_units = DELAY_M;
        case 3: delay_units = DELAY_CM;
        case 4: delay_units = DELAY_FT;
        case 5: delay_units = DELAY_IN;
      }
      continue;
    }

    // -delay_l
    if (is_arg(argv[iarg], "delay_l", argt_num))
    {
      delays[CH_L] = float(arg_num(argv[iarg]));
      continue;
    }
    // -delay_c
    if (is_arg(argv[iarg], "delay_c", argt_num))
    {
      delays[CH_C] = float(arg_num(argv[iarg]));
      continue;
    }
    // -delay_r
    if (is_arg(argv[iarg], "delay_r", argt_num))
    {
      delays[CH_R] = float(arg_num(argv[iarg]));
      continue;
    }
    // -delay_sl
    if (is_arg(argv[iarg], "delay_sl", argt_num))
    {
      delays[CH_SL] = float(arg_num(argv[iarg]));
      continue;
    }
    // -delay_sr
    if (is_arg(argv[iarg], "delay_sr", argt_num))
    {
      delays[CH_SR] = float(arg_num(argv[iarg]));
      continue;
    }
    // -delay_lfe
    if (is_arg(argv[iarg], "delay_lfe", argt_num))
    {
      delays[CH_LFE] = float(arg_num(argv[iarg]));
      continue;
    }

    printf("Error: unknown option: %s\n", argv[iarg]);
    return 1;
  }

  Speakers spk;

  /////////////////////////////////////////////////////////
  // Open file
  /////////////////////////////////////////////////////////

  printf("Opening file %s...\n", input_filename);

  if (parser && file.open(parser, input_filename))
    ; // use format specified by user
  else if (file.open(&ac3, input_filename) && file.probe())
    parser = &ac3;
  else if (file.open(&dts, input_filename) && file.probe())
    parser = &dts;
  else if (file.open(&mpa, input_filename) && file.probe())
    parser = &mpa;
  else
  {
    printf("Error: Cannot open file '%s' or incorrect file format!", input_filename);
    return 1;
  }

  file.stats();

  /////////////////////////////////////////////////////////
  // Print bitstream info
  /////////////////////////////////////////////////////////

  if (print_info)
  {
    char info[1024];
    file.get_info(info, 1024);
    printf("%s\n", info);
  }

  /////////////////////////////////////////////////////////
  // Setup processing
  /////////////////////////////////////////////////////////

  // spk = input format

  spk = file.get_spk();

  if (spdif)
  {
    if (parser != &ac3 && parser != &dts && parser != &mpa)
    {
      printf("This format does not allow SPDIF transmision.\n");
      printf("Using general audio output.\n");
      spdif = false;
    }
    else
    {
      spk.format = FORMAT_SPDIF;
      filter = &spdifer;
    }
  }

  if (!spdif)
  {
    filter = &proc;

    proc.set_delay_units(delay_units);
    proc.set_delays(delays);
    proc.set_output_gains(gains);
    proc.set_input_order(std_order);
    proc.set_output_order(win_order);

    if (!proc.set_input(spk))
    {
      printf("Error: unsupported input format");
      return 1;
    }

    if (mask)
      spk.mask = mask;

    spk.format = format;
    spk.level = 1.0;
    switch (format)
    {
      case FORMAT_PCM16: spk.level = 32767; break;
      case FORMAT_PCM24: spk.level = 8388607; break;
      case FORMAT_PCM32: spk.level = 2147483647; break;
    }

    if (!proc.set_output(spk))
    {
      printf("Error: unsupported output format");
      return 1;
    }
    if (spk != proc.get_output())
      printf("Warning: using different output format\n");
  }

  // spk = output format

  /////////////////////////////////////////////////////////
  // Print processing options
  /////////////////////////////////////////////////////////

  //
  // TODO
  //

  /////////////////////////////////////////////////////////
  // Setup output
  /////////////////////////////////////////////////////////

  printf("Opening %s %s %iHz audio output...\n", spk.format_text(), spk.mode_text(), spk.sample_rate);
  if (!sink->open(spk))
  {
    printf("Error: Cannot open audio output!");
    return 1;
  }

  /////////////////////////////////////////////////////////
  // Process
  /////////////////////////////////////////////////////////

  Chunk chunk;

  CPUMeter cpu_current;
  CPUMeter cpu_total;

  cpu_current.start();
  cpu_total.start();

  double ms = 0;
  double old_ms = 0;
  sample_t levels[NCHANNELS];
  sample_t level;
  int i;

  fprintf(stderr, " 0.0%% Frs:      0 Err: 0 Time:   0:00.000i Level:    0dB FPS:    0 CPU: 0%%\r"); 
  while (!file.eof())
    if (file.load_frame())
    {
      if (spdif)
        chunk.set(Speakers(FORMAT_UNKNOWN, 0, 0), parser->get_frame(), parser->get_frame_size());
      else
      {
        // Decode
        if (!file.decode_frame())
          continue;

        chunk.set(file.get_spk(), file.get_samples(), file.get_nsamples());
      }

      /////////////////////////////////////////////////////
      // Processing & output

      if (!filter->process_to(&chunk, sink))
      {
        printf("\nProcessing error!\n");
        return 1;
      }

      /////////////////////////////////////////////////////
      // Statistics

      ms = double(cpu_total.get_system_time() * 1000);
      if (ms > old_ms + 100)
      {
        old_ms = ms;

        // Levels
        if (!spdif)
        {
          proc.get_output_levels(sink->get_time(), levels);
          level = levels[0];
          for (i = 1; i < NCHANNELS; i++)
            if (levels[i] > level)
              level = levels[i];
        }

        fprintf(stderr, "%4.1f%% Frs: %-6i Err: %-i Time: %3i:%02i.%03i Level: %-4idB FPS: %-4i CPU: %.1f%%  \r", 
          file.get_pos(file.relative) * 100, 
          file.get_frames(), file.get_errors(),
          int(ms/60000), int(ms) % 60000/1000, int(ms) % 1000,
          int(value2db(level)),
          int(file.get_frames() * 1000 / (ms+1)),
          cpu_current.usage() * 100);
      }
    } // if (file.frame())
  // while (!file.eof()) 

  /////////////////////////////////////////////////////
  // Flushing

  if (spdif)
    chunk.set(Speakers(FORMAT_UNKNOWN, 0, 0), 0, 0);
  else
    chunk.set(file.get_spk(), 0, 0);
  chunk.set_eos(true);

  if (!filter->process_to(&chunk, sink))
  {
    printf("\nProcessing error!\n");
    return 1;
  }

  /////////////////////////////////////////////////////
  // Final statistics

  fprintf(stderr, "%2.1f%% Frs: %-6i Err: %-i Time: %3i:%02i.%03i Level: %-4idB FPS: %-4i CPU: %.1f%%  \n", 
    file.get_pos(file.relative) * 100, 
    file.get_frames(), file.get_errors(),
    int(ms/60000), int(ms) % 60000/1000, int(ms) % 1000,
    int(value2db(level)),
    int(file.get_frames() * 1000 / (ms+1)),
    cpu_current.usage() * 100);

  cpu_current.stop();
  cpu_total.stop();

  printf("System time: %ims\n", int(cpu_total.get_system_time() * 1000));
  printf("Process time: %ims\n", int(cpu_total.get_thread_time() * 1000 ));
  printf("Approx. %.2f%% realtime CPU usage\n", double(cpu_total.get_thread_time() * 100000) / file.get_size(file.ms));

  /////////////////////////////////////////////////////////
  // Print levels histogram
  /////////////////////////////////////////////////////////

  if (print_hist && !spdif)
  {
    double hist[MAX_HISTOGRAM];
    int dbpb;
    int i, j;

    proc.get_output_histogram(hist, MAX_HISTOGRAM);
    dbpb = proc.get_dbpb();

    printf("\nHistogram:\n");
    printf("------------------------------------------------------------------------------\n");
    for (i = 0; i*dbpb < 100 && i < MAX_HISTOGRAM; i++)
    {
      printf("%2idB: %4.1f ", i * dbpb, hist[i] * 100);
      for (j = 0; j < 67 && j < hist[i] * 67; j++)
        printf("*");
      printf("\n");
    }
    printf("------------------------------------------------------------------------------\n");
    printf("dbpb;%i\nhistogram;", dbpb);
    for (i = 0; i < MAX_HISTOGRAM; i++)
      printf("%.4f;", hist[i]);
    printf("\n");
    printf("------------------------------------------------------------------------------\n");
  }

  return 0;
}
