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

#define PLUG_METER_TEXT_LABEL_STRING_SIZE 64
#define PLUG_KNOB_TEXT_LABEL_STRING_SIZE 16
#define PLUG_MODE_TEXT_LABEL_STRING_SIZE 512
#define PLUG_GUIDE_TEXT_LABEL_STRING_SIZE PLUG_MODE_TEXT_LABEL_STRING_SIZE
#define PLUG_VERSION_TEXT_LABEL_STRING_SIZE 32
#define PLUG_TP_LABEL_STRING_SIZE 32

#ifdef _WIN32
#define PLUG_KNOB_TEXT_LABEL_FONT_SIZE 12
#define PLUG_METER_TEXT_LABEL_FONT_SIZE 12
#define PLUG_GUIDE_TEXT_LABEL_FONT_SIZE 14
#define PLUG_VERSION_TEXT_LABEL_FONT_SIZE 10
#elif defined(__APPLE__)
#define PLUG_KNOB_TEXT_LABEL_FONT_SIZE 13
#define PLUG_METER_TEXT_LABEL_FONT_SIZE 13
#define PLUG_GUIDE_TEXT_LABEL_FONT_SIZE 15
#define PLUG_VERSION_TEXT_LABEL_FONT_SIZE 11
#endif
#define PLUG_TP_LABEL_FONT_SIZE PLUG_GUIDE_TEXT_LABEL_FONT_SIZE

/* Knob values are not real-life units,
use PLUG_KNOB_PEAK_DOUBLE() to convert them to linear gain*/
#define PLUG_KNOB_PEAK_MIN -20
#define PLUG_KNOB_PEAK_MAX 0
#define PLUG_KNOB_PEAK_DEFAULT -10
#define PLUG_KNOB_PEAK_SCALE 10.
#define PLUG_KNOB_PEAK_DOUBLE(i) ((double)i/PLUG_KNOB_PEAK_SCALE)

#define PLUG_DEFAULT_PRESET_NAME "Default"
#define PLUG_LIMITER_ATTACK_MILLISECONDS 0.1
#define PLUG_LIMITER_DEFAULT_THRESHOLD_DB 0.;

#define PLUG_LUFS_RANGE_MIN -40.
#define PLUG_LUFS_RANGE_MAX 3.

#define PLUG_GR_RANGE_MIN -0.
#define PLUG_GR_RANGE_MAX -43.

#define PLUG_METERING_BAR_W 74
#define PLUG_METERING_BAR_H 537
#define PLUG_METERING_BAR_IRECT IRECT(0, 0, PLUG_METERING_BAR_W, PLUG_METERING_BAR_H)

#define PLUG_TP_ALERT_VALUE_DB -0.
#define PLUG_TP_LABEL_IRECT IRECT(460, 14, 555, 34)
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

#define PLUG_GUIDE_TEXT_ALIGNMENT kAlignNear
#define PLUG_TP_TEXT_ALIGNMENT kAlignFar

#ifdef _WIN32
#define PLUG_METER_TEXT_ALIGNMENT kAlignFar
#elif defined(__APPLE__)
#define PLUG_METER_TEXT_ALIGNMENT kAlignNear
#endif

enum EParams
{
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
  kIModeTextControl,
  kTpTextControl,
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,

  // LUFS meter
  kLufsMeter_X = 486,
  kLufsMeter_Y = 12,

  // GR meter
  kGrMeter_X = kLufsMeter_X + 87,
  kGrMeter_Y = kLufsMeter_Y,
  
  // Peaking knob
  kCeilingX = 302,
  kCeilingY = 90,
  kKnobFrames = 21,

#ifdef _WIN32
  // LUFS Text reading
  kILoudnessTextControl_W = 80,
  kILoudnessTextControl_H = 40,
  kILoudnessTextControl_X = kLufsMeter_X - (kILoudnessTextControl_W - PLUG_METERING_BAR_W),
  kILoudnessTextControl_Y = 552,

  // Gain reduction Text reading
  kIGrTextControl_W = 80,
  kIGrTextControl_H = 40,
  kIGrTextControl_X = kGrMeter_X - (kIGrTextControl_W - PLUG_METERING_BAR_W),
  kIGrTextControl_Y = kILoudnessTextControl_Y,

#elif defined(__APPLE__)
  // LUFS Text reading
  kILoudnessTextControl_X = kLufsMeter_X,
  kILoudnessTextControl_Y = 552,
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

  // Learn-master-off
  kModeSwitch_N = 3,
  kModeSwitch_X = 122,
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
  /* Switches for Youtube-Spotify-etc. - end */

#ifdef _PLUG_VERSION_H 
  // Text version label
  kTextVersion_W = 128,
  kTextVersion_H = 12,
  kTextVersion_X = kWidth-kTextVersion_W - 1,
  kTextVersion_Y = kHeight-kTextVersion_H - 1,
  kTextVersion_ColorMono = 64,
#endif

  // Meter reset switch
  kIContactControl_N = 2,

  // Meter overlays
  kLoudnessLabelOverlay_X = kLufsMeter_X + 35,
  kLoudnessLabelOverlay_Y = 440,

  kGrLabelOverlay_X = kGrMeter_X + 35,
  kGrLabelOverlay_Y = 383,
};

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
  ((PLUG_Mode)int(GetParam(idx)->Value()+1))
#define PLUG_REVERSE_PLATFORM_SWITCH_VALUE(n) (PLUG_PLATFORM_OPTIONS - 1 - n)
#define PLUG_NORMALIZE_PLATFORM_SWITCH_VALUE(n) (((double)n)/PLUG_PLATFORM_OPTIONS)

#define PLUG_CONVERT_PLUG_MODE_TO_SWITCH_VALUE(m) (m-1)

class StreamMaster : public IPlug{
public:
  StreamMaster(IPlugInstanceInfo instanceInfo);
  ~StreamMaster();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
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
  ITextControl *tModeTextControl;
  ITextControl *tTpAlertTextControl;
  ITextControl *tTpOkTextControl;
  ITextControl *tTpTextControl;
  IKnobMultiControl *tPeakingKnob;
  IKnobMultiControl *tPlatformSelector;
  Plug::ILevelMeteringBar* tILevelMeteringBar;
  Plug::ILevelMeteringBar* tIGrMeteringBar;
  IContactControl *TIGrContactControl;
  IContactControl *TILufsContactControl;
  IRadioButtonsControl *tPlatformSelectorClickable;
  IBitmapControl *tLoudnessLabelOverlay;
  IBitmapControl *tGrLabelOverlay;
  // Shared statistic variables
  double fMaxGainReductionPerFrame;
  double fMaxGainReductionPerSessionDb;
  bool bLufsTooLow;
  // Operation related stuff
  double fTargetLoudness;
  unsigned short nCurrentTargetIndex;
  double fDefaultLimiterThreshold;
  // Plugin starts up in this mode
  PLUG_Mode tPlugCurrentMode;
  // Vars for mastering mode
  double fMasteringGainDb;
  double fTargetLufsIntegratedDb;
  double fSourceLufsIntegratedDb;
  double fLimiterCeilingDb;
  double fLimiterCeilingLinear;
  double fMasteringGainLinear;

}; //class StreamMaster

#endif
