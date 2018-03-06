#include "StreamMaster.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "PLUG_IControlExtras.h"
#include "resource.h"
#include "lib_chunkware/SimpleLimit.h"
#include "PLUG/PLUG_LoudnessMeter.h"

// Number of presets
const int kNumPrograms = 1;

/* TODO: Move those into StreamMaster class - start */
// Limiter
chunkware_simple::SimpleLimit tLimiter;
const double fDefaultLimiterThreshold = PLUG_LIMITER_DEFAULT_THRESHOLD_DB;
// IPlug GUI stuff
IGraphics* pGraphics;
ITextControl *tLoudnessTextControl;
ITextControl *tGrTextControl;
ITextControl *tPeakingTextControl;
ITextControl *tModeTextControl;
IKnobMultiControl *tPeakingKnob;
IKnobMultiControl *tPlatformSelector;
Plug::ILevelMeteringBar* tILevelMeteringBar;
Plug::ILevelMeteringBar* tIGrMeteringBar;
// Shared statistic variables
double fMaxGainReductionPerFrame = PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET;
double fMaxGainReductionPerSessionDb = PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET;
/* Move those into StreamMaster class - end */

// Operation related stuff
double fTargetLoudness = PLUG_DEFAULT_TARGET_LOUDNESS;

// Plugin starts up in this mode
PLUG_Mode tPlugCurrentMode = PLUG_INITIAL_MODE;
// Vars for mastering mode
double \
  fMasteringGainDb = PLUG_MASTERING_GAIN_DB_RESET, \
  fTargetLufsIntegratedDb = PLUG_TARGET_LUFS_INTERGRATED_DB_RESET, \
  fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTERGRATED_DB_RESET, \
  fLimiterCeilingDb = PLUG_LIMITER_CEILING_DB_RESET, \
  fMasteringGainLinear = PLUG_MASTERING_GAIN_LINEAR_RESET;

/*
\brief All GUI relates data is updated in this thread
\note Actual redrawing happens in IPlug's craphics core automatically at a given FPS, not here. 
*/
void StreamMaster::UpdateGui()
{
  char sLoudnessString[64];
  char sGrString[64];
  double fMaxGainReductionPerFrameDb = 0.;
  double fLufs = tLoudnessMeter->GetLufs();
  double fFastLufs = tLoudnessMeter->GetMomentaryLufs();

  // Text below LUFS meter bar
  sprintf(sLoudnessString, "Integrated: %4.1fLUFS\nMomentary: %4.1fLUFS", fLufs, fFastLufs);
  tLoudnessTextControl->SetTextFromPlug(sLoudnessString);

  // Updating LUFS bar values
  tILevelMeteringBar->SetValue(fLufs);

  // Updating gain reduction bar values
  if (tPlugCurrentMode == PLUG_MASTER_MODE){
    fMaxGainReductionPerFrameDb = LINEAR_TO_LOG(fMaxGainReductionPerFrame);
    // ("<" because gain reduction is negative in log domain)
    // "MaxGainReductionPerSession" is displayed as a notch on a GR bar
    if (fMaxGainReductionPerFrameDb < fMaxGainReductionPerSessionDb)
      fMaxGainReductionPerSessionDb = fMaxGainReductionPerFrameDb;
    tIGrMeteringBar->SetValue(fMaxGainReductionPerFrameDb);
    tIGrMeteringBar->SetNotchValue(fMaxGainReductionPerSessionDb);
  }

  // Text below Gain reduction bar
  sprintf(sGrString, "GR: %4.2fdB\nMax: %4.2fdB", 
    fMaxGainReductionPerFrameDb,
    fMaxGainReductionPerSessionDb);
  tGrTextControl->SetTextFromPlug(sGrString);
}

