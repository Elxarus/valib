#include <string.h>
#include "mpeg_demux.h"

// todo: check marker bits


bool
MPEGDemux::is_audio()
{
  return (((stream    & 0xe0) != 0xc0) ||   // MPEG audio stream
          ((substream & 0xf8) != 0x80) ||   // AC3 audio substream
          ((substream & 0xf8) != 0x88) ||   // DTS audio substream
          ((substream & 0xf8) != 0xA0));    // LPCM audio substream
}

Speakers 
MPEGDemux::spk()
{
  // convert LPCM number of channels to channel mask
  static const int nch2mask[8] = 
  {
    MODE_MONO, 
    MODE_STEREO,
    MODE_3_1,
    MODE_QUADRO,
    MODE_3_2, 
    MODE_5_1,
    0, 0
  };

  if      ((stream    & 0xe0) == 0xc0) return Speakers(FORMAT_MPA, 0, 0);   // MPEG audio stream
  else if ((substream & 0xf8) == 0x80) return Speakers(FORMAT_AC3, 0, 0);   // AC3 audio substream
  else if ((substream & 0xf8) == 0x88) return Speakers(FORMAT_DTS, 0, 0);   // DTS audio substream
  else if ((substream & 0xf8) == 0xA0)                                 // LPCM audio substream
  {
    // parse LPCM header
    int format, mask, sample_rate;
    sample_t level;

    switch (subheader[4] >> 6)
    {
      case 0: format = FORMAT_PCM16_LE; level = 32767;    break;
      case 2: format = FORMAT_PCM24_LE; level = 8388607;  break;
      default: return unk_spk;
    }

    mask = nch2mask[subheader[4] & 7];
    if (!mask) return unk_spk;

    switch ((subheader[4] >> 4) & 3)
    {
      case 0: sample_rate = 48000; break;
      case 1: sample_rate = 96000; break;
      default: return unk_spk;
    }

    return Speakers(format, mask, sample_rate, level);

  }
  else
    // not an audio format
    return unk_spk;
}

