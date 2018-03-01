#ifndef __STREAMMASTER__
#define __STREAMMASTER__

#include "IPlug_include_in_plug_hdr.h"

class StreamMaster : public IPlug
{
public:
  StreamMaster(IPlugInstanceInfo instanceInfo);
  ~StreamMaster();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mGain;
};

#endif
