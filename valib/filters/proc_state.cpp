#include "proc_state.h"

AudioProcessorState::AudioProcessorState()
{
  eq_master_freq = 0;
  eq_master_gain = 0;
  for (int ch = 0; ch < NCHANNELS; ch++)
  {
    eq_freq[ch] = 0;
    eq_gain[ch] = 0;
  }  
}

AudioProcessorState::~AudioProcessorState()
{
  safe_delete(eq_master_freq);
  safe_delete(eq_master_gain);
  for (int ch = 0; ch < NCHANNELS; ch++)
  {
    safe_delete(eq_freq[ch]);
    safe_delete(eq_gain[ch]);
  }  
}
