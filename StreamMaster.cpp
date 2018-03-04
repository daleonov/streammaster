#include "StreamMaster.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include "lib_chunkware/SimpleLimit.h"
#include "PLUG/PLUG_LoudnessMeter.h"

//using namespace chunkware_simple;

const int kNumPrograms = 1;
chunkware_simple::SimpleLimit tLimiter;
const double fDefaultLimiterThreshold = 0.;

Plug::LoudnessMeter tLoudnessMeter;
ITextControl * tLoudnessTextControl;

StreamMaster::StreamMaster(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Threshold", fDefaultLimiterThreshold, -60., 5.0, 0.1, "dB");
  GetParam(kGain)->SetShape(2.);
  GetParam(kIContactControl)->InitBool("IContactControl", 0, "");

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);

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
  tLoudnessMeter.SetSampleRate(PLUG_DEFAULT_SAMPLERATE);
  tLoudnessMeter.SetNumberOfChannels(PLUG_DEFAULT_CHANNEL_NUMBER);  
  
}

StreamMaster::~StreamMaster() {}

void StreamMaster::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1;
    *out2 = *in2;
    tLimiter.process(*out1, *out2);
  }
  tLoudnessMeter.AddSamples(in2, nFrames);

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
        fLufs = tLoudnessMeter.GetLufs();
        sprintf(sLoudnessString, "%4.2f dB LUFS", fLufs);
        tLoudnessTextControl->SetTextFromPlug(sLoudnessString);
        tLoudnessTextControl->Redraw();
      }
      break;

    default:
      break;
  }
}
