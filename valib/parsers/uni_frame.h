/*
  Universal frame parser
*/

#ifndef UNI_FRAME_H
#define UNI_FRAME_H

#include "parsers\mpa\mpa_frame.h"
#include "parsers\ac3\ac3_frame.h"
#include "parsers\dts\dts_frame.h"

class UNIFrame : public FrameParser
{
public:
  UNIFrame();

  /////////////////////////////////////////////////////////
  // FrameParser overrides

  virtual const HeaderParser *header_parser();

  virtual void reset();
  virtual bool parse_frame(uint8_t *frame, size_t size);

  virtual Speakers  get_spk()      const { return spk;      }
  virtual samples_t get_samples()  const { return samples;  }
  virtual size_t    get_nsamples() const { return nsamples; }

  virtual size_t stream_info(char *buf, size_t size) const;
  virtual size_t frame_info(char *buf, size_t size) const;

protected:
  Speakers  spk;
  samples_t samples;
  int       nsamples;

  FrameParser *parser;

  MPAFrame mpa;
  AC3Frame ac3;
  DTSFrame dts;
};

#endif
