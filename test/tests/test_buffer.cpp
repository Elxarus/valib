/*
  This test verifies that filter does not alter the data outside of given 
  range. This test requires 2 identical instances of the same filter.
*/

#include <stdio.h>
#include "..\log.h"
#include "filter.h"

int test_buffer(Log *log, const char *filter_name, Speakers spk, Filter *f1, Filter *f2, size_t buf_size, int nbufs, int ntests);
int test_buffer_rawdata(Log *log, const char *filter_name, Speakers spk, Filter *f1, Filter *f2, size_t buf_size, int nbufs, int ntests);
int test_buffer_samples(Log *log, const char *filter_name, Speakers spk, Filter *f1, Filter *f2, size_t buf_size, int nbufs, int ntests);
                                                                                                                                                               
static const int formats[] = 
{ 
  FORMAT_PCM16,    FORMAT_PCM24,    FORMAT_PCM32,    FORMAT_PCMFLOAT, 
  FORMAT_PCM16_LE, FORMAT_PCM24_LE, FORMAT_PCM32_LE, FORMAT_PCMFLOAT_LE 
};

static const int modes[] = 
{ 
  MODE_1_0,     MODE_2_0,     MODE_3_0,     MODE_2_1,     MODE_3_1,     MODE_2_2,     MODE_3_2, 
  MODE_1_0_LFE, MODE_2_0_LFE, MODE_3_0_LFE, MODE_2_1_LFE, MODE_3_1_LFE, MODE_2_2_LFE, MODE_3_2_LFE 
};

static const int sample_rates[] = 
{  
  8000,  16000, 32000, 64000, 128000, 
  11250, 22500, 44100, 88200, 176400, 
  12000, 24000, 48000, 96000, 192000 
};

static const int buf_sizes[] = 
{
  // primes
     1,    2,    3,    5,    7,   11,   13,   17,   19,   23, 
    29,   31,   37,   41,   43,   47,   53,   59,   61,   67, 
    71,   73,   79,   83,   89,   97,  101,  103,  107,  109, 
   113,  127,  131,  137,  139,  149,  151,  157,  163,  167, 
   173,  179,  181,  191,  193,  197,  199,  211,  223,  227, 
   229,  233,  239,  241,  251,  257,  263,  269,  271,  277, 
   281,  283,  293,  307,  311,  313,  317,  331,  337,  347, 
   349,  353,  359,  367,  373,  379,  383,  389,  397,  401, 
   409,  419,  421,  431,  433,  439,  443,  449,  457,  461, 
   463,  467,  479,  487,  491,  499,  503,  509,  521,  523, 
   541,  547,  557,  563,  569,  571,  577,  587,  593,  599, 
   601,  607,  613,  617,  619,  631,  641,  643,  647,  653, 
   659,  661,  673,  677,  683,  691,  701,  709,  719,  727, 
   733,  739,  743,  751,  757,  761,  769,  773,  787,  797, 
   809,  811,  821,  823,  827,  829,  839,  853,  857,  859, 
   863,  877,  881,  883,  887,  907,  911,  919,  929,  937, 
   941,  947,  953,  967,  971,  977,  983,  991,  997, 1009, 
  1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 
  1069, 1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 
  1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 
  1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 
  1291, 1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 
  1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 
  1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 
  1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 
  1583, 1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 
  1657, 1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 
  1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 
  1811, 1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 
  1889, 1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 
  1987, 1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 
  2053, 2063, 2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 
  2129, 2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 
  2213, 2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 
  2287, 2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 
  2357, 2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 
  2423, 2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 
  2531, 2539, 2543, 2549, 2551, 2557, 2579, 2591, 2593, 2609, 
  2617, 2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 
  2687, 2689, 2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 
  2741, 2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 
  2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 
  2903, 2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 
  2999, 3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 
  3079, 3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 
  3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 
  3257, 3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 
  3331, 3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 
  3413, 3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 
  3511, 3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 
  3571, 3581, 3583, 3593, 3607, 3613, 3617, 3623, 3631, 3637, 
  3643, 3659, 3671, 3673, 3677, 3691, 3697, 3701, 3709, 3719, 
  3727, 3733, 3739, 3761, 3767, 3769, 3779, 3793, 3797, 3803, 
  3821, 3823, 3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889, 
  3907, 3911, 3917, 3919, 3923, 3929, 3931, 3943, 3947, 3967, 
  3989, 4001, 4003, 4007, 4013, 4019, 4021, 4027, 4049, 4051, 
  4057, 4073, 4079, 4091, 4093,

           15,     63,      255, 511, 1023, 2047, 4095, // 2^n - 1
     4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, // 2^n
        9,     33, 65, 129,      513, 1025, 2049, 4097, // 2^n + 1
}


