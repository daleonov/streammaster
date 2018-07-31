#include "StreamMaster.h"
#include "IPlug_include_in_plug_src.h"
#include <cmath>
#include "DLPG_FeedbackSender.h"

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
  double fLufsShortTerm = tLoudnessMeter->GetShortTermLufs();
  double fTruePeakingDb, fTruePeakingShortTermDb, fTruePeakingOfWindowDb;
  char sLufsInt[PLUG_DB_VALUE_STRING_SIZE];
  char sLufsShortTerm[PLUG_DB_VALUE_STRING_SIZE];
  // Text below LUFS meter bar
  // If values are to low, they would be displayed as "-oo"
  if((fLufs < PLUG_DB_VALUE_TOO_LOW) || std::isnan(fLufs))
    memcpy(sLufsInt, sDbValueMinusInf, PLUG_DB_VALUE_MINUS_INF_LEN);
  else
    sprintf(sLufsInt, "%4.1f", fLufs);
  if((fLufsShortTerm < PLUG_DB_VALUE_TOO_LOW) || std::isnan(fLufsShortTerm))
    memcpy(sLufsShortTerm, sDbValueMinusInf, PLUG_DB_VALUE_MINUS_INF_LEN);
  else
    sprintf(sLufsShortTerm, "%4.1f", fLufsShortTerm);
  sprintf(sLoudnessString, "Int.: %sLUFS\nShort: %sLUFS", sLufsInt, sLufsShortTerm);
  tLoudnessTextControl->SetTextFromPlug(sLoudnessString);

  // Updating LUFS bar values
  tILevelMeteringBar->SetValue(fLufs);
  double fGrMeterGainLinear, fGrMeterCurrentPeakDb;
  static double fGrMeterPreviousPeakDb = 0.;
  // Updating gain reduction bar values
  if (tPlugCurrentMode == PLUG_MASTER_MODE){
    fMaxGainReductionPerFrameDb = LINEAR_TO_LOG(fMaxGainReductionPerFrame);
    // ("<" because gain reduction is negative in log domain)
    if (fMaxGainReductionPerFrameDb < fMaxGainReductionPerSessionDb){
      fMaxGainReductionPerSessionDb = fMaxGainReductionPerFrameDb;
      tIGrMeteringBar->SetNotchValue(fMaxGainReductionPerSessionDb);
    }
    // Filter GR meter values so it won't flash too fast
    fGrMeterGainLinear = \
      (fMaxGainReductionPerFrameDb > fGrMeterPreviousPeakDb ? PLUG_GR_METER_DECAY : PLUG_GR_METER_ATTACK);
    fGrMeterCurrentPeakDb = \
      fMaxGainReductionPerFrameDb * fGrMeterGainLinear + \
      fGrMeterPreviousPeakDb * (1.0 - fGrMeterGainLinear);
    fGrMeterPreviousPeakDb = fGrMeterCurrentPeakDb;
    tIGrMeteringBar->SetValue(fGrMeterCurrentPeakDb);
  }

  // Text below Gain reduction bar
  sprintf(sGrString, "GR: %4.2fdB\nMax: %4.2fdB", 
    fMaxGainReductionPerFrameDb,
    fMaxGainReductionPerSessionDb);
  tGrTextControl->SetTextFromPlug(sGrString);

  // True peaking text label
  if(tPlugCurrentMode == PLUG_MASTER_MODE){
    // TP
    fTruePeakingDb = LINEAR_TO_LOG(tLoudnessMeter->GetTruePeaking());
    fTruePeakingShortTermDb = LINEAR_TO_LOG(tLoudnessMeter->GetTruePeakingShortTerm());

    UpdateTruePeak(fTruePeakingDb);
    /* 
    Dynamic range (PSR)
    PSR = TP_short_term - LUFS_short_term
    Both parameters have to have the same window (3s). LUFS short term has a
    correct window as it is, but we have to do extra stuff with TP.
    */
    tPeakingBuffer->Add(fTruePeakingShortTermDb);
    fTruePeakingOfWindowDb = tPeakingBuffer->GetMax();

    double fCurrentPsrDb = fTruePeakingOfWindowDb - fLufsShortTerm;
    UpdateDynamicRange(fCurrentPsrDb);
  }
}

void StreamMaster::UpdateDynamicRange(double fDynamicRangeDb){
  static double fPreviousDynamicRangeDb = PLUG_DR_WARNING_VALUE_DB + 0.1;
  char sDrValue[PLUG_DB_VALUE_STRING_SIZE];
  char sDrString[PLUG_DR_LABEL_STRING_SIZE];
  IText tDrLabel;

  /*
  The higher the PSR value is (in log domain), the more dynamics the track has.
  If PSR falls below a certain level (e.g. +8dB), it's considered not healthy,
  so it should not be a full-on alert, just a friendly warning. 
  */

  // If we crossed PSR threshold down, do the warning stuff
  if(
    (fDynamicRangeDb <= PLUG_DR_WARNING_VALUE_DB) &&
    ((fPreviousDynamicRangeDb > PLUG_DR_WARNING_VALUE_DB) ||
    tDrTextControl == tDrResetTextControl)
    ){
    tDrWarningTextControl->Hide(false);
    tDrOkTextControl->Hide(true);
    tDrResetTextControl->Hide(true);
    tDrTextControl = tDrWarningTextControl;
  }
  // If we crossed PSR threshold up, undo the warning stuff
  if(
    (fDynamicRangeDb > PLUG_DR_WARNING_VALUE_DB) &&
    ((fPreviousDynamicRangeDb < PLUG_DR_WARNING_VALUE_DB) ||
    tDrTextControl == tDrResetTextControl)
    ){
    tDrWarningTextControl->Hide(true);
    tDrOkTextControl->Hide(false);
    tDrResetTextControl->Hide(true);
    tDrTextControl = tDrOkTextControl;
  }

  /* If the value is too high, it would be displayed as "+oo".
     It may be also "nan" during start up, in that case it's "+oo" as well. */
  if(
    (fDynamicRangeDb > PLUG_DB_VALUE_TOO_HIGH) ||
    (fDynamicRangeDb < PLUG_DB_VALUE_TOO_LOW) ||
    std::isnan(fDynamicRangeDb)
    ){
    if(tDrTextControl != tDrResetTextControl){
      tDrWarningTextControl->Hide(true);
      tDrOkTextControl->Hide(true);
      tDrResetTextControl->Hide(false);
      tDrTextControl = tDrResetTextControl;
    }
    memcpy(sDrValue, sDbValuePlusInf, PLUG_DB_VALUE_PLUS_INF_LEN);
  }
  else
    sprintf(sDrValue, "%3.1f", fDynamicRangeDb);

  fPreviousDynamicRangeDb = fDynamicRangeDb;
  sprintf(sDrString, "PSR: %sdB", sDrValue);
  tDrTextControl->SetTextFromPlug(sDrString);
}

