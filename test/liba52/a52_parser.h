#ifndef A52_PARSER_H

#include "parser.h"
#include "a52.h"

class A52Parser : public Parser
{
public:
  a52_state_t *a52_state;
  int mode;
  int sample_rate;

  enum { state_sync, state_load_frame, state_decode } state;
  uint8_t frame_buf[4192];
  int frame_size; // frame size
  int frame_data; // size of data in frame_buf

  Speakers  spk;
  SampleBuf samples;

public:
  int frames;
  int errors;

  A52Parser();
  ~A52Parser();

  /////////////////////////////////////////////////////////
  // Parser interface

  virtual void reset();

  // load/decode frame
  virtual unsigned load_frame(uint8_t **buf, uint8_t *end);
  virtual void     drop_frame() { frame_data = 0; state = state_sync; }
  virtual bool     is_frame_loaded() const { return frame_data && (frame_data >= frame_size); }
  virtual bool     decode_frame();

  // Stream information
  virtual Speakers get_spk()        const { return spk;        }
  virtual unsigned get_frame_size() const { return frame_size; }
  virtual unsigned get_nsamples()   const { return 1536; }
  virtual void     get_info(char *, unsigned) const {}

  virtual unsigned get_frames()     const { return frames;     }
  virtual unsigned get_errors()     const { return errors;     }

  // Buffers
  virtual uint8_t *get_frame()      const { return (uint8_t*)frame_buf;  }
  virtual samples_t get_samples()   const { return samples;    }
};

#endif