/*
\brief Locks and unlocks controls depending on current mode. Called every time the mode is changed. 
*/
void StreamMaster::UpdateAvailableControls(){
  switch (tPlugCurrentMode){
  case PLUG_LEARN_MODE:
    // Learn mode
    // Only mode switch and LUFS meter
    tIGrMeteringBar->GrayOut(true);
    tPeakingKnob->GrayOut(true);
    tPlatformSelector->GrayOut(true);
    tPeakingTextControl->GrayOut(true);
    tILevelMeteringBar->GrayOut(false);
    break;
  case PLUG_MASTER_MODE:
    // Master mode
    // Everything unlocked
    tIGrMeteringBar->GrayOut(false);
    tPeakingKnob->GrayOut(false);
    tPlatformSelector->GrayOut(false);
    tPeakingTextControl->GrayOut(false);
    tILevelMeteringBar->GrayOut(false);
    break;
  case PLUG_OFF_MODE:
    // Learn mode
    // Only mode switch
    tIGrMeteringBar->GrayOut(true);
    tPeakingKnob->GrayOut(true);
    tPlatformSelector->GrayOut(true);
    tPeakingTextControl->GrayOut(true);
    tILevelMeteringBar->GrayOut(true);
    break;
  }
}

/*
\brief We set up GUI and related processing classes here. 
*/
StreamMaster::StreamMaster(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{

  TRACE;

  // Setting up values for all the controls
  //arguments are: name, defaultVal, minVal, maxVal, step, label

  // Ceiling knob
  /* We need precise values for ceiling knob,
  so we have to use integer values and convert them into double later.
  For example, "9" represents -0.1dB, "0" is for -1.0dB etc. if the actual range is -1.0..-0.0dB*/
  GetParam(kGain)->InitInt("Ceiling", 9, 0, 10, "dB");
  GetParam(kGain)->SetShape(1.);
  // LUFS and GR bars
  GetParam(kILevelMeteringBar)->InitDouble("Loudness", -24., -40., 3.0, 0.1, "LUFS");
  GetParam(kIGrMeteringBar)->InitDouble("Gain reduction", -0., -43., 0.0, 0.1, "dB");
  // Mode and platform switches
  GetParam(kModeSwitch)->InitInt("Mode", PLUG_CONVERT_PLUG_MODE_TO_SWITCH_VALUE(tPlugCurrentMode), 0, 2, "");
  GetParam(kPlatformSwitch)->InitInt("Target platform", PLUG_DEFAULT_TARGET_PLATFORM, 0, 4, "");

  // *** Initing actual GUI controls

  // Plug window, background
  pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  
  // LUFS meter
  tILevelMeteringBar = new Plug::ILevelMeteringBar(this, kLufsMeter_X, kLufsMeter_Y, METERING_BAR_DEFAULT_SIZE_IRECT, kILevelMeteringBar);
  tILevelMeteringBar->SetNotchValue(PLUG_DEFAULT_TARGET_LOUDNESS);
  pGraphics->AttachControl(tILevelMeteringBar);

  // Gain Reduction meter
  tIGrMeteringBar = new Plug::ILevelMeteringBar(this, kGrMeter_X, kGrMeter_Y, METERING_BAR_DEFAULT_SIZE_IRECT, kIGrMeteringBar, \
    true, &GR_BAR_DEFAULT_FG_ICOLOR, &GR_BAR_DEFAULT_NOTCH_ICOLOR, &METERING_BAR_ABOVE_NOTCH_ICOLOR);
  pGraphics->AttachControl(tIGrMeteringBar);
    
  // Limiter knob
  IBitmap tBmp = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  tPeakingKnob = new IKnobMultiControl(this, kGainX, kGainY, kGain, &tBmp);
  pGraphics->AttachControl(tPeakingKnob);
  
  // Mode selector (learn-master-off)
  tBmp = pGraphics->LoadIBitmap(MODESWITCH_ID, MODESWITCH_FN, kModeSwitch_N);
  pGraphics->AttachControl(new ISwitchControl(this, kModeSwitch_X, kModeSwitch_Y, kModeSwitch, &tBmp));

  // Platform selector (YT, spotify etc.)
  tBmp = pGraphics->LoadIBitmap(PLATFORMSWITCH_ID, PLATFORMSWITCH_FN, kPlatformSwitch_N);
  tPlatformSelector = new IKnobMultiControl(this, kPlatformSwitch_X, kPlatformSwitch_Y, kPlatformSwitch, &tBmp);
  pGraphics->AttachControl(tPlatformSelector);

  // Text LUFS meter
  IText tDefaultLoudnessLabel = IText(PLUG_METER_TEXT_LABEL_STRING_SIZE);
  tDefaultLoudnessLabel.mColor = PLUG_METER_TEXT_LABEL_COLOR;
  tDefaultLoudnessLabel.mSize = PLUG_METER_TEXT_LABEL_FONT_SIZE;
  tDefaultLoudnessLabel.mAlign = tDefaultLoudnessLabel.kAlignFar;
  tLoudnessTextControl = new ITextControl(
    this,
    IRECT(
      kILoudnessTextControl_X,
      kILoudnessTextControl_Y,
      (kILoudnessTextControl_X + kILoudnessTextControl_W),
      (kILoudnessTextControl_Y + kILoudnessTextControl_H)
    ),
    &tDefaultLoudnessLabel,
    "-");
  pGraphics->AttachControl(tLoudnessTextControl);

  // Text GR meter
  IText tGrLabel = IText(PLUG_METER_TEXT_LABEL_STRING_SIZE);  
  tGrLabel.mColor = PLUG_METER_TEXT_LABEL_COLOR;
  tGrLabel.mSize = PLUG_METER_TEXT_LABEL_FONT_SIZE;
  tGrLabel.mAlign = tGrLabel.kAlignFar;
  tGrTextControl = new ITextControl(
    this,
    IRECT(
      kIGrTextControl_X,
      kIGrTextControl_Y,
      (kIGrTextControl_X + kIGrTextControl_W),
      (kIGrTextControl_Y + kIGrTextControl_H)
    ),
    &tGrLabel,
    "-");
  pGraphics->AttachControl(tGrTextControl);

  // Text Peaking knob guide
  IText tPeakingLabel = IText(PLUG_KNOB_TEXT_LABEL_STRING_SIZE);
  tPeakingLabel.mColor = PLUG_KNOB_TEXT_LABEL_COLOR;
  tPeakingLabel.mSize = PLUG_KNOB_TEXT_LABEL_FONT_SIZE;
  tPeakingLabel.mAlign = tPeakingLabel.kAlignCenter;
  tPeakingTextControl = new ITextControl(
    this,
    IRECT(
      kIPeakingTextControl_X,
      kIPeakingTextControl_Y,
      (kIPeakingTextControl_X + kIPeakingTextControl_W),
      (kIPeakingTextControl_Y + kIPeakingTextControl_H)
    ),
    &tPeakingLabel,
    "-");
  pGraphics->AttachControl(tPeakingTextControl);

  // Text for the mastering mode
  IText tModeLabel = IText(PLUG_MODE_TEXT_LABEL_STRING_SIZE);
  tModeLabel.mColor = PLUG_KNOB_TEXT_LABEL_COLOR;
  tModeLabel.mSize = PLUG_KNOB_TEXT_LABEL_FONT_SIZE;
  tModeLabel.mAlign = tModeLabel.kAlignCenter;
  tModeTextControl = new ITextControl(
    this,
    IRECT(
      kIModeTextControl_X,
      kIModeTextControl_Y,
      (kIModeTextControl_X + kIModeTextControl_W),
      (kIModeTextControl_Y + kIModeTextControl_H)
    ),
    &tModeLabel,
    "-");
  pGraphics->AttachControl(tModeTextControl);

  // Guide message
  char sModeString[] = PLUG_OFF_STARTUP_MESSAGE;
  tModeTextControl->SetTextFromPlug(sModeString);
  
  // Finally - feed all the controls to IPlug's graphics gizmo
  AttachGraphics(pGraphics);
  // *** Initing actual GUI controls - end
  
  // *** Stuff for signal processing
  //Limiter 
  tLimiter.setThresh(fDefaultLimiterThreshold);
  tLimiter.setSampleRate(PLUG_DEFAULT_SAMPLERATE);
  tLimiter.setAttack(PLUG_LIMITER_ATTACK_MILLISECONDS);
  tLimiter.initRuntime();

  //LUFS loudness meter 
  tLoudnessMeter = new Plug::LoudnessMeter();
  tLoudnessMeter->SetSampleRate(PLUG_DEFAULT_SAMPLERATE);
  tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER);  

  // *** General plugin shenanigans
  // Presets displayed in the plugin's hosts
  // We have only one preset
  MakeDefaultPreset((char *)PLUG_DEFAULT_PRESET_NAME, kNumPrograms);

  // *** Interface stuff
  // Set initial values to controls that can't be handled
  // in the "GetParam()->Init...() section of this constructor"
  UpdateAvailableControls();
}

