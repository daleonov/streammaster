#ifndef __STREAMMASTER__
#define __STREAMMASTER__

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
#include "PLUG_LoudnessMeter.h"

#define PLUG_DEFAULT_SAMPLERATE 44100.
#define PLUG_DEFAULT_CHANNEL_NUMBER 2


#define LOG_TO_LINEAR(v) pow(10, v/20.)
#define LINEAR_TO_LOG(v) (20.*log10(v))


#define GR_BAR_DEFAULT_FG_ICOLOR IColor(255, 200, 0, 0)
#define GR_BAR_DEFAULT_NOTCH_ICOLOR IColor(255, 128, 0, 0)

#define PLUG_METER_TEXT_LABEL_COLOR IColor(255, 255, 255, 255)
#define PLUG_METER_TEXT_LABEL_STRING_SIZE 32
#define PLUG_METER_TEXT_LABEL_FONT_SIZE 12

#define PLUG_KNOB_TEXT_LABEL_COLOR IColor(255, 84, 84, 84)
#define PLUG_KNOB_TEXT_LABEL_STRING_SIZE 16
#define PLUG_KNOB_TEXT_LABEL_FONT_SIZE 12

#define PLUG_KNOB_PEAK_MIN 0
#define PLUG_KNOB_PEAK_MAX 10
#define PLUG_KNOB_PEAK_DEFAULT 7
#define PLUG_KNOB_PEAK_DOUBLE(i) (-1.+(i/10.))

#define PLUG_MODE_TEXT_LABEL_STRING_SIZE 256

class StreamMaster : public IPlug
{
public:
  StreamMaster(IPlugInstanceInfo instanceInfo);
  ~StreamMaster();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void UpdateGui();
  void UpdateAvailableControls();
  void UpdatePreMastering();

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
  kIPeakingTextControl,
  kIModeTextControl,
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
  kGainY = 185,
  kKnobFrames = 11,

  // LUFS Text reading
  kILoudnessTextControl_X = 414,
  kILoudnessTextControl_Y = 695,
  kILoudnessTextControl_W = 80,
  kILoudnessTextControl_H = 40,

  // Gain reduction Text reading
  kIGrTextControl_X = 526,
  kIGrTextControl_Y = 695,
  kIGrTextControl_W = 80,
  kIGrTextControl_H = 40,

  // Peaking knob Text reading
  kIPeakingTextControl_X = 260,
  kIPeakingTextControl_Y = 170,
  kIPeakingTextControl_W = 80,
  kIPeakingTextControl_H = 20,
  
  // Mode text guide
  kIModeTextControl_X = 0,
  kIModeTextControl_Y = 130,
  kIModeTextControl_W = GUI_WIDTH,
  kIModeTextControl_H = 20,

  // Learn-master-off
  kModeSwitch_N = 3,
  kModeSwitch_X = 45,
  kModeSwitch_Y = 180,
  
  // Youtube-Spotify-etc.
  kPlatformSwitch_N = 5,
  kPlatformSwitch_X = 226,
  kPlatformSwitch_Y = 480,
};

// Loudness stuff
#define PLUG_PLATFORM_OPTIONS 5
double afTargetLufs[PLUG_PLATFORM_OPTIONS] = {
  -23., /* #0 Broadcast - EBU R128 guideline */
  -16., /* #1 Apple Music, SoundCheck, AES */
  -14., /* #2 Tidal */
  -14., /* #3 Spotify */
  -13.  /* #4 Youtube */
};
#define PLUG_DEFAULT_TARGET_PLATFORM 4
#define PLUG_GET_TARGET_LOUDNESS(i) (afTargetLufs[i])
#define PLUG_DEFAULT_TARGET_LOUDNESS PLUG_GET_TARGET_LOUDNESS(PLUG_DEFAULT_TARGET_PLATFORM)

typedef enum{
  PLUG_LEARN_MODE = 1,
  PLUG_MASTER_MODE = 2,
  PLUG_OFF_MODE = 3
}PLUG_Mode;

#define PLUG_INITIAL_MODE 1

#endif
