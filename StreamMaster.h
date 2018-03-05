#ifndef __STREAMMASTER__
#define __STREAMMASTER__

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
#include "PLUG_LoudnessMeter.h"

#define PLUG_DEFAULT_SAMPLERATE 44100.
#define PLUG_DEFAULT_CHANNEL_NUMBER 2


#define GR_BAR_DEFAULT_FG_ICOLOR IColor(255, 255, 50, 50)
#define GR_BAR_DEFAULT_NOTCH_ICOLOR IColor(255, 200, 0, 0)

class StreamMaster : public IPlug
{
public:
  StreamMaster(IPlugInstanceInfo instanceInfo);
  ~StreamMaster();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void UpdateGui();

private:

  Plug::LoudnessMeter* tLoudnessMeter;
  double mGain;
};


enum EParams
{
  kGain = 0,
  kModeSwitch = 1,
  kPlatformSwitch = 2,
  kILevelMeteringBar,
  kIGrMeteringBar,
  kNumParams,
  kInvisibleSwitchIndicator   // the user after kNumParams so they get a param id
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,

  // LUFS meter
  kLufsMeter_X = 420,
  kLufsMeter_Y = 197,

  // GR meter
  kGrMeter_X = 532,
  kGrMeter_Y = 197,
  
  // Peaking knob
  kGainX = 226,
  kGainY = 180,
  kKnobFrames = 11,

  // LUFS Text reading
  kILoudnessTextControl_X = 320,
  kILoudnessTextControl_Y = 700,
  kILoudnessTextControl_W = 280,
  kILoudnessTextControl_H = 20,
  
  // Learn-master-off
  kModeSwitch_N = 3,
  kModeSwitch_X = 45,
  kModeSwitch_Y = 180,
  
  // Youtube-Spotify-etc.
  kPlatformSwitch_N = 5,
  kPlatformSwitch_X = 226,
  kPlatformSwitch_Y = 480,
};

#endif