/*
\brief DSP thread. All signall processing happens here. Also the GUI values are updated fron this thread.
*/
StreamMaster::~StreamMaster() {}
void StreamMaster::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  // Assuming we're wirking with a stereo signal
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  double *afInterleavedSamples = new double[nFrames * 2];

  switch(tPlugCurrentMode){
    case PLUG_LEARN_MODE:
    // Learn mode
    // Feed in the audio into the LUFS meter, but not changing it
    for (int frame = 0, sample = 0; frame < nFrames; ++frame, ++in1, ++in2, ++out1, ++out2, sample += 2)
    {
      *out1 = *in1;
      *out2 = *in2;
      afInterleavedSamples[sample]   = *in1;
      afInterleavedSamples[sample+1] = *in2;
    }
    tLoudnessMeter->AddSamples(afInterleavedSamples, nFrames);
    delete[] afInterleavedSamples;
    break;

    case PLUG_MASTER_MODE:
    // Master mode
    // Process the audio, then feed it to the meter
    // Make sure unprocessed audio's loudness is stored in fSourceLufsIntegratedDb once
    // after switching the mode and not changed until the mode in changed to "off"!
    for (int frame = 0, sample = 0; frame < nFrames; ++frame, ++in1, ++in2, ++out1, ++out2, sample += 2)
    {
      // Apply gain to the samples 
      *out1 = fMasteringGainLinear * *in1;
      *out2 = fMasteringGainLinear * *in2;

      // Feed them to the limiter and read GR value
      tLimiter.process(*out1, *out2);
      tLimiter.getGr(&fMaxGainReductionPerFrame);

      // Collect samples for loudness measurement. After limiter obviously. 
      afInterleavedSamples[sample]   = *out1;
      afInterleavedSamples[sample+1] = *out2;
    }
    tLoudnessMeter->AddSamples(afInterleavedSamples, nFrames);
    delete[] afInterleavedSamples;
    break;

    case PLUG_OFF_MODE:
    // Off mode
    // Do nothing to the audio
    for (int frame = 0; frame < nFrames; ++frame, ++in1, ++in2, ++out1, ++out2)
    {
      *out1 = *in1;
      *out2 = *in2;
    }    

    break;
  }

  // GUI also updates in this thread. So make sure it's simple. 
  UpdateGui();
}

