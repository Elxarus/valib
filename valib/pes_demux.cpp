#include <string.h>
#include "pes_demux.h"

// todo: check marker bits

int 
PESDemux::streaming(uint8_t *_buf, int len)
{
  int size;
  int required_size  = 0;
  uint8_t *buf_read  = _buf;
  uint8_t *buf_write = _buf;

  int current_stream;
  int current_substream;

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
    state = DEMUX_DROP; \
    continue;           \
  }

  #define SYNC          \
  {                     \
    data_size = 0;      \
    required_size = 4;  \
    state = DEMUX_SYNC; \
    continue;           \
  }

  while (1)
  {
    /////////////////////////////////////////////////////////////////
    // Load header data

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

      case DEMUX_DROP:
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
      } // case DEMUX_DROP:

      /////////////////////////////////////////////////////////////////
      // Sync

      case DEMUX_SYNC:
      {
        REQUIRE(4);
        uint32_t sync = swab32(*(uint32_t *)header);

        while ((sync & 0xffffff00) != 0x00000100 && len)
        {
          sync <<= 8;
          sync += *buf_read;
          buf_read++;
          len--;
        }

        data_size = 4;
        *(uint32_t *)header = swab32(sync);

        if ((sync & 0xffffff00) == 0x00000100)
          state = DEMUX_HEADER;
        else
          return buf_write - _buf;

        // no break: now we go to DEMUX_HEADER
      } // case DEMUX_SYNC:

      /////////////////////////////////////////////////////////////////
      // Parse header

      case DEMUX_HEADER:
      {
        switch (header[3])
        {
          /////////////////////////////////////////////////////////////////
          // Program end code

          case 0xb9:
          {         
            data_size = 0;
            state = DEMUX_SYNC;
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

        state = DEMUX_DATA;
        frames++;
        // no break: now we go to DEMUX_DATA
      } // case DEMUX_HEADER:

      /////////////////////////////////////////////////////////////////
      // Extract data

      case DEMUX_DATA:
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
      } // case DEMUX_DATA:
    } // switch (state)
  } // while (1)
}


int 
PESDemux::packet(uint8_t *buf, int len, int *gone)
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
    state = DEMUX_DROP; \
    continue;           \
  }

  #define SYNC          \
  {                     \
    data_size = 0;      \
    required_size = 4;  \
    state = DEMUX_SYNC; \
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

      case DEMUX_DROP:
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
      } // case DEMUX_DROP:

      /////////////////////////////////////////////////////////////////
      // Sync

      case DEMUX_SYNC:
      {
        REQUIRE(4);
        uint32_t sync = swab32(*(uint32_t *)header);

        while ((sync & 0xffffff00) != 0x00000100 && len)
        {
          sync <<= 8;
          sync += *buf;
          buf++;
          len--;
          (*gone)++;
        }

        data_size = 4;
        *(uint32_t *)header = swab32(sync);

        if ((sync & 0xffffff00) == 0x00000100)
          state = DEMUX_HEADER;
        else
          return 0;

        // no break: now we go to DEMUX_HEADER
      } // case DEMUX_SYNC:

      /////////////////////////////////////////////////////////////////
      // Parse header

      case DEMUX_HEADER:
      {
        switch (header[3])
        {
          /////////////////////////////////////////////////////////////////
          // Program end code

          case 0xb9:
          {         
            data_size = 0;
            state = DEMUX_SYNC;
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
        state = DEMUX_SYNC;

        frames++;
        return size;
        // note: never be here
      } // case DEMUX_HEADER:

    } // switch (state)
  } // while (1)
}