void StreamMaster::UpdateTruePeak(double fTruePeakingDb){
  static double fPreviousTruePeakingDb=LINEAR_TO_LOG(0.);
  char sTpValue[PLUG_DB_VALUE_STRING_SIZE];
  char sTpString[PLUG_TP_LABEL_STRING_SIZE];
  IText tTpLabel;

  // If we crossed TP threshold up, do some alert stuff
  if(
    (fTruePeakingDb >= PLUG_TP_ALERT_VALUE_DB) &&
    ((fPreviousTruePeakingDb < PLUG_TP_ALERT_VALUE_DB) ||
    tTpTextControl == tTpResetTextControl)
    ){
    tTpOkTextControl->Hide(true);
    tTpAlertTextControl->Hide(false);
    tTpResetTextControl->Hide(true);
    tTpTextControl = tTpAlertTextControl;
  }

  // If we crossed TP threshold down, undo the alert stuff
  if(
    (fTruePeakingDb < PLUG_TP_ALERT_VALUE_DB) &&
    ((fPreviousTruePeakingDb > PLUG_TP_ALERT_VALUE_DB) ||
    tTpTextControl == tTpResetTextControl)
    ){
    tTpAlertTextControl->Hide(true);
    tTpOkTextControl->Hide(false);
    tTpResetTextControl->Hide(true);
    tTpTextControl = tTpOkTextControl;
  }

  /* If the value is too low, it would be displayed as "-oo".
     It may be also "nan" during start up, in that case it's "-oo" as well. */
  if((fTruePeakingDb < PLUG_DB_VALUE_TOO_LOW) || std::isnan(fTruePeakingDb)){
    if(tTpTextControl != tTpResetTextControl){
      tTpAlertTextControl->Hide(true);
      tTpOkTextControl->Hide(true);
      tTpResetTextControl->Hide(false);
      tTpTextControl = tTpResetTextControl;
    }
    memcpy(sTpValue, sDbValueMinusInf, PLUG_DB_VALUE_MINUS_INF_LEN);
  }
  else
    sprintf(sTpValue, "%+4.1f", fTruePeakingDb);

  fPreviousTruePeakingDb = fTruePeakingDb;
  sprintf(sTpString, "TP: %sdB", sTpValue);
  tTpTextControl->SetTextFromPlug(sTpString);
}

/*
\brief Locks and unlocks controls depending on current mode. Called every time the mode is changed. 
*/
void StreamMaster::UpdateAvailableControls(){
  bool
    bIsModeSwitchDisabled,
    bIsGrMeteringDisabled,
    bIsLoudnessMeteringDisabled,
    bIsMasteringKnobsDisabled,
    bIsPlatformSwitchesDisabled,
    bIsTpMeteringDisabled;

  #ifdef PLUG_DO_NOT_BLOCK_CONTROLS
  // Unlock all controls in debug mode
  bIsModeSwitchDisabled = false;
  bIsGrMeteringDisabled = false;
  bIsLoudnessMeteringDisabled = false;
  bIsMasteringKnobsDisabled = false;
  bIsPlatformSwitchesDisabled = false;
  bIsTpMeteringDisabled = false;
  #else
  if(bIsBypassed){
    // Everything is locked if the plug is bypassed
    bIsModeSwitchDisabled = true;
    bIsGrMeteringDisabled = true;
    bIsLoudnessMeteringDisabled = true;
    bIsMasteringKnobsDisabled = true;
    bIsPlatformSwitchesDisabled = true;
    bIsTpMeteringDisabled = true;
  }
  else{
    // Not bypassed
    switch (tPlugCurrentMode){
    case PLUG_LEARN_MODE:
      // Learn mode
      // Only mode switch and LUFS meter
      bIsModeSwitchDisabled = false;
      bIsGrMeteringDisabled = true;
      bIsLoudnessMeteringDisabled = false;
      bIsMasteringKnobsDisabled = true;
      bIsPlatformSwitchesDisabled = true;
      bIsTpMeteringDisabled = true;
      break;
    case PLUG_MASTER_MODE:
      // Master mode
      if(bLufsTooLow)
      {
        // If LUFS was too low in learning mode...
        // Only mode switch
        // TODO: Do we need to show LUFS meter?
        bIsModeSwitchDisabled = false;
        bIsGrMeteringDisabled = true;
        bIsLoudnessMeteringDisabled = false;
        bIsMasteringKnobsDisabled = true;
        bIsPlatformSwitchesDisabled = true;
        bIsTpMeteringDisabled = true;
      }
      else{
        // Everything unlocked
        bIsModeSwitchDisabled = false;
        bIsGrMeteringDisabled = false;
        bIsLoudnessMeteringDisabled = false;
        bIsMasteringKnobsDisabled = false;
        bIsPlatformSwitchesDisabled = false;
        bIsTpMeteringDisabled = false;
      }
      break;
    case PLUG_OFF_MODE:
      // Learn mode
      // Only mode switch
      bIsModeSwitchDisabled = false;
      bIsGrMeteringDisabled = true;
      bIsLoudnessMeteringDisabled = true;
      bIsMasteringKnobsDisabled = true;
      bIsPlatformSwitchesDisabled = true;
      bIsTpMeteringDisabled = true;
      break;
    } //switch
  } // bIsBypassed
  #endif
  tModeSwitch->GrayOut(bIsModeSwitchDisabled);
  tIGrMeteringBar->GrayOut(bIsGrMeteringDisabled);
  tPeakingKnob->GrayOut(bIsMasteringKnobsDisabled);
  tAdjustKnob->GrayOut(bIsMasteringKnobsDisabled);
  tPeakingTextControl->Hide(bIsMasteringKnobsDisabled);
  tAdjustTextControl->Hide(bIsMasteringKnobsDisabled);
  tPlatformSelector->GrayOut(bIsPlatformSwitchesDisabled);
  tPlatformSelectorClickable->Hide(bIsPlatformSwitchesDisabled);
  tPlatformSelectorClickableGreyOut->Hide(!bIsPlatformSwitchesDisabled);
  tILevelMeteringBar->GrayOut(bIsLoudnessMeteringDisabled);
  TIGrContactControl->GrayOut(bIsGrMeteringDisabled);
  TILufsContactControl->GrayOut(bIsLoudnessMeteringDisabled);
  tLoudnessTextControl->Hide(bIsLoudnessMeteringDisabled);
  tGrTextControl->Hide(bIsGrMeteringDisabled);
  tTpTextControl->Hide(bIsTpMeteringDisabled);
  tDrTextControl->Hide(bIsTpMeteringDisabled);
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

  // TP buffer

  tPeakingBuffer->Resize(
    PLUG_DR_METER_WINDOW_SECONDS,
    fCurrentSampleRate,
    GetBlockSize()
    );
}