int 
MPEGDemux::streaming(uint8_t *_buf, int len)
{
  int size;
  int required_size  = 0;
  uint8_t *buf_read  = _buf;
  uint8_t *buf_write = _buf;

  int current_stream;
  int current_substream;

  #define REQUIRE(bytes)     \
  if (data_size < (bytes))   \
  {                          \
    required_size = (bytes); \
    continue;                \
  }

  #define DROP(bytes)   \
  {                     \
    data_size = bytes;  \
    state = demux_drop; \
    continue;           \
  }

  #define SYNC          \
  {                     \
    data_size = 0;      \
    required_size = 4;  \
    state = demux_sync; \
    continue;           \
  }

  while (1)
  {
    /////////////////////////////////////////////////////////////////
    // Load requested data

    if (data_size < required_size)
      if (len < required_size - data_size)
      {
        memcpy(header + data_size, buf_read, len);
        data_size += len;
        return buf_write - _buf;
      }
      else
      {
        size = required_size - data_size;
        memcpy(header + data_size, buf_read, size);
        buf_read += size;
        len -= size;
        data_size = required_size;
      }

    switch (state)
    {
      /////////////////////////////////////////////////////////////////
      // Drop unneeded data

      case demux_drop:
      {
        if (len > data_size)
        {
          buf_read += data_size;
          len -= data_size;
          SYNC;
        }
        else
        {
          data_size -= len;
          return buf_write - _buf;
        }
      } // case demux_drop:

      /////////////////////////////////////////////////////////////////
      // Sync

      case demux_sync:
      {
        REQUIRE(4);
        uint32_t sync = swab_u32(*(uint32_t *)header);

        while ((sync & 0xffffff00) != 0x00000100 && len)
        {
          sync <<= 8;
          sync += *buf_read;
          buf_read++;
          len--;
        }

        data_size = 4;
        *(uint32_t *)header = swab_u32(sync);

        if ((sync & 0xffffff00) == 0x00000100)
          state = demux_header;
        else
          return buf_write - _buf;

        // no break: now we go to demux_header
      } // case demux_sync:

      /////////////////////////////////////////////////////////////////
      // Parse header

      case demux_header:
      {
        switch (header[3])
        {
          /////////////////////////////////////////////////////////////////
          // Program end code

          case 0xb9:
          {         
            data_size = 0;
            state = demux_sync;
            return buf_write - _buf;
          } // case 0xb9

          /////////////////////////////////////////////////////////////////
          // Pack header

          case 0xba:
          {
            REQUIRE(12);
            if ((header[4] & 0xf0) == 0x20) 
              // MPEG1
              SYNC
            else if ((header[4] & 0xc0) == 0x40) 
            {
              // MPEG2
              REQUIRE(14);
              REQUIRE(14 + (header[13] & 7));
              SYNC;
            } 
            else 
            {
              // Unknown
              errors++;
              SYNC;
            }
            // never be here
          }

          /////////////////////////////////////////////////////////////////
          // System header

          case 0xbb:  // system header
          {
            // drop
            REQUIRE(6);
            DROP((header[4] << 8) + header[5]);
          }

          /////////////////////////////////////////////////////////////////
          // Reserved and stuffing streams

          case 0xbc:  // reserved stream
          case 0xbe:  // stuffing stream
          {
            // padding packet length?
            // drop packet
            REQUIRE(6);
            DROP((header[4] << 8) + header[5]); 
          } // case 0xbe:  // stuffing stream
        } // switch (header[3])

        /////////////////////////////////////////////////////////////////
        // Disallowed stream number

        if (header[3] < 0xb9)
        {
          errors++;  
          SYNC;
        }

        /////////////////////////////////////////////////////////////////
        // Reserved stream

        if ((header[3] & 0xf0) == 0xf0)
        {
          // drop packet
          REQUIRE(6);
          DROP(6 + (header[4] << 8) + header[5] - data_size);
        }

        /////////////////////////////////////////////////////////////////
        // Actual data packet

        current_stream = header[3];
        current_substream = 0;
        if (stream && stream != current_stream)
        {
          REQUIRE(6);
          DROP(6 + (header[4] << 8) + header[5] - data_size);
        }

        REQUIRE(7);
        int pos = 6;
        if (stream != 0xbf) // Private Stream 2 have no following flags
        {
          if ((header[pos] & 0xc0) == 0x80)
          {
            // MPEG2
            REQUIRE(9);
            pos = header[8] + 9;
            REQUIRE(pos);
          } 
          else 
          {
            // MPEG1
            while (header[pos] == 0xff && pos < data_size)
              pos++;

            if (pos == data_size)
              if (data_size < 24)
                REQUIRE(pos+1)
              else
              {
                errors++; // too much stuffing
                SYNC;
              }

            if ((header[pos] & 0xc0) == 0x40)
            {
              pos += 2;
              REQUIRE(pos+1);
            }

            if ((header[pos] & 0xf0) == 0x20)
              pos += 5;
            else if ((header[pos] & 0xf0) == 0x30)
              pos += 10;
            else // if (header[pos] == 0x0f)
              pos++;

            REQUIRE(pos);
          }
        } // if (stream != 0xbf) // Private Stream 2 have no following flags

        /////////////////////////////////////////////////////////////////
        // Substream header

        if (current_stream == 0xbd)
        {
          pos++;
          REQUIRE(pos);

          current_substream = header[pos-1]; 
          if (substream && substream != current_substream)
          {
            DROP(6 + (header[4] << 8) + header[5] - data_size);
            // went out...
          }

          // AC3/DTS substream
          if ((current_substream & 0xf0) == 0x80)
          {
            pos += 3;
            REQUIRE(pos);

            subheader[0] = header[pos-3];
            subheader[1] = header[pos-2];
            subheader[2] = header[pos-1];
          }
          // LPCM substeam
          if ((current_substream & 0xf0) == 0xa0)
          {
            pos += 6;
            REQUIRE(pos);

            subheader[0] = header[pos-6];
            subheader[1] = header[pos-5];
            subheader[2] = header[pos-4];
            subheader[3] = header[pos-3];
            subheader[4] = header[pos-2];
            subheader[5] = header[pos-1];
          }
        }

        ///////////////////////////////////////////////
        // FINAL
        ///////////////////////////////////////////////

        data_size = 6 + (header[4] << 8) + header[5] - data_size;
        if (data_size <= 0)
        {
          errors++;
          SYNC;
        }

        // Lock on the first found stream
        if (!stream) stream = current_stream;
        if (!substream) substream = current_substream;

        state = demux_data;
        frames++;
        // no break: now we go to demux_data
      } // case demux_header:

      /////////////////////////////////////////////////////////////////
      // Extract data

      case demux_data:
      {
        if (len > data_size)
        {
          memmove(buf_write, buf_read, data_size);
          buf_read += data_size;
          buf_write += data_size;
          len -= data_size;
          SYNC;
        }
        else
        {
          memmove(buf_write, buf_read, len);
          data_size -= len;
          return buf_write + len - _buf;
        }
        // never be here
      } // case demux_data:
    } // switch (state)
  } // while (1)
}


