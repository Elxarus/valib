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
  uint8_t buf[buf_size]; // one frame of pcm16 stereo samples
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
      if (frames == 314)
        frames = 314;

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
        if (b == 3)
          b = 3;

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
            // dequantize encoded dct coefs and compare with decoded coefs
            double v = 0;
            int bap = enc.bap[ch][b][s];
            int m = enc.mant[ch][b][s];
            int e = enc.exp[ch][b][s];

            switch (bap)
            {
              case 0:  break;
              case 1:  v = ((sym_quant(m, 3) - 1) << 16) / 3; break;
              case 2:  v = ((sym_quant(m, 5) - 2) << 16) / 5; break;
              case 3:  v = ((sym_quant(m, 7) - 3) << 16) / 7; break;
              case 4:  v = ((sym_quant(m,11) - 5) << 16) /11; break;
              case 5:  v = ((sym_quant(m,15) - 7) << 16) /15; break;
              case 15: v = m; break;
              case 14: v = (m >> 2) << 2; break;
              default: v = (m >> (16 - bap + 1)) << (16 - bap + 1); break;
            }
            v /= 32768;
            v /= (1<<e);
            if (bap != 0 && fabs(v / dec.get_samples()[ch][s + b * AC3_BLOCK_SAMPLES] - 1) > 0.001)
              printf("strange sample f=%i ch=%i b=%i s=%i bap=%i...\n", frames, ch, b, s, bap);
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