/*
\brief We set up GUI and related processing classes here. 
*/
StreamMaster::StreamMaster(IPlugInstanceInfo instanceInfo):
  IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo),
  fLowestDynamicRangeDb(PLUG_DB_VALUE_TOO_HIGH)
  {

  TRACE;

  //bJustRecalledSourceLufs = false;

  int i; /* For enum inits */
  IBitmap tBmp;

  /*
  Hide some parameters from DAW's automation menu; this also affects
  whether or not the parameters are displayed in generic UI view.
  Note: this does not affect all plugins formats. For instance, it works
  in VST3, it should work in AAX, AU and RTAS, but is ignored in VST2.
  */
  // Bar values are not for automation obviously
  GetParam(kILevelMeteringBar)->SetCanAutomate(false);
  GetParam(kIGrMeteringBar)->SetCanAutomate(false);
  // There are two linked controls for platform selection, so we disable one of them
  GetParam(kPlatformSwitchClickable)->SetCanAutomate(false);
  // Meter reset buttons are not necessary for automation as well
  GetParam(kIGrContactControl)->SetCanAutomate(false);
  GetParam(kILufsContactControl)->SetCanAutomate(false);

  // Setting up values for all the controls
  // arguments are: name, defaultVal, minVal, maxVal, step, label

  // Bypass switch
  GetParam(kBypassSwitch)->InitBool("OnOff", 0, "");

  // Ceiling knob
  /*
  We want to only have values with 0.1 step for ceiling knob,
  plus make it feel "clicky" opposed to a usual smooth knob,
  so we have to use integer values and convert them into double later.
  For example, "9" represents -0.1dB, "0" is for -1.0dB etc.
  if the actual range is -1.0..0.0dB
  */
  GetParam(kCeiling)->InitInt(
    "Ceiling",
    PLUG_KNOB_PEAK_DEFAULT,
    PLUG_KNOB_PEAK_MIN,
    PLUG_KNOB_PEAK_MAX,
    "tenths of dB"
    );
  GetParam(kCeiling)->SetShape(1.);

  // Adjust knob
  GetParam(kAdjust)->InitDouble(
    "Target LUFS Adjust",
    PLUG_KNOB_ADJUST_DEFAULT,
    PLUG_KNOB_ADJUST_MIN,
    PLUG_KNOB_ADJUST_MAX,
    PLUG_KNOB_ADJUST_STEP,
    "LUFS"
    );
  GetParam(kAdjust)->SetShape(1.);

  /* LUFS and GR bars */
  // Loudness meter
  GetParam(kILevelMeteringBar)->InitDouble(
    "[Loudness]",
    PLUG_LUFS_RANGE_MIN,
    PLUG_LUFS_RANGE_MIN,
    PLUG_LUFS_RANGE_MAX,
    0.1,
    "LUFS"
    );

  /* GR meter
  Min and max values are the other way around here
  because -6dB of gain reduction is more than -3dB,
  but -6 is less than -3 obviously. */
  GetParam(kIGrMeteringBar)->InitDouble(
    "[Gain reduction]",
    PLUG_GR_RANGE_MIN,
    PLUG_GR_RANGE_MAX,
    PLUG_GR_RANGE_MIN,
    0.1,
    "dB");

  // Mode and platform switches
  GetParam(kModeSwitch)->InitEnum(
    "Mode",
    PLUG_CONVERT_PLUG_MODE_TO_SWITCH_VALUE(PLUG_INITIAL_MODE),
    3
    );
  for (i=0; i<3; i++)
    GetParam(kModeSwitch)->SetDisplayText(i, asModeNames[i]);

  GetParam(kPlatformSwitchClickable)->InitEnum(
    "[Target platform]",
    PLUG_REVERSE_PLATFORM_SWITCH_VALUE(PLUG_DEFAULT_TARGET_PLATFORM),
    PLUG_PLATFORM_OPTIONS
    );
  for (i=0; i<PLUG_PLATFORM_OPTIONS; i++)
    GetParam(kPlatformSwitchClickable)->SetDisplayText(PLUG_REVERSE_PLATFORM_SWITCH_VALUE(i), asTargetNames[i]);

  GetParam(kPlatformSwitch)->InitEnum(
    "Target platform",
    PLUG_DEFAULT_TARGET_PLATFORM,
    PLUG_PLATFORM_OPTIONS
    );
  for (i=0; i<PLUG_PLATFORM_OPTIONS; i++)
    GetParam(kPlatformSwitch)->SetDisplayText(i, asTargetNames[i]);

  // GR and LUFS overlay switches for resetting  
  GetParam(kIGrContactControl)->InitBool("[GR meter reset]", 0, "");
  GetParam(kILufsContactControl)->InitBool("[Loudness meter reset]", 0, "");

  // *** Initing actual GUI controls

  // Plug window, background
  pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);

  /* Meters - start */

  // True peaking label
  // One for "okay" state, one for "alert" state (different color),
  // and one that stores the current text label pointer
  // for okay state
  IText tTpOkLabel = IText(PLUG_TP_LABEL_STRING_SIZE);
  tTpOkLabel.mColor = PLUG_TP_LABEL_OK_COLOR;
  tTpOkLabel.mSize = PLUG_TP_LABEL_FONT_SIZE;
  tTpOkLabel.mAlign = tTpOkLabel.PLUG_TP_TEXT_ALIGNMENT;
  tTpOkTextControl = new ITextControl(
    this,
    PLUG_TP_LABEL_IRECT,
    &tTpOkLabel,
    PLUG_TP_LABEL_DEFAULT_TEXT);
  // for alert state
  IText tTpAlertLabel = IText(tTpOkLabel);
  tTpAlertLabel.mColor = PLUG_TP_LABEL_ALERT_COLOR;
  tTpAlertTextControl = new ITextControl(
    this,
    PLUG_TP_LABEL_IRECT,
    &tTpAlertLabel,
    PLUG_TP_LABEL_DEFAULT_TEXT);
  // for reset (default) state
  IText tTpResetLabel = IText(tTpOkLabel);
  tTpResetLabel.mColor = PLUG_TP_LABEL_RESET_COLOR;
  tTpResetTextControl = new ITextControl(
    this,
    PLUG_TP_LABEL_IRECT,
    &tTpResetLabel,
    PLUG_TP_LABEL_DEFAULT_TEXT);
  // for current state
  tTpTextControl = tTpResetTextControl;

  // TP labels should be attached later, so they are sandwitched
  // between meter bars and meter reset buttons. Otherwise they
  // may get in the way of clicking. 


  // Dynamic range label - similar to true peaking label
  IText tDrOkLabel = IText(PLUG_DR_LABEL_STRING_SIZE);
  tDrOkLabel.mColor = PLUG_DR_LABEL_OK_COLOR;
  tDrOkLabel.mSize = PLUG_DR_LABEL_FONT_SIZE;
  tDrOkLabel.mAlign = tDrOkLabel.PLUG_DR_TEXT_ALIGNMENT;
  tDrOkTextControl = new ITextControl(
    this,
    PLUG_DR_LABEL_IRECT,
    &tDrOkLabel,
    PLUG_DR_LABEL_DEFAULT_TEXT);
  // for warning state
  IText tDrWarningLabel = IText(tDrOkLabel);
  tDrWarningLabel.mColor = PLUG_DR_LABEL_WARNING_COLOR;
  tDrWarningTextControl = new ITextControl(
    this,
    PLUG_DR_LABEL_IRECT,
    &tDrWarningLabel,
    PLUG_DR_LABEL_DEFAULT_TEXT);
  // for reset state
  IText tDrResetLabel = IText(tDrOkLabel);
  tDrResetLabel.mColor = PLUG_DR_LABEL_RESET_COLOR;
  tDrResetTextControl = new ITextControl(
    this,
    PLUG_DR_LABEL_IRECT,
    &tDrResetLabel,
    PLUG_DR_LABEL_DEFAULT_TEXT);
  // for current state
  tDrTextControl = tDrResetTextControl;

  // Bars
  // LUFS meter
  tILevelMeteringBar = new Plug::ILevelMeteringBar(
    this,
    kLufsMeter_X,
    kLufsMeter_Y,
    PLUG_LUFS_METERING_BAR_IRECT,
    kILevelMeteringBar,
    false,
    &LOUDNESS_BAR_FG_ICOLOR
    );
  tILevelMeteringBar->SetNotchValue(PLUG_LUFS_RANGE_MAX);
  pGraphics->AttachControl(tILevelMeteringBar);

  // Gain Reduction meter
  tIGrMeteringBar = new Plug::ILevelMeteringBar(
    this,
    kGrMeter_X,
    kGrMeter_Y,
    PLUG_GR_METERING_BAR_IRECT,
    kIGrMeteringBar,
    true,
    &GR_BAR_DEFAULT_FG_ICOLOR,
    &GR_BAR_DEFAULT_NOTCH_ICOLOR,
    &METERING_BAR_ABOVE_NOTCH_ICOLOR);
  pGraphics->AttachControl(tIGrMeteringBar);

  // Overlay labels
  // "Loudness"
  tBmp = pGraphics->LoadIBitmap(
    LOUDNESSLABELOVERLAY_ID,
    LOUDNESSLABELOVERLAY_FN,
    1
    );
  tLoudnessLabelOverlay = new IBitmapControl(this, kLoudnessLabelOverlay_X, kLoudnessLabelOverlay_Y, &tBmp);
  pGraphics->AttachControl(tLoudnessLabelOverlay);
  // "Gain reduction"
  tBmp = pGraphics->LoadIBitmap(
    GRLABELOVERLAY_ID,
    GRLABELOVERLAY_FN,
    1
    );
  tGrLabelOverlay = new IBitmapControl(this, kGrLabelOverlay_X, kGrLabelOverlay_Y, &tBmp);
  pGraphics->AttachControl(tGrLabelOverlay);

  // Attaching TP and Dynamic range labels so they are below reset buttons
  pGraphics->AttachControl(tTpOkTextControl);
  pGraphics->AttachControl(tTpAlertTextControl);
  pGraphics->AttachControl(tTpResetTextControl);
  tTpOkTextControl->Hide(true);
  tTpAlertTextControl->Hide(true);
  tTpResetTextControl->Hide(false);
  pGraphics->AttachControl(tDrOkTextControl);
  pGraphics->AttachControl(tDrWarningTextControl);
  pGraphics->AttachControl(tDrResetTextControl);
  tDrOkTextControl->Hide(true);
  tDrWarningTextControl->Hide(true);
  tDrResetTextControl->Hide(false);

  // Reset buttons - same coordinates as the actual meter bar
  tBmp = pGraphics->LoadIBitmap(
    METEROVERLAYSWITCH_ID,
    METEROVERLAYSWITCH_FN,
    kIContactControl_N
    );
  TIGrContactControl = new IContactControl(
    this, kGrMeter_X, kGrMeter_Y, kIGrContactControl, &tBmp);
  pGraphics->AttachControl(TIGrContactControl);
    TILufsContactControl = new IContactControl(
    this, kLufsMeter_X, kLufsMeter_Y, kILufsContactControl, &tBmp);
  pGraphics->AttachControl(TILufsContactControl);

  /* Meters - end */

  // Bypass switch
  tBmp = pGraphics->LoadIBitmap(BYPASSSWITCH_ID, BYPASSSWITCH_FN, kBypassSwitchFrames);
  tBypassSwitch = new ISwitchControl(this, kBypassSwitchX, kBypassSwitchY, kBypassSwitch, &tBmp);
  pGraphics->AttachControl(tBypassSwitch);

  // Limiter knob
  tBmp = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  tPeakingKnob = new IKnobMultiControl(this, kCeilingX, kCeilingY, kCeiling, &tBmp);
  pGraphics->AttachControl(tPeakingKnob);

  // Adjust knob
  tBmp = pGraphics->LoadIBitmap(ADJUST_ID, ADJUST_FN, kAdjustFrames);
  tAdjustKnob = new IKnobMultiControl(this, kAdjustX, kAdjustY, kAdjust, &tBmp);
  pGraphics->AttachControl(tAdjustKnob);
  
  // Mode selector (learn-master-off)
  tBmp = pGraphics->LoadIBitmap(MODESWITCH_ID, MODESWITCH_FN, kModeSwitch_N);
  tModeSwitch = new ISwitchControl(this, kModeSwitch_X, kModeSwitch_Y, kModeSwitch, &tBmp);
  pGraphics->AttachControl(tModeSwitch);

  // Platform selector (YT, spotify etc.)
  // Rotatable control
  tBmp = pGraphics->LoadIBitmap(PLATFORMSWITCH_ID, PLATFORMSWITCH_FN, kPlatformSwitch_N);
  tPlatformSelector = new IKnobMultiControl(
    this,
    kPlatformSwitch_X,
    kPlatformSwitch_Y,
    kPlatformSwitch,
    &tBmp
    );
  pGraphics->AttachControl(tPlatformSelector);
  // Same control, but lest user click on platform names too
  tBmp = pGraphics->LoadIBitmap(MODESWITCHCLICKABLE_ID, MODESWITCHCLICKABLE_FN, kPlatformSwitchClickable_N);
  tPlatformSelectorClickable = new IRadioButtonsControl(
    this,
    tPlatformSwitchClickableIRect,
    kPlatformSwitchClickable,
    kPlatformSwitchClickable_TOTAL,
    &tBmp
    );
  //IRadioButtonsControl(this, IRECT(kIRadioButtonsControl_V_X, kIRadioButtonsControl_V_Y, kIRadioButtonsControl_V_X + (kIRBC_W*kIRBC_VN), kIRadioButtonsControl_V_Y + (kIRBC_H*kIRBC_VN)), kIRadioButtonsControl_V, kIRBC_VN, &bitmap));
  pGraphics->AttachControl(tPlatformSelectorClickable);
  // Grey out overlay over platform names
  tBmp = pGraphics->LoadIBitmap(MODERADIOSWITCHGREYOUT_ID, MODERADIOSWITCHGREYOUT_FN, 1);
  tPlatformSelectorClickableGreyOut = new IBitmapControl(
    this,
    kModeRadioSwitchGreyOut_X,
    kModeRadioSwitchGreyOut_Y,
    &tBmp
    );
  pGraphics->AttachControl(tPlatformSelectorClickableGreyOut);
  tPlatformSelectorClickableGreyOut->Hide(true);
  
  // Text LUFS meter
  static IText tDefaultLoudnessLabel = IText(PLUG_METER_TEXT_LABEL_STRING_SIZE);
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
  static IText tGrLabel = IText(PLUG_METER_TEXT_LABEL_STRING_SIZE);  
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
  static IText tPeakingLabel = IText(PLUG_KNOB_TEXT_LABEL_STRING_SIZE);
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

  // Text Adjust knob guide
  static IText tAdjustLabel = IText(PLUG_KNOB_TEXT_LABEL_STRING_SIZE);
  tAdjustLabel.mColor = PLUG_KNOB_TEXT_LABEL_COLOR;
  tAdjustLabel.mSize = PLUG_KNOB_TEXT_LABEL_FONT_SIZE;
  tAdjustLabel.mAlign = tPeakingLabel.kAlignCenter;
  tAdjustTextControl = new ITextControl(
    this,
    IRECT(
      kAdjustTextControlX,
      kAdjustTextControlY,
      (kAdjustTextControlX + kAdjustTextControlW),
      (kAdjustTextControlY + kAdjustTextControlH)
    ),
    &tAdjustLabel,
    "-");
  pGraphics->AttachControl(tAdjustTextControl);
  tAdjustTextControl->Hide(true);

  // Text for the mastering mode
  static IText tModeLabel = IText(PLUG_MODE_TEXT_LABEL_STRING_SIZE);
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

  // Text label with current version of the plug
  // TODO: Make it clickable so it leads to a website or something
  static IText tTextVersion = IText(PLUG_VERSION_TEXT_LABEL_STRING_SIZE);
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
  #ifdef _PLUG_VERSION_H
  sprintf(
    sDisplayedVersion,
    PLUG_VERSTION_TEXT,
    VST3_VER_STR,
    (char*)sPlugVersionGitHead,
    (char*)sPlugVersionDate
    );
  #else
  sprintf(
    sDisplayedVersion,
    PLUG_VERSTION_TEXT,
    VST3_VER_STR
    );
  #endif
  tTextVersion.mColor = tTextVersionColor;
  tTextVersion.mSize = PLUG_VERSION_TEXT_LABEL_FONT_SIZE;
  tTextVersion.mAlign = tTextVersion.kAlignNear;
  pGraphics->AttachControl(
    new ITextControl(
      this,
      tTextVersionRect,
      &tTextVersion,
      (const char*)&sDisplayedVersion
      )
    );

  // Clickable area leading to a website
  tWebsiteLink = new IURLControl(this, tWebsiteLinkIRect, PLUG_WEBSITE_LINK);
  pGraphics->AttachControl(tWebsiteLink);

  // Guide message
  sModeString = new char[PLUG_MODE_TEXT_LABEL_STRING_SIZE];
  sprintf(sModeString, PLUG_OFF_STARTUP_MESSAGE);
  tModeTextControl->SetTextFromPlug(sModeString);

  // Bugreport link
  static IText tBugreportLabel = IText(DLPG_BUGREPORT_LABEL_STRING_SIZE);
  tBugreportLabel.mColor = tBugreportLabelColor;
  tBugreportLabel.mSize = DLPG_BUGREPORT_LABEL_FONT_SIZE;
  tBugreportLabel.mAlign = tBugreportLabel.kAlignNear;
  pGraphics->AttachControl(
    new ITextControl(
      this,
      tBugreportLabelIrect,
      &tBugreportLabel,
      DLPG_BUGREPORT_LABEL_TEXT
      )
    );
  // Clickable area for bugreports
  MakeFeedbackUrl(sFeedbackUrl);
  tFeedbackLink = new IURLControl(this, tFeedbackLinkIRect, sFeedbackUrl);
  pGraphics->AttachControl(tFeedbackLink);

  // Finally - feed all the controls to IPlug's graphics gizmo
  AttachGraphics(pGraphics);
  // *** Initing actual GUI controls - end

  // *** Stuff for signal processing
  //Limiter 
  tLimiter = new chunkware_simple::SimpleLimit();
  /* Limiter's threshold is always PLUG_LIMITER_DEFAULT_THRESHOLD_DB,
  which is 0dB by default. The actual threshold is controlled by pre-
  and post-limiter gain. */
  
  tLimiter->setThresh(PLUG_LIMITER_DEFAULT_THRESHOLD_DB);
  tLimiter->setAttack(PLUG_LIMITER_ATTACK_MILLISECONDS);
  tLimiter->initRuntime();

  // LUFS loudness meter 
  tLoudnessMeter = new Plug::LoudnessMeter();
  tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 
  
  /*
  Value inits
  Since we have to consider not only the case when user adds the plugin
  for the first time, but also the case when user recalls previously saved
  session, we have to be careful with which walues we should reset excplicitly,
  and which ones we should leave to host (DAW) to initiate.
  
  Recallable parameters:
  fTargetLoudness - updated in UpdatePlatform()
  nCurrentTargetIndex - dealt with below
  fMasteringGainDb - updated in UpdatePreMastering() called from UpdatePlatform()
  fTargetLufsIntegratedDb - updated in UpdatePreMastering() called from UpdatePlatform()
  fSourceLufsIntegratedDb - dealt with in OnParamChange()
  fLimiterCeilingDb - updated in UpdatePreMastering() called from UpdatePlatform()
  fLimiterCeilingLinear - updated in UpdatePreMastering() called from UpdatePlatform()
  fMasteringGainLinear - updated in UpdatePreMastering() called from UpdatePlatform()
  tPlugCurrentMode - dealt with below
  bLufsTooLow - dealt with below
  bIsBypassed - dealt with below
  */

  // Bypass switch state
  bIsBypassed = (bool)GetParam(kBypassSwitch)->Bool();

  // GR meter values - non-recallable
  fMaxGainReductionPerFrame = PLUG_MAX_GAIN_REDUCTION_PER_FRAME_DB_RESET;
  fMaxGainReductionPerSessionDb = PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET;

  // Off, Learn or Master
  tPlugCurrentMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);

  /* Source LUFS
  If the mode on startup is "master", i.e. not the default one, that means that
  the plugin is being recalled from a previously saved project. In that case we
  get the value from UnserializeState() where it should be stored with regular
  control values. Otherwise we initialize with some default value.
  */
  bNeedToRecallSourceLufs = (tPlugCurrentMode == PLUG_MASTER_MODE);
  if (tPlugCurrentMode == PLUG_INITIAL_MODE)
    fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTEGRATED_DB_RESET;

  // Target platform
  nCurrentTargetIndex = (unsigned short)GetParam(kPlatformSwitch)->Int();

  // Target loudness user adjustement
  fAdjustLufsDb = PLUG_KNOB_ADJUST_ROUND(GetParam(kAdjust)->Value());

  // ###
  /* Low (-inf) LUFS flag. Doesn't allow user to go into mastering mode
  unless plugin successfully got source LUFS reading first. */
  bLufsTooLow = !((fSourceLufsIntegratedDb > PLUG_LUFS_TOO_LOW) || PLUG_ALWAYS_ALLOW_MASTERING);

  /* Calling UpdatePlatform() updates:
  fTargetLoudness
  fTargetLufsIntegratedDb
  fLimiterCeilingDb
  fLimiterCeilingLinear
  fMasteringGainDb
  fMasteringGainLinear
  */
  /*
  if(tPlugCurrentMode == PLUG_MASTER_MODE)
    UpdatePlatform((PLUG_Target)nCurrentTargetIndex);
  */
  // Set up TP buffer for PSR calculation
  tPeakingBuffer = new DLPG::PeakingBuffer(0, PLUG_DB_VALUE_TOO_LOW);
  
  tPeakingBuffer->Resize(
    PLUG_DR_METER_WINDOW_SECONDS,
    GetSampleRate(),
    GetBlockSize()
    );

  // Knob gearing
  tPeakingKnob->SetGearing(PLUG_PEAKING_KNOB_GEARING);
  tPlatformSelector->SetGearing(PLUG_PLATFORM_SELECTOR_GEARING);
  tAdjustKnob->SetGearing(PLUG_ADJUST_KNOB_GEARING);

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

  // Assuming we're working with a stereo signal
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  double *afInterleavedSamples;

  if(bIsBypassed){
    // True bypass
    std::memcpy(out1, in1, sizeof(double) * nFrames);
    std::memcpy(out2, in2, sizeof(double) * nFrames);
  }
  else{
    // If plugin is active
    switch(tPlugCurrentMode){
      case PLUG_LEARN_MODE:
      // Learn mode
      // Feed in the audio into the LUFS meter, but not changing it
      afInterleavedSamples = new double[nFrames * 2];
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
      afInterleavedSamples = new double[nFrames * 2];
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

        // Collect samples for loudness measurement, post-limiter
        afInterleavedSamples[sample]   = *out1;
        afInterleavedSamples[sample+1] = *out2;
      }
      tLoudnessMeter->AddSamples(afInterleavedSamples, nFrames);
      delete[] afInterleavedSamples;
      break;

      case PLUG_OFF_MODE:
      // Off mode
      // Do nothing to the audio
      std::memcpy(out1, in1, sizeof(double) * nFrames);
      std::memcpy(out2, in2, sizeof(double) * nFrames);

      break;
    } //switch
  } // if(bIsBypassed)

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
MasteringGainDb = (TargetLufsIntegratedDb + AdjustLufsDb) - SourceLufsIntegratedDb - LimiterCeilingDb,
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
  double fAdjustedTargetLufsIntegratedDb;

  // *** Target LUFS 
  fTargetLufsIntegratedDb = PLUG_GET_TARGET_LOUDNESS((int)mPlatform);

  // *** Limiter ceiling
  fLimiterCeilingDb = PLUG_KNOB_PEAK_DOUBLE(GetParam(kCeiling)->Value());
  fLimiterCeilingLinear = LOG_TO_LINEAR(fLimiterCeilingDb);

  // *** Target loudness user adjustement
  fAdjustLufsDb = PLUG_KNOB_ADJUST_ROUND(GetParam(kAdjust)->Value());
  fAdjustedTargetLufsIntegratedDb = fTargetLufsIntegratedDb + fAdjustLufsDb;

  // *** Mastering gain in dB
  fMasteringGainDb = \
    fAdjustedTargetLufsIntegratedDb - fSourceLufsIntegratedDb - fLimiterCeilingDb;

  // *** Mastering gain in linear
  fMasteringGainLinear = LOG_TO_LINEAR(fMasteringGainDb);

  // Now that we know mastering gain, we just apply it so the audio stream
  // and feed it into the limiter afterwards
  // ...but before, let's output some text
  sprintf(sModeString,
    PLUG_MASTER_GUIDE_MESSAGE,
    asTargetDescription[nCurrentTargetIndex],
    fSourceLufsIntegratedDb,
    fAdjustedTargetLufsIntegratedDb,
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
  double fAdjust = PLUG_KNOB_ADJUST_ROUND(GetParam(kAdjust)->Value());
  fTargetLoudness = PLUG_GET_TARGET_LOUDNESS((int)mPlatform);
  // Update the notch on the LUFS meter
  tILevelMeteringBar->SetNotchValue(fTargetLoudness + fAdjust);

  if (tPlugCurrentMode == PLUG_MASTER_MODE)
    UpdatePreMastering((PLUG_Target)mPlatform);
} 

