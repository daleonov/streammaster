#ifndef __STREAMMASTER__
#define __STREAMMASTER__

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
#include "PLUG_LoudnessMeter.h"
#include "PLUG_IControlExtras.h"
#include "resource.h"
#include "lib_chunkware/SimpleLimit.h"

// If this file is missing, run "git_version" script from project folder,
// or create an empty file if you don't use git. 
#include "PLUG_Version.h"

/* Build-related macros - start */

// Uncomment following line if you want to tinker with controls
// in every mode (for debug purposes)
//#define PLUG_DO_NOT_BLOCK_CONTROLS

// Set to false if you want to force user to let the plug learn
// song loudness properly to allow him to go into mastering mode
#define PLUG_ALWAYS_ALLOW_MASTERING true

/********************************************************************
Other application level macros dependencies:

__APPLE__ or _WIN32 - different text settings for each OS

_PLUG_VERSION_H - if the version header is generated correctly,
the plugin version is displayed in the GUI
(see git_version.bat and git_version.sh scripts)

********************************************************************/

/* Build-related macros - end */

#define PLUG_DEFAULT_SAMPLERATE 44100.
#define PLUG_DEFAULT_CHANNEL_NUMBER 2

#define LOG_TO_LINEAR(v) pow(10, v/20.)
#define LINEAR_TO_LOG(v) (20.*log10(v))

const IColor GR_BAR_DEFAULT_FG_ICOLOR(255, 255, 0, 49);
const IColor GR_BAR_DEFAULT_NOTCH_ICOLOR(255, 128, 0, 0);
const IColor PLUG_METER_TEXT_LABEL_COLOR(255, 255, 255, 255);
const IColor PLUG_KNOB_TEXT_LABEL_COLOR(255, 110, 110, 110);
const IColor PLUG_GUIDE_TEXT_LABEL_COLOR(255, 255, 255, 255);
const IColor LOUDNESS_BAR_FG_ICOLOR(255, 0, 184, 67);

const IColor PLUG_TP_LABEL_OK_COLOR(255, 0, 184, 67);
const IColor PLUG_TP_LABEL_ALERT_COLOR(255, 255, 0, 49);

/*
Those are the lengths for classic "char[]" strings,
so make sure keep them up to date whith whatever
strings you want them to fit
*/
#define PLUG_METER_TEXT_LABEL_STRING_SIZE 64
#define PLUG_KNOB_TEXT_LABEL_STRING_SIZE 16
#define PLUG_MODE_TEXT_LABEL_STRING_SIZE 512
#define PLUG_GUIDE_TEXT_LABEL_STRING_SIZE PLUG_MODE_TEXT_LABEL_STRING_SIZE
#define PLUG_VERSION_TEXT_LABEL_STRING_SIZE 96
#define PLUG_TP_LABEL_STRING_SIZE 32

#ifdef _WIN32
#define PLUG_KNOB_TEXT_LABEL_FONT_SIZE 12
#define PLUG_METER_TEXT_LABEL_FONT_SIZE 12
#define PLUG_GUIDE_TEXT_LABEL_FONT_SIZE 14
#define PLUG_VERSION_TEXT_LABEL_FONT_SIZE 14
#elif defined(__APPLE__)
#define PLUG_KNOB_TEXT_LABEL_FONT_SIZE 13
#define PLUG_METER_TEXT_LABEL_FONT_SIZE 13
#define PLUG_GUIDE_TEXT_LABEL_FONT_SIZE 15
#define PLUG_VERSION_TEXT_LABEL_FONT_SIZE 15
#endif
#define PLUG_TP_LABEL_FONT_SIZE PLUG_METER_TEXT_LABEL_FONT_SIZE

/* Knob values are not real-life units,
use PLUG_KNOB_PEAK_DOUBLE() to convert them to linear gain*/
#define PLUG_KNOB_PEAK_MIN -20
#define PLUG_KNOB_PEAK_MAX 0
#define PLUG_KNOB_PEAK_DEFAULT -10
#define PLUG_KNOB_PEAK_SCALE 10.
#define PLUG_KNOB_PEAK_DOUBLE(i) ((double)i/PLUG_KNOB_PEAK_SCALE)

// Adjust knob values, these are real life ones (LUFS)
/* Step doesn't seep to matter in this version of framework,
   so we have to round it via PLUG_KNOB_ADJUST_ROUND every time we read it */
#define PLUG_KNOB_ADJUST_MIN -6.
#define PLUG_KNOB_ADJUST_MAX 6.
#define PLUG_KNOB_ADJUST_STEP .1
#define PLUG_KNOB_ADJUST_DEFAULT 0.
#define PLUG_KNOB_ADJUST_ROUND(i) (std::floor((i * 10.) + .5) / 10.)

