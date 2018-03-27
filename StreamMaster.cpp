#include "StreamMaster.h"
#include "IPlug_include_in_plug_src.h"

// Number of presets
const int kNumPrograms = 1;

/*
@brief All GUI relates data is updated in this thread
@note Actual redrawing happens in IPlug's craphics core automatically at a given FPS, not here. 
*/
void StreamMaster::UpdateGui()
{
  char sLoudnessString[PLUG_METER_TEXT_LABEL_STRING_SIZE];
  char sGrString[PLUG_METER_TEXT_LABEL_STRING_SIZE];
  double fMaxGainReductionPerFrameDb = 0.;
  double fLufs = tLoudnessMeter->GetLufs();
  double fFastLufs = tLoudnessMeter->GetMomentaryLufs();

  // Text below LUFS meter bar
  sprintf(sLoudnessString, "Int.: %4.1fLUFS\nMom.: %4.1fLUFS", fLufs, fFastLufs);
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
  #ifdef PLUG_DO_NOT_BLOCK_CONTROLS
  // Unlock all controls in debug mode
  tIGrMeteringBar->GrayOut(false);
  tPeakingKnob->GrayOut(false);
  tPeakingTextControl->Hide(false);
  tPlatformSelector->GrayOut(false);
  tPlatformSelectorClickable->GrayOut(false);
  tPeakingTextControl->GrayOut(false);
  tILevelMeteringBar->GrayOut(false);
  TIGrContactControl->GrayOut(false);
  TILufsContactControl->GrayOut(false);
  tLoudnessTextControl->Hide(false);
  tGrTextControl->Hide(false);
  #else
  switch (tPlugCurrentMode){
  case PLUG_LEARN_MODE:
    // Learn mode
    // Only mode switch and LUFS meter
    tIGrMeteringBar->GrayOut(true);
    tPeakingKnob->GrayOut(true);
    tPeakingTextControl->Hide(true);
    tPlatformSelector->GrayOut(true);
    tPlatformSelectorClickable->GrayOut(true);
    tPeakingTextControl->GrayOut(true);
    tILevelMeteringBar->GrayOut(false);
    /* GR is blocked since there's no gain reduction applied,
    but you can reset LUFS meter */
    TIGrContactControl->GrayOut(true);
    TILufsContactControl->GrayOut(false);
    // Only display LUFS value
    tLoudnessTextControl->Hide(false);
    tGrTextControl->Hide(true);
    break;
  case PLUG_MASTER_MODE:
    // Master mode
    if(bLufsTooLow)
    {
      // If LUFS was too low in learning mode...
      // Only mode switch and LUFS meter
      tIGrMeteringBar->GrayOut(true);
      tPeakingKnob->GrayOut(true);
      tPeakingTextControl->Hide(true);
      tPlatformSelector->GrayOut(true);
      tPlatformSelectorClickable->GrayOut(true);
      tPeakingTextControl->GrayOut(true);
      tILevelMeteringBar->GrayOut(false);
      /* Unlike learning mode, we should
      lock both reset switches here*/
      TIGrContactControl->GrayOut(true);
      TILufsContactControl->GrayOut(true);
      // No gain reduction is happening in that mode
      tLoudnessTextControl->Hide(true);
      tGrTextControl->Hide(true);
    }
    else{
      // Everything unlocked
      tIGrMeteringBar->GrayOut(false);
      tPeakingKnob->GrayOut(false);
      tPeakingTextControl->Hide(false);
      tPlatformSelector->GrayOut(false);
      tPlatformSelectorClickable->GrayOut(false);
      tPeakingTextControl->GrayOut(false);
      tILevelMeteringBar->GrayOut(false);
      TIGrContactControl->GrayOut(false);
      TILufsContactControl->GrayOut(false);
      // Both GR and LUFS readings are displayed
      tLoudnessTextControl->Hide(false);
      tGrTextControl->Hide(false);
    }
    break;
  case PLUG_OFF_MODE:
    // Learn mode
    // Only mode switch
    tIGrMeteringBar->GrayOut(true);
    tPeakingKnob->GrayOut(true);
    tPeakingTextControl->Hide(true);
    tPlatformSelector->GrayOut(true);
    tPlatformSelectorClickable->GrayOut(true);
    tPeakingTextControl->GrayOut(true);
    tILevelMeteringBar->GrayOut(true);    
    TIGrContactControl->GrayOut(true);
    TILufsContactControl->GrayOut(true);
    // Both GR and LUFS readings are off
    tLoudnessTextControl->Hide(true);
    tGrTextControl->Hide(true);
    break;
  }
  #endif
}