/*
\brief Handles all user interactions with the controls
*/
void StreamMaster::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  double fPeaking, fAdjust;
  unsigned int nIndex;
  char sPeakingString[PLUG_KNOB_TEXT_LABEL_STRING_SIZE];
  char sAdjustString[PLUG_KNOB_TEXT_LABEL_STRING_SIZE];
  PLUG_Mode tPlugNewMode;
  int nModeNumber, nConvertedModeNumber;
  double fNormalizedConvertedModeNumber;
  static bool bPlatformSwitchClickableSentMe = false;
  static bool bPlatformSwitchRotarySentMe = false;
  char* psMessageToDisplay;

  // General control handling
  // Locking and unlocking of controls happens in UpdateAvailableControls()
  switch (paramIdx)
  {
    // Clicked on GR bar
    case kIGrContactControl:
      // Just reset the peak GR value on the GR bar
      fMaxGainReductionPerSessionDb = PLUG_MAX_GAIN_REDUCTION_PER_SESSION_DB_RESET;
      tIGrMeteringBar->SetNotchValue(fMaxGainReductionPerSessionDb);
      break;

    // Clicked on Loudness bar
    case kILufsContactControl:
      // Resetting LUFS meter bar and the actual meter
      /* TODO: this switch probably falsely triggers during the startup once,
      so it's a good idea to avoid unnecessary destruction */
      delete tLoudnessMeter;
      tLoudnessMeter = new Plug::LoudnessMeter();
      tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER); 
      UpdateSampleRate();
      // Set notch on LUFS meter
      fAdjust = PLUG_KNOB_ADJUST_ROUND(GetParam(kAdjust)->Value());
      tILevelMeteringBar->SetValue(PLUG_SOURCE_LUFS_INTEGRATED_DB_RESET);
      tILevelMeteringBar->SetNotchValue(fTargetLufsIntegratedDb + fAdjust);
      // Reset source LUFS if we were measuring it
      tPlugCurrentMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);
      if (tPlugCurrentMode == PLUG_LEARN_MODE) fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTEGRATED_DB_RESET;
      // Reset dynimic range meter
      if(tPlugCurrentMode == PLUG_MASTER_MODE){
        tPeakingBuffer->Clear();
      }
      break;

    // Tweaked ceiling knob
    case kCeiling:
      fPeaking = PLUG_KNOB_PEAK_DOUBLE(GetParam(kCeiling)->Value());

      // Text label of knob's value
      sprintf(sPeakingString, "%5.2fdB", fPeaking);
      tPeakingTextControl->SetTextFromPlug(sPeakingString);

      nIndex = GetParam(kPlatformSwitch)->Int();
      if (tPlugCurrentMode == PLUG_MASTER_MODE)
        UpdatePreMastering((PLUG_Target)nIndex);
      break;

    // Tweaked adjust knob
    case kAdjust:
      fAdjust = PLUG_KNOB_ADJUST_ROUND(GetParam(kAdjust)->Value());

      // Text label of knob's value
      sprintf(sAdjustString, "%+5.2fLUFS", fAdjust);
      tAdjustTextControl->SetTextFromPlug(sAdjustString);

      nIndex = GetParam(kPlatformSwitch)->Int();
      if (tPlugCurrentMode == PLUG_MASTER_MODE)
        UpdatePreMastering((PLUG_Target)nIndex);
        tILevelMeteringBar->SetNotchValue(PLUG_GET_TARGET_LOUDNESS(nIndex) + fAdjust);
      break;

    // Changed target platform via rotary switch
    case kPlatformSwitch:

      nCurrentTargetIndex = (unsigned int)GetParam(kPlatformSwitch)->Int();

      /*
      Since rotary and clickable controls are bonded with each other,
      we have to notify the other control of the change, but make sure
      it doesn't cause an infinite loop between them (hence use two flags).
      Plus VST and AU needs slightly different implementation to work properly.
      */
      nConvertedModeNumber = PLUG_REVERSE_PLATFORM_SWITCH_VALUE(nCurrentTargetIndex);

      #if defined(VST_API) || defined(AU_API)
      // VST2 and AU
      fNormalizedConvertedModeNumber = PLUG_NORMALIZE_PLATFORM_SWITCH_VALUE(nCurrentTargetIndex);
      bPlatformSwitchClickableSentMe = false;
      #else
      // VST3
      fNormalizedConvertedModeNumber = GetParam(kPlatformSwitch)->GetNormalized();
      #endif
      bPlatformSwitchRotarySentMe = true;
      if(bPlatformSwitchClickableSentMe){
        bPlatformSwitchClickableSentMe = false;
        break;
      }
      else{
        GetGUI()->SetParameterFromPlug(kPlatformSwitchClickable, nConvertedModeNumber, false);
        InformHostOfParamChange(kPlatformSwitchClickable, 1.-fNormalizedConvertedModeNumber);
      }

      // Update all related values
      UpdatePlatform((PLUG_Target)nCurrentTargetIndex);

      break;

    // Changed target platform via clicking on a platfom's name
    case kPlatformSwitchClickable:
      /*
      Apparently it can be falsely triggered during startup,
      so we have to ignore that one
      */
      tPlugNewMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);
      if (tPlugNewMode != PLUG_MASTER_MODE) break;

      /*
      Since rotary and clickable controls are bonded with each other,
      we have to notify the other control of the change, but make sure
      it doesn't cause an infinite loop between them (hence use two flags).
      Plus VST2 and AU needs slightly different implementation to work properly.
      */
      nModeNumber = GetParam(kPlatformSwitchClickable)->Int();
      nConvertedModeNumber = PLUG_REVERSE_PLATFORM_SWITCH_VALUE(nModeNumber);

      #if defined(VST_API) || defined(AU_API)
      // VST2 and AU
      fNormalizedConvertedModeNumber = PLUG_NORMALIZE_PLATFORM_SWITCH_VALUE(nModeNumber);
      bPlatformSwitchRotarySentMe = false;
      #else
      // VST3
      fNormalizedConvertedModeNumber = GetParam(kPlatformSwitchClickable)->GetNormalized();
      #endif
      bPlatformSwitchClickableSentMe = true;
      if(bPlatformSwitchRotarySentMe){
        bPlatformSwitchRotarySentMe = false;
        break;
      }
      else{
        GetGUI()->SetParameterFromPlug(kPlatformSwitch, nConvertedModeNumber, false);
        InformHostOfParamChange(kPlatformSwitch, 1.-fNormalizedConvertedModeNumber);
      }
      // This is a global value used by other methods. Careful with it!
      nCurrentTargetIndex = nConvertedModeNumber;

      // Update all related values
      UpdatePlatform((PLUG_Target)nCurrentTargetIndex);
      break;

    // Clicked mode switch
    case kModeSwitch:
      /*
      Make sure we update all following variables:
      v fMasteringGainDb,
      v fTargetLufsIntegratedDb,
      v fSourceLufsIntegratedDb,
      v fLimiterCeilingDb,
      v fLimiterCeilingLinear,
      v fMasteringGainLinear;
      */
      // Converting switch's value to mode
      tPlugNewMode = PLUG_CONVERT_SWITCH_VALUE_TO_PLUG_MODE(kModeSwitch);

      if ((tPlugNewMode == tPlugCurrentMode) && !bNeedToRecallSourceLufs){
          // Apparently it can be falsely triggered during startup, 
          // so we have to ignore that one
          break;
      }

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

        // Getting source loudness - either saved or measured value
        if(bNeedToRecallSourceLufs){
          /*
          In that case source's LUFS is recalled via UnserializeState() after
          the plugin initialization. That flag is set in the constructor though,
          UnserializeState() has its own flag, which is dealt with below.
          */
          bNeedToRecallSourceLufs = false;
        }
        else{
          if ((bJustRecalledSourceLufs &&
            (fSourceLufsIntegratedDb > PLUG_LUFS_TOO_LOW)) ||
            PLUG_ALWAYS_ALLOW_MASTERING)
            /* 
            In case we just recalled LUFS value from a previously saved project,
            we don't want to override it. This flag is set in UnserializeState(). 
            */
            bJustRecalledSourceLufs = false;
          else
            /*
            This is the case when the user switches the mode from "learn" to "master". 
            Here we're just getting loudness value from LUFS metering algorithm's object. 
            */
            fSourceLufsIntegratedDb = tLoudnessMeter->GetLufs();
          /*
          Storing this value is handled by SerializeState() automatically
          every time the project (or poreset) is saved in the host.
          */
        }

        // *** Check if learn mode was successful - start
        /*
        We check if source loudness is -oo, or equals to default value,
        or there's a debug flag that tells us to ignore that step
        */
        if((fSourceLufsIntegratedDb > PLUG_LUFS_TOO_LOW) || PLUG_ALWAYS_ALLOW_MASTERING){

          bLufsTooLow = false; 

          // Resetting the meter
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
        // *** Check if learn mode was successful - end

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
        fTargetLufsIntegratedDb = PLUG_TARGET_LUFS_INTEGRATED_DB_RESET;
        fSourceLufsIntegratedDb = PLUG_SOURCE_LUFS_INTEGRATED_DB_RESET;
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

    // Clicked On/off bypass switch
    case kBypassSwitch:
      bIsBypassed = (bool)GetParam(kBypassSwitch)->Bool();
      psMessageToDisplay = bIsBypassed ? sBypassString : sModeString;
      tModeTextControl->SetTextFromPlug(psMessageToDisplay);
      UpdateAvailableControls();
    default:
      break;
  }
}

