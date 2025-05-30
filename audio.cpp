#include "basetypes.h"
#include "NuonEnvironment.h"
#include "audio.h"
#include "NuonMemoryMap.h"

/*
  The Nuon uses the Burr-Brown 1723 DAC chip for audio playback.  Burr-Brown
  has merged with Texas Instruments and this chip is now sold as the PCM1723.
*/
extern NuonEnvironment nuonEnv;

//((float)(nuonAudioBufferSize >> (nuonAudioStreamModeShift + nuonAudioInterruptRateShift)))/nuonPlaybackRate

void AudioMute(MPE &mpe)
{  
  //Mute audio
  nuonEnv.MuteAudio(mpe.regs[0]);
}

void AudioReset(MPE &mpe)
{
  nuonEnv.RestartAudio();
  nuonEnv.pNuonAudioBuffer = nullptr;
  nuonEnv.MuteAudio(true);
}

void AudioQuerySampleRates(MPE &mpe)
{
  mpe.regs[0] = nuonEnv.nuonSupportedPlaybackRates;
}

void AudioSetSampleRate(MPE &mpe)
{
  const uint32 newRate = mpe.regs[0];

  if(!(newRate & nuonEnv.nuonSupportedPlaybackRates))
    return;

  // NISE always triggers 16bit stereo at 32kHz per its spec

  // search for bits from most significant bit onwards, as specified
  if(newRate & RATE_16_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 16000;
  else if(newRate & RATE_64_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 64000;
  else if(newRate & RATE_32_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 32000;
  else if(newRate & RATE_24_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 24000;
  else if(newRate & RATE_96_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 96000;
  else if(newRate & RATE_48_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 48000;
  else if(newRate & RATE_22_05_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 22050;
  else if(newRate & RATE_88_2_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 88200;
  else if(newRate & RATE_44_1_KHZ)
    nuonEnv.nuonAudioPlaybackRate = 44100;
  else
    //default to 32KHZ
    nuonEnv.nuonAudioPlaybackRate = 32000;

  nuonEnv.SetAudioPlaybackRate();
}

void AudioQueryChannelMode(MPE &mpe)
{
  mpe.regs[0] = nuonEnv.nuonAudioChannelMode;
}

void AudioSetChannelMode(MPE &mpe)
{
  const uint32 newMode = mpe.regs[0];

  // if only these 2 flags change, do not re-init things
  if((nuonEnv.nuonAudioChannelMode & ~(ENABLE_WRAP_INT | ENABLE_HALF_INT)) == (newMode & ~(ENABLE_WRAP_INT | ENABLE_HALF_INT)))
  {
    nuonEnv.nuonAudioChannelMode = newMode;
    return;
  }

  // NISE always triggers 16bit stereo at 32kHz per its spec

  assert((newMode & STREAM_FOUR_16_BIT) == 0);
  assert((newMode & STREAM_TWO_32_BIT) == 0);
  assert((newMode & STREAM_EIGHT_16_BIT) == 0);
  assert((newMode & STREAM_EIGHT_32_BIT) == 0);
  assert((newMode & STREAM_FOUR_32_BIT) == 0);

  nuonEnv.nuonAudioChannelMode = newMode;
  nuonEnv.nuonAudioBufferSize = nuonEnv.GetBufferSize(newMode);
  if(newMode & ENABLE_SAMP_INT)
    nuonEnv.cyclesPerAudioInterrupt = 54000000/nuonEnv.nuonAudioPlaybackRate;

  nuonEnv.RestartAudio();
  //nuonEnv.SetAudioVolume(255); //!! not necessary as never changed anywhere?!
}

void AudioSetDMABuffer(MPE &mpe)
{
  const uint32 pAudioBuffer = mpe.regs[0];
  
  //if(!pAudioBuffer)
  //{
  //  pAudioBuffer = pAudioBuffer;
  //}

  if((pAudioBuffer >= MPE_ADDR_SPACE_BASE) && (pAudioBuffer < SRAM_0_BASE))
    nuonEnv.pNuonAudioBuffer = (uint8 *)nuonEnv.GetPointerToMemory(mpe.mpeIndex, pAudioBuffer, false);
}