/*
@brief Makes sure all DSP stuff works at the correct sample rate
*/
void StreamMaster::UpdateSampleRate(){
  const double fCurrentSampleRate = GetSampleRate();

  // Limiter's envelope has sample rate in Hz, double
  tLimiter->setSampleRate(fCurrentSampleRate);

  // Loudness meter has sample rate in Hz, unsigned long
  tLoudnessMeter->SetSampleRate((unsigned long)fCurrentSampleRate);
}

/*
\brief We set up GUI and related processing classes here. 
*/
StreamMaster::StreamMaster(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{

  TRACE;

  int i; /* For enum inits */

  // Value inits
  fMaxGainReductionPerFrame = PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET;
  fMaxGainReductionPerSessionDb = PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET;
  // Operation related stuff
  fTargetLoudness = PLUG_DEFAULT_TARGET_LOUDNESS;
  nCurrentTargetIndex = PLUG_DEFAULT_TARGET_PLATFORM;
  fDefaultLimiterThreshold = PLUG_LIMITER_DEFAULT_THRESHOLD_DB;
  // Plugin starts up in this mode
  tPlugCurrentMode = PLUG_INITIAL_MODE;
  // Vars for mastering mode
  fMasteringGainDb = PLUG_MASTERING_GAIN_DB_RESET;
  fTargetLufsIntegratedDb = PLUG_TARGET_LUFS_INTERGRATED_DB_RESET;
  fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTERGRATED_DB_RESET;
  fLimiterCeilingDb = PLUG_LIMITER_CEILING_DB_RESET;
  fLimiterCeilingLinear = PLUG_LIMITER_CEILING_LINEAR_RESET;
  fMasteringGainLinear = PLUG_MASTERING_GAIN_LINEAR_RESET;
  // Low (-inf) LUFS flag. Doesn't allow user to go into mastering mode
  // unless plugin successfully got source LUFS reading first.
  bLufsTooLow = true; 

  // Setting up values for all the controls
  //arguments are: name, defaultVal, minVal, maxVal, step, label

  // Ceiling knob
  /* We need precise values for ceiling knob,
  so we have to use integer values and convert them into double later.
  For example, "9" represents -0.1dB, "0" is for -1.0dB etc. if the actual range is -1.0..-0.0dB*/
  GetParam(kCeiling)->InitInt("Ceiling", PLUG_KNOB_PEAK_DEFAULT, PLUG_KNOB_PEAK_MIN, PLUG_KNOB_PEAK_MAX, "tenths of dB");
  GetParam(kCeiling)->SetShape(1.);
  // LUFS and GR bars
  GetParam(kILevelMeteringBar)->InitDouble(
    "Loudness",
    PLUG_LUFS_RANGE_MAX,
    PLUG_LUFS_RANGE_MIN,
    PLUG_LUFS_RANGE_MAX,
    0.1,
    "LUFS"
    );

  // Min and max values are the other way around here
  // because -6dB of gain reduction is more than -3dB,
  // but -6 is less than -3 obviously.
  GetParam(kIGrMeteringBar)->InitDouble(
    "Gain reduction",
    PLUG_GR_RANGE_MIN,
    PLUG_GR_RANGE_MAX,
    PLUG_GR_RANGE_MIN,
    0.1,
    "dB");

  // Mode and platform switches
  GetParam(kModeSwitch)->InitEnum("Mode", PLUG_CONVERT_PLUG_MODE_TO_SWITCH_VALUE(tPlugCurrentMode), 3);
  for (i=0; i<3; i++)
    GetParam(kModeSwitch)->SetDisplayText(i, asModeNames[i]);

  GetParam(kPlatformSwitchClickable)->InitEnum("Target platform", PLUG_REVERSE_PLATFORM_SWITCH_VALUE(PLUG_DEFAULT_TARGET_PLATFORM), 5);
  for (i=0; i<PLUG_PLATFORM_OPTIONS; i++)
    GetParam(kPlatformSwitchClickable)->SetDisplayText(PLUG_REVERSE_PLATFORM_SWITCH_VALUE(i), asTargetNames[i]);

  GetParam(kPlatformSwitch)->InitEnum("Target platform", PLUG_DEFAULT_TARGET_PLATFORM, PLUG_PLATFORM_OPTIONS);
  for (i=0; i<PLUG_PLATFORM_OPTIONS; i++)
    GetParam(kPlatformSwitch)->SetDisplayText(i, asTargetNames[i]);

  // GR and LUFS overlay switches for resetting  
  GetParam(kIGrContactControl)->InitBool("GR meter reset", 0, "");
  GetParam(kILufsContactControl)->InitBool("Loudness meter reset", 0, "");

  // *** Initing actual GUI controls

  // Plug window, background
  pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  
  // Bitmap for an invisible reset switch above metering bars
  IBitmap tBmp = pGraphics->LoadIBitmap(
    METEROVERLAYSWITCH_ID,
    METEROVERLAYSWITCH_FN,
    kIContactControl_N
    );

  // LUFS meter
  tILevelMeteringBar = new Plug::ILevelMeteringBar(this, kLufsMeter_X, kLufsMeter_Y, METERING_BAR_DEFAULT_SIZE_IRECT, kILevelMeteringBar);
  tILevelMeteringBar->SetNotchValue(PLUG_LUFS_RANGE_MAX);
  pGraphics->AttachControl(tILevelMeteringBar);
  // Same coordinates as the actial meter bar
  TILufsContactControl = new IContactControl(
    this, kLufsMeter_X, kLufsMeter_Y, kILufsContactControl, &tBmp);
  pGraphics->AttachControl(TILufsContactControl);

  // Gain Reduction meter
  tIGrMeteringBar = new Plug::ILevelMeteringBar(this, kGrMeter_X, kGrMeter_Y, METERING_BAR_DEFAULT_SIZE_IRECT, kIGrMeteringBar, \
    true, &GR_BAR_DEFAULT_FG_ICOLOR, &GR_BAR_DEFAULT_NOTCH_ICOLOR, &METERING_BAR_ABOVE_NOTCH_ICOLOR);
  pGraphics->AttachControl(tIGrMeteringBar);
  // Same coordinates as the actial meter bar
  TIGrContactControl = new IContactControl(
    this, kGrMeter_X, kGrMeter_Y, kIGrContactControl, &tBmp);
  pGraphics->AttachControl(TIGrContactControl);

    
  // Limiter knob
  tBmp = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  tPeakingKnob = new IKnobMultiControl(this, kCeilingX, kCeilingY, kCeiling, &tBmp);
  pGraphics->AttachControl(tPeakingKnob);
  
  // Mode selector (learn-master-off)
  tBmp = pGraphics->LoadIBitmap(MODESWITCH_ID, MODESWITCH_FN, kModeSwitch_N);
  pGraphics->AttachControl(new ISwitchControl(this, kModeSwitch_X, kModeSwitch_Y, kModeSwitch, &tBmp));

  // Platform selector (YT, spotify etc.)
  // Rotatable control
  tBmp = pGraphics->LoadIBitmap(PLATFORMSWITCH_ID, PLATFORMSWITCH_FN, kPlatformSwitch_N);
  tPlatformSelector = new IKnobMultiControl(this, kPlatformSwitch_X, kPlatformSwitch_Y, kPlatformSwitch, &tBmp);
  pGraphics->AttachControl(tPlatformSelector);
  // Same control, but lest user click on platform names too
  tBmp = pGraphics->LoadIBitmap(MODESWITCHCLICKABLE_ID, MODESWITCHCLICKABLE_FN, kPlatformSwitchClickable_N);
  tPlatformSelectorClickable = new IRadioButtonsControl(this, tPlatformSwitchClickableIRect, kPlatformSwitchClickable, kPlatformSwitchClickable_TOTAL, &tBmp);
  //IRadioButtonsControl(this, IRECT(kIRadioButtonsControl_V_X, kIRadioButtonsControl_V_Y, kIRadioButtonsControl_V_X + (kIRBC_W*kIRBC_VN), kIRadioButtonsControl_V_Y + (kIRBC_H*kIRBC_VN)), kIRadioButtonsControl_V, kIRBC_VN, &bitmap));
  pGraphics->AttachControl(tPlatformSelectorClickable);
  

  // Text LUFS meter
  IText tDefaultLoudnessLabel = IText(PLUG_METER_TEXT_LABEL_STRING_SIZE);
  tDefaultLoudnessLabel.mColor = PLUG_METER_TEXT_LABEL_COLOR;
  tDefaultLoudnessLabel.mSize = PLUG_METER_TEXT_LABEL_FONT_SIZE;
  tDefaultLoudnessLabel.mAlign = tDefaultLoudnessLabel.PLUG_METER_TEXT_ALIGNMENT;
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
  tGrLabel.mAlign = tGrLabel.PLUG_METER_TEXT_ALIGNMENT;
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
  tPeakingTextControl->Hide(true);

  // Text for the mastering mode
  IText tModeLabel = IText(PLUG_MODE_TEXT_LABEL_STRING_SIZE);
  tModeLabel.mColor = PLUG_GUIDE_TEXT_LABEL_COLOR;
  tModeLabel.mSize = PLUG_GUIDE_TEXT_LABEL_FONT_SIZE;
  tModeLabel.mAlign = tModeLabel.PLUG_GUIDE_TEXT_ALIGNMENT;
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

  #ifdef _PLUG_VERSION_H 
  // Text label with current version of the plug
  IText tTextVersion = IText(PLUG_VERSION_TEXT_LABEL_STRING_SIZE);
  char sDisplayedVersion[PLUG_VERSION_TEXT_LABEL_STRING_SIZE];
  const IColor tTextVersionColor(
    255,
    kTextVersion_ColorMono,
    kTextVersion_ColorMono,
    kTextVersion_ColorMono
    );
  const IRECT tTextVersionRect(
    kTextVersion_X,
    kTextVersion_Y,
    (kTextVersion_X + kTextVersion_W),
    (kTextVersion_Y + kTextVersion_H)
    );
  sprintf(sDisplayedVersion, "Ver. %s (%s)", &sPlugVersionGitHead, &sPlugVersionDate);
  tTextVersion.mColor = tTextVersionColor;
  tTextVersion.mSize = PLUG_VERSION_TEXT_LABEL_FONT_SIZE;
  tTextVersion.mAlign = tTextVersion.kAlignFar;
  pGraphics->AttachControl(new ITextControl(this, tTextVersionRect, &tTextVersion, (const char*)&sDisplayedVersion));
  AttachGraphics(pGraphics);
  #endif

  // Hide some parameters from DAW's automation menu
  // (doesn't seem to hide the parameters, at least in Reaper 5.77)
  // Bar values are not for automation obviously
  GetParam(kILevelMeteringBar)->SetCanAutomate(false);
  GetParam(kIGrMeteringBar)->SetCanAutomate(false);
  // There are two linked controls for platform selection, so we disable one of them
  GetParam(kPlatformSwitch)->SetCanAutomate(false);

  // Guide message
  char sModeString[] = PLUG_OFF_STARTUP_MESSAGE;
  tModeTextControl->SetTextFromPlug(sModeString);
  
  // Finally - feed all the controls to IPlug's graphics gizmo
  AttachGraphics(pGraphics);
  // *** Initing actual GUI controls - end
  
  // *** Stuff for signal processing
  //Limiter 
  tLimiter = new chunkware_simple::SimpleLimit();
  tLimiter->setThresh(fDefaultLimiterThreshold);
  tLimiter->setAttack(PLUG_LIMITER_ATTACK_MILLISECONDS);
  tLimiter->initRuntime();

  //LUFS loudness meter 
  tLoudnessMeter = new Plug::LoudnessMeter();
  tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 

  // *** General plugin shenanigans
  // Presets displayed in the plugin's hosts
  // We have only one preset
  MakeDefaultPreset((char *)PLUG_DEFAULT_PRESET_NAME, kNumPrograms);

  // *** Interface stuff
  // Set initial values to controls that can't be handled
  // in the "GetParam()->Init...() section of this constructor"
  UpdateAvailableControls();

  // Tell DSP related objects what the current sample rate is
  UpdateSampleRate();
}

StreamMaster::~StreamMaster() {}

/*
\brief DSP thread. All signall processing happens here. Also the GUI values are updated fron this thread.
*/
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
      tLimiter->process(*out1, *out2);
      tLimiter->getGr(&fMaxGainReductionPerFrame);

      // Applying ceiling value (post-limiter gain)
      *out1 *= fLimiterCeilingLinear;
      *out2 *= fLimiterCeilingLinear;

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
  IMutexLock lock(this);
  UpdateSampleRate();
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
void StreamMaster::UpdatePreMastering(PLUG_Target mPlatform){
  //static int nIndex;
  char sModeString[PLUG_MODE_TEXT_LABEL_STRING_SIZE];
  // *** Target LUFS 
  //nIndex = GetParam(kPlatformSwitch)->Int();
  fTargetLufsIntegratedDb = PLUG_GET_TARGET_LOUDNESS((int)mPlatform);

  // *** Limiter ceiling
  fLimiterCeilingDb = PLUG_KNOB_PEAK_DOUBLE(GetParam(kCeiling)->Value());
  fLimiterCeilingLinear = LOG_TO_LINEAR(fLimiterCeilingDb);

  // *** Mastering gain in dB
  fMasteringGainDb = fTargetLufsIntegratedDb - fSourceLufsIntegratedDb - fLimiterCeilingDb;

  // *** Mastering gain in linear
  fMasteringGainLinear = LOG_TO_LINEAR(fMasteringGainDb);

  // Now that we know mastering gain, we just apply it so the audio stream
  // and feed it into the limiter afterwards

  // ...but before, let's output some text
  sprintf(sModeString,
    PLUG_MASTER_GUIDE_MESSAGE,
    asTargetDescription[nCurrentTargetIndex],
    fSourceLufsIntegratedDb,
    fTargetLufsIntegratedDb,
    fLimiterCeilingDb,
    fMasteringGainDb
    );
  tModeTextControl->SetTextFromPlug(sModeString);
}

/*
@brief Updates related values when user changes target platform
@param mPlatform Target platfrom chosen by user
*/
void StreamMaster::UpdatePlatform(PLUG_Target mPlatform){
  //static int nIndex = GetParam(kPlatformSwitch)->Int();

  fTargetLoudness = PLUG_GET_TARGET_LOUDNESS((int)mPlatform);
  // Update the notch on the LUFS meter
  tILevelMeteringBar->SetNotchValue(fTargetLoudness);
  // Reset gain reduction meter bar
  tIGrMeteringBar->SetValue(PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET);
  tIGrMeteringBar->SetNotchValue(PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET);

  if (tPlugCurrentMode == PLUG_MASTER_MODE)
    UpdatePreMastering((PLUG_Target)mPlatform);
} 

/*
\brief Handles all user interactions with the controls
*/
void StreamMaster::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  double fPeaking;
  unsigned int nIndex;
  char sPeakingString[PLUG_KNOB_TEXT_LABEL_STRING_SIZE];
  char sModeString[PLUG_MODE_TEXT_LABEL_STRING_SIZE];
  const double fMaxGainReductionPerFrameDb = LINEAR_TO_LOG(fMaxGainReductionPerFrame);
  PLUG_Mode tPlugNewMode;
  int nModeNumber, nConvertedModeNumber;
  double fNormalizedConvertedModeNumber;

  // General control handling
  // Locking and unlocking of controls happens in UpdateAvailableControls()
  switch (paramIdx)
  {
    case kIGrContactControl:
      // Just reset the peak GR value on the GR bar
      fMaxGainReductionPerSessionDb = PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET;
      tIGrMeteringBar->SetNotchValue(fMaxGainReductionPerSessionDb);
      break;
    case kILufsContactControl:
      // Resetting LUFS meter bar and the actual meter
      /* TODO: this switch probably falsely triggers during the startup once,
      so it's a good idea to avoid unnecessary destruction */
      delete tLoudnessMeter;
      tLoudnessMeter = new Plug::LoudnessMeter();
      tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 
      UpdateSampleRate();
      // Set notch on LUFS meter
      tILevelMeteringBar->SetValue(PLUG_SOURCE_LUFS_INTERGRATED_DB_RESET);
      tILevelMeteringBar->SetNotchValue(fTargetLufsIntegratedDb);
      // Reset source LUFS if we were measuring it
      tPlugCurrentMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);
      if (tPlugCurrentMode == PLUG_LEARN_MODE) fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTERGRATED_DB_RESET;
      break;
    case kCeiling:
      fPeaking = PLUG_KNOB_PEAK_DOUBLE(GetParam(kCeiling)->Value());
      sprintf(sPeakingString, "%5.2fdB", fPeaking);
      tPeakingTextControl->SetTextFromPlug(sPeakingString);
      tPlugNewMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);
      nIndex = GetParam(kPlatformSwitch)->Int();
      if (tPlugCurrentMode == PLUG_MASTER_MODE)
        UpdatePreMastering((PLUG_Target)nIndex);
      break;
    //Platform control
    case kPlatformSwitch:

      nCurrentTargetIndex = (unsigned int)GetParam(kPlatformSwitch)->Value();

      // Rotating and clickable platform switches are working together
      nConvertedModeNumber = PLUG_REVERSE_PLATFORM_SWITCH_VALUE(nCurrentTargetIndex);
      fNormalizedConvertedModeNumber = PLUG_NORMALIZE_PLATFORM_SWITCH_VALUE(nConvertedModeNumber);
      GetGUI()->SetParameterFromPlug(kPlatformSwitchClickable, nConvertedModeNumber, false);
      InformHostOfParamChange(kPlatformSwitchClickable, ((double)nCurrentTargetIndex) / 5.);

      // Apparently it can be falsely triggered during startup, 
      // so we have to ignore that one
      tPlugNewMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);
      if (tPlugNewMode != PLUG_MASTER_MODE) break;

      // Update all related values
      UpdatePlatform((PLUG_Target)nCurrentTargetIndex);

      break;
      /*
      Make sure we update all following variables:
        v fMasteringGainDb,
        v fTargetLufsIntegratedDb,
        v fSourceLufsIntegratedDb,
        v fLimiterCeilingDb,
        v fLimiterCeilingLinear,
        v fMasteringGainLinear;
      */
    case kModeSwitch:
      // Converting switch's value to mode
      tPlugNewMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);

      if (tPlugNewMode == tPlugCurrentMode){
          // Apparently it can be falsely triggered during startup, 
          // so we have to ignore that one
          break;
      }

      nModeNumber = (int)GetParam(kModeSwitch)->Value();

      switch (tPlugNewMode){
      case PLUG_LEARN_MODE:
        // Learn mode

        // Guide message
        sprintf(sModeString, PLUG_LEARN_GUIDE_MESSAGE);
        tModeTextControl->SetTextFromPlug(sModeString);

        // Set LUFS notch to max so it doesn't confuse the user
        tILevelMeteringBar->SetNotchValue(PLUG_LUFS_RANGE_MAX);

        break;
      case PLUG_MASTER_MODE:
        // Master mode

        // Store source Loudness and other values 

        // Storing source Loudness value
        fSourceLufsIntegratedDb = tLoudnessMeter->GetLufs();

        // *** Check if learn mode wass successful - start
        if((fSourceLufsIntegratedDb > PLUG_LUFS_TOO_LOW) || PLUG_ALWAYS_ALLOW_MASTERING){         
          bLufsTooLow = false; 
          delete tLoudnessMeter;
          tLoudnessMeter = new Plug::LoudnessMeter();
          tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 
          UpdateSampleRate();
          
          nIndex = GetParam(kPlatformSwitch)->Int();
          UpdatePreMastering((PLUG_Target)nIndex);

          // Set notch on LUFS meter
          tILevelMeteringBar->SetNotchValue(fTargetLufsIntegratedDb);
        }
        else{
          // In case the source LUFS was too low
          bLufsTooLow = true;

          // Guide message
          sprintf(sModeString, PLUG_TOO_QUIET_GUIDE_MESSAGE);
          tModeTextControl->SetTextFromPlug(sModeString);

          fMasteringGainDb = PLUG_MASTERING_GAIN_DB_RESET;
          fLimiterCeilingDb = PLUG_LIMITER_CEILING_DB_RESET;
          fLimiterCeilingLinear = PLUG_LIMITER_CEILING_LINEAR_RESET;
          fMasteringGainLinear = PLUG_MASTERING_GAIN_LINEAR_RESET;
        }
        // *** Check if learn mode wass successful - end
        //Resetting the meter

        // The rest is handled in kPlatformSwitch part
        // Since we can switch it multiple times in that mode
        break;
      case PLUG_OFF_MODE:
        // Off mode
        // Reset everything and chill

        //Resetting the meter
        delete tLoudnessMeter;
        tLoudnessMeter = new Plug::LoudnessMeter();
        tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 
        UpdateSampleRate();

        fMasteringGainDb = PLUG_MASTERING_GAIN_DB_RESET;
        fTargetLufsIntegratedDb = PLUG_TARGET_LUFS_INTERGRATED_DB_RESET;
        fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTERGRATED_DB_RESET;
        fLimiterCeilingDb = PLUG_LIMITER_CEILING_DB_RESET;
        fMasteringGainLinear = PLUG_MASTERING_GAIN_LINEAR_RESET;
        fMaxGainReductionPerFrame = PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET;
        fMaxGainReductionPerSessionDb = PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET;

        // Reset gain reduction meter bar
        tIGrMeteringBar->SetValue(PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET);
        tIGrMeteringBar->SetNotchValue(PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET);

        // Reset notch on LUFS meter
        tILevelMeteringBar->SetNotchValue(PLUG_LUFS_RANGE_MAX);

        // Guide message
        sprintf(sModeString, PLUG_OFF_GUIDE_MESSAGE);
        tModeTextControl->SetTextFromPlug(sModeString);
              }
      tPlugCurrentMode = tPlugNewMode;
      UpdateAvailableControls();
      break;

    case kPlatformSwitchClickable:      
      // Apparently it can be falsely triggered during startup, 
      // so we have to ignore that one
      tPlugNewMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);
      if (tPlugNewMode != PLUG_MASTER_MODE) break;

      nModeNumber = (int)GetParam(kPlatformSwitchClickable)->Value();
      // Rotating and clickable platform switches are working together
      nConvertedModeNumber = PLUG_REVERSE_PLATFORM_SWITCH_VALUE(nModeNumber);
      fNormalizedConvertedModeNumber = PLUG_NORMALIZE_PLATFORM_SWITCH_VALUE(nConvertedModeNumber);
      GetGUI()->SetParameterFromPlug(kPlatformSwitch, nConvertedModeNumber, false);
      InformHostOfParamChange(kPlatformSwitch, fNormalizedConvertedModeNumber);

      // This is a global value used by other methods. Careful with it!
      nCurrentTargetIndex = nConvertedModeNumber;

      // Update all related values      
      UpdatePlatform((PLUG_Target)nCurrentTargetIndex);
      break;

    default:
      break;
  }
}
