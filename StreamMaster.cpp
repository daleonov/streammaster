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

ITextControl * tLoudnessTextControl;
IGraphics* pGraphics;
Plug::ILevelMeteringBar* tILevelMeteringBar;
Plug::ILevelMeteringBar* tIGrMeteringBar;
double fAudioFramesPerSecond = 1.;


void StreamMaster::UpdateGui()
{
  const IColor* tLufsBarColor = new IColor(255, 255, 255, 255);
  IRECT *tLufsBarRect = new IRECT(10,10,20,100);
  char sLoudnessString[64];
  double fLufs = tLoudnessMeter->GetLufs();
    if (fabs(fLufs - HUGE_VAL) < std::numeric_limits<double>::epsilon()) {
      sprintf(sLoudnessString, "-oo LUFS");
    }
    else {
      sprintf(sLoudnessString, "%4.2f LUFS", fLufs);
    }
    //double fTestLoudnessValue = -15.*((double)rand() / RAND_MAX);
    tLoudnessTextControl->SetTextFromPlug(sLoudnessString);
    tILevelMeteringBar->SetValue(fLufs);

    unsigned int nMetersWait = 0;
    int nMeterUpdatesPerSecond = 5;
    int nMetersWaitIterations = fAudioFramesPerSecond/ nMeterUpdatesPerSecond;

    //fAudioFramesPerSecond
    if (nMetersWait++ % nMetersWaitIterations) {
      tILevelMeteringBar->Draw(pGraphics);
      tILevelMeteringBar->Redraw();
      pGraphics->Draw(&IRECT(0, 0, GUI_WIDTH, GUI_HEIGHT));
      nMetersWait = 0;
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
  GetParam(kILevelMeteringBar)->InitDouble("Loudness", -24., -60., 3.0, 0.1, "LUFS");
  GetParam(kModeSwitch)->InitInt("Mode", 0, 0, 2, "");
  GetParam(kPlatformSwitch)->InitInt("Target platform", 4, 0, 4, "");

  pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  
  // LUFS meter
  tILevelMeteringBar = new Plug::ILevelMeteringBar(this, kLufsMeter_X, kLufsMeter_Y, METERING_BAR_DEFAULT_SIZE_IRECT, kILevelMeteringBar);
  pGraphics->AttachControl(tILevelMeteringBar);

  // Gain Reduction meter
  tIGrMeteringBar = new Plug::ILevelMeteringBar(this, kGrMeter_X, kGrMeter_Y, METERING_BAR_DEFAULT_SIZE_IRECT, kIGrMeteringBar);
  pGraphics->AttachControl(tIGrMeteringBar);
    
  // Limiter knob
  IBitmap tBmp = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX, kGainY, kGain, &tBmp));
  
  // Mode selector (learn-master-off)
  tBmp = pGraphics->LoadIBitmap(MODESWITCH_ID, MODESWITCH_FN, kModeSwitch_N);
  pGraphics->AttachControl(new ISwitchControl(this, kModeSwitch_X, kModeSwitch_Y, kModeSwitch, &tBmp));

  tBmp = pGraphics->LoadIBitmap(PLATFORMSWITCH_ID, PLATFORMSWITCH_FN, kPlatformSwitch_N);
  pGraphics->AttachControl(new IKnobMultiControl(this, kPlatformSwitch_X, kPlatformSwitch_Y, kPlatformSwitch, &tBmp));

  // Text LUFS meter
  IText tDefaultLoudnessLabel = IText(32);

  tDefaultLoudnessLabel.mColor = IColor(255, 255, 255, 255);
  tDefaultLoudnessLabel.mSize = 20;
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

  AttachGraphics(pGraphics);
  
  //Limiter 
  tLimiter.setThresh(fDefaultLimiterThreshold);
  tLimiter.setSampleRate(PLUG_DEFAULT_SAMPLERATE);
  tLimiter.initRuntime();

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);

  //LUFS loudness meter 
  tLoudnessMeter = new Plug::LoudnessMeter();
  tLoudnessMeter->SetSampleRate(PLUG_DEFAULT_SAMPLERATE);
  tLoudnessMeter->SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER);  
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
  char sLoudnessString[64];


  switch (paramIdx)
  {
    case kGain:
      //mGain = GetParam(kGain)->Value() / 100.;
      tLimiter.setThresh(GetParam(kGain)->Value());
      break;
      /*
    case kIContactControl:
      if (GetParam(kIContactControl)->Value()) {
        fLufs = tLoudnessMeter->GetLufs();
        if (fabs(fLufs - HUGE_VAL) < std::numeric_limits<double>::epsilon()) {
          sprintf(sLoudnessString, "-oo LUFS");
        }
        else {
          sprintf(sLoudnessString, "%4.2f LUFS, %4.4f", fLufs, fSampleSample);
        }
        tLoudnessTextControl->SetTextFromPlug(sLoudnessString);
        tLoudnessTextControl->Redraw();
      }
      break;*/

    default:
      break;
  }
}