#define PLUG_DEFAULT_PRESET_NAME "Default"
#define PLUG_LIMITER_ATTACK_MILLISECONDS 0.1
#define PLUG_LIMITER_DEFAULT_THRESHOLD_DB 0.

#define PLUG_LUFS_RANGE_MIN -40.
#define PLUG_LUFS_RANGE_MAX 3.
#define PLUG_LUFS_NORMALIZED(l) \
(((double)(l - PLUG_LUFS_RANGE_MIN)) / (PLUG_LUFS_RANGE_MAX - PLUG_LUFS_RANGE_MIN))

#define PLUG_GR_RANGE_MIN -0.
#define PLUG_GR_RANGE_MAX -43.

#define PLUG_METERING_BAR_W 74
#define PLUG_METERING_BAR_H 537
#define PLUG_METERING_BAR_IRECT IRECT(0, 0, PLUG_METERING_BAR_W, PLUG_METERING_BAR_H)

#define PLUG_TP_ALERT_VALUE_DB -0.
#define PLUG_TP_LABEL_DEFAULT_TEXT "-"

/* Defaults for:
  fMasteringGainDb,
  fTargetLufsIntegratedDb,
  fSourceLufsIntegratedDb,
  fLimiterCeilingDb,
  fLimiterCeilingLinear,
  fMasteringGainLinear
*/
#define PLUG_MASTERING_GAIN_DB_RESET 0.
#define PLUG_TARGET_LUFS_INTEGRATED_DB_RESET PLUG_LUFS_RANGE_MAX
#define PLUG_SOURCE_LUFS_INTEGRATED_DB_RESET -60.
#define PLUG_LIMITER_CEILING_DB_RESET 0.
#define PLUG_MASTERING_GAIN_LINEAR_RESET 1.
#define PLUG_LIMITER_CEILING_LINEAR_RESET 1.

#define PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET -0.
#define PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET -0.

// TODO: setting it to -INF would be smarter, but would need careful testing
#define PLUG_LUFS_TOO_LOW -500.
#define PLUG_EPSILON (std::numeric_limits<double>::epsilon())

#ifdef _PLUG_VERSION_H 
#define PLUG_VERSTION_TEXT \
"StreamMaster v%s\n\
by Daniel Leonov Plugs\n\
danielleonovplugs.com\n\
(%s@%s)"
#else
"StreamMaster v%s\n\
by Daniel Leonov Plugs\n
danielleonovplugs.com"
#endif

#define PLUG_OFF_STARTUP_MESSAGE \
"\nPress Mode switch to start adjusting song's loudness"

#define PLUG_OFF_GUIDE_MESSAGE \
"\nI'm just chilling now. Press Mode switch again\n\
to do another song or measurement."

#define PLUG_MASTER_GUIDE_MESSAGE \
"I'm ready to process the track! Mastering for:\n\
%s\n\
Input loudness: %0.2fLUFS, Target loudness: up to %0.2fLUFS\n\
Limiter ceiling: %5.2fdB, applying %0.2fdB of pre-limiter gain"

#define PLUG_LEARN_GUIDE_MESSAGE \
"Press Play in your DAW to let me measure the song's loudness,\n\
then stop the playback and press Mode switch to jump into mastering. \n\
Play me the whole song if you need precision,\n\
or just show me the loudest section if you\'re in a hurry. "

#define PLUG_TOO_QUIET_GUIDE_MESSAGE \
"Source is too quiet, or you didn't show me the source.\n\
Please repeat learning cycle again. \n\
(press Mode switch twice to go to learning mode)"

char* sBypassString = "\nBypassed";

#define PLUG_GUIDE_TEXT_ALIGNMENT kAlignNear
#define PLUG_TP_TEXT_ALIGNMENT kAlignCenter

#ifdef _WIN32
#define PLUG_METER_TEXT_ALIGNMENT kAlignFar
#elif defined(__APPLE__)
#define PLUG_METER_TEXT_ALIGNMENT kAlignNear
#endif

enum EParams
{
  kBypassSwitch,
  kAdjust,
  kCeiling,
  kModeSwitch,
  kPlatformSwitch,
  kIGrContactControl,
  kILufsContactControl,
  kILevelMeteringBar,
  kIGrMeteringBar,
  kPlatformSwitchClickable,
  kNumParams, /* Anything below that line will be non-automatable */
  kIPeakingTextControl,
  kAdjustTextControl,
  kIModeTextControl,
  kTpTextControl,
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,

  // LUFS meter
  kLufsMeter_X = 665,
  kLufsMeter_Y = 12,