void StreamMaster::Reset()
{
  TRACE;
  // TODO: Sample rate handler
  IMutexLock lock(this);
}

/*
Quick maths:
MasteringGainDb = TargetLufsIntegratedDb - SourceLufsIntegratedDb - LimiterCeilingDb,
Where LimiterCeilingDb is a negative value. Loudness (in dB) is usually negative too, although it can be above zero.
MasteringGainLinear = LOG_TO_LINEAR(MasteringGainDb)
SignalOutLinear = MasteringGainLinear * SignalInLinear, pre-limiter!
then feed it into the limiter. 
Done! 
*/
/*
\brief calculate all relevant values before we start mastering
*/
void StreamMaster::UpdatePreMastering(){
  unsigned int nIndex;
  char sModeString[PLUG_MODE_TEXT_LABEL_STRING_SIZE];
  // *** Target LUFS 
  nIndex = (unsigned int)GetParam(kPlatformSwitch)->Value();
  fTargetLufsIntegratedDb = PLUG_GET_TARGET_LOUDNESS(nIndex);

  // *** Limiter ceiling
  fLimiterCeilingDb = PLUG_KNOB_PEAK_DOUBLE(GetParam(kGain)->Value());

  // *** Mastering gain in dB
  fMasteringGainDb = fTargetLufsIntegratedDb - fSourceLufsIntegratedDb - fLimiterCeilingDb;

  // *** Mastering gain in linear
  fMasteringGainLinear = LOG_TO_LINEAR(fMasteringGainDb);

  // Now that we know mastering gain, we just apply it so the audio stream
  // and feed it into the limiter afterwards

  // ...but before, let's output some text
  sprintf(sModeString,
    PLUG_MASTER_GUIDE_MESSAGE,
    fSourceLufsIntegratedDb,
    fTargetLufsIntegratedDb,
    fLimiterCeilingDb,
    fMasteringGainDb
    );
  tModeTextControl->SetTextFromPlug(sModeString);
}

