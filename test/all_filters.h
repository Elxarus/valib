/*
  Raw data
  ========
  Convert
  Counter
  Decoder
  Demux
  Spdifer

  Processing
  ==========
  AGC
  Mixer
  Delay
  BassRedir
  Levels
  Dejitter

  Aggregates
  ==========
  AudioProcessor
  FilterChain
  DVDDecoder
*/


// Raw data
#include "filters\convert.h"
#include "filters\counter.h"
#include "filters\decoder.h"
#include "filters\demux.h"
#include "filters\spdifer.h"
#include "parsers\ac3\ac3_enc.h"

// Processing
#include "filters\agc.h"
#include "filters\mixer.h"
#include "filters\delay.h"
#include "filters\bass_redir.h"
#include "filters\levels.h"
#include "filters\dejitter.h"

// Aggregates
#include "filters\proc.h"
#include "filters\filter_chain.h"
#include "filter_graph.h"