int test_pcm_bounds(Log *log, Filter *filter, const char *filter_name)
{
  log->open_group("PCM out-of-bounds filter test (samples). Filter: %s", filter_name);

  int format, mode, sample_rate;

  DataBuf data;
  DataBuf copy;

  data.allocate(5 * 48000 * 6 * 4) // 5 buffers (1sec 6ch at 48kHz)
  data.allocate(5 * 48000 * 6 * 4) // 5 buffers (1sec 6ch at 48kHz)

  for (format = 0; format < array_size(formats); format++)
    for (mode = 0; mode < array_size(modes); mode++)
      for (sample_rate = 0; array_size(sample_rates); sample_rate++)
      {
        Speakers spk(format, mode, sample_rate);

        ///////////////////////////////////////////////////
        // setup filter

        if (!f->query_input(spk))
          // filter does not support this configuration
          continue;

        log->msg("Speakers: %s %s %iHz", spk.format_text(), spk.mode_text(), spk.sample_rate);

        if (!f->set_input(spk))
        {
          log->err("set_input() returns false after successful query_input()");
          continue;
        }

        ///////////////////////////////////////////////////
        // prepare data

        memset(data, 0x55, data.size());



        for (int buf_size = 1
        if (format == FORMAT_LINEAR)
          test_buffer_samples(log, f1, f2, 
      }
  }

}

bool test_buffer(Log *log, Filter *f, Chunk *chunk, Chunk *subchunk, int buf_size)
{
  size_t  big_buf_size = buf_size * nbufs * 2 + buf_size;

  size_t i;
  uint8_t *big_buf = new uint8_t[big_buf_size]
  uint8_t *buf = big_buf1 + buf_size * nbufs;

  big_buf1 = new uint8_t[big_buf_size];

  memset(big_buf1, 0x55, big_buf_size);
  memset(big_buf2, 0xcc, big_buf_size);

  buf1 = big_buf1 + buf_size * nbufs;
  buf2 = big_buf2 + buf_size * nbufs;

  Chunk ichunk1;
  Chunk ochunk1;

  ichunk1.set(spk, buf1, buf_size);
  ichunk2.set(spk, buf2, buf_size);

  while (ntests--)
  {
    // fill processing buffer with noise
    for (i = 0; i < buf_size; i++)
      buf2[i] = buf1[i] = rand() * 256 / RAND_MAX;

    // push data
    if (!f1->process(&ichunk1) || f2->process(&ichunk2))
    {
      log->err("process() failed");
      return false;
    }

    // pull data
    while (!f1->is_empty())
    {
      if (f2->is_empty())
      {
        log->err("empty status does not match");
        return false;
      }

      if (!f1->get_chunk(&ochunk1) || !f2->get_chunk(&ochunk2))
      {
        log->err("get_chunk() failed");
        return false;
      }

      if (ochunk1.spk != ochunk2.spk || ochunk1.size != ochunk2.size)
      {
        log->err("output chunks are different");
        return false;
      }

      if (memcmp(ochunk1.get_rawdata(), ochunk2.get_rawdata(), ichunk1.get_size))
      {
        log->err("output data differs");
        return false;
      }
    }

    // check that input buffers still equal
    if (memcmp(buf1, buf2, buf_size))
    {
      log->err("buffers are different");
      return false;
    }

    // check stuffing integrity
    for (i = 0; i < buf_size * nbufs; i++)
      if (big_buf1[i] != 0x55 || big_buf2[i] != 0xcc)
      {
        log->err("buffer is corrupted");
        return false;
      }

    for (i = buf_size * nbufs + buf_size; i < big_buf_size; i++)
      if (big_buf1[i] != 0x55 || big_buf2[i] != 0xcc)
      {
        log->err("buffer is corrupted");
        return false;
      }
  }

  return true;
}