/*
@brief Storing some hidden data when the project is saved in a host
*/
bool StreamMaster::SerializeState(ByteChunk* pChunk)
{
  TRACE;
  IMutexLock lock(this);
  double fFloatData;

  // Storing source loudness
  fFloatData = fSourceLufsIntegratedDb;
  pChunk->Put(&fFloatData);

  // Make sure to always call UnserializeParams() at the end
  return IPlugBase::SerializeParams(pChunk);
}

/*
@brief Recalling some hidden data when the saved project with this plugin is re-opened
*/ 
int StreamMaster::UnserializeState(ByteChunk* pChunk, int nStartPos)
{
  TRACE;
  IMutexLock lock(this);

  // Recalling source loudness
  nStartPos = pChunk->Get(&fSourceLufsIntegratedDb, nStartPos);

  // Setting a flag so we won't accidentally override it during the re-initialization
  // We imply that we have a valid recalled value if it's above the default startup one
  //bJustRecalledSourceLufs |= (fSourceLufsIntegratedDb > PLUG_SOURCE_LUFS_INTEGRATED_DB_RESET + PLUG_EPSILON);
  bJustRecalledSourceLufs = true;

  /* Low (-inf) LUFS flag. Doesn't allow user to go into mastering mode
  unless plugin successfully got source LUFS reading first. */
  bLufsTooLow = !(fSourceLufsIntegratedDb > PLUG_LUFS_TOO_LOW);

  /* Calling UpdatePlatform() updates:
  fTargetLoudness
  fTargetLufsIntegratedDb
  fLimiterCeilingDb
  fLimiterCeilingLinear
  fMasteringGainDb
  fMasteringGainLinear
  */

  if(tPlugCurrentMode == PLUG_MASTER_MODE)
    UpdatePlatform((PLUG_Target)nCurrentTargetIndex);

  // Make sure to always call UnserializeParams() at the end
  return IPlugBase::UnserializeParams(pChunk, nStartPos);
}

bool StreamMaster::CompareState(const unsigned char* incomingState, int startPos)
{
  bool bIsEqual;
  size_t nCustomDataSize;
  const double* pfInData = (const double*) incomingState;

  // Size of all custom parameters
  // Currently it's just one double variable
  nCustomDataSize = sizeof(double);
  startPos = nCustomDataSize;

  // Compare custom params
  bIsEqual = (memcmp(pfInData, &fSourceLufsIntegratedDb, nCustomDataSize) == 0);
  // Fuzzy compare regular params
  bIsEqual &= IPlugBase::CompareState(incomingState, startPos);
  
  return bIsEqual;
}

void StreamMaster::PresetsChangedByHost()
{
  TRACE;
  IMutexLock lock(this);

  if(GetGUI())
    GetGUI()->SetAllControlsDirty();
}