  // GR meter
  kGrMeter_X = kLufsMeter_X + 87,
  kGrMeter_Y = kLufsMeter_Y,
  
  // Peaking knob
  kCeilingX = 482,
  kCeilingY = 90,
  kKnobFrames = 21,

  // Bypass switch
  kBypassSwitchX = 122,
  kBypassSwitchY = 90,
  kBypassSwitchFrames = 2,

  // Adjust knob
  kAdjustX = kCeilingX,
  kAdjustY = 365,
  kAdjustFrames = 100,

#ifdef _WIN32
  // LUFS Text reading
  kILoudnessTextControl_W = 80,
  kILoudnessTextControl_H = 40,
  kILoudnessTextControl_X = kLufsMeter_X - (kILoudnessTextControl_W - PLUG_METERING_BAR_W),
  kILoudnessTextControl_Y = 554,

  // Gain reduction Text reading
  kIGrTextControl_W = 80,
  kIGrTextControl_H = 40,
  kIGrTextControl_X = kGrMeter_X - (kIGrTextControl_W - PLUG_METERING_BAR_W),
  kIGrTextControl_Y = kILoudnessTextControl_Y,

#elif defined(__APPLE__)
  // LUFS Text reading
  kILoudnessTextControl_X = kLufsMeter_X,
  kILoudnessTextControl_Y = 554,
  kILoudnessTextControl_W = 80,
  kILoudnessTextControl_H = 40,

  // Gain reduction Text reading
  kIGrTextControl_X = kGrMeter_X,
  kIGrTextControl_Y = kILoudnessTextControl_Y,
  kIGrTextControl_W = 80,
  kIGrTextControl_H = 40,

#endif

  // Mode text guide
  kIModeTextControl_X = 23,
  kIModeTextControl_Y = 15,
  kIModeTextControl_W = kWidth,
  kIModeTextControl_H = 20,

  // Peaking knob Text reading
  kIPeakingTextControl_X = kCeilingX,
  kIPeakingTextControl_Y = 78,
  kIPeakingTextControl_W = 155,
  kIPeakingTextControl_H = 20,

  // Adjust knob Text reading
  kAdjustTextControlX = kAdjustX,
  kAdjustTextControlY = 353,
  kAdjustTextControlW = kIPeakingTextControl_W,
  kAdjustTextControlH = kIPeakingTextControl_H,

  // Learn-master-off
  kModeSwitch_N = 3,
  kModeSwitch_X = 302,
  kModeSwitch_Y = kCeilingY,
  
  /* Switches for Youtube-Spotify-etc. - start */
  // Mode switch, rotatable
  kPlatformSwitch_N = 5,
  kPlatformSwitch_X = 302,
  kPlatformSwitch_Y = 366,
  // Mode switch, clickable
  kPlatformSwitchClickable_N = 2,
  kPlatformSwitchClickable_W = 159,  // width of bitmap
  kPlatformSwitchClickable_H = 59,  // height of one of the bitmap images
  kPlatformSwitchClickable_X = 105,
  kPlatformSwitchClickable_Y = 293,
  kPlatformSwitchClickable_TOTAL = 5, // total number of radio buttons
  // Grey out overlay over platform names
  kModeRadioSwitchGreyOut_X = kPlatformSwitchClickable_X,
  kModeRadioSwitchGreyOut_Y = kPlatformSwitchClickable_Y,
  /* Switches for Youtube-Spotify-etc. - end */

  // Text version label
  kTextVersion_W = 128,
  kTextVersion_H = 12,
  kTextVersion_X = 480,
  kTextVersion_Y = 15,
  kTextVersion_ColorMono = 110,

  // Meter reset switch
  kIContactControl_N = 2,

  // Meter overlays
  kLoudnessLabelOverlay_X = kLufsMeter_X + 35,
  kLoudnessLabelOverlay_Y = 440,

  kGrLabelOverlay_X = kGrMeter_X + 35,
  kGrLabelOverlay_Y = 383,
};

const IRECT PLUG_TP_LABEL_IRECT(
	kLufsMeter_X,
	kLufsMeter_Y+5,
	kLufsMeter_X + 74,
	kLufsMeter_Y+25
	);

