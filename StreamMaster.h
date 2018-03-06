#ifndef __STREAMMASTER__
#define __STREAMMASTER__

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
#include "PLUG_LoudnessMeter.h"

#define PLUG_DEFAULT_SAMPLERATE 44100.
#define PLUG_DEFAULT_CHANNEL_NUMBER 2

#define LOG_TO_LINEAR(v) pow(10, v/20.)
#define LINEAR_TO_LOG(v) (20.*log10(v))

const IColor GR_BAR_DEFAULT_FG_ICOLOR(255, 200, 0, 0);
const IColor GR_BAR_DEFAULT_NOTCH_ICOLOR(255, 128, 0, 0);
const IColor PLUG_METER_TEXT_LABEL_COLOR(255, 255, 255, 255);
const IColor PLUG_KNOB_TEXT_LABEL_COLOR(255, 84, 84, 84);
const IColor PLUG_GUIDE_TEXT_LABEL_COLOR(255, 200, 200, 200);

#define PLUG_METER_TEXT_LABEL_STRING_SIZE 32
#define PLUG_METER_TEXT_LABEL_FONT_SIZE 12

#define PLUG_KNOB_TEXT_LABEL_STRING_SIZE 16
#define PLUG_KNOB_TEXT_LABEL_FONT_SIZE 12

#define PLUG_MODE_TEXT_LABEL_STRING_SIZE 512
#define PLUG_GUIDE_TEXT_LABEL_STRING_SIZE PLUG_MODE_TEXT_LABEL_STRING_SIZE
#define PLUG_GUIDE_TEXT_LABEL_FONT_SIZE 14

#define PLUG_VERSION_TEXT_LABEL_STRING_SIZE 32
#define PLUG_VERSION_TEXT_LABEL_FONT_SIZE 10

#define PLUG_KNOB_PEAK_MIN 0
#define PLUG_KNOB_PEAK_MAX 10
#define PLUG_KNOB_PEAK_DEFAULT 7
#define PLUG_KNOB_PEAK_DOUBLE(i) (-1.+(i/10.))

// Aftermath of tweaking UI size
#define PLUG_Y_OFFSET (27)

#define PLUG_DEFAULT_PRESET_NAME "Default"
#define PLUG_LIMITER_ATTACK_MILLISECONDS 0.1
#define PLUG_LIMITER_DEFAULT_THRESHOLD_DB 0.;

#define PLUG_LUFS_RANGE_MIN -40.
#define PLUG_LUFS_RANGE_MAX 3.

#define PLUG_GR_RANGE_MIN -0.
#define PLUG_GR_RANGE_MAX -43.

/* Defaults for:
  fMasteringGainDb,
  fTargetLufsIntegratedDb,
  fSourceLufsIntegratedDb,
  fLimiterCeilingDb,
  fMasteringGainLinear,
  fMasteringGainLinear
*/
#define PLUG_MASTERING_GAIN_DB_RESET 0.
#define PLUG_TARGET_LUFS_INTERGRATED_DB_RESET PLUG_LUFS_RANGE_MAX
#define PLUG_SOURCE_LUFS_INTERGRATED_DB_RESET -60.
#define PLUG_LIMITER_CEILING_DB_RESET 0.
#define PLUG_MASTERING_GAIN_LINEAR_RESET 1.
#define PLUG_LIMITER_CEILING_LINEAR_RESET 1.

#define PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET -0.
#define PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET -0.

#define PLUG_LUFS_TOO_LOW -500.

#define PLUG_OFF_STARTUP_MESSAGE \
"\nPress Mode switch to start adjusting song's loudness"

#define PLUG_OFF_GUIDE_MESSAGE \
"\nI'm just chilling now. Press Mode switch again\n\
to do another song or measurement."

#define PLUG_MASTER_GUIDE_MESSAGE \
"I'm ready to process the track! Mastering for:\n\
%s\n\
Input loudness: %0.2fLUFS, Target loudness: up to %0.2fLUFS\n\
Limiter ceiling: %0.2fdB, applying %0.2fdB of pre-limiter gain"