/*
\brief Handles all user interactions with the controls
*/
void StreamMaster::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
  double fLufs, fPeaking;
  unsigned int nIndex;
  char sPeakingString[PLUG_KNOB_TEXT_LABEL_STRING_SIZE];
  char sModeString[PLUG_MODE_TEXT_LABEL_STRING_SIZE];
  const double fMaxGainReductionPerFrameDb = LINEAR_TO_LOG(fMaxGainReductionPerFrame);
  PLUG_Mode tPlugNewMode;

  // General control handling
  // Locking and unlocking of controls happens in UpdateAvailableControls()
  switch (paramIdx)
  {
    case kGain:
      fPeaking = PLUG_KNOB_PEAK_DOUBLE(GetParam(kGain)->Value());
      //tLimiter.setThresh(GetParam(kGain)->Value());
      sprintf(sPeakingString, "%5.2fdB", fPeaking);
      tPeakingTextControl->SetTextFromPlug(sPeakingString);
      if (tPlugCurrentMode == PLUG_MASTER_MODE)
        UpdatePreMastering();
      break;
    //Platform control
    case kPlatformSwitch:
      nIndex = (unsigned int)GetParam(kPlatformSwitch)->Value();
      fTargetLoudness = PLUG_GET_TARGET_LOUDNESS(nIndex);
      // Update the notch on the LUFS meter
      tILevelMeteringBar->SetNotchValue(fTargetLoudness);
      tILevelMeteringBar->Redraw();
      if (tPlugCurrentMode == PLUG_MASTER_MODE)
        UpdatePreMastering();
      break;
      /*
      Make sure we update all following variables:
        v fMasteringGainDb,
        v fTargetLufsIntegratedDb,
        v fSourceLufsIntegratedDb,
        v fLimiterCeilingDb,
        v fMasteringGainLinear;
      */
    case kModeSwitch:
      // Converting switch's value to mode
      tPlugNewMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);
      // Apparently it can be falsely triggered during startup, 
      // so we have to ignore that one
      if (tPlugNewMode == tPlugCurrentMode) break;
      
      switch (tPlugNewMode){
      case PLUG_LEARN_MODE:
        // Learn mode

        // Guide message
        sprintf(sModeString, PLUG_LEARN_GUIDE_MESSAGE);
        tModeTextControl->SetTextFromPlug(sModeString);
        break;
      case PLUG_MASTER_MODE:
        // Master mode
        // Store source Loudness and other values 

        // Storing source Loudness value
        fSourceLufsIntegratedDb = tLoudnessMeter->GetLufs();

        //Resetting the meter
        delete tLoudnessMeter;
        tLoudnessMeter = new Plug::LoudnessMeter();
        //TODO: Sample Rate!
        tLoudnessMeter->SetSampleRate(PLUG_DEFAULT_SAMPLERATE);
        tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 
        UpdatePreMastering();
        // The rest is handled in kPlatformSwitch part
        // Since we can switch it multiple times in that mode

        break;
      case PLUG_OFF_MODE:
        // Off mode
        // Reset everything and chill

        //Resetting the meter
        delete tLoudnessMeter;
        tLoudnessMeter = new Plug::LoudnessMeter();
        //TODO: Sample Rate!
        tLoudnessMeter->SetSampleRate(PLUG_DEFAULT_SAMPLERATE);
        tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 

        fMasteringGainDb = PLUG_MASTERING_GAIN_DB_RESET;
        fTargetLufsIntegratedDb = PLUG_TARGET_LUFS_INTERGRATED_DB_RESET;
        fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTERGRATED_DB_RESET;
        fLimiterCeilingDb = PLUG_LIMITER_CEILING_DB_RESET;
        fMasteringGainLinear = PLUG_MASTERING_GAIN_LINEAR_RESET;
        fMaxGainReductionPerFrame = PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET;
        fMaxGainReductionPerSessionDb = PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET;

        // Reset gain reduction meter bar
        tIGrMeteringBar->SetValue(fMaxGainReductionPerFrameDb);
        tIGrMeteringBar->SetNotchValue(fMaxGainReductionPerSessionDb);

        // Guide message
        sprintf(sModeString, PLUG_OFF_GUIDE_MESSAGE);
        tModeTextControl->SetTextFromPlug(sModeString);
        break;
      }

      tPlugCurrentMode = tPlugNewMode;
      UpdateAvailableControls();
      break;

    default:
      break;
  }
}
