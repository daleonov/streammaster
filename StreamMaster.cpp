#include "StreamMaster.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include "lib_chunkware/SimpleLimit.h"
#include "PLUG/PLUG_LoudnessMeter.h"


const int kNumPrograms = 1;
chunkware_simple::SimpleLimit tLimiter;
const double fDefaultLimiterThreshold = 0.;

ITextControl * tLoudnessTextControl;
IGraphics* pGraphics;

void StreamMaster::UpdateGui()
{
  char sLoudnessString[64];
  double fLufs = tLoudnessMeter->GetLufs();
    if (fabs(fLufs - HUGE_VAL) < std::numeric_limits<double>::epsilon()) {
      sprintf(sLoudnessString, "-oo LUFS");
    }
    else {
      sprintf(sLoudnessString, "%4.2f LUFS", fLufs);
    }
    tLoudnessTextControl->SetTextFromPlug(sLoudnessString);
    //tLoudnessTextControl->Redraw();

}

StreamMaster::StreamMaster(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{

  TRACE;
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Threshold", fDefaultLimiterThreshold, -60., 5.0, 0.1, "dB");
  GetParam(kGain)->SetShape(2.);
  GetParam(kIContactControl)->InitBool("IContactControl", 0, "");

  pGraphics = MakeGraphics(this, kWidth, kHeight);

  IColor tBgColor = IColor(255, 128, 0, 0);
  pGraphics->AttachPanelBackground(&tBgColor);
  
  // Limiter knob
  IBitmap tBmp = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX, kGainY, kGain, &tBmp));
  
  // LUFS read button
  tBmp = pGraphics->LoadIBitmap(ICONTACTCONTROL_ID, ICONTACTCONTROL_FN, kIContactControl_N);
  pGraphics->AttachControl(new IContactControl(this, kIContactControl_X, kIContactControl_Y, kIContactControl, &tBmp));

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
  // Creating an interleaved buffer
  /*
  for (int frame = 0, sample = 0; frame < nFrames; frame++, sample+=2) {
    fRandom = -1. + static_cast <double> (rand()) / (static_cast <double> (RAND_MAX / (1. - (-1.))));
    afInterleavedSamples[sample] = fRandom/2;
    afInterleavedSamples[sample + 1] = fRandom/2;

    //afInterleavedSamples[sample]   = in1[sample];
    //afInterleavedSamples[sample+1] = in2[sample];
  }*/
  tLoudnessMeter->AddSamples(afInterleavedSamples, nFrames);
  delete[] afInterleavedSamples;
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
      break;

    default:
      break;
  }
}
