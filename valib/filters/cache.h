/**************************************************************************//**
  \file cache.h
  \brief CacheFilter: Cache data to synchronize audio visualization with actual
  playback.
******************************************************************************/

#ifndef VALIB_CACHE_H
#define VALIB_CACHE_H

#include "../filter.h"
#include "../buffer.h"

/**************************************************************************//**
  \class CacheFilter
  \brief Cache data to synchronize audio visualization with actual playback.

  Due to buffering done by playback device, audio visualization is not
  synchronized with audio playback. To show current activity we should access
  data back in time. This filter buffers audio data and allows to access
  the moment we currently play. 

  \fn vtime_t CacheFilter::get_time() const;
    Returns the time of the last sample passed through the filter.

  \fn vtime_t CacheFilter::get_size() const;
    Returns the current cache size (in time units).

  \fn void CacheFilter::set_size(vtime_t size);
    \param size New buffer size.

    [Re]allocate the buffer.

    When buffer size decreases, the data in the buffer that fits the new size
    remains unchanged.

  \fn size_t CacheFilter::get_samples(int ch_name, vtime_t time, sample_t *buf, size_t size);
    \param ch_name Channel to access
    \param time    Time to return the data from
    \param buf     Output buffer
    \param size    Output buffer size on samples

    Copy cached data into the output buffer. Only one channel is copyed,
    \c ch_name defines this channel. Channel is defined by \b name, not by
    index. Also, you can get the sum of all channels by specifying CH_NONE.

    Example: Get LFE channel (if exists)
    \code
      Samples buf(buf_size);
      Cache cache(cache_size);
      ...
      size_t data_size = cache.get(CH_LFE, time, buf.data(), buf.size());
    \endcode

    When channel does not exist this function returns zero.

    The time window to retrieve the data from is defined by the starting time
    (\c time) and the number of samples to return (\c size).

    The time window is adjusted to fit the actual time window of the buffered
    data:
    \verbatim
     | time window requested |
            | time window returned  |
    --------|----------------------------------|-----------------> time
            v                                  v
            buffer limit                       last data buffered
            get_time() - get_size()            get_time()
    \endverbatim

    If the number of samples requested is larger than the amount of data
    buffered, the time window shrinks and the function returns the actual
    number of samples returned.

    Returns actual number of samples copied.
******************************************************************************/

class CacheFilter : public SamplesFilter
{
protected:
  vtime_t stream_time; //!< Time after the last cached sample
  SampleBuf buf;       //!< Cache buffer (circular buffer)

  vtime_t buf_size;    //!< Size of the buffer in time units
  int buf_samples;     //!< Size fo the buffer in samples
  int cached_samples;  //!< Number of samples cached
  int pos;             //!< Position of the end of the circular buffer

public:
  CacheFilter();
  CacheFilter(vtime_t size);

  void    set_size(vtime_t size);
  vtime_t get_size() const;
  size_t  get_nsamples() const;

  vtime_t get_time() const;
  size_t  get_samples(int ch_name, vtime_t time, sample_t *buf, size_t size);

  /////////////////////////////////////////////////////////
  // SamplesFilter overrides

  virtual bool init();
  virtual void uninit();
  virtual void reset();
  virtual bool process(Chunk &in, Chunk &out);
};

#endif
