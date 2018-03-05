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
IKnobMultiControl *tPeakingKnob;
IKnobMultiControl *tPlatformSelector;
IGraphics* pGraphics;
Plug::ILevelMeteringBar* tILevelMeteringBar;
Plug::ILevelMeteringBar* tIGrMeteringBar;
double fAudioFramesPerSecond = 1.;
double fMaxGainReductionPerFrame = 0.;
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
    tILevelMeteringBar->GrayOut(false);
    break;
  case PLUG_MASTER_MODE:
    // Master mode
    // Everything unlocked
    tIGrMeteringBar->GrayOut(false);
    tPeakingKnob->GrayOut(false);
    tPlatformSelector->GrayOut(false);
    tILevelMeteringBar->GrayOut(false);
    break;
  case PLUG_OFF_MODE:
    // Learn mode
    // Only mode switch
    tIGrMeteringBar->GrayOut(true);
    tPeakingKnob->GrayOut(true);
    tPlatformSelector->GrayOut(true);
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
  GetParam(kGain)->InitDouble("Peaking", -0.3, -1., 0.0, 0.1, "dB");
  GetParam(kGain)->SetShape(2.);
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
  IText tDefaultLoudnessLabel = IText(32);
  tDefaultLoudnessLabel.mColor = IColor(255, 255, 255, 255);
  tDefaultLoudnessLabel.mSize = 12;
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
  IText tGrLabel = IText(32);  
  tGrLabel.mColor = IColor(255, 255, 255, 255);
  tGrLabel.mSize = 12;
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

double fSampleSample;
StreamMaster::~StreamMaster() {}
void StreamMaster::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  double *afInterleavedSamples = new double[nFrames * 2];

  for (int frame = 0, sample = 0; frame < nFrames; ++frame, ++in1, ++in2, ++out1, ++out2, sample += 2)
  {
    *out1 = *in1;
    *out2 = *in2;
    tLimiter.process(*out1, *out2);
    tLimiter.getGr(&fMaxGainReductionPerFrame);
    fSampleSample = *in1;
    afInterleavedSamples[sample]   = *in1;
    afInterleavedSamples[sample+1] = *in2;
  }


  in1 = inputs[0];
  in2 = inputs[1];

  double fRandom;

  tLoudnessMeter->AddSamples(afInterleavedSamples, nFrames);
  delete[] afInterleavedSamples;
  fAudioFramesPerSecond = GetSampleRate()/ nFrames;
  UpdateGui();

}

void StreamMaster::Reset()
{
  TRACE;
  IMutexLock lock(this);
}

void StreamMaster::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
  double fLufs;
  unsigned int nIndex;

  switch (paramIdx)
  {
    case kGain:
      tLimiter.setThresh(GetParam(kGain)->Value());
      break;
    //Platform control
    case kPlatformSwitch:
      nIndex = (unsigned int)GetParam(kPlatformSwitch)->Value();
      fTargetLoudness = PLUG_GET_TARGET_LOUDNESS(nIndex);
      tILevelMeteringBar->SetNotchValue(fTargetLoudness);
      tILevelMeteringBar->Redraw();
      break;

    case kModeSwitch:
      // Converting switch's value to mode
      tPlugCurrentMode = (PLUG_Mode)int(GetParam(kModeSwitch)->Value()+1);
      UpdateAvailableControls();
      break;

    default:
      break;
  }
}
