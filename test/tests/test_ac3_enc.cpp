/*
  AC3 encoder test
  DEcode encoder output and compare internal data
*/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "parsers\ac3\ac3_parser.h"
#include "parsers\ac3\ac3_enc.h"
#include "filters\convert.h"
#include "filters\filter_chain.h"
#include "auto_file.h"

int 
test_ac3_enc(const char *_raw_filename, const char *_desc, Speakers _spk, int _bitrate, int _nframes)
{
  printf("Testing file %s (%s)...\n", _raw_filename, _desc);

  AutoFile f(_raw_filename);
  if (!f.is_open())
  {
    printf("!!!Error: cannot open file '%s'\n", _raw_filename);
    return 1;
  }

  const int buf_size = 32000;
  uint8_t buf[buf_size];
  int buf_data;

  int frames = 0;
  uint8_t *frame_pos;
  Speakers raw_spk = _spk;
  Speakers lin_spk = _spk;
  lin_spk.format = FORMAT_LINEAR;

  Converter conv;
  AC3Enc    enc;
  AC3Parser dec;

  FilterChain chain;
  chain.add(&conv, "Converter");
  chain.add(&enc,  "Encoder");

  conv.set_buffer(AC3_FRAME_SAMPLES);
  conv.set_format(FORMAT_LINEAR);
  conv.set_order(win_order);

  if (!enc.set_bitrate(_bitrate) ||
      !enc.set_input(lin_spk))
  {
    printf("!!!Error: cannot init encoder!\n");
    return 1;
  }

  Chunk raw;
  Chunk ac3;
  
  while (!f.eof())
  {
    buf_data = f.read(buf, buf_size);

    raw.set_spk(raw_spk);
    raw.set_buf(buf, buf_data);
    raw.set_time(false);

    if (!chain.process(&raw))
    {
      printf("!!!Error: chain.process()!\n");
      return 1;
    }

    while (!chain.is_empty())
    {
      if (!chain.get_chunk(&ac3))
      {
        printf("!!!Error: chain.get_chunk()!\n");
        return 1;
      }

      if (ac3.is_empty())
        continue;

      frame_pos = ac3.buf;
      if (!dec.load_frame(&frame_pos, frame_pos + ac3.size))
      {
        printf("!!!Error: AC3 parser frame load error!\n");
        return 1;
      }

      for (int b = 0; b < AC3_NBLOCKS; b++)
      {
        if (!dec.decode_block())
        {
          printf("!!!Error: block %i decode error!\n", b);
          return 1;
        }

        if (memcmp(enc.exp[0][b], dec.exps[0], 223) || 
            memcmp(enc.exp[1][b], dec.exps[1], 223))
        {
          printf("!!!Error: exponents error!\n");
          return 1;
        }

        for (int ch = 0; ch < _spk.nch(); ch++)
        {
          int endmant;
          if (_spk.lfe() && ch != _spk.nch())
            // lfe channel
            endmant = 7;
          else
            // fbw channel
            if (enc.nmant[ch] != dec.endmant[ch])
            {
              printf("!!!Error: number of mantissas does not match!\n");
              return 1;
            }
            else
              endmant = enc.nmant[ch];

          for (int s = 0; s < endmant; s++)
          {
            // quantize/dequantize encoded dct coefs and compare with decoded coefs
            double v = 0;
            double v1 = 0;
            int bap = enc.bap[ch][b][s];
            int m = enc.mant[ch][b][s];
            int e = enc.exp[ch][b][s];

            switch (bap)
            {
              case 0:  break;
              // asymmetric quantization
              case 1:  v = sym_quant(m, 3)  * 2.0/3.0  - 2.0/3.0;   break;
              case 2:  v = sym_quant(m, 5)  * 2.0/5.0  - 4.0/5.0;   break;
              case 3:  v = sym_quant(m, 7)  * 2.0/7.0  - 6.0/7.0;   break;
              case 4:  v = sym_quant(m, 11) * 2.0/11.0 - 10.0/11.0; break;
              case 5:  v = sym_quant(m, 15) * 2.0/15.0 - 14.0/15.0; break;
              // symmetric quantization
              case 6:  v = int16_t(asym_quant(m, 5)  << 11) / 32768.0; break;
              case 7:  v = int16_t(asym_quant(m, 6)  << 10) / 32768.0; break;
              case 8:  v = int16_t(asym_quant(m, 7)  << 9)  / 32768.0; break;
              case 9:  v = int16_t(asym_quant(m, 8)  << 8)  / 32768.0; break;
              case 10: v = int16_t(asym_quant(m, 9)  << 7)  / 32768.0; break;
              case 11: v = int16_t(asym_quant(m, 10) << 6)  / 32768.0; break;
              case 12: v = int16_t(asym_quant(m, 11) << 5)  / 32768.0; break;
              case 13: v = int16_t(asym_quant(m, 12) << 4)  / 32768.0; break;
              case 14: v = int16_t(asym_quant(m, 14) << 2)  / 32768.0; break;
              case 15: v = int16_t(asym_quant(m, 16))       / 32768.0; break;
            }

            v1 = dec.get_samples()[ch][s + b * AC3_BLOCK_SAMPLES] * (1 << e);
            if (fabs(v - v1) > 1e-6)
              printf("strange sample f=%i ch=%i b=%i s=%i; bap=%i, mant=%i, exp=%i, v=%e, s=%e, v-s=%e...\n", frames, ch, b, s, bap, m, e, double(sym_quant(m, 11)) * 2.0/11.0 - 10.0/11.0, dec.get_samples()[ch][s + b * AC3_BLOCK_SAMPLES], fabs(v - v1));
          }
        } // for (int ch = 0; ch < _spk.nch(); ch++)
      } // for (int b = 0; b < AC3_NBLOCKS; b++)

      frames++;
      printf("Frame %i    \r", frames);
    }
  }

  if (_nframes && _nframes != frames)
  {
    printf("!!!Error: number of encoded frames (%i) does not match correct number (%i)!\n", frames, _nframes);
    return 1;
  }
  return 0;
}