const IRECT tPlatformSwitchClickableIRect(
	kPlatformSwitchClickable_X,
	kPlatformSwitchClickable_Y,
	kPlatformSwitchClickable_X + kPlatformSwitchClickable_W,
	kPlatformSwitchClickable_Y + kPlatformSwitchClickable_H * kPlatformSwitchClickable_TOTAL
	);

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
  "Apple Music, SoundCheck standard, and general MP3. AES guideline. ",
  "Platforms that don't manage loudness",
  "Spotify and Tidal",
  "Youtube music videos"
};
char *asTargetNames[] = {
  "Podcasts and general broadcast",
  "Apple Music, SoundCheck, AES",
  "Radio and MP3",
  "Spotify and Tidal",
  "Youtube",
};
#define PLUG_DEFAULT_TARGET_PLATFORM 1
#define PLUG_GET_TARGET_LOUDNESS(i) (afTargetLufs[i])
#define PLUG_DEFAULT_TARGET_LOUDNESS PLUG_GET_TARGET_LOUDNESS(PLUG_DEFAULT_TARGET_PLATFORM)

char *asModeNames[] = {
  "Learn",
  "Master",
  "Off",
};

typedef enum{
  PLUG_LEARN_MODE = 1,
  PLUG_MASTER_MODE = 2,
  PLUG_OFF_MODE = 3
}PLUG_Mode;

typedef enum{
  PLUG_Broadcast = 0,
  PLUG_Apple = 1,
  PLUG_Radio = 2,
  PLUG_Spotify = 3,
  PLUG_YouTube = 4
}PLUG_Target;

#define PLUG_INITIAL_MODE PLUG_OFF_MODE
#define PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(idx) \
  ((PLUG_Mode)(GetParam(idx)->Int()+1))
#define PLUG_REVERSE_PLATFORM_SWITCH_VALUE(n) (PLUG_PLATFORM_OPTIONS - 1 - n)

/*
For VST3 we use GetNormalized() method, but it
doesn't work well for VST2 and AU, hence this macro.
*/
#if defined(VST_API) || defined(AU_API)
#define PLUG_NORMALIZE_PLATFORM_SWITCH_VALUE(n) (((double)n)/PLUG_PLATFORM_OPTIONS)
#endif

#define PLUG_CONVERT_PLUG_MODE_TO_SWITCH_VALUE(m) (m-1)

class StreamMaster : public IPlug{
public:
  StreamMaster(IPlugInstanceInfo instanceInfo);
  ~StreamMaster();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  // Storing/recalling some hidden data
  bool SerializeState(ByteChunk* pChunk);
  int UnserializeState(ByteChunk* pChunk, int startPos);
  void PresetsChangedByHost();
  bool CompareState(const unsigned char* incomingState, int startPos);
  // Update stuff
  void UpdateGui();
  void UpdateAvailableControls();
  void UpdatePreMastering(PLUG_Target mPlatform);
  void UpdateSampleRate();
  void UpdatePlatform(PLUG_Target mPlatform);
  void UpdateTruePeak();
  // Limiter and loudness meter
  chunkware_simple::SimpleLimit* tLimiter;
  Plug::LoudnessMeter* tLoudnessMeter;
  // IPlug GUI stuff
  IGraphics* pGraphics;
  ITextControl *tLoudnessTextControl;
  ITextControl *tGrTextControl;
  ITextControl *tPeakingTextControl;
  ITextControl *tAdjustTextControl;
  ITextControl *tModeTextControl;
  ITextControl *tTpAlertTextControl;
  ITextControl *tTpOkTextControl;
  ITextControl *tTpTextControl;
  ISwitchControl *tBypassSwitch;
  ISwitchControl *tModeSwitch;
  IKnobMultiControl *tPeakingKnob;
  IKnobMultiControl *tPlatformSelector;
  IKnobMultiControl *tAdjustKnob;
  Plug::ILevelMeteringBar* tILevelMeteringBar;
  Plug::ILevelMeteringBar* tIGrMeteringBar;
  IContactControl *TIGrContactControl;
  IContactControl *TILufsContactControl;
  IRadioButtonsControl *tPlatformSelectorClickable;
  IBitmapControl *tLoudnessLabelOverlay;
  IBitmapControl *tGrLabelOverlay;
  IBitmapControl *tPlatformSelectorClickableGreyOut;
  // Shared statistic variables
  double fMaxGainReductionPerFrame;
  double fMaxGainReductionPerSessionDb;
  bool bLufsTooLow;
  // Operation related stuff
  double fTargetLoudness;
  unsigned short nCurrentTargetIndex;
  bool bNeedToRecallSourceLufs;
  bool bJustRecalledSourceLufs;
  bool bIsBypassed;
  // Plugin starts up in this mode
  PLUG_Mode tPlugCurrentMode;
  // Vars for mastering mode
  double fMasteringGainDb;
  double fTargetLufsIntegratedDb;
  double fSourceLufsIntegratedDb;
  double fLimiterCeilingDb;
  double fLimiterCeilingLinear;
  double fMasteringGainLinear;
  double fAdjustLufsDb;
  // Text guide message
  char* sModeString;

}; //class StreamMaster

#endif
