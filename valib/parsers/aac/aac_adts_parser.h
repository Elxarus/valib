#ifndef VALIB_AAC_ADTS_PARSER_H
#define VALIB_AAC_ADTS_PARSER_H

#include "../../parser.h"

class ADTSParser : public FrameParser
{
public:
  ADTSParser();
  ~ADTSParser();

  /////////////////////////////////////////////////////////
  // FrameParser overrides

  virtual const HeaderParser *header_parser() const;

  virtual void reset();
  virtual bool process(uint8_t *frame, size_t size);

  virtual Speakers  get_output()   const { return spk;          }
  virtual samples_t get_samples()  const { samples_t samples; samples.zero(); return samples; }
  virtual size_t    get_nsamples() const { return 0;            }
  virtual uint8_t  *get_rawdata()  const { return data;         }
  virtual size_t    get_rawsize()  const { return data_size;    }

  virtual string info() const;

protected:
  Speakers  spk;
  uint8_t  *data;
  size_t    data_size;
};

#endif