int 
MPEGDemux::packet(uint8_t *buf, int len, int *gone)
{
  *gone = 0;

  int size;
  int required_size  = 0;

  #define REQUIRE(bytes)       \
  {                            \
    if (data_size < (bytes))   \
    {                          \
      required_size = (bytes); \
      continue;                \
    }                          \
  }

  #define DROP(bytes)   \
  {                     \
    data_size = bytes;  \
    state = demux_drop; \
    continue;           \
  }

  #define SYNC          \
  {                     \
    data_size = 0;      \
    required_size = 4;  \
    state = demux_sync; \
    continue;           \
  }

  while (1)
  {
    /////////////////////////////////////////////////////////////////
    // Load header data

    if (data_size < required_size)
      if (len < required_size - data_size)
      {
        memcpy(header + data_size, buf, len);
        data_size += len;
        *gone += len;
        return 0;
      }
      else
      {
        size = required_size - data_size;
        memcpy(header + data_size, buf, size);
        buf += size;
        len -= size;
        *gone += size;
        data_size = required_size;
      }

    switch (state)
    {
      /////////////////////////////////////////////////////////////////
      // Drop unneeded data

      case demux_drop:
      {
        if (len > data_size)
        {
          buf += data_size;
          len -= data_size;
          *gone += data_size;
          SYNC;
        }
        else
        {
          *gone += len;
          data_size -= len;
          return 0;
        }
      } // case demux_drop:

      /////////////////////////////////////////////////////////////////
      // Sync

      case demux_sync:
      {
        REQUIRE(4);
        uint32_t sync = swab_u32(*(uint32_t *)header);

        while ((sync & 0xffffff00) != 0x00000100 && len)
        {
          sync <<= 8;
          sync += *buf;
          buf++;
          len--;
          (*gone)++;
        }

        data_size = 4;
        *(uint32_t *)header = swab_u32(sync);

        if ((sync & 0xffffff00) == 0x00000100)
          state = demux_header;
        else
          return 0;

        // no break: now we go to demux_header
      } // case demux_sync:

      /////////////////////////////////////////////////////////////////
      // Parse header

      case demux_header:
      {
        switch (header[3])
        {
          /////////////////////////////////////////////////////////////////
          // Program end code

          case 0xb9:
          {         
            data_size = 0;
            state = demux_sync;
            return 0;
          } // case 0xb9

          /////////////////////////////////////////////////////////////////
          // Pack header

          case 0xba:
          {
            REQUIRE(12);
            if ((header[4] & 0xf0) == 0x20) 
              // MPEG1
              SYNC
            else if ((header[4] & 0xc0) == 0x40) 
            {
              // MPEG2
              REQUIRE(14);
              REQUIRE(14 + (header[13] & 7));
              SYNC;
            } 
            else 
            {
              // Unknown
              errors++;
              SYNC;
            }
            // never be here
          }

          /////////////////////////////////////////////////////////////////
          // System header

          case 0xbb:  // system header
          {
            // drop
            REQUIRE(6);
            DROP((header[4] << 8) + header[5]);
          }

          /////////////////////////////////////////////////////////////////
          // Reserved and stuffing streams

          case 0xbc:  // reserved stream
          case 0xbe:  // stuffing stream
          {
            // padding packet length????????
            // drop packet
            REQUIRE(6);
            DROP((header[4] << 8) + header[5]);
          } // case 0xbe:  // stuffing stream
        } // switch (header[3])

        /////////////////////////////////////////////////////////////////
        // Disallowed stream number

        if (header[3] < 0xb9)
        {
          errors++;  
          SYNC;
        }

        /////////////////////////////////////////////////////////////////
        // Reserved stream

        if ((header[3] & 0xf0) == 0xf0)
        {
          // drop packet
          REQUIRE(6);
          DROP(6 + (header[4] << 8) + header[5] - data_size);
        }

        /////////////////////////////////////////////////////////////////
        // Actual data packet

        stream = header[3];
        substream = 0;

        REQUIRE(7);
        int pos = 6;
        if (stream != 0xbf) // Private Stream 2 have no following flags
        {
          if ((header[pos] & 0xc0) == 0x80)
          {
            // MPEG2
            REQUIRE(9);
            pos = header[8] + 9;
            REQUIRE(pos);
          } 
          else 
          {
            // MPEG1
            while (header[pos] == 0xff && pos < data_size)
              pos++;

            if (pos == data_size)
              if (data_size < 24)
                REQUIRE(pos+1)
              else
              {
                errors++; // too much stuffing
                SYNC;
              }

            if ((header[pos] & 0xc0) == 0x40)
            {
              pos += 2;
              REQUIRE(pos+1);
            }

            if ((header[pos] & 0xf0) == 0x20)
              pos += 5;
            else if ((header[pos] & 0xf0) == 0x30)
              pos += 10;
            else // if (header[pos] == 0x0f)
              pos++;

            REQUIRE(pos);
          }
        } // if (stream != 0xbf) // Private Stream 2 have no following flags

        /////////////////////////////////////////////////////////////////
        // Substream header

        if (stream == 0xbd)
        {
          pos++;
          REQUIRE(pos);
          substream = header[pos-1];

          // AC3/DTS substream
          if ((substream & 0xf0) == 0x80)
          {
            pos += 3;
            REQUIRE(pos);

            subheader[0] = header[pos-3];
            subheader[1] = header[pos-2];
            subheader[2] = header[pos-1];
          }
          // LPCM substeam
          if ((substream & 0xf0) == 0xa0)
          {
            pos += 6;
            REQUIRE(pos);

            subheader[0] = header[pos-6];
            subheader[1] = header[pos-5];
            subheader[2] = header[pos-4];
            subheader[3] = header[pos-3];
            subheader[4] = header[pos-2];
            subheader[5] = header[pos-1];
          }
        }

        ///////////////////////////////////////////////
        // FINAL
        ///////////////////////////////////////////////

        size = 6 + (header[4] << 8) + header[5] - data_size;
        if (size <= 0)
        {
          errors++;
          SYNC;
        }

        data_size = 0;
        required_size = 4;
        state = demux_sync;

        frames++;
        return size;
        // note: never be here
      } // case demux_header:

    } // switch (state)
  } // while (1)
}