#define PLUG_LEARN_GUIDE_MESSAGE \
"Press Play in your DAW to let me measure the song's loudness,\n\
then stop the playback and press Mode switch to jump into mastering. \n\
Play me the whole song if you need precision,\n\
or just show me the loudest section if you\'re in a hurry. "

#define PLUG_TOO_QUIET_GUIDE_MESSAGE \
"Source is too quiet, or you didn't show me the source.\n\
Please repeat learning cycle again. \n\
(press Mode switch twice to go to learning mode)"


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
  kLufsMeter_Y = 197+PLUG_Y_OFFSET,

  // GR meter
  kGrMeter_X = 532,
  kGrMeter_Y = 197+PLUG_Y_OFFSET,
  
  // Peaking knob
  kGainX = 226,
  kGainY = 180+PLUG_Y_OFFSET,
  kKnobFrames = 11,

  // LUFS Text reading
  kILoudnessTextControl_X = 414,
  kILoudnessTextControl_Y = 692+PLUG_Y_OFFSET,
  kILoudnessTextControl_W = 80,
  kILoudnessTextControl_H = 40,

  // Gain reduction Text reading
  kIGrTextControl_X = 526,
  kIGrTextControl_Y = kILoudnessTextControl_Y,
  kIGrTextControl_W = 80,
  kIGrTextControl_H = 40,

  // Peaking knob Text reading
  kIPeakingTextControl_X = 260,
  kIPeakingTextControl_Y = 165+PLUG_Y_OFFSET,
  kIPeakingTextControl_W = 80,
  kIPeakingTextControl_H = 20,
  
  // Mode text guide
  kIModeTextControl_X = 0,
  kIModeTextControl_Y = 95+PLUG_Y_OFFSET,
  kIModeTextControl_W = GUI_WIDTH,
  kIModeTextControl_H = 20,

  // Learn-master-off
  kModeSwitch_N = 3,
  kModeSwitch_X = 45,
  kModeSwitch_Y = kGainY,
  
  // Youtube-Spotify-etc.
  kPlatformSwitch_N = 5,
  kPlatformSwitch_X = 226,
  kPlatformSwitch_Y = 480+PLUG_Y_OFFSET,

  // Text version label
  kTextVersion_X = 5,
  kTextVersion_Y = GUI_HEIGHT-15,
  kTextVersion_W = 64,
  kTextVersion_H = 20,
  kTextVersion_ColorMono = 64,
};

// Loudness stuff
#define PLUG_PLATFORM_OPTIONS 5
double afTargetLufs[PLUG_PLATFORM_OPTIONS] = {
  -23., /* #0 Broadcast - EBU R128 guideline */
  -16., /* #1 Apple Music, SoundCheck, AES */
  -6.,  /* #2 Mp3, Soundcloud, radio etc. But not really.  */
  -14., /* #3 Spotify & Tidal */
  -13.  /* #4 Youtube */
};
char *asTargetDescription[]={
  "Podcasts, dialogue videos etc. For sources that are mostly speech. ",
  "Apple Music, SoundCheck standard, and general MP3. AES guideline for music loudness level. ",
  "Platforms that don't manage loudness and let you get away with extremely loud audio",
  "Spotify and Tidal",
  "Youtube music videos"
};
#define PLUG_DEFAULT_TARGET_PLATFORM 1
#define PLUG_GET_TARGET_LOUDNESS(i) (afTargetLufs[i])
#define PLUG_DEFAULT_TARGET_LOUDNESS PLUG_GET_TARGET_LOUDNESS(PLUG_DEFAULT_TARGET_PLATFORM)

typedef enum{
  PLUG_LEARN_MODE = 1,
  PLUG_MASTER_MODE = 2,
  PLUG_OFF_MODE = 3
}PLUG_Mode;

#define PLUG_INITIAL_MODE PLUG_OFF_MODE
#define PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(idx) \
  ((PLUG_Mode)int(GetParam(idx)->Value()+1))

#define PLUG_CONVERT_PLUG_MODE_TO_SWITCH_VALUE(m) (m-1)

#endif
