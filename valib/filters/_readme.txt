This directory contains audio processing filters (Filter class descendants)

Top-level filters are:
  DVDDecoder: complete audio decoder and processor
  AudioDecoder: universal audio decoder
  AudioProcessor: all-in-one audio processor

All filters:
* AGC (agc.h): Auto gain control filter. Gain, overflow protection, clipping, 
  dynamic range compression, one-pass normalization.

* BassRedir (bass_redir.h): Bass redirection filter. Copies all basses
  to subwoofer channel

* Convert (convert.h): conversions between PCM audio formats and Linear 
  (separated channels) format.

* AudioDecoder (decoder.h): unversal audio decoder filter. Compresed stream
  at input linear or spdif at output.

* Dejitter (dejitter.h): Removes input timestamps jitter

* Delay (delay.h): per-channel audio delay. Supports different units:
  samples, ms, m, cm, in, ft.

* Demux (demux.h): demuxes container streams

* DVDDecoder (dvd_decoder.h): Complete audio decoder and processor.
  Now supports AC3, MPEGAudio and MPEG1/2 PES with AC3, MPEG Audio and LPCM
  streams. SPDIF passthough ability.

* FilterChain (filter_chain.h): represents filter sequence as one filter

* Levels (levels.h): Report about current audio levels. Supports levels 
  caching to sycronize levels display with audio output.

* Mixer (mixer.h): matrix mixer. Multiply input samples by conversion matrix.
  Allows to change number of channels. Automatically calculates matrices for
  common transforms.

* AudioProcessor (proc.h): audio processor. input/output format conversions,
  AGC, Mixer, Delay, input/output levels, channel reorder.

* Spdifer (spdifer.h): Encapsulates compressed stream in SPDIF 
  according to IEC 61937


                  IP  BUF
AGC                -   +
BassRedir          +   -
Convert            -   -
AudioDecoder           
Dejitter           +   -
Delay              +   +
Demux
DVDDecoder
FilterChain
Levels             +   -
Mixer             +/-  +
AudioProcessor
Spdifer            -