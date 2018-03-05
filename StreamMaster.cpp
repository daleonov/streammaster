#include "StreamMaster.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "PLUG_IControlExtras.h"
#include "resource.h"
#include "lib_chunkware/SimpleLimit.h"
#include "PLUG/PLUG_LoudnessMeter.h"


const int kNumPrograms = 1;
chunkware_simple::SimpleLimit tLimiter;
const double fDefaultLimiterThreshold = 0.;

ITextControl *tLoudnessTextControl;
ITextControl *tGrTextControl;
ITextControl *tPeakingTextControl;
ITextControl *tModeTextControl;
IKnobMultiControl *tPeakingKnob;
IKnobMultiControl *tPlatformSelector;
IGraphics* pGraphics;
Plug::ILevelMeteringBar* tILevelMeteringBar;
Plug::ILevelMeteringBar* tIGrMeteringBar;
double fAudioFramesPerSecond = 1.;
double fMaxGainReductionPerFrame = 1.;
double fMaxGainReductionPerSessionDb = -0.;
double fTargetLoudness = PLUG_DEFAULT_TARGET_LOUDNESS;

// Plugin starts up in this mode
PLUG_Mode tPlugCurrentMode = PLUG_LEARN_MODE;

void StreamMaster::UpdateGui()
{
  const IColor* tLufsBarColor = new IColor(255, 255, 255, 255);
  IRECT *tLufsBarRect = new IRECT(10,10,20,100);
  char sLoudnessString[64];
  char sGrString[64];
  double fMaxGainReductionPerFrameDb = 0.;

  double fLufs = tLoudnessMeter->GetLufs();
  double fFastLufs = tLoudnessMeter->GetMomentaryLufs();

  sprintf(sLoudnessString, "Integrated: %4.1fLUFS\nMomentary: %4.1fLUFS", fLufs, fFastLufs);
    //double fTestLoudnessValue = -15.*((double)rand() / RAND_MAX);
    tLoudnessTextControl->SetTextFromPlug(sLoudnessString);
    tILevelMeteringBar->SetValue(fLufs);

    // Gain reduction
    fMaxGainReductionPerFrameDb = LINEAR_TO_LOG(fMaxGainReductionPerFrame);
    // ("<" because gain reduction is negative in log domain)
    if (fMaxGainReductionPerFrameDb < fMaxGainReductionPerSessionDb)
      fMaxGainReductionPerSessionDb = fMaxGainReductionPerFrameDb;
    tIGrMeteringBar->SetValue(fMaxGainReductionPerFrameDb);
    tIGrMeteringBar->SetNotchValue(fMaxGainReductionPerSessionDb);
    sprintf(sGrString, "GR: %4.2fdB\nMax: %4.2fdB", fMaxGainReductionPerFrameDb, fMaxGainReductionPerSessionDb);
    tGrTextControl->SetTextFromPlug(sGrString);

    unsigned int nMetersWait = 0;
    int nMeterUpdatesPerSecond = 5;
    int nMetersWaitIterations = fAudioFramesPerSecond/ nMeterUpdatesPerSecond;

}
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

StreamMaster::StreamMaster(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{

  TRACE;

  const IColor* tLufsBarColor = new IColor(255, 255, 128, 0);
  IRECT *tLufsBarRect = new IRECT(0, 10, 40, 100);

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  // We need precise values, so we have to use integer values and convert them into double later
  GetParam(kGain)->InitInt("Peaking", 0,0, 10,  "dB");
  GetParam(kGain)->SetShape(1.);
  GetParam(kILevelMeteringBar)->InitDouble("Loudness", -24., -40., 3.0, 0.1, "LUFS");
  GetParam(kIGrMeteringBar)->InitDouble("Gain reduction", -0., -43., 0.0, 0.1, "dB");
  GetParam(kModeSwitch)->InitInt("Mode", 0, 0, 2, "");
  GetParam(kPlatformSwitch)->InitInt("Target platform", PLUG_DEFAULT_TARGET_PLATFORM, 0, 4, "");

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
  

  AttachGraphics(pGraphics);
  
  //Limiter 
  tLimiter.setThresh(fDefaultLimiterThreshold);
  tLimiter.setSampleRate(PLUG_DEFAULT_SAMPLERATE);
  tLimiter.setAttack(0.1);
  tLimiter.initRuntime();

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);

  //LUFS loudness meter 
  tLoudnessMeter = new Plug::LoudnessMeter();
  tLoudnessMeter->SetSampleRate(PLUG_DEFAULT_SAMPLERATE);
  tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER);  

  // Interface stuff
  UpdateAvailableControls();
}



double \
  fMasteringGainDb = 0., \
  fTargetLufsIntegratedDb = 14., \
  fSourceLufsIntegratedDb = -60., \
  fLimiterCeilingDb = 0., \
  fMasteringGainLinear = 1.;

StreamMaster::~StreamMaster() {}
void StreamMaster::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.
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
  sprintf(sModeString, "I'm ready to process the track!\nInput loudness: %0.2fLUFS, Target loudness: %0.2fLUFS\nCeiling: %0.2fdB, Applied gain: %0.2fdB",
    fSourceLufsIntegratedDb, fTargetLufsIntegratedDb, fLimiterCeilingDb, fMasteringGainDb);
  tModeTextControl->SetTextFromPlug(sModeString);
}
void StreamMaster::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
  double fLufs, fPeaking;
  unsigned int nIndex;
  char sPeakingString[PLUG_KNOB_TEXT_LABEL_STRING_SIZE];
  char sModeString[PLUG_MODE_TEXT_LABEL_STRING_SIZE];

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
      tILevelMeteringBar->SetNotchValue(fTargetLoudness);
      tILevelMeteringBar->Redraw();
      if (tPlugCurrentMode == PLUG_MASTER_MODE)
        UpdatePreMastering();
      break;
      /*
      double \
        v fMasteringGainDb, \
        v fTargetLufsIntegratedDb, \
        v fSourceLufsIntegratedDb, \
        v fLimiterCeilingDb, \
        v fMasteringGainLinear;*/

    case kModeSwitch:
      // Converting switch's value to mode
      tPlugCurrentMode = (PLUG_Mode)int(GetParam(kModeSwitch)->Value()+1);
      UpdateAvailableControls();
        switch (tPlugCurrentMode){
      case PLUG_LEARN_MODE:
        // Learn mode
        sprintf(sModeString, "Press play in your DAW to measure song's loudness,\nthen press Mode switch to auto adjust loudness");
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

        fMasteringGainLinear = 1.;
        fMasteringGainDb = 0.;
        fTargetLufsIntegratedDb = 0.;
        fSourceLufsIntegratedDb = -60.;
        fMaxGainReductionPerFrame = 1.;

        sprintf(sModeString, "I'm just chilling now. Press Mode switch again\nto do another song or measurement.");
        tModeTextControl->SetTextFromPlug(sModeString);

        break;
      }
      break;

    default:
      break;
  }
}